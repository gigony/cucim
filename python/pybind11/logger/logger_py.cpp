/*
 * Copyright (c) 2022, NVIDIA CORPORATION.
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

#include "logger_py.h"
#include "logger_pydoc.h"

#include <cucim/logger/logger.h>
#include <cucim/cuimage.h>

using namespace pybind11::literals;
namespace py = pybind11;

namespace cucim::logger
{

void init_logger(py::module& logger)
{
    py::enum_<LogLevel>(logger, "LogLevel") //
        .value("NOTSET", LogLevel::kNotSet) //
        .value("DEBUG", LogLevel::kDebug) //
        .value("INFO", LogLevel::kInfo) //
        .value("WARNING", LogLevel::kWarning) //
        .value("ERROR", LogLevel::kError) //
        .value("FATAL", LogLevel::kFatal);

    py::class_<Logger, std::shared_ptr<Logger>>(logger, "Logger")
        .def_property("config", &py_config, nullptr, doc::Logger::doc_config, py::call_guard<py::gil_scoped_release>())
        .def("level", &py_level, doc::Logger::doc_level, py::call_guard<py::gil_scoped_release>(), //
             py::arg("value") = py::none() //
        );
}

LogLevel py_level(Logger& logger, py::object value)
{
    if (value.is_none())
    {
        return logger.level();
    }
    else if (py::isinstance<py::str>(value))
    {
        LogLevel v = get_level_from_name(std::string(value.cast<py::str>()));
        logger.level(v);
        return v;
    }
    else if (py::isinstance<LogLevel>(value))
    {
        LogLevel v = value.cast<LogLevel>();
        logger.level(v);
        return v;
    }
    else
    {
        throw std::invalid_argument(fmt::format("Only 'NoneType', 'str', or 'LogLevel' is available for the argument"));
    }
}

py::dict py_config(Logger& logger)
{
    LoggerConfig& config = logger.config();

    return py::dict{
        "level"_a = py::str(get_level_name(config.level)) //
    };
}

} // namespace cucim::logger
