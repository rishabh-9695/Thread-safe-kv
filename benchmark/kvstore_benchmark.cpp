#include <grpcpp/grpcpp.h>
#include "../shard_node/kvstore.grpc.pb.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <atomic>
#include <iomanip>

class KVStoreBenchmark {
private:
    std::shared_ptr<grpc::Channel> channel_;
    std::atomic<int> successful_ops_{0};
    std::atomic<int> failed_ops_{0};

public:
    KVStoreBenchmark(std::shared_ptr<grpc::Channel> channel)
        : channel_(channel) {}

    // Benchmark PUT operations
    void benchmarkPut(int num_operations, int num_threads) {
        std::cout << "\n=== PUT Benchmark ===" << std::endl;
        std::cout << "Operations: " << num_operations << ", Threads: " << num_threads << std::endl;
        
        successful_ops_ = 0;
        failed_ops_ = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        int ops_per_thread = num_operations / num_threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread]() {
                // Create stub per thread for better connection reuse
                auto stub = kvstore::KVStore::NewStub(channel_);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(1, 1000000);
                
                for (int i = 0; i < ops_per_thread; ++i) {
                    kvstore::PutRequest request;
                    kvstore::PutResponse response;
                    grpc::ClientContext context;
                    
                    std::string key = "key_" + std::to_string(t) + "_" + std::to_string(i);
                    std::string value = "value_" + std::to_string(dis(gen));
                    
                    request.set_key(key);
                    request.set_value(value);
                    request.set_ttl_ms(0); // No TTL
                    
                    grpc::Status status = stub->Put(&context, request, &response);
                    
                    if (status.ok() && response.success()) {
                        successful_ops_++;
                    } else {
                        failed_ops_++;
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double ops_per_second = (double)successful_ops_ / (duration.count() / 1000.0);
        
        std::cout << "Duration: " << duration.count() << " ms" << std::endl;
        std::cout << "Successful operations: " << successful_ops_ << std::endl;
        std::cout << "Failed operations: " << failed_ops_ << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << ops_per_second << " ops/sec" << std::endl;
    }
    
    // Benchmark GET operations
    void benchmarkGet(int num_operations, int num_threads) {
        std::cout << "\n=== GET Benchmark ===" << std::endl;
        std::cout << "Operations: " << num_operations << ", Threads: " << num_threads << std::endl;
        
        // First, populate some data
        std::cout << "Populating data for GET benchmark..." << std::endl;
        auto setup_stub = kvstore::KVStore::NewStub(channel_);
        for (int i = 0; i < 1000; ++i) {
            kvstore::PutRequest request;
            kvstore::PutResponse response;
            grpc::ClientContext context;
            
            request.set_key("benchmark_key_" + std::to_string(i));
            request.set_value("benchmark_value_" + std::to_string(i));
            request.set_ttl_ms(0);
            
            setup_stub->Put(&context, request, &response);
        }
        
        successful_ops_ = 0;
        failed_ops_ = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        int ops_per_thread = num_operations / num_threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread]() {
                // Create stub per thread for better connection reuse
                auto stub = kvstore::KVStore::NewStub(channel_);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, 999);
                
                for (int i = 0; i < ops_per_thread; ++i) {
                    kvstore::GetRequest request;
                    kvstore::GetResponse response;
                    grpc::ClientContext context;
                    
                    std::string key = "benchmark_key_" + std::to_string(dis(gen));
                    request.set_key(key);
                    
                    grpc::Status status = stub->Get(&context, request, &response);
                    
                    if (status.ok()) {
                        successful_ops_++;
                    } else {
                        failed_ops_++;
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double ops_per_second = (double)successful_ops_ / (duration.count() / 1000.0);
        
        std::cout << "Duration: " << duration.count() << " ms" << std::endl;
        std::cout << "Successful operations: " << successful_ops_ << std::endl;
        std::cout << "Failed operations: " << failed_ops_ << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << ops_per_second << " ops/sec" << std::endl;
    }
    
    // Mixed workload benchmark
    void benchmarkMixed(int num_operations, int num_threads, double read_ratio = 0.8) {
        std::cout << "\n=== MIXED Benchmark (Read ratio: " << read_ratio << ") ===" << std::endl;
        std::cout << "Operations: " << num_operations << ", Threads: " << num_threads << std::endl;
        
        // First, populate some initial data
        std::cout << "Populating initial data..." << std::endl;
        auto setup_stub = kvstore::KVStore::NewStub(channel_);
        for (int i = 0; i < 1000; ++i) {
            kvstore::PutRequest request;
            kvstore::PutResponse response;
            grpc::ClientContext context;
            
            request.set_key("mixed_key_" + std::to_string(i));
            request.set_value("initial_value_" + std::to_string(i));
            request.set_ttl_ms(0);
            
            setup_stub->Put(&context, request, &response);
        }
        
        successful_ops_ = 0;
        failed_ops_ = 0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        int ops_per_thread = num_operations / num_threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread, read_ratio]() {
                // Create stub per thread for better connection reuse
                auto stub = kvstore::KVStore::NewStub(channel_);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> key_dis(0, 999);
                std::uniform_real_distribution<> op_dis(0.0, 1.0);
                
                for (int i = 0; i < ops_per_thread; ++i) {
                    if (op_dis(gen) < read_ratio) {
                        // GET operation
                        kvstore::GetRequest request;
                        kvstore::GetResponse response;
                        grpc::ClientContext context;
                        
                        std::string key = "mixed_key_" + std::to_string(key_dis(gen));
                        request.set_key(key);
                        
                        grpc::Status status = stub->Get(&context, request, &response);
                        if (status.ok()) successful_ops_++;
                        else failed_ops_++;
                    } else {
                        // PUT operation
                        kvstore::PutRequest request;
                        kvstore::PutResponse response;
                        grpc::ClientContext context;
                        
                        std::string key = "mixed_key_" + std::to_string(key_dis(gen));
                        std::string value = "updated_value_" + std::to_string(t) + "_" + std::to_string(i);
                        
                        request.set_key(key);
                        request.set_value(value);
                        request.set_ttl_ms(0);
                        
                        grpc::Status status = stub->Put(&context, request, &response);
                        if (status.ok() && response.success()) successful_ops_++;
                        else failed_ops_++;
                    }
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        double ops_per_second = (double)successful_ops_ / (duration.count() / 1000.0);
        
        std::cout << "Duration: " << duration.count() << " ms" << std::endl;
        std::cout << "Successful operations: " << successful_ops_ << std::endl;
        std::cout << "Failed operations: " << failed_ops_ << std::endl;
        std::cout << "Throughput: " << std::fixed << std::setprecision(2) << ops_per_second << " ops/sec" << std::endl;
    }
};

int main(int argc, char** argv) {
    std::string server_address("localhost:50051");
    
    // Create channel
    auto channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    
    // Wait for the server to be ready
    std::cout << "Connecting to KVStore server at " << server_address << "..." << std::endl;
    if (!channel->WaitForConnected(std::chrono::system_clock::now() + std::chrono::seconds(10))) {
        std::cerr << "Failed to connect to server!" << std::endl;
        return 1;
    }
    std::cout << "Connected successfully!" << std::endl;
    
    KVStoreBenchmark benchmark(channel);
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "KVStore Performance Benchmark" << std::endl;
    std::cout << std::string(50, '=') << std::endl;
    
    // Run different benchmark scenarios
    benchmark.benchmarkPut(100000, 8);
    benchmark.benchmarkGet(100000, 8);
    benchmark.benchmarkMixed(100000, 8, 0.8);
    
    std::cout << "\n" << std::string(50, '=') << std::endl;
    std::cout << "Benchmark completed!" << std::endl;
    std::cout << std::string(50, '=') << std::endl;

    
    std::cout << "Exiting benchmark..." << std::endl;
    std::cin.get();
    
    return 0;
}
