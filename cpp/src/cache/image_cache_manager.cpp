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

ImageCacheManager::ImageCacheManager() : cache_(create_cache())
{
}

ImageCache& ImageCacheManager::cache() const
{
    return *cache_;
}

std::shared_ptr<cucim::cache::ImageCache> ImageCacheManager::cache(const ImageCacheConfig& config)
{
    cache_ = create_cache(config);
    return cache_;
}

std::shared_ptr<cucim::cache::ImageCache> ImageCacheManager::get_cache() const
{
    return cache_;
}

void ImageCacheManager::reserve(uint32_t new_capacity, uint32_t new_memory_capacity)
{
    ImageCacheConfig cache_config;
    cache_config.capacity = new_capacity;
    cache_config.memory_capacity = new_memory_capacity;

    cache_->reserve(cache_config);
}

std::unique_ptr<ImageCache> ImageCacheManager::create_cache() const
{
    ImageCacheConfig& cache_config = cucim::CuImage::get_config()->cache();

    return create_cache(cache_config);
}

std::unique_ptr<ImageCache> ImageCacheManager::create_cache(const ImageCacheConfig& cache_config) const
{
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
