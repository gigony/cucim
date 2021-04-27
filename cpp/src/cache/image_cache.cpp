/*
 * Apache License, Version 2.0
 * Copyright 2021 NVIDIA Corporation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cucim/cache/image_cache.h"

#include "cucim/memory/memory_manager.h"

#include <type_traits>
#include <boost/interprocess/smart_ptr/shared_ptr.hpp>
#include <boost/interprocess/allocators/allocator.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
// #include <boost/interprocess/containers/string.hpp>
#include <libcuckoo/cuckoohash_map.hh>
#include <fmt/format.h>


namespace cucim::cache
{

template <class P>
struct null_deleter
{
private:
    P p_;

public:
    null_deleter(P const& p) : p_(p)
    {
    }
    void operator()(void const*)
    {
        p_.reset();
    }

    P const& get() const
    {
        return p_;
    }
};

template <class T>
using deleter_type = boost::interprocess::shared_ptr<
    T,
    boost::interprocess::allocator<
        void,
        boost::interprocess::segment_manager<char,
                                             boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family>,
                                             boost::interprocess::iset_index>>,
    boost::interprocess::deleter<
        T,
        boost::interprocess::segment_manager<char,
                                             boost::interprocess::rbtree_best_fit<boost::interprocess::mutex_family>,
                                             boost::interprocess::iset_index>>>;

ImageCacheKey::ImageCacheKey(uint64_t file_hash, uint64_t index) : file_hash(file_hash), location_hash(index)
{
}

std::shared_ptr<ImageCacheKey> ImageCacheKey::create(uint64_t file_hash, uint64_t index, std::shared_ptr<void> seg)
{
    const auto& segment = std::static_pointer_cast<boost::interprocess::managed_shared_memory>(seg);

    auto key = boost::interprocess::make_managed_shared_ptr(
        segment->find_or_construct<ImageCacheKey>(boost::interprocess::anonymous_instance)(file_hash, index), *segment);

    return std::shared_ptr<ImageCacheKey>(key.get().get(), null_deleter<decltype(key)>(key));
}


ImageCacheValue::ImageCacheValue(void* data, uint64_t size) : data(data), size(size)
{
}

ImageCacheValue::~ImageCacheValue()
{
    cucim_free(data);
}

ImageCacheValue::operator bool() const
{
    return data != nullptr;
}

std::shared_ptr<ImageCacheValue> ImageCacheValue::create(void* data, uint64_t size, std::shared_ptr<void>& seg)
{
    const auto& segment = std::static_pointer_cast<boost::interprocess::managed_shared_memory>(seg);

    auto value = boost::interprocess::make_managed_shared_ptr(
        segment->find_or_construct<ImageCacheValue>(boost::interprocess::anonymous_instance)(data, size), *segment);

    return std::shared_ptr<ImageCacheValue>(value.get().get(), null_deleter<decltype(value)>(value));
}


struct ImageCacheItemDetail
{
    ImageCacheItemDetail(deleter_type<ImageCacheKey>& key, deleter_type<ImageCacheValue>& value)
        : key(key), value(value)
    {
    }
    deleter_type<ImageCacheKey> key;
    deleter_type<ImageCacheValue> value;
};


ImageCacheKey& ImageCacheItem::key()
{
    ImageCacheItemDetail* item = reinterpret_cast<ImageCacheItemDetail*>(item_);
    return *item->key.get();
}

ImageCacheValue& ImageCacheItem::value()
{
    ImageCacheItemDetail* item = reinterpret_cast<ImageCacheItemDetail*>(item_);
    return *item->value.get();
}


ImageCacheItem::ImageCacheItem(void* item, std::shared_ptr<void> deleter) : item_(item), deleter_(deleter)
{
}

std::shared_ptr<ImageCacheItem> create_cache_item(std::shared_ptr<ImageCacheKey>& key,
                                                  std::shared_ptr<ImageCacheValue>& value,
                                                  const std::shared_ptr<void>& seg)
{
    const auto& segment = std::static_pointer_cast<boost::interprocess::managed_shared_memory>(seg);
    auto key_impl = std::get_deleter<null_deleter<deleter_type<ImageCacheKey>>>(key)->get();
    auto value_impl = std::get_deleter<null_deleter<deleter_type<ImageCacheValue>>>(value)->get();

    auto item = boost::interprocess::make_managed_shared_ptr(
        segment->find_or_construct<ImageCacheItemDetail>(boost::interprocess::anonymous_instance)(
            key_impl, value_impl),
        *segment);
    return std::make_shared<ImageCacheItem>(
        item.get().get(), std::make_shared<null_deleter<decltype(item)>>(item));
}

using MapKey = boost::interprocess::managed_shared_ptr<ImageCacheKey, boost::interprocess::managed_shared_memory>;

using MapValue =
    boost::interprocess::managed_shared_ptr<ImageCacheItemDetail, boost::interprocess::managed_shared_memory>;

using KeyValuePair = std::pair<MapKey, MapValue>;
using ImageCacheAllocator =
    boost::interprocess::allocator<KeyValuePair, boost::interprocess::managed_shared_memory::segment_manager>;
using MapKeyHasher = boost::hash<MapKey>;
using MakKeyEqual = std::equal_to<MapKey>;

} // namespace cucim::cache

template <>
struct boost::hash<cucim::cache::MapKey>
{
    typedef cucim::cache::MapKey argument_type;
    typedef size_t result_type;
    result_type operator()(argument_type::type& s) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(s->file_hash);
        std::size_t h2 = std::hash<uint64_t>{}(s->location_hash);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }

    result_type operator()(const argument_type::type& s) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(s->file_hash);
        std::size_t h2 = std::hash<uint64_t>{}(s->location_hash);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }

    result_type operator()(const cucim::cache::ImageCacheKey& s) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(s.file_hash);
        std::size_t h2 = std::hash<uint64_t>{}(s.location_hash);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }

    result_type operator()(const std::shared_ptr<cucim::cache::ImageCacheKey>& s) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(s->file_hash);
        std::size_t h2 = std::hash<uint64_t>{}(s->location_hash);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
};

template <>
struct std::equal_to<cucim::cache::MapKey>
{
    typedef cucim::cache::MapKey argument_type;

    bool operator()(const argument_type::type& lhs, const argument_type::type& rhs) const
    {
        return lhs->location_hash == rhs->location_hash && lhs->file_hash == rhs->file_hash;
    }

    bool operator()(const argument_type::type& lhs, const cucim::cache::ImageCacheKey& rhs) const
    {
        return lhs->location_hash == rhs.location_hash && lhs->file_hash == rhs.file_hash;
    }

    bool operator()(const cucim::cache::ImageCacheKey& lhs, const std::shared_ptr<cucim::cache::ImageCacheKey>& rhs) const
    {
        return lhs.location_hash == rhs->location_hash && lhs.file_hash == rhs->file_hash;
    }
};

namespace cucim::cache
{

using ImageCacheType =
    libcuckoo::cuckoohash_map<MapKey::type, MapValue::type, boost::hash<MapKey>, std::equal_to<MapKey>, ImageCacheAllocator>;

// using ImageCacheType = libcuckoo::cuckoohash_map<std::shared_ptr<ImageCacheKey>, std::shared_ptr<ImageCacheItem>>;

constexpr uint32_t LIST_PADDING = 64; /// additional buffer for multi-threaded environment
constexpr const char* kSegmentName = "cucim-0";
constexpr size_t kSegmentSize = (1UL << 20) * 500;





ImageCache::ImageCache(uint32_t capacity, uint64_t mem_capacity, bool record_stat)
    : segment_(std::make_shared<boost::interprocess::managed_shared_memory>(
          boost::interprocess::open_or_create, kSegmentName, kSegmentSize)),
    //   hashmap_(std::make_shared<ImageCacheType>(capacity)),
      capacity_(capacity),
      list_capacity_(capacity + LIST_PADDING),
      capacity_nbytes_(mem_capacity),
      stat_is_recorded_(record_stat),
      list_(list_capacity_)
{
    const auto& segment = std::static_pointer_cast<boost::interprocess::managed_shared_memory>(segment_);

    auto hashmap = boost::interprocess::make_managed_shared_ptr(
        segment->find_or_construct<ImageCacheType>("cucim-hashmap")(
            capacity, MapKeyHasher(), MakKeyEqual(), ImageCacheAllocator(segment->get_segment_manager())),
        *segment);

    hashmap_ =  std::shared_ptr<ImageCacheType>(hashmap.get().get(), null_deleter<decltype(hashmap)>(hashmap));

    // auto tt = boost::interprocess::make_managed_shared_ptr(
    //     segment->find_or_construct<ImageCacheType2>("cucim-hashmap")(
    //         (1U << 16) * 4, MapKeyHasher(), MakKeyEqual(), ImageCacheAllocator(segment->get_segment_manager())),
    //     *segment); // capacity

    // (void)tt;
    // tt->reserve((int)1000);
    // // auto item = segment->find_or_construct<TempCacheItemInternal>("cucim-item")(1, 3);
    // auto valA = boost::interprocess::make_managed_shared_ptr(
    //     segment->find_or_construct<TempCacheItemInternal::ValA::type::element_type>(
    //         boost::interprocess::anonymous_instance)(1),
    //     *segment);
    // auto valB = boost::interprocess::make_managed_shared_ptr(
    //     segment->find_or_construct<TempCacheItemInternal::ValB::type::element_type>(
    //         boost::interprocess::anonymous_instance)(3),
    //     *segment);

    // auto item = boost::interprocess::make_managed_shared_ptr(
    //     segment->find_or_construct<TempCacheItemInternal>("cucim-item")(valA, valB), *segment);

    // auto key = boost::interprocess::make_managed_shared_ptr(
    //     segment->find_or_construct<TempCacheKey>(boost::interprocess::anonymous_instance)(1, 2), *segment);


    // // segment->find_or_construct<ImageCacheType2>("//TempCacheItemInternal{ 1, 3 };
    // bool succeed = tt->insert(key, item);

    // auto val = tt->find(key);

    // fmt::print("## {} \n", val->b);


    //         // ImageCacheType2* kk = segment->construct<ImageCacheType2>("cucim-hashmap")(
    //         //     capacity, MapKeyHasher(), MakKeyEqual(), ImageCacheAllocator(segment->get_segment_manager()));
    //         // (void)kk;
    //         // auto tt = std::make_shared<ImageCacheType2>(capacity);
    //         // if (succeed)
    //         // {
    //         //     return;
    //         // }
    // int b = key->file_hash;
    // if (b == 0 && succeed)
    // {
    //     return;
    // }


    // auto hashmap = std::static_pointer_cast<ImageCacheType>(hashmap_);

    // list_.reserve(list_capacity_); // keep enough buffer
    // hashmap->reserve((int)capacity);
};

std::shared_ptr<ImageCacheKey> ImageCache::create_key(uint64_t file_hash, uint64_t index)
{
    return ImageCacheKey::create(file_hash, index, segment_);

}
std::shared_ptr<ImageCacheValue> ImageCache::create_value(void* data, uint64_t size)
{
    return ImageCacheValue::create(data, size, segment_);
}

bool ImageCache::insert(std::shared_ptr<ImageCacheKey> key, std::shared_ptr<ImageCacheValue> value)
{
    // If image size is larger than memory capacity, return false;
    if (value->size > capacity_nbytes_)
    {
        return false;
    }
    const auto& segment = std::static_pointer_cast<boost::interprocess::managed_shared_memory>(segment_);
    const auto& hashmap = std::static_pointer_cast<ImageCacheType>(hashmap_);
    while (is_list_full() || is_mem_full())
    {
        remove_front();
    }
    auto item = create_cache_item(key, value, segment);
    push_back(item);
    auto key_impl = std::get_deleter<null_deleter<deleter_type<ImageCacheKey>>>(key)->get();
    auto item_impl = std::static_pointer_cast<null_deleter<MapValue::type>>(item->deleter_)->get();
    bool succeed = hashmap->insert(key_impl, item_impl);
    return succeed;
}

bool ImageCache::is_list_full() const
{
    if (size() >= capacity_)
    {
        return true;
    }
    return false;
}

bool ImageCache::is_mem_full() const
{
    if (size_nbytes_.load(std::memory_order_relaxed) >= capacity_nbytes_)
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ImageCache::remove_front()
{
    const auto& hashmap = std::static_pointer_cast<ImageCacheType>(hashmap_);
    while (true)
    {
        uint32_t head = list_head_.load(std::memory_order_relaxed);
        uint32_t tail = list_tail_.load(std::memory_order_relaxed);
        if (head != tail)
        {
            // Remove front by increasing head
            if (list_head_.compare_exchange_weak(
                    head, (head + 1) % list_capacity_, std::memory_order_release, std::memory_order_relaxed))
            {
                std::shared_ptr<ImageCacheItem> head_item = list_[head];
                size_nbytes_.fetch_sub(head_item->value().size, std::memory_order_relaxed);
                hashmap->erase(head_item->key());
                list_[head].reset(); // decrease refcount
                break;
            }
        }
        else
        {
            break; // already empty
        }
    }
}

void ImageCache::push_back(std::shared_ptr<ImageCacheItem> item)
{
    // uint32_t head = list_head_.load(std::memory_order_relaxed);
    uint32_t tail = list_tail_.load(std::memory_order_relaxed);
    while (true)
    {
        // Push back by increasing tail
        if (list_tail_.compare_exchange_weak(
                tail, (tail + 1) % list_capacity_, std::memory_order_release, std::memory_order_relaxed))
        {
            list_[tail] = item;
            size_nbytes_.fetch_add(item->value().size, std::memory_order_relaxed);
            break;
        }

        // head = list_head_.load(std::memory_order_relaxed);
        tail = list_tail_.load(std::memory_order_relaxed);
    }
}

uint32_t ImageCache::size() const
{
    uint32_t head = list_head_.load(std::memory_order_relaxed);
    uint32_t tail = list_tail_.load(std::memory_order_relaxed);

    return (tail + list_capacity_ - head) % list_capacity_;
}

void ImageCache::record(bool value)
{
    stat_hit_ = 0;
    stat_miss_ = 0;
    stat_is_recorded_ = value;
}

bool ImageCache::record() const
{
    return stat_is_recorded_;
}

uint64_t ImageCache::hit_count() const
{
    return stat_hit_;
}
uint64_t ImageCache::miss_count() const
{
    return stat_miss_;
}


void ImageCache::reserve(uint32_t new_capacity, uint64_t new_mem_capacity)
{
    const auto& hashmap = std::static_pointer_cast<ImageCacheType>(hashmap_);

    if (capacity_ < new_capacity)
    {
        uint32_t old_list_capacity = list_capacity_;

        capacity_ = new_capacity;
        list_capacity_ = new_capacity + LIST_PADDING;

        list_.reserve(list_capacity_);
        hashmap->reserve(new_capacity);

        // Move items in the vector
        uint32_t head = list_head_.load(std::memory_order_relaxed);
        uint32_t tail = list_tail_.load(std::memory_order_relaxed);
        if (tail < head)
        {
            head = 0;
            uint32_t new_head = old_list_capacity;

            while (head != tail)
            {
                list_[new_head] = list_[head];
                list_[head] = nullptr;

                head = (head + 1) % old_list_capacity;
                new_head = (new_head + 1) % list_capacity_;
            }
            // Set new tail
            list_tail_.store(new_head, std::memory_order_relaxed);
        }
    }

    if (capacity_nbytes_ < new_mem_capacity)
    {
        capacity_nbytes_ = new_mem_capacity;
    }
}

std::shared_ptr<ImageCacheValue> ImageCache::find(const std::shared_ptr<ImageCacheKey>& key)
{
    const auto& hashmap = std::static_pointer_cast<ImageCacheType>(hashmap_);

    MapValue::type item;
    auto key_impl = std::get_deleter<null_deleter<deleter_type<ImageCacheKey>>>(key)->get();
    const bool found = hashmap->find(key_impl, item);
    if (stat_is_recorded_)
    {
        if (found)
        {
            stat_hit_.fetch_add(1, std::memory_order_relaxed);
            return std::shared_ptr<ImageCacheValue>(item->value.get().get(), null_deleter<decltype(item)>(item));
        }
        else
        {
            stat_hit_.fetch_sub(1, std::memory_order_relaxed);
        }
    }
    else
    {
        if (found)
        {
            return std::shared_ptr<ImageCacheValue>(item->value.get().get(), null_deleter<decltype(item)>(item));
        }
    }
    return std::shared_ptr<ImageCacheValue>();
}

bool ImageCache::erase(const std::shared_ptr<ImageCacheKey>& key)
{
    const auto& hashmap = std::static_pointer_cast<ImageCacheType>(hashmap_);
    auto key_impl = std::get_deleter<null_deleter<deleter_type<ImageCacheKey>>>(key)->get();
    const bool succeed = hashmap->erase(key_impl);
    return succeed;
}

} // namespace cucim::cache
