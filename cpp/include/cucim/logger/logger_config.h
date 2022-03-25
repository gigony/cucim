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

#ifndef CUCIM_LOGGER_LOGGER_CONFIG_H
#define CUCIM_LOGGER_LOGGER_CONFIG_H

#include "cucim/core/framework.h"

namespace cucim::logger
{

enum class LogLevel : uint8_t
{
    kNotSet = 0,
    kDebug,
    kInfo,
    kWarning,
    kError,
    kFatal,
    kNumLevels
};

constexpr LogLevel kDefaultLoggerLevel = LogLevel::kWarning;

EXPORT_VISIBLE std::string get_level_name(LogLevel level);
EXPORT_VISIBLE LogLevel get_level_from_name(const std::string_view& level_name);

struct EXPORT_VISIBLE LoggerConfig
{
    void load_config(const void* json_obj);

    LogLevel level = kDefaultLoggerLevel;
};

} // namespace cucim::logger

#endif // CUCIM_LOGGER_LOGGER_CONFIG_H
