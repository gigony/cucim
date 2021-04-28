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

    static std::shared_ptr<ImageCacheValue> create(void* data, uint64_t size, std::shared_ptr<void>& seg);

    operator bool() const;

    void* data = nullptr;
    uint64_t size = 0;
};
struct EXPORT_VISIBLE ImageCacheItem
{
    ImageCacheItem(void* item, std::shared_ptr<void> deleter);

    ImageCacheKey& key();
    ImageCacheValue& value();

    void* item_ = nullptr;
    std::shared_ptr<void> deleter_;
};

template <class T>
struct shared_mem_deleter
{
    shared_mem_deleter(void* segment);
    void operator()(T* p);

private:
    void* segment_ = nullptr;
};

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

    std::unique_ptr<uint32_t, shared_mem_deleter<uint32_t>> capacity_; /// capacity of hashmap
    std::unique_ptr<uint32_t, shared_mem_deleter<uint32_t>> list_capacity_; /// capacity of list
    std::unique_ptr<std::atomic<uint64_t>, shared_mem_deleter<std::atomic<uint64_t>>> size_nbytes_; /// size of cache
                                                                                                    /// memory used
    std::unique_ptr<uint64_t, shared_mem_deleter<uint64_t>> capacity_nbytes_; /// size of cache memory allocated

    std::unique_ptr<std::atomic<uint64_t>, shared_mem_deleter<std::atomic<uint64_t>>> stat_hit_; /// cache hit count
    std::unique_ptr<std::atomic<uint64_t>, shared_mem_deleter<std::atomic<uint64_t>>> stat_miss_; /// cache miss mcount
    std::unique_ptr<bool, shared_mem_deleter<bool>> stat_is_recorded_; /// whether if cache stat is recorded or not

    std::shared_ptr<void> list_;
    std::unique_ptr<std::atomic<uint32_t>, shared_mem_deleter<std::atomic<uint32_t>>> list_head_; /// head
    std::unique_ptr<std::atomic<uint32_t>, shared_mem_deleter<std::atomic<uint32_t>>> list_tail_; /// tail
};

} // namespace cucim::cache

#endif // CUCIM_CACHE_IMAGE_CACHE_H
