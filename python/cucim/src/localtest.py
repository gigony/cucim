#
# Copyright (c) 2020, NVIDIA CORPORATION.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

from openslide import OpenSlide
import os
import json
import concurrent.futures
from contextlib import ContextDecorator
from time import perf_counter
from tifffile import TiffFile
import sys
import numpy as np
from cucim import CuImage

# cache = CuImage.cache("per_process", memory_capacity=1024)

img = CuImage("notebooks/input/TUPAC-TR-467.svs")

locations = [[0, 0], [100, 0], [200, 0], [300, 0],
             [0, 200], [100, 200], [200, 200], [300, 200]]
# locations = [[0, 0], [100, 0], [200, 0], [300, 0]]
locations = np.array(locations)

region = img.read_region(locations, (224, 224), batch_size=4, num_workers=8)
print(region.shape)

print("done!")


sys.exit(0)


cache = CuImage.cache("per_process", memory_capacity=1024)

img = CuImage("notebooks/input/TUPAC-TR-467.svs")


class Timer(ContextDecorator):
    def __init__(self, message):
        self.message = message
        self.end = None

    def elapsed_time(self):
        self.end = perf_counter()
        return self.end - self.start

    def __enter__(self):
        self.start = perf_counter()
        return self

    def __exit__(self, exc_type, exc, exc_tb):
        if not self.end:
            self.elapsed_time()
        print("{} : {}".format(self.message, self.end - self.start))


# with Timer("  Thread elapsed time (cuCIM)") as timer:
#     a = img.read_region(num_workers=16)
#     print(a.shape)


# with Timer("  Thread elapsed time (tifffile)") as timer:
#     with TiffFile("notebooks/input/TUPAC-TR-467.svs") as tif:
#         a = tif.asarray()
#         print(a.shape)

locations = [[0, 0], [100, 0], [200, 0], [300, 0],
             [0, 200], [100, 200], [200, 200], [300, 200]]
locations = np.array(locations)

region = img.read_region(locations, (224, 224), batch_size=4, num_workers=8)
print(region.shape)
# from cucim import CuImage

# # img = CuImage("notebooks/input/image.tif")
# # print(img.read_region((0, 0, 200, 200), (200, 200), num_workers=2).shape)

sys.exit(0)


input_file = "notebooks/input/image2.tif"

# img = CuImage(input_file)
# # True if image data is loaded & available.
# print(img.is_loaded)
# # A device type.
# print(img.device)
# # The number of dimensions.
# print(img.ndim)
# # A string containing a list of dimensions being requested.
# print(img.dims)
# # A tuple of dimension sizes (in the order of `dims`).
# print(img.shape)
# # Returns size as a tuple for the given dimension order.
# print(img.size('XYC'))
# # The data type of the image.
# print(img.dtype)
# # A channel name list.
# print(img.channel_names)
# # Returns physical size in tuple.
# print(img.spacing())
# # Units for each spacing element (size is same with `ndim`).
# print(img.spacing_units())
# # Physical location of (0, 0, 0) (size is always 3).
# print(img.origin)
# # Direction cosines (size is always 3x3).
# print(img.direction)
# # Coordinate frame in which the direction cosines are measured.
# # Available Coordinate frame is not finalized yet.
# print(img.coord_sys)
# # Returns a set of associated image names.
# print(img.associated_images)
# # Returns a dict that includes resolution information.
# print(json.dumps(img.resolutions, indent=2))
# # A metadata object as `dict`
# print(json.dumps(img.metadata, indent=2))
# # A raw metadata string.
# print(img.raw_metadata)


class Timer(ContextDecorator):
    def __init__(self, message):
        self.message = message
        self.end = None

    def elapsed_time(self):
        self.end = perf_counter()
        return self.end - self.start

    def __enter__(self):
        self.start = perf_counter()
        return self

    def __exit__(self, exc_type, exc, exc_tb):
        if not self.end:
            self.elapsed_time()
        print("{} : {}".format(self.message, self.end - self.start))


num_threads = os.cpu_count()

start_location = 0
tile_size = 256


def load_tile_openslide(slide, start_loc, tile_size):
    _ = slide.read_region(start_loc, 0, [tile_size, tile_size])


def load_tile_cucim(slide, start_loc, tile_size):
    _ = slide.read_region(start_loc, [tile_size, tile_size], 0)


openslide_tot_time = 0
cucim_tot_time = 0
for num_workers in (1,):  # range(1, num_threads + 1):
    # with OpenSlide(input_file) as slide:
    #     width, height = slide.dimensions

    #     count = 0
    #     for h in range(start_location, height, tile_size):
    #         for w in range(start_location, width, tile_size):
    #             count += 1
    #     start_loc_iter = ((w, h)
    #                       for h in range(start_location, height, tile_size)
    #                       for w in range(start_location, width, tile_size))
    #     with Timer("  Thread elapsed time (OpenSlide)") as timer:
    #         with concurrent.futures.ThreadPoolExecutor(
    #             max_workers=num_workers
    #         ) as executor:
    #             executor.map(
    #                 lambda start_loc: load_tile_openslide(
    #                     slide, start_loc, tile_size),
    #                 start_loc_iter,
    #             )
    #         openslide_time = timer.elapsed_time()
    #         openslide_tot_time += openslide_time

    cucim_time = 0
    slide = CuImage(input_file)
    width, height = slide.size('XY')
    start_loc_iter = ((w, h)
                      for h in range(start_location, height, tile_size)
                      for w in range(start_location, width, tile_size))
    with Timer("  Thread elapsed time (cuCIM)") as timer:
        with concurrent.futures.ThreadPoolExecutor(
            max_workers=num_workers
        ) as executor:
            executor.map(
                lambda start_loc: load_tile_cucim(slide, start_loc, tile_size),
                start_loc_iter,
            )
        cucim_time = timer.elapsed_time()
        cucim_tot_time += cucim_time
    # print("  Performance gain (OpenSlide/cuCIM): {}".format(
    #     openslide_time / cucim_time))

# print("Total time (OpenSlide):", openslide_tot_time)
print("Total time (cuCIM):", cucim_tot_time)
# print("Average performance gain (OpenSlide/cuCIM): {}".format(
#     openslide_tot_time / cucim_tot_time))
