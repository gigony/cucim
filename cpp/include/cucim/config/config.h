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

#ifndef CUCIM_CONFIG_CONFIG_H
#define CUCIM_CONFIG_CONFIG_H

#include "cucim/cache/cache_type.h"

#include <string>

#include <sys/types.h>
#include <unistd.h>

namespace cucim::config
{

constexpr uint32_t kDefaultCacheCapacity = 8196;
constexpr uint64_t kDefaultMemoryCapacity = 1UL * 1024 * 1024 * 3 * 920;

class Config
{
public:
    Config();

    uint32_t cache_capacity() const;
    uint64_t cache_memory_capacity() const;

    std::string shm_name() const;
    pid_t pid() const;
    pid_t ppid() const;

private:
    std::string get_config_path() const;
    bool parse_config(std::string& path);
    void set_default_configuration();

    std::string source_path_;

    cucim::cache::CacheType cache_type_ = cucim::cache::CacheType::kSharedMemory;
    uint32_t cache_capacity_ = kDefaultCacheCapacity;
    uint64_t cache_memory_capacity_ = kDefaultMemoryCapacity;
};

} // namespace cucim::config

#endif // CUCIM_CONFIG_CONFIG_H
