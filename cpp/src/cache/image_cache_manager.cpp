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

#include "cucim/cache/image_cache_manager.h"

#include "image_cache_empty.h"
#include "image_cache_per_process.h"
#include "image_cache_shared_memory.h"
#include "cucim/cuimage.h"

#include <cstdlib>
#include <fmt/format.h>


namespace cucim::cache
{

ImageCacheManager::ImageCacheManager() : cache_(std::move(create_cache()))
{
}

cucim::cache::ImageCache* ImageCacheManager::get_cache()
{
    return cache_.get();
}

void ImageCacheManager::reserve(uint32_t new_capacity, uint64_t new_memory_capacity)
{
    ImageCacheConfig cache_config;
    cache_config.capacity = new_capacity;
    cache_config.memory_capacity = new_memory_capacity;

    cache_->reserve(cache_config);
}

CacheType ImageCacheManager::default_type() const
{
    return cucim::CuImage::get_config()->cache_type();
}

uint32_t ImageCacheManager::default_capacity() const
{
    return cucim::CuImage::get_config()->cache_capacity();
}

uint64_t ImageCacheManager::default_memory_capacity() const
{
    return cucim::CuImage::get_config()->cache_memory_capacity();
}

uint32_t ImageCacheManager::default_mutex_pool_capacity() const
{
    return cucim::CuImage::get_config()->cache_mutex_pool_capacity();
}

uint32_t ImageCacheManager::default_list_padding() const
{
    return cucim::CuImage::get_config()->cache_list_padding();
}

uint32_t ImageCacheManager::default_extra_shared_memory_size() const
{
    return cucim::CuImage::get_config()->cache_extra_shared_memory_size();
}

bool ImageCacheManager::default_record_stat() const
{
    return cucim::CuImage::get_config()->cache_record_stat();
}

std::unique_ptr<ImageCache> ImageCacheManager::create_cache() const
{
    ImageCacheConfig cache_config;
    cache_config.type = default_type();
    cache_config.capacity = default_capacity();
    cache_config.memory_capacity = default_memory_capacity();
    cache_config.mutex_pool_capacity = default_mutex_pool_capacity();
    cache_config.list_padding = default_list_padding();
    cache_config.extra_shared_memory_size = default_extra_shared_memory_size();
    cache_config.record_stat = default_record_stat();

    switch (cache_config.type)
    {
    case CacheType::kNoCache:
        return std::make_unique<EmptyImageCache>(cache_config);
    case CacheType::kPerProcess:
        return std::make_unique<PerProcessImageCache>(cache_config);
    case CacheType::kSharedMemory:
        return std::make_unique<SharedMemoryImageCache>(cache_config);
    default:
        return std::make_unique<EmptyImageCache>(cache_config);
    }
}

} // namespace cucim::cache
