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

#ifndef CUCIM_LOGGER_LOGGER_H
#define CUCIM_LOGGER_LOGGER_H

#include "cucim/core/framework.h"

#include <memory>
#include <string>

#include "cucim/logger/logger_config.h"


namespace cucim::logger
{

/**
 * @brief Logger class
 *
 * Holds the logger state and provides the interface to configure it.
 *
 */

class EXPORT_VISIBLE Logger : public std::enable_shared_from_this<Logger>
{
public:
    Logger() = delete;
    Logger(LoggerConfig& config);
    virtual ~Logger(){};

    LoggerConfig& config();
    LoggerConfig get_config() const;

    void level(LogLevel value);
    void level(std::string& value);

    /**
     * @brief Return the log level
     *
     * @return log level
     */
    LogLevel level() const;

protected:
    LoggerConfig& config_;
};

} // namespace cucim::logger

#endif // CUCIM_LOGGER_LOGGER_H
