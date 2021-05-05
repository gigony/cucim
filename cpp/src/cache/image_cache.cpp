/*
 * Apache License, Version 2.0
 * Copyright 2021 NVIDIA Corporation
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

#include "cucim/cache/image_cache.h"

#include "cucim/cuimage.h"

namespace cucim::cache
{

ImageCacheKey::ImageCacheKey(uint64_t file_hash, uint64_t index) : file_hash(file_hash), location_hash(index)
{
}

ImageCacheValue::ImageCacheValue(void* data, uint64_t size, void* user_obj) : data(data), size(size), user_obj(user_obj)
{
}

ImageCacheValue::operator bool() const
{
    return data != nullptr;
}

ImageCacheItem::ImageCacheItem(void* item, std::shared_ptr<void> deleter) : item_(item), deleter_(deleter)
{
}

ImageCache::ImageCache(uint32_t, uint64_t, bool){};

} // namespace cucim::cache
