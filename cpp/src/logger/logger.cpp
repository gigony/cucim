/*
 * Apache License, Version 2.0
 * Copyright 2022 NVIDIA Corporation
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

#include "cucim/logger/logger.h"

#include "cucim/cuimage.h"

namespace cucim::logger
{

Logger::Logger(LoggerConfig& config) : config_(config){};

LoggerConfig& Logger::config()
{
    return config_;
}

LoggerConfig Logger::get_config() const
{
    return config_;
}

void Logger::level(LogLevel value)
{
    config_.level = value;
}

void Logger::level(std::string& value)
{
    config_.level = get_level_from_name(value);
}

LogLevel Logger::level() const
{
    return config_.level;
}

} // namespace cucim::logger
