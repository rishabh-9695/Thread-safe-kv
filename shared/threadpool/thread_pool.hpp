#pragma once
#include "thread_task.hpp"
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>
#include <future>
#include <iostream>

class ThreadPool {
    private:
        std::queue<std::shared_ptr<ThreadTaskBase>> task_queue; // Queue to hold tasks
        std::vector<std::thread> workers; // Vector to hold worker threads

        std::mutex queue_mutex; // Mutex to protect access to the task queue
        std::condition_variable condition; // Condition variable to notify worker threads
        std::atomic<bool> stop; // Flag to indicate if the thread pool is stopping
    public:
        ThreadPool(size_t num_threads = std::thread::hardware_concurrency()) : stop(false) {
            for(size_t i = 0; i < num_threads; ++i) {
                workers.emplace_back([this, i=i]() {
                    // Removed worker startup logging for performance
                    while (true) {
                        std::shared_ptr<ThreadTaskBase> task;
                        {
                            std::unique_lock<std::mutex> lock(queue_mutex);
                            condition.wait(lock, [this] { return stop || !task_queue.empty(); });
                            if (stop && task_queue.empty()) return; // Exit if stopping and no tasks
                            task = std::move(task_queue.front());
                            task_queue.pop();
                        }

                        task->execute(); // Execute the task
                    }
                });
            }
        }

        template<class F, class... Args>
        auto submit(F&& f, Args&&... args) 
            -> std::future<typename std::invoke_result<F, Args...>::type> {
            // Removed task submission logging for performance
            using ReturnType = typename std::invoke_result<F, Args...>::type;

            auto task = std::make_shared<ThreadTask<ReturnType>>(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
            auto future = task->get_future();
            {
                std::unique_lock<std::mutex> lock(queue_mutex);
                if (stop) throw std::runtime_error("ThreadPool is stopping, cannot submit new tasks.");
                task_queue.push(task);
                
            }
            condition.notify_one(); // Notify one worker thread
            return future; // Return the future associated with the task
        }

        ~ThreadPool() {
            stop = true; // Set the stop flag
            condition.notify_all(); // Notify all worker threads to wake up
            for (std::thread &worker : workers) {
                if (worker.joinable()) {
                    worker.join(); // Wait for all worker threads to finish
                }
            }
        }
    };