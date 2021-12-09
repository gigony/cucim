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

#ifndef CUCIM_CONCURRENT_THREADPOOL_H
#define CUCIM_CONCURRENT_THREADPOOL_H

#include "cucim/macros/api_header.h"

#include <future>
#include <thread>
#include <vector>

namespace cucim::concurrent
{

class EXPORT_VISIBLE ThreadPool
{
public:
    explicit ThreadPool(size_t num_workers);
    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;

    ~ThreadPool();

    struct Task
    {
        explicit Task(bool stop = false);
        Task(Task&& x) noexcept = default;
        Task& operator=(const Task& x) = delete;
        Task& operator=(Task&& x) noexcept = default;

        std::function<void()> function;
        // std::shared_ptr<std::packaged_task<void()>> function;
        std::promise<void> promise;
        bool stop;
    };

    bool enqueue(Task& task);

    // template <typename Func, class... Args>
    // auto run(Func&& func, const Args&... args)
    template <typename Func>
    auto run(const Func& func)
    {
        Task task;
        std::future<void> future = task.promise.get_future();

        // task.function =
        //     std::make_shared<std::packaged_task<void()>>([func, args...] { func(std::forward<const Args>(args)...);
        //     });

        task.function = [func] { func(); };
        // task.function = std::make_shared<std::packaged_task<void()>>([func] { func(); });

        enqueue(task);

        return future;
    }

private:
    static void task_runner(ThreadPool* pool);

    std::vector<std::thread> workers_;

    struct ConcurrentQueue;
    std::unique_ptr<ConcurrentQueue> tasks_;
};

} // namespace cucim::concurrent

#endif // CUCIM_CONCURRENT_THREADPOOL_H
