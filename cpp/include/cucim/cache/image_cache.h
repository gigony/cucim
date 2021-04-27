/*
 * Copyright (c) 2021, NVIDIA CORPORATION.
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

#ifndef CUCIM_CACHE_IMAGE_CACHE_H
#define CUCIM_CACHE_IMAGE_CACHE_H

#include "cucim/core/framework.h"

#include <memory>
#include <atomic>
#include <functional>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

namespace cucim::cache
{


struct EXPORT_VISIBLE ImageCacheKey
{
    ImageCacheKey(uint64_t file_hash, uint64_t index);

    static std::shared_ptr<ImageCacheKey> create(uint64_t file_hash, uint64_t index, std::shared_ptr<void> seg);

    uint64_t file_hash = 0; /// st_dev + st_ino + st_mtime + ifd_index
    uint64_t location_hash; ///  tile_index or (x , y)
};


struct EXPORT_VISIBLE ImageCacheValue
{
    ImageCacheValue(void* data, uint64_t size);
    ~ImageCacheValue();

    static std::shared_ptr<ImageCacheValue> create(void* data, uint64_t size, std::shared_ptr<void> seg);

    operator bool() const;

    void* data = nullptr;
    uint64_t size = 0;
};
struct EXPORT_VISIBLE ImageCacheItem
{
    ImageCacheItem(void* item, std::shared_ptr<void> deleter);

    // static std::shared_ptr<void> ImageCacheItem::create(std::shared_ptr<ImageCacheKey>& key,
    //                                                     std::shared_ptr<ImageCacheValue>& value,
    //                                                     std::shared_ptr<void> seg);

    ImageCacheKey& key();
    ImageCacheValue& value();

    void* item_ = nullptr;
    std::shared_ptr<void> deleter_;
};

} // namespace cucim::cache


namespace std
{
template <>
struct hash<std::shared_ptr<cucim::cache::ImageCacheKey>>
{
    size_t operator()(const cucim::cache::ImageCacheKey& s) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(s.file_hash);
        std::size_t h2 = std::hash<uint64_t>{}(s.location_hash);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
    size_t operator()(cucim::cache::ImageCacheKey* s) const
    {
        std::size_t h1 = std::hash<uint64_t>{}(s->file_hash);
        std::size_t h2 = std::hash<uint64_t>{}(s->location_hash);
        return h1 ^ (h2 << 1); // or use boost::hash_combine
    }
};


template <>
struct equal_to<std::shared_ptr<cucim::cache::ImageCacheKey>>
{
    bool operator()(const cucim::cache::ImageCacheKey& lhs, const cucim::cache::ImageCacheKey& rhs) const
    {
        return lhs.location_hash == rhs.location_hash && lhs.file_hash == rhs.file_hash;
    }
    bool operator()(const std::shared_ptr<cucim::cache::ImageCacheKey>& lhs,
                    const std::shared_ptr<cucim::cache::ImageCacheKey>& rhs) const
    {
        return lhs->location_hash == rhs->location_hash && lhs->file_hash == rhs->file_hash;
    }


    bool operator()(const cucim::cache::ImageCacheKey& lhs, const std::shared_ptr<cucim::cache::ImageCacheKey>& rhs) const
    {
        return lhs.location_hash == rhs->location_hash && lhs.file_hash == rhs->file_hash;
    }
};
} // namespace std


namespace cucim::cache
{
/**
 * @brief Image Cache for loading tiles.
 *
 * FIFO is used for cache replacement policy here.
 *
 */
class EXPORT_VISIBLE ImageCache
{
public:
    ImageCache() = delete;
    ImageCache(uint32_t capacity, uint64_t mem_capacity, bool record_stat = false);

    std::shared_ptr<ImageCacheKey> create_key(uint64_t file_hash, uint64_t index);
    std::shared_ptr<ImageCacheValue> create_value(void* data, uint64_t size);

    bool insert(std::shared_ptr<ImageCacheKey> key, std::shared_ptr<ImageCacheValue> value);

    bool is_list_full() const;
    bool is_mem_full() const;

    void remove_front();
    void push_back(std::shared_ptr<ImageCacheItem> item);

    uint32_t size() const;

    /**
     * @brief Record cache stat.
     *
     * Stat values would be reset with this method.
     *
     * @param value Whether if cache stat would be recorded or not
     */
    void record(bool value);

    /**
     * @brief Return whether if cache stat is recorded or not
     *
     * @return true if cache stat is recorded. false otherwise
     */
    bool record() const;

    uint64_t hit_count() const;
    uint64_t miss_count() const;

    /**
     * @brief Attempt to preallocate enough memory for specified number of elements and memory size.
     *
     * This method is not thread-safe.
     *
     * @param capacity Number of elements required
     * @param mem_capacity Size of memory required in bytes
     */
    void reserve(uint32_t new_capacity, uint64_t new_mem_capacity);

    std::shared_ptr<ImageCacheValue> find(const std::shared_ptr<ImageCacheKey>& key);
    bool erase(const std::shared_ptr<ImageCacheKey>& key);

private:
    std::shared_ptr<void> segment_;
    std::shared_ptr<void> hashmap_;

    uint32_t capacity_ = 0; /// capacity of hashmap
    uint32_t list_capacity_ = 0; /// capacity of list
    std::atomic<uint64_t> size_nbytes_ = 0; /// size of cache memory used
    uint64_t capacity_nbytes_ = 0; /// size of cache memory allocated

    std::atomic<uint64_t> stat_hit_ = 0; /// cache hit count
    std::atomic<uint64_t> stat_miss_ = 0; /// cache miss mcount
    bool stat_is_recorded_ = false; /// whether if cache stat is recorded or not

    std::vector<std::shared_ptr<ImageCacheItem>> list_; /// circular list using vector
    std::atomic<uint32_t> list_head_ = 0; /// head
    std::atomic<uint32_t> list_tail_ = 0; /// tail
};

} // namespace cucim::cache

#endif // CUCIM_CACHE_IMAGE_CACHE_H
