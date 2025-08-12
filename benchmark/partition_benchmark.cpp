#include <grpcpp/grpcpp.h>
#include "../shard_node/kvstore.grpc.pb.h"
#include <chrono>
#include <iostream>
#include <vector>
#include <thread>
#include <random>
#include <atomic>
#include <iomanip>
#include <map>

class PartitionBenchmark {
private:
    std::shared_ptr<grpc::Channel> channel_;
    std::atomic<int> successful_ops_{0};
    std::atomic<int> failed_ops_{0};

public:
    PartitionBenchmark(std::shared_ptr<grpc::Channel> channel)
        : channel_(channel) {}

    struct BenchmarkResult {
        int partitions;
        double put_ops_per_sec;
        double get_ops_per_sec;
        double mixed_ops_per_sec;
        int duration_ms;
        int operations;
        int threads;
    };

    // Quick benchmark for partition testing
    BenchmarkResult quickBenchmark(int num_operations = 10000, int num_threads = 4) {
        BenchmarkResult result = {};
        result.operations = num_operations;
        result.threads = num_threads;
        
        // PUT Benchmark
        successful_ops_ = 0;
        failed_ops_ = 0;
        auto start = std::chrono::high_resolution_clock::now();
        
        std::vector<std::thread> threads;
        int ops_per_thread = num_operations / num_threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread]() {
                auto stub = kvstore::KVStore::NewStub(channel_);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(1, 1000000);
                
                for (int i = 0; i < ops_per_thread; ++i) {
                    kvstore::PutRequest request;
                    kvstore::PutResponse response;
                    grpc::ClientContext context;
                    
                    std::string key = "part_key_" + std::to_string(t) + "_" + std::to_string(i);
                    std::string value = "value_" + std::to_string(dis(gen));
                    
                    request.set_key(key);
                    request.set_value(value);
                    request.set_ttl_ms(0);
                    
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
        result.put_ops_per_sec = (double)successful_ops_ / (duration.count() / 1000.0);
        
        // GET Benchmark (quick)
        successful_ops_ = 0;
        failed_ops_ = 0;
        start = std::chrono::high_resolution_clock::now();
        
        threads.clear();
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread]() {
                auto stub = kvstore::KVStore::NewStub(channel_);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, ops_per_thread - 1);
                
                for (int i = 0; i < ops_per_thread; ++i) {
                    kvstore::GetRequest request;
                    kvstore::GetResponse response;
                    grpc::ClientContext context;
                    
                    std::string key = "part_key_" + std::to_string(t) + "_" + std::to_string(dis(gen));
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
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        result.get_ops_per_sec = (double)successful_ops_ / (duration.count() / 1000.0);
        
        // Mixed Benchmark (quick)
        successful_ops_ = 0;
        failed_ops_ = 0;
        start = std::chrono::high_resolution_clock::now();
        
        threads.clear();
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([this, t, ops_per_thread]() {
                auto stub = kvstore::KVStore::NewStub(channel_);
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> key_dis(0, ops_per_thread - 1);
                std::uniform_real_distribution<> op_dis(0.0, 1.0);
                
                for (int i = 0; i < ops_per_thread; ++i) {
                    if (op_dis(gen) < 0.7) { // 70% reads
                        kvstore::GetRequest request;
                        kvstore::GetResponse response;
                        grpc::ClientContext context;
                        
                        std::string key = "part_key_" + std::to_string(t) + "_" + std::to_string(key_dis(gen));
                        request.set_key(key);
                        
                        grpc::Status status = stub->Get(&context, request, &response);
                        if (status.ok()) successful_ops_++;
                        else failed_ops_++;
                    } else { // 30% writes
                        kvstore::PutRequest request;
                        kvstore::PutResponse response;
                        grpc::ClientContext context;
                        
                        std::string key = "part_key_" + std::to_string(t) + "_" + std::to_string(key_dis(gen));
                        std::string value = "updated_" + std::to_string(i);
                        
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
        
        end = std::chrono::high_resolution_clock::now();
        duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        result.mixed_ops_per_sec = (double)successful_ops_ / (duration.count() / 1000.0);
        result.duration_ms = duration.count();
        
        return result;
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
    
    PartitionBenchmark benchmark(channel);
    
    std::cout << "\n" << std::string(80, '=') << std::endl;
    std::cout << "KVStore Partition Performance Analysis" << std::endl;
    std::cout << std::string(80, '=') << std::endl;
    
    auto result = benchmark.quickBenchmark();
    
    std::cout << "\n" << std::string(60, '=') << std::endl;
    std::cout << "PARTITION BENCHMARK RESULTS" << std::endl;
    std::cout << std::string(60, '=') << std::endl;
    std::cout << "Operations: " << result.operations << " | Threads: " << result.threads << std::endl;
    std::cout << std::string(60, '-') << std::endl;
    std::cout << "PUT:   " << std::fixed << std::setprecision(2) << result.put_ops_per_sec << " ops/sec" << std::endl;
    std::cout << "GET:   " << std::fixed << std::setprecision(2) << result.get_ops_per_sec << " ops/sec" << std::endl;
    std::cout << "MIXED: " << std::fixed << std::setprecision(2) << result.mixed_ops_per_sec << " ops/sec" << std::endl;
    
    std::cout << std::string(60, '=') << std::endl;
    
    return 0;
}
