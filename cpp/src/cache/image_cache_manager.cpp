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

void ImageCacheManager::reserve(uint32_t new_capacity, uint64_t new_mem_capacity)
{
    cache_->reserve(new_capacity, new_mem_capacity);
}

uint32_t ImageCacheManager::default_capacity() const
{
    return cucim::CuImage::get_config()->cache_capacity();
}

uint64_t ImageCacheManager::default_memory_capacity() const
{
    return cucim::CuImage::get_config()->cache_memory_capacity();
}

std::unique_ptr<ImageCache> ImageCacheManager::create_cache() const
{
    auto cache = std::make_unique<EmptyImageCache>(default_capacity(), default_memory_capacity());
    return cache;
}

} // namespace cucim::cache
