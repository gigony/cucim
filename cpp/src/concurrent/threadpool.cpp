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

#include "cucim/concurrent/threadpool.h"

#include <fmt/format.h>
#include <taskflow/taskflow.hpp>

namespace cucim::concurrent
{

struct ThreadPool::TaskQueue : public tf::Taskflow
{
    // inherits  Constructor
    using tf::Taskflow::Taskflow;
};

struct ThreadPool::Executor : public tf::Executor
{
    // inherits  Constructor
    using tf::Executor::Executor;
};


ThreadPool::ThreadPool(size_t num_workers)
{
    if (num_workers > 0)
    {
        // num_workers = std::thread::hardware_concurrency();
        tasks_ = std::make_unique<TaskQueue>();
        executor_ = std::make_unique<Executor>(num_workers);
    }
}

ThreadPool::~ThreadPool()
{
    if (tasks_)
    {
        executor_->run(*tasks_).wait();
    }
}

bool ThreadPool::enqueue(std::function<void()> task)
{
    auto t = tasks_->emplace([task]() { task(); });
    return !t.empty();
}

void ThreadPool::wait()
{
    if (tasks_)
    {
        executor_->run(*tasks_).wait();
    }
}

} // namespace cucim::concurrent
