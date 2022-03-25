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

#include "cucim/logger/logger_config.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace cucim::logger
{

std::string get_level_name(LogLevel level)
{
    switch (level)
    {
    case LogLevel::kDebug:
        return "debug";
    case LogLevel::kInfo:
        return "info";
    case LogLevel::kWarning:
        return "warning";
    case LogLevel::kError:
        return "error";
    case LogLevel::kFatal:
        return "fatal";
    default:
        return "";
    }
}

LogLevel get_level_from_name(const std::string_view& level_name)
{
    if (level_name == "debug")
    {
        return LogLevel::kDebug;
    }
    else if (level_name == "info")
    {
        return LogLevel::kInfo;
    }
    else if (level_name == "warning")
    {
        return LogLevel::kWarning;
    }
    else if (level_name == "error")
    {
        return LogLevel::kError;
    }
    else if (level_name == "fatal")
    {
        return LogLevel::kFatal;
    }
    else
    {
        return LogLevel::kNotSet;
    }
}

void LoggerConfig::load_config(const void* json_obj)
{
    const json& logger_config = *(static_cast<const json*>(json_obj));

    if (logger_config.contains("level") && logger_config["level"].is_string())
    {
        const std::string& level_str = logger_config.value("level", "error");
        level = get_level_from_name(level_str);
    }
}

} // namespace cucim::logger
