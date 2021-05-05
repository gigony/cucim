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

#ifndef CUCIM_CACHE_IMAGE_CACHE_PER_PROCESS_H
#define CUCIM_CACHE_IMAGE_CACHE_PER_PROCESS_H

#include "cucim/cache/image_cache.h"

#include <libcuckoo/cuckoohash_map.hh>
#include <memory>
#include <functional>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


namespace cucim::cache
{

/**
 * @brief Image Cache for loading tiles.
 *
 * FIFO is used for cache replacement policy here.
 *
 */

class EXPORT_VISIBLE PerProcessImageCache : public ImageCache
{
public:
    PerProcessImageCache(uint32_t capacity, uint64_t mem_capacity, bool record_stat = true);
    ~PerProcessImageCache();

    std::shared_ptr<ImageCacheKey> create_key(uint64_t file_hash, uint64_t index) override;
    std::shared_ptr<ImageCacheValue> create_value(void* data, uint64_t size) override;

    void* allocate(std::size_t n) override;
    void lock(uint64_t index) override;
    void unlock(uint64_t index) override;

    bool insert(std::shared_ptr<ImageCacheKey> key, std::shared_ptr<ImageCacheValue> value) override;

    uint32_t size() const override;
    uint64_t memory_size() const override;

    uint32_t capacity() const override;
    uint64_t memory_capacity() const override;
    uint64_t free_memory() const override;

    void record(bool value) override;
    bool record() const override;

    uint64_t hit_count() const override;
    uint64_t miss_count() const override;

    void reserve(uint32_t new_capacity, uint64_t new_mem_capacity) override;

    std::shared_ptr<ImageCacheValue> find(const std::shared_ptr<ImageCacheKey>& key) override;
};

} // namespace cucim::cache

#endif // CUCIM_CACHE_IMAGE_CACHE_PER_PROCESS_H
