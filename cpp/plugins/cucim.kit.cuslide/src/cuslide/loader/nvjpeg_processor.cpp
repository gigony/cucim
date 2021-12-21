/*
 * Apache License, Version 2.0
 * Copyright 2021 NVIDIA Corporation
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

#include "nvjpeg_processor.h"

#include "cucim/cache/image_cache_manager.h"
#include "cucim/io/device.h"

namespace cuslide::loader
{

NvJpegProcessor::NvJpegProcessor(uint32_t maximum_tile_count) : cucim::loader::BatchDataProcessor()
{
    if (maximum_tile_count > 1)
    {
        // Calculate nearlest power of 2 that is equal or larger than the given number.
        // (Test with https://godbolt.org/z/n7qhPYzfP)
        int next_candidate = maximum_tile_count & (maximum_tile_count - 1);
        if (next_candidate > 0)
        {
            maximum_tile_count <<= 1;
            while (true)
            {
                next_candidate = maximum_tile_count & (maximum_tile_count - 1);
                if (next_candidate == 0)
                {
                    break;
                }
                maximum_tile_count = next_candidate;
            }
        }

        uint32_t cuda_batch_size = maximum_tile_count;

        // Update prefetch_factor
        // (We can decode/cache tiles at least two times of the number of tiles needed for nvjpeg)
        // E.g., (128 - 1) / 32 + 1 ~= 4 => 8 (for 256 tiles) for cuda_batch_size(=128) and batch_size(=32)
        preferred_loader_prefetch_factor_ = ((cuda_batch_size - 1) / batch_size_ + 1) * 2;

        // Create cuda image cache
        cucim::cache::ImageCacheConfig cache_config{};
        cache_config.type = cucim::cache::CacheType::kPerProcess;
        cache_config.memory_capacity = 1024 * 1024; // 1TB: set to fairly large memory so that memory_capacity is not a
                                                    // limiter.
        cache_config.capacity = cuda_batch_size; // limit the number of cache item to cuda_batch_size
        cuda_image_cache_ =
            std::move(cucim::cache::ImageCacheManager::create_cache(cache_config, cucim::io::DeviceType::kCUDA));
    }
}

NvJpegProcessor::~NvJpegProcessor()
{
}

uint32_t NvJpegProcessor::preferred_loader_prefetch_factor()
{
    return preferred_loader_prefetch_factor_;
}

} // namespace cuslide::loader
