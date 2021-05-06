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

#include "image_cache_per_process.h"

#include "cucim/memory/memory_manager.h"

#include <fmt/format.h>

// #include <functional>

// #include <sys/types.h>
// #include <sys/stat.h>
// #include <unistd.h>

namespace std
{

size_t hash<std::shared_ptr<cucim::cache::ImageCacheKey>>::operator()(
    const std::shared_ptr<cucim::cache::ImageCacheKey>& s) const
{
    std::size_t h1 = std::hash<uint64_t>{}(s->file_hash);
    std::size_t h2 = std::hash<uint64_t>{}(s->location_hash);
    return h1 ^ (h2 << 1); // or use boost::hash_combine
}

bool equal_to<std::shared_ptr<cucim::cache::ImageCacheKey>>::operator()(
    const std::shared_ptr<cucim::cache::ImageCacheKey>& lhs, const std::shared_ptr<cucim::cache::ImageCacheKey>& rhs) const
{
    return lhs->location_hash == rhs->location_hash;
}

} // namespace std
namespace cucim::cache
{

constexpr uint32_t kListPadding = 64; /// additional buffer for multi-threaded environment

struct ImageCacheItem
{
    ImageCacheItem(std::shared_ptr<ImageCacheKey>& key, std::shared_ptr<ImageCacheValue>& value)
        : key(key), value(value)
    {
    }

    std::shared_ptr<ImageCacheKey> key;
    std::shared_ptr<ImageCacheValue> value;
};

PerProcessImageCacheValue::PerProcessImageCacheValue(void* data, uint64_t size, void* user_obj)
    : ImageCacheValue(data, size, user_obj){};
PerProcessImageCacheValue::~PerProcessImageCacheValue()
{
    cucim_free(data);
};

PerProcessImageCache::PerProcessImageCache(uint32_t capacity, uint64_t mem_capacity, bool record_stat)
    : ImageCache(capacity, mem_capacity),
      hashmap_(capacity),
      capacity_(capacity),
      list_capacity_(capacity + kListPadding),
      capacity_nbytes_(mem_capacity),
      stat_is_recorded_(record_stat)
{
    list_.reserve(list_capacity_); // keep enough buffer
    hashmap_.reserve((int)capacity);
};

PerProcessImageCache::~PerProcessImageCache()
{
}

std::shared_ptr<ImageCacheKey> PerProcessImageCache::create_key(uint64_t file_hash, uint64_t index)
{
    return std::make_shared<ImageCacheKey>(file_hash, index);
}
std::shared_ptr<ImageCacheValue> PerProcessImageCache::create_value(void* data, uint64_t size)
{
    return std::make_shared<PerProcessImageCacheValue>(data, size);
}

void* PerProcessImageCache::allocate(std::size_t n)
{
    return cucim_malloc(n);
}

void PerProcessImageCache::lock(uint64_t)
{
    return;
}

void PerProcessImageCache::unlock(uint64_t)
{
    return;
}

bool PerProcessImageCache::insert(std::shared_ptr<ImageCacheKey> key, std::shared_ptr<ImageCacheValue> value)
{
    while (is_list_full() || is_mem_full())
    {
        remove_front();
    }
    auto item = std::make_shared<ImageCacheItem>(key, value);
    push_back(item);
    bool succeed = hashmap_.insert(key, item);
    return succeed;
}

uint32_t PerProcessImageCache::size() const
{
    return 0;
}

uint64_t PerProcessImageCache::memory_size() const
{
    return 0;
}
uint32_t PerProcessImageCache::capacity() const
{
    return 0;
}
uint64_t PerProcessImageCache::memory_capacity() const
{
    return 0;
}
uint64_t PerProcessImageCache::free_memory() const
{
    return 0;
}

void PerProcessImageCache::record(bool)
{
    return;
}

bool PerProcessImageCache::record() const
{
    return false;
}

uint64_t PerProcessImageCache::hit_count() const
{
    return 0;
}
uint64_t PerProcessImageCache::miss_count() const
{
    return 0;
}

void PerProcessImageCache::reserve(uint32_t, uint64_t)
{
}

std::shared_ptr<ImageCacheValue> PerProcessImageCache::find(const std::shared_ptr<ImageCacheKey>& key)
{
    std::shared_ptr<ImageCacheItem> item;
    const bool found = hashmap_.find(key, item);
    if (found)
    {
        return item->value;
    }
    else
    {
    }
    return std::shared_ptr<ImageCacheValue>();
}

bool PerProcessImageCache::is_list_full() const
{
    uint32_t head = list_head_.load(std::memory_order_relaxed);
    uint32_t tail = list_tail_.load(std::memory_order_relaxed);
    if ((tail + list_capacity_ - head) % list_capacity_ >= capacity_)
    {
        return true;
    }
    return false;
}

bool PerProcessImageCache::is_mem_full() const
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
void PerProcessImageCache::remove_front()
{
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
                size_nbytes_.fetch_sub(head_item->value->size, std::memory_order_relaxed);
                hashmap_.erase(head_item->key);
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


void PerProcessImageCache::push_back(std::shared_ptr<ImageCacheItem>& item)
{
    uint32_t tail = list_tail_.load(std::memory_order_relaxed);
    while (true)
    {
        // Push back by increasing tail
        if (list_tail_.compare_exchange_weak(
                tail, (tail + 1) % list_capacity_, std::memory_order_release, std::memory_order_relaxed))
        {
            list_[tail] = item;
            size_nbytes_.fetch_add(item->value->size, std::memory_order_relaxed);
            break;
        }

        // head = list_head_.load(std::memory_order_relaxed);
        tail = list_tail_.load(std::memory_order_relaxed);
    }
}

bool PerProcessImageCache::erase(const std::shared_ptr<ImageCacheKey>& key)
{
    const bool succeed = hashmap_.erase(key);
    return succeed;
}

} // namespace cucim::cache
