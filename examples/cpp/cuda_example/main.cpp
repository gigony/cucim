/*
 * Copyright (c) 2020, NVIDIA CORPORATION.
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

#include <cucim/cuimage.h>
#include <fmt/format.h>
#include <fmt/ranges.h>
#include <cuda_runtime.h>


__global__ void increase_values(uint8_t* inp, int n, int val)
{
    int i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < n)
    {
        int v = inp[i] + val;
        inp[i] = (v > 255) ? 255 : v;
    }
}

int main(int argc, char* argv[])
{
    // Check the number of parameters
    if (argc < 3)
    {
        fmt::print(stderr, "Usage: {} INPUT_FILE_PATH OUTPUT_FOLDER\n", argv[0]);
        return 1;
    }
    const char* input_file_path = argv[1];
    const char* output_folder_path = argv[2];

    cucim::CuImage image = cucim::CuImage(input_file_path);

    fmt::print("is_loaded: {}\n", image.is_loaded());
    fmt::print("device: {}\n", std::string(image.device()));
    fmt::print("metadata: {}\n", image.metadata());
    fmt::print("dims: {}\n", image.dims());
    fmt::print("shape: ({})\n", fmt::join(image.shape(), ", "));
    fmt::print("size('XYC'): ({})\n", fmt::join(image.size("XYC"), ", "));
    fmt::print("channel_names: ({})\n", fmt::join(image.channel_names(), ", "));

    auto resolutions = image.resolutions();
    fmt::print("level_count: {}\n", resolutions.level_count());
    fmt::print("level_dimensions: ({})\n", fmt::join(resolutions.level_dimensions(), ", "));
    fmt::print("level_dimension (level 0): ({})\n", fmt::join(resolutions.level_dimension(0), ", "));
    fmt::print("level_downsamples: ({})\n", fmt::join(resolutions.level_downsamples(), ", "));

    auto associated_images = image.associated_images();
    fmt::print("associated_images: ({})\n", fmt::join(associated_images, ", "));

    fmt::print("#macro\n");
    auto associated_image = image.associated_image("macro");
    fmt::print("is_loaded: {}\n", associated_image.is_loaded());
    fmt::print("device: {}\n", std::string(associated_image.device()));
    fmt::print("metadata: {}\n", associated_image.metadata());
    fmt::print("dims: {}\n", associated_image.dims());
    fmt::print("shape: ({})\n", fmt::join(associated_image.shape(), ", "));
    fmt::print("size('XYC'): ({})\n", fmt::join(associated_image.size("XYC"), ", "));
    fmt::print("channel_names: ({})\n", fmt::join(associated_image.channel_names(), ", "));
    fmt::print("\n");

    cucim::CuImage region =
        image.read_region({ 10000, 10000 }, { 1024, 1024 }, 0, cucim::DimIndices{}, "cpu", nullptr, "");

    fmt::print("is_loaded: {}\n", region.is_loaded());
    fmt::print("device: {}\n", std::string(region.device()));
    fmt::print("metadata: {}\n", region.metadata());
    fmt::print("dims: {}\n", region.dims());
    fmt::print("shape: ({})\n", fmt::join(region.shape(), ", "));
    fmt::print("size('XY'): ({})\n", fmt::join(region.size("XY"), ", "));
    fmt::print("channel_names: ({})\n", fmt::join(region.channel_names(), ", "));

    resolutions = region.resolutions();
    fmt::print("level_count: {}\n", resolutions.level_count());
    fmt::print("level_dimensions: ({})\n", fmt::join(resolutions.level_dimensions(), ", "));
    fmt::print("level_dimension (level 0): ({})\n", fmt::join(resolutions.level_dimension(0), ", "));
    fmt::print("level_downsamples: ({})\n", fmt::join(resolutions.level_downsamples(), ", "));

    associated_images = region.associated_images();
    fmt::print("associated_images: ({})\n", fmt::join(associated_images, ", "));
    fmt::print("\n");

    auto container = region.container();
    size_t image_size_nbytes = container.size();

    auto tensor = static_cast<DLTensor>(container);

    uint8_t* h_image = static_cast<uint8_t*>(tensor.data);
    uint8_t* d_image = nullptr;

    cudaMalloc(&d_image, image_size_nbytes);
    cudaMemcpy(d_image, h_image, image_size_nbytes, ::cudaMemcpyHostToDevice);

    int N = image_size_nbytes;
    int M = 256;

    fmt::print("before: {}\n", h_image[0]);
    region.save(fmt::format("{}/output.ppm", output_folder_path));

    increase_values<<<(N + M - 1) / M, M>>>(d_image, N, 50);
    cudaDeviceSynchronize();
    cudaMemcpy(h_image, d_image, image_size_nbytes, ::cudaMemcpyDeviceToHost);

    fmt::print("after: {}\n", h_image[0]);
    region.save(fmt::format("{}/output2.ppm", output_folder_path));

    cudaFree(d_image);

    return 0;
}
