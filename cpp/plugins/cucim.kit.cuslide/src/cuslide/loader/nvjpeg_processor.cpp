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

#include <vector>

#include <cucim/cache/image_cache_manager.h>
#include <cucim/io/device.h>
#include <cucim/util/cuda.h>
#include <fmt/format.h>

#define ALIGN_UP(x, align_to) (((uint64_t)(x) + ((uint64_t)(align_to)-1)) & ~((uint64_t)(align_to)-1))
#define ALIGN_DOWN(x, align_to) ((uint64_t)(x) & ~((uint64_t)(align_to)-1))

namespace cuslide::loader
{

NvJpegProcessor::NvJpegProcessor(CuCIMFileHandle* file_handle,
                                 const cuslide::tiff::IFD* ifd,
                                 uint32_t batch_size,
                                 uint32_t maximum_tile_count,
                                 const uint8_t* jpegtable_data,
                                 const uint32_t jpegtable_size)
    : cucim::loader::BatchDataProcessor(batch_size), file_handle_(file_handle), ifd_(ifd)
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
        // (We can decode/cache tiles at least two times of the number of tiles for batch decoding)
        // E.g., (128 - 1) / 32 + 1 ~= 4 => 8 (for 256 tiles) for cuda_batch_size(=128) and batch_size(=32)
        preferred_loader_prefetch_factor_ = ((cuda_batch_size - 1) / batch_size_ + 1) * 2;

        // Create cuda image cache
        cucim::cache::ImageCacheConfig cache_config{};
        cache_config.type = cucim::cache::CacheType::kPerProcess;
        cache_config.memory_capacity = 1024 * 1024; // 1TB: set to fairly large memory so that memory_capacity is not a
                                                    // limiter.
        cache_config.capacity = cuda_batch_size * 2; // limit the number of cache item to cuda_batch_size * 2
        cuda_image_cache_ =
            std::move(cucim::cache::ImageCacheManager::create_cache(cache_config, cucim::io::DeviceType::kCUDA));

        cuda_batch_size_ = cuda_batch_size;

        // Initialize nvjpeg
        cudaError_t cuda_status;

        // cudaDeviceProp props;
        // CUDA_ERROR(cudaFree(0));
        // CUDA_ERROR(cudaSetDevice(dev_));
        // CUDA_ERROR(cudaGetDeviceProperties(&props, dev_));

        if (NVJPEG_STATUS_SUCCESS != nvjpegCreate(backend_, NULL, &handle_))
        {
            throw std::runtime_error(fmt::format("NVJPEG initialization error"));
        }
        if (NVJPEG_STATUS_SUCCESS != nvjpegJpegStateCreate(handle_, &state_))
        {
            throw std::runtime_error(fmt::format("JPEG state initialization error"));
        }

        nvjpegDecodeBatchedParseJpegTables(handle_, state_, jpegtable_data, jpegtable_size);
        nvjpegDecodeBatchedInitialize(handle_, state_, cuda_batch_size_, 1, output_format_);

        CUDA_ERROR(cudaStreamCreateWithFlags(&stream_, cudaStreamNonBlocking));

        raw_cuda_inputs_.reserve(cuda_batch_size_);
        raw_cuda_inputs_len_.reserve(cuda_batch_size_);

        for (uint32_t i = 0; i < cuda_batch_size_; ++i)
        {
            raw_cuda_outputs_.emplace_back(); // add all-zero nvjpegImage_t object
        }

        cufile_ = cucim::filesystem::open(file_handle->fd, true /* no_gds */);
        file_start_offset_ = 0;
        tile_width_bytes_ = ifd->tile_width() * ifd->pixel_size_nbytes();
        tile_height_ = ifd->tile_height();

        struct stat sb;
        fstat(file_handle_->fd, &sb);
        uint64_t file_size = sb.st_size;

        constexpr int BLOCK_SECTOR_SIZE = 4096;
        unaligned_host_ = static_cast<uint8_t*>(cucim_malloc(file_size + BLOCK_SECTOR_SIZE * 2));
        aligned_host_ = reinterpret_cast<uint8_t*>(ALIGN_UP(unaligned_host_, BLOCK_SECTOR_SIZE));

        // Read whole data
        cufile_->pread(aligned_host_, file_size, file_start_offset_);
    }
}

NvJpegProcessor::~NvJpegProcessor()
{
    cudaError_t cuda_status;
    if (unaligned_host_)
    {
        cucim_free(unaligned_host_);
        unaligned_host_ = nullptr;
    }
    // CUDA_ERROR(cudaFree(unaligned_device));

    for (uint32_t i = 0; i < cuda_batch_size_; i++)
    {
        CUDA_ERROR(cudaFree(raw_cuda_outputs_[i].channel[0]));
    }
}

uint32_t NvJpegProcessor::request(std::deque<uint32_t> batch_item_counts, uint32_t num_remaining_items)
{
    (void)batch_item_counts;
    (void)num_remaining_items;
    std::vector<cucim::loader::TileInfo> tile_to_request;
    for (auto tile : tiles_)
    {
        auto index = tile.index;
        if (tile_to_request.size() >= cuda_batch_size_)
        {
            break;
        }
        if (cache_tile_map_.find(index) == cache_tile_map_.end())
        {
            if (tile.size == 0)
            {
                continue;
            }
            cache_tile_queue_.push(index);
            cache_tile_map_.emplace(index, tile);
            tile_to_request.emplace_back(tile);
        }
    }

    cudaError_t cuda_status;

    if (raw_cuda_inputs_.empty())
    {
        // Initialize batch data with the first data
        for (uint32_t i = 0; i < cuda_batch_size_; ++i)
        {
            uint8_t* mem_offset = aligned_host_ + tile_to_request[0].offset - file_start_offset_;
            raw_cuda_inputs_.push_back((const unsigned char*)mem_offset);
            raw_cuda_inputs_len_.push_back(tile_to_request[0].size);
            CUDA_ERROR(cudaMallocPitch(
                &raw_cuda_outputs_[i].channel[0], &raw_cuda_outputs_[i].pitch[0], tile_width_bytes_, tile_height_));
        }
    }

    size_t request_count = tile_to_request.size();
    for (uint32_t i = 0; i < request_count; ++i)
    {
        uint8_t* mem_offset = aligned_host_ + tile_to_request[i].offset - file_start_offset_;
        raw_cuda_inputs_[i] = mem_offset;
        raw_cuda_inputs_len_[i] = tile_to_request[i].size;
    }

    CUDA_ERROR(cudaStreamSynchronize(stream_));

    int error_code = nvjpegDecodeBatched(
        handle_, state_, raw_cuda_inputs_.data(), raw_cuda_inputs_len_.data(), raw_cuda_outputs_.data(), stream_);
    if (NVJPEG_STATUS_SUCCESS != error_code)
    {
        throw std::runtime_error(fmt::format("Error in batched decode: {}", error_code));
    }
    CUDA_ERROR(cudaStreamSynchronize(stream_));

    processed_cuda_batch_count_ = 1;

    cuda_batch_cond_.notify_all();
    return 0;

    // // uint8_t* tile_data;
    // auto key = cuda_image_cache_->create_key(0, index);
    // cuda_image_cache_->lock(index);

    // auto value = cuda_image_cache_->find(key);
    // if (value)
    // {
    //     image_cache.unlock(index);
    //     // tile_data = static_cast<uint8_t*>(value->data);
    // }
    // else
    // {
    //     tile_to_request.push_back(index);
    //     value = cuda_image_cache_.create_value(tile_data, tile_raster_nbytes);
    //                 image_cache.insert(key, value);
    //                 image_cache.unlock(index_hash);
    // }
    // }
}

void NvJpegProcessor::wait_for_processing()
{
    std::unique_lock<std::mutex> lock(cuda_batch_mutex_);
    cuda_batch_cond_.wait(lock, [this] { return processed_cuda_batch_count_ > 0; });
}

uint32_t NvJpegProcessor::preferred_loader_prefetch_factor()
{
    return preferred_loader_prefetch_factor_;
}

} // namespace cuslide::loader
