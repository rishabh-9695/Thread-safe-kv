#include "thread_pool.hpp"
#include <iostream>
#include <future>
#include <thread>
#include <chrono>

int foo(int a, int b){
    std::cout << "Executing foo with arguments: " << a << " and " << b << std::endl << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    return a + b;
}

int main() {
    ThreadPool pool; // Create a thread pool with default number of threads
    std::vector<std::future<int>> futures; // Vector to hold futures for results
    std::cout << "ThreadPool created with " << std::thread::hardware_concurrency() << " threads." << std::endl;
    // Submit a task to the thread pool
    for(int i=0; i<10; i++) {
        futures.emplace_back( pool.submit(foo, i, i*2+1));
    } 

    // Wait for the result and print it
    for(auto& future : futures) {
        try {
            std::cout << "Result: " << future.get() << std::endl; // Get the result of the task
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl; // Handle any exceptions thrown by the task
        }
    }

    std::cin.get(); // Wait for user input before exiting
    return 0;
}