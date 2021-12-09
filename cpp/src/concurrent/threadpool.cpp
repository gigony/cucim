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

#include <blockingconcurrentqueue.h>
#include <fmt/format.h>

namespace cucim::concurrent
{

struct ThreadPool::ConcurrentQueue : public moodycamel::BlockingConcurrentQueue<ThreadPool::Task>
{
};


ThreadPool::ThreadPool(size_t num_workers) : tasks_(std::make_unique<ConcurrentQueue>())
{
    workers_.reserve(num_workers);
    for (size_t i = 0; i != num_workers; ++i)
    {
        workers_.push_back(std::thread(&task_runner, this));
    }
}

ThreadPool::~ThreadPool()
{
    for (size_t i = 0; i != workers_.size(); ++i)
    {
        tasks_->enqueue(Task(true)); // stop task
    }
    for (auto& worker : workers_)
    {
        worker.join();
    }
}

ThreadPool::Task::Task(bool stop /* = false */) : stop(stop)
{
}

bool ThreadPool::enqueue(Task& task)
{
    return tasks_->enqueue(std::move(task));
}

void ThreadPool::task_runner(ThreadPool* pool)
{
    Task task;
    moodycamel::ConsumerToken tok(*(pool->tasks_));
    while (true)
    {
        pool->tasks_->wait_dequeue(tok, task);
        if (task.stop)
        {
            break;
        }
        // (*task.function)();
        task.function();
        task.promise.set_value();
    }
}

} // namespace cucim::concurrent
