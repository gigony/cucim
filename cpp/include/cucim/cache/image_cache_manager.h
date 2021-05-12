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

#ifndef CUCIM_CACHE_IMAGE_CACHE_MANAGER_H
#define CUCIM_CACHE_IMAGE_CACHE_MANAGER_H

#include "cucim/core/framework.h"

#include "cucim/cache/image_cache.h"

namespace cucim::cache
{

class EXPORT_VISIBLE ImageCacheManager
{
public:
    ImageCacheManager();

    ImageCache& cache() const;
    std::shared_ptr<ImageCache> cache(const ImageCacheConfig& config);
    std::shared_ptr<ImageCache> get_cache() const;
    void reserve(uint32_t new_capacity, uint32_t new_memory_capacity);

private:
    std::unique_ptr<ImageCache> create_cache() const;
    std::unique_ptr<ImageCache> create_cache(const ImageCacheConfig& cache_config) const;

    std::shared_ptr<ImageCache> cache_;
};

} // namespace cucim::cache

#endif // CUCIM_CACHE_IMAGE_CACHE_MANAGER_H
