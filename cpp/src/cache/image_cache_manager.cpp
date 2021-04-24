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

#include <cstdlib>


namespace cucim::cache
{

ImageCacheManager::ImageCacheManager() : cache_(default_capacity(), default_memory_capacity())
{
    // Read config from cucim.json (current, /home/.config/cucim/cucim.json)
    // Override environment variable
}

cucim::cache::ImageCache& ImageCacheManager::get_cache()
{
    return cache_;
}

void ImageCacheManager::reserve(uint32_t new_capacity, uint64_t new_mem_capacity)
{
    cache_.reserve(new_capacity, new_mem_capacity);
}

uint32_t ImageCacheManager::default_capacity() const
{
    // access
    return DEFAULT_CAPACITY;
}
uint64_t ImageCacheManager::default_memory_capacity() const
{
    return DEFAULT_MEMORY_CAPACITY;
}

} // namespace cucim::cache

// std::string plugin_path = default_value;
// if (const char* env_p = std::getenv("CUCIM_TEST_PLUGIN_PATH"))
// {
//     plugin_path = env_p;
// }
