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
#ifndef PYCUCIM_LOGGER_LOGGER_PYDOC_H
#define PYCUCIM_LOGGER_LOGGER_PYDOC_H

#include "../macros.h"

namespace cucim::logger::doc::Logger
{

PYDOC(config, R"doc(
Returns the dictionary of configuration.
)doc")

// void level(LogLevel value);
// LogLevel level() const;
PYDOC(level, R"doc(
Log level
)doc")

} // namespace cucim::logger::doc::Logger

#endif // PYCUCIM_LOGGER_LOGGER_PYDOC_H
