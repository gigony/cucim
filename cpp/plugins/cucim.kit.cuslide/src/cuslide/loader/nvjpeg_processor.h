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
#ifndef CUSLIDE_NVJPEG_PROCESSOR_H
#define CUSLIDE_NVJPEG_PROCESSOR_H


#include <condition_variable>
#include <memory>
#include <vector>

#include <nvjpeg.h>

#include <cucim/cache/image_cache.h>
#include <cucim/io/device.h>
#include <cucim/loader/batch_data_processor.h>


namespace cuslide::loader
{

class NvJpegProcessor : public cucim::loader::BatchDataProcessor
{
public:
    NvJpegProcessor(uint32_t maximum_tile_count);
    ~NvJpegProcessor();

    uint32_t preferred_loader_prefetch_factor();

private:
    uint32_t preferred_loader_prefetch_factor_ = 2;
    uint32_t batch_size_ = 1;
    std::deque<uint32_t> indices_;

    std::condition_variable cuda_batch_cond_;
    std::unique_ptr<cucim::cache::ImageCache> cuda_image_cache_;

    std::vector<const unsigned char*> raw_cuda_inputs_;
    std::vector<size_t> raw_cuda_intputs_len;
    std::vector<nvjpegImage_t> raw_cuda_outputs_;

    std::vector<cudaStream_t> streams_;
};

} // namespace cuslide::loader


#endif // CUSLIDE_NVJPEG_PROCESSOR_H
