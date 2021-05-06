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

#ifndef CUCIM_CACHE_IMAGE_CACHE_SHARED_MEMORY_H
#define CUCIM_CACHE_IMAGE_CACHE_SHARED_MEMORY_H

#include "cucim/cache/image_cache.h"

namespace cucim::cache
{


struct EXPORT_VISIBLE ImageCacheItem
{
    ImageCacheItem(void* item, std::shared_ptr<void> deleter = nullptr);

    virtual ImageCacheKey* key()
    {
        return nullptr;
    };
    virtual ImageCacheValue* value()
    {
        return nullptr;
    };

    void* item_ = nullptr;
    std::shared_ptr<void> deleter_;
};


// struct EmptyImageCacheValue : public ImageCacheValue
// {
//     EmptyImageCacheValue(void* data, uint64_t size, void* user_obj = nullptr) : ImageCacheValue(data, size,
//     user_obj){}; ~EmptyImageCacheValue() override{};
// };

// struct EmptyImageCacheItem : public ImageCacheItem
// {
//     EmptyImageCacheItem(void* item, std::shared_ptr<void> deleter = nullptr) : ImageCacheItem(item, deleter){};

//     ImageCacheKey* key() override
//     {
//         return nullptr;
//     };
//     ImageCacheValue* value() override
//     {
//         return nullptr;
//     };
// };

/**
 * @brief Image Cache for loading tiles.
 *
 * FIFO is used for cache replacement policy here.
 *
 */

class EXPORT_VISIBLE SharedMemoryImageCache : public ImageCache
{
public:
    SharedMemoryImageCache(std::unique_ptr<ImageCacheStrategy> strategy);
    ~SharedMemoryImageCache();

private:
    bool is_list_full() const;
    bool is_mem_full() const;

    void remove_front();
    void push_back(std::shared_ptr<ImageCacheItem> item);
    bool erase(const std::shared_ptr<ImageCacheKey>& key);

    static bool remove_shmem();

    uint32_t calc_hashmap_capacity(uint32_t capacity);
    std::shared_ptr<void> create_segment(uint32_t capacity, uint64_t mem_capacity);

    std::shared_ptr<void> segment_;
    std::shared_ptr<void> list_;
    std::shared_ptr<void> hashmap_;

    void* mutex_array_ = nullptr;

    std::unique_ptr<uint32_t, shared_mem_deleter<uint32_t>> capacity_; /// capacity of hashmap
    std::unique_ptr<uint32_t, shared_mem_deleter<uint32_t>> list_capacity_; /// capacity of list
    std::unique_ptr<std::atomic<uint64_t>, shared_mem_deleter<std::atomic<uint64_t>>> size_nbytes_; /// size of cache
                                                                                                    /// memory used
    std::unique_ptr<uint64_t, shared_mem_deleter<uint64_t>> capacity_nbytes_; /// size of cache memory allocated

    std::unique_ptr<std::atomic<uint64_t>, shared_mem_deleter<std::atomic<uint64_t>>> stat_hit_; /// cache hit count
    std::unique_ptr<std::atomic<uint64_t>, shared_mem_deleter<std::atomic<uint64_t>>> stat_miss_; /// cache miss mcount
    std::unique_ptr<bool, shared_mem_deleter<bool>> stat_is_recorded_; /// whether if cache stat is recorded or not

    std::unique_ptr<std::atomic<uint32_t>, shared_mem_deleter<std::atomic<uint32_t>>> list_head_; /// head
    std::unique_ptr<std::atomic<uint32_t>, shared_mem_deleter<std::atomic<uint32_t>>> list_tail_; /// tail
};

} // namespace cucim::cache

#endif // CUCIM_CACHE_IMAGE_CACHE_SHARED_MEMORY_H
