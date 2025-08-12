#include <future>
#include <functional>
#include <memory>
#include <iostream>

class ThreadTaskBase {
    public:
        virtual ~ThreadTaskBase() = default;
        virtual void execute() = 0; // Pure virtual function to execute the task
};

template <typename ReturnType>
class ThreadTask : public ThreadTaskBase{ 
    private:
        std::packaged_task<ReturnType()> task;
        std::future<ReturnType> future; // Future to retrieve the result of the task
        
    public:
        explicit ThreadTask(std::function<ReturnType()> func)
            : task(std::move(func)), future(task.get_future()) {};
        
        void execute() override {
            // Removed task execution logging for performance
            task(); // Execute the task
        }

        std::future<ReturnType> get_future() {
            return std::move(future); // Return the future associated with the task
        }
};