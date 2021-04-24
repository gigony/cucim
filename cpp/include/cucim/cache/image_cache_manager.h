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
#include <cucim/cache/image_cache.h>

namespace cucim::cache
{

constexpr uint32_t DEFAULT_CAPACITY = 1024;
constexpr uint64_t DEFAULT_MEMORY_CAPACITY = 1024 * 1024 * 3 * 50;

class EXPORT_VISIBLE ImageCacheManager
{
public:
    ImageCacheManager();

    ImageCache& get_cache();
    void reserve(uint32_t capacity, uint64_t mem_capacity);

    uint32_t default_capacity() const;
    uint64_t default_memory_capacity() const;

private:
    ImageCache cache_{ DEFAULT_CAPACITY, DEFAULT_MEMORY_CAPACITY };
};

} // namespace cucim::cache

#endif // CUCIM_CACHE_IMAGE_CACHE_MANAGER_H
