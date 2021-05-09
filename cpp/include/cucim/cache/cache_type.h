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

#ifndef CUCIM_CACHE_CACHE_TYPE_H
#define CUCIM_CACHE_CACHE_TYPE_H

#include <array>
#include <cstdint>
#include <string_view>

namespace cucim::cache
{

enum class CacheType : uint8_t
{
    kNoCache,
    kPerProcess,
    kSharedMemory
};

template <typename Key = std::string_view, typename Value = CacheType, std::size_t Size = 3>
struct CacheTypeMap
{
    std::array<std::pair<Key, Value>, Size> data;

    [[nodiscard]] constexpr Value at(const Key& key) const;
};

CacheType lookup_cache_type(const std::string_view sv);

} // namespace cucim::cache

#endif // CUCIM_CACHE_CACHE_TYPE_H
