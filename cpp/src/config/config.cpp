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

#include "cucim/config/config.h"

#include "cucim/cache/cache_type.h"
#include "cucim/util/file.h"

#include <fmt/format.h>
#include <nlohmann/json.hpp>

#include <iostream>
#include <fstream>
#include <filesystem>

using json = nlohmann::json;

namespace cucim::config
{

Config::Config()
{
    std::string config_path = get_config_path();

    bool is_configured_from_file = false;
    if (!config_path.empty())
    {
        is_configured_from_file = parse_config(config_path);
    }
    if (is_configured_from_file)
    {
        source_path_ = config_path;
    }
    else
    {
        set_default_configuration();
    }
}

cucim::cache::CacheType Config::cache_type() const
{
    return cache_type_;
}
uint32_t Config::cache_capacity() const
{
    return cache_capacity_;
}

uint64_t Config::cache_memory_capacity() const
{
    return cache_memory_capacity_;
}

uint32_t Config::cache_mutex_pool_capacity() const
{
    return cache_mutex_pool_capacity_;
}

uint32_t Config::cache_list_padding() const
{
    return cache_list_padding_;
}

bool Config::cache_record_stat() const
{
    return cache_record_stat_;
}

std::string Config::shm_name() const
{
    return fmt::format("cucim-shm.{}", pid());
}


pid_t Config::pid() const
{
    return getpid();
}
pid_t Config::ppid() const
{
    return getppid();
}


std::string Config::get_config_path() const
{
    // Read config file from:
    //   1. A path specified by 'CUCIM_CONFIG_PATH'
    //   2. (current folder)/.cucim.json
    //   3. $HOME/.cucim.json
    std::string config_path;

    if (const char* env_p = std::getenv("CUCIM_CONFIG_PATH"))
    {
        if (cucim::util::file_exists(env_p))
        {
            config_path = env_p;
        }
    }
    if (config_path.empty() && cucim::util::file_exists(kDefaultConfigFileName))
    {
        config_path = kDefaultConfigFileName;
    }
    if (config_path.empty())
    {
        if (const char* env_p = std::getenv("HOME"))
        {
            if (cucim::util::file_exists(kDefaultConfigFileName))
            {
                config_path = env_p;
            }
        }
    }
    return config_path;
}
bool Config::parse_config(std::string& path)
{
    try
    {
        std::ifstream ifs(path);
        json obj = json::parse(ifs, nullptr /*cb*/, true /*allow_exceptions*/, true /*ignore_comments*/);
        json cache = obj["cache"];
        if (cache.is_object())
        {
            if (cache["type"].is_string())
            {
                auto cache_type = cache.value("type", kDefaultCacheType);
                cache_type_ = cucim::cache::lookup_cache_type(cache_type);
            }
            if (cache["memory_capacity"].is_number_unsigned())
            {
                cache_memory_capacity_ = cache.value("memory_capacity", kDefaultCacheMemoryCapacity) * kOneMiB;
                cache_capacity_ = calc_default_cache_capacity(cache_memory_capacity_);
            }
            if (cache["capacity"].is_number_unsigned())
            {
                cache_capacity_ = cache.value("capacity", calc_default_cache_capacity(cache_memory_capacity_));
            }
            if (cache["mutex_pool_capacity"].is_number_unsigned())
            {
                cache_mutex_pool_capacity_ = cache.value("mutex_pool_capacity", kDefaultCacheMutexPoolCapacity);
            }
            if (cache["list_padding"].is_number_unsigned())
            {
                cache_list_padding_ = cache.value("list_padding", kDefaultCacheListPadding);
            }
            if (cache["record_stat"].is_boolean())
            {
                cache_record_stat_ = cache.value("record_stat", kDefaultCacheRecordStat);
            }
            fmt::print("# cache_capacity: {}\n", cache_capacity_);
            fmt::print("# cache_memory_capacity: {}\n", cache_memory_capacity_);
            fmt::print("# cache_mutex_pool_capacity: {}\n", cache_mutex_pool_capacity_);
            fmt::print("# cache_list_padding: {}\n", cache_list_padding_);
        }
    }
    catch (const json::parse_error& e)
    {
        fmt::print(stderr,
                   "Failed to load configuration file: {}\n"
                   "  message: {}\n"
                   "  exception id: {}\n"
                   "  byte position of error: {}\n",
                   path, e.what(), e.id, e.byte);
        return false;
    }
    return true;
}
void Config::set_default_configuration()
{
    // Override if the initializer of Config class is not enough.
}

} // namespace cucim::config