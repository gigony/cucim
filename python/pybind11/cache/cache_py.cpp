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

#include "init.h"
#include "cache_pydoc.h"
#include "image_cache_py.h"
#include "image_cache_pydoc.h"

#include <cucim/cache/image_cache.h>

using namespace pybind11::literals;
namespace py = pybind11;

namespace cucim::cache
{

void init_cache(py::module& cache)
{
    py::enum_<CacheType>(cache, "CacheType") //
        .value("NoCache", CacheType::kNoCache) //
        .value("PerProcess", CacheType::kPerProcess) //
        .value("SharedMemory", CacheType::kSharedMemory);

    py::class_<ImageCache, std::shared_ptr<ImageCache>>(cache, "ImageCache")
        .def_property(
            "type", &ImageCache::type, nullptr, doc::ImageCache::doc_type, py::call_guard<py::gil_scoped_release>())
        .def_property("config", &py_config, nullptr, doc::ImageCache::doc_type, py::call_guard<py::gil_scoped_release>())
        .def_property(
            "size", &ImageCache::size, nullptr, doc::ImageCache::doc_size, py::call_guard<py::gil_scoped_release>())
        .def_property("memory_size", &ImageCache::memory_size, nullptr, doc::ImageCache::doc_memory_size,
                      py::call_guard<py::gil_scoped_release>())
        .def_property("capacity", &ImageCache::capacity, nullptr, doc::ImageCache::doc_capacity,
                      py::call_guard<py::gil_scoped_release>())
        .def_property("memory_capacity", &ImageCache::memory_capacity, nullptr, doc::ImageCache::doc_memory_capacity,
                      py::call_guard<py::gil_scoped_release>())
        .def_property("free_memory", &ImageCache::free_memory, nullptr, doc::ImageCache::doc_free_memory,
                      py::call_guard<py::gil_scoped_release>())
        .def("record", &py_record, doc::ImageCache::doc_record, py::call_guard<py::gil_scoped_release>(), //
             py::arg("value") = py::none())
        .def_property("hit_count", &ImageCache::hit_count, nullptr, doc::ImageCache::doc_hit_count,
                      py::call_guard<py::gil_scoped_release>())
        .def_property("miss_count", &ImageCache::miss_count, nullptr, doc::ImageCache::doc_miss_count,
                      py::call_guard<py::gil_scoped_release>())
        .def("reserve", &ImageCache::reserve, doc::ImageCache::doc_reserve, py::call_guard<py::gil_scoped_release>());
}

bool py_record(ImageCache& cache, py::object value)
{
    if (value.is_none())
    {
        return cache.record();
    }
    else if (py::isinstance<py::bool_>(value))
    {
        py::bool_ v = value.cast<py::bool_>();
        cache.record(v);
        return v;
    }
    else
    {
        throw std::invalid_argument(fmt::format("Only 'NoneType' or 'bool' is available for the argument"));
    }
}

py::dict py_config(ImageCache& cache)
{
    ImageCacheConfig& config = cache.config();

    return py::dict{
        "type"_a = pybind11::str(std::string(lookup_cache_type_str(config.type))), //
        "memory_capacity"_a = pybind11::int_(config.memory_capacity), //
        "capacity"_a = pybind11::int_(config.capacity), //
        "mutex_pool_capacity"_a = pybind11::int_(config.mutex_pool_capacity), //
        "list_padding"_a = pybind11::int_(config.list_padding), //
        "extra_shared_memory_size"_a = pybind11::int_(config.extra_shared_memory_size), //
        "record_stat"_a = pybind11::bool_(config.record_stat) //
    };
}

} // namespace cucim::cache