#include "../../shard_node/wal.hpp"
#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <vector>
#include <chrono>

class WALTest : public ::testing::Test {
protected:
    void SetUp() override {
        test_file_ = "test_wal_" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + ".log";
        // Clean up any existing test file
        std::filesystem::remove(test_file_);
    }

    void TearDown() override {
        // Clean up test file
        std::filesystem::remove(test_file_);
    }

    std::string test_file_;
};

// Test basic WAL operations
TEST_F(WALTest, BasicWriteAndFlush) {
    WriteAheadLog wal(test_file_);
    
    // Test basic append
    wal.append("PUT key1 value1");
    wal.append("PUT key2 value2");
    wal.flush();
    
    // Verify file exists and has content
    EXPECT_TRUE(std::filesystem::exists(test_file_));
    EXPECT_GT(std::filesystem::file_size(test_file_), 0);
}

// Test batch writing functionality
TEST_F(WALTest, BatchWriting) {
    WriteAheadLog wal(test_file_);
    
    // Add multiple entries without explicit flush
    for (int i = 0; i < 10; ++i) {
        wal.append("PUT key" + std::to_string(i) + " value" + std::to_string(i));
    }
    
    // Batch should be written automatically or on flush
    wal.flush();
    
    // Verify all entries are written
    std::ifstream file(test_file_);
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        EXPECT_TRUE(line.find("PUT key") != std::string::npos);
        count++;
    }
    EXPECT_EQ(count, 10);
}

// Test concurrent WAL writes
TEST_F(WALTest, ConcurrentWrites) {
    WriteAheadLog wal(test_file_);
    const int num_threads = 4;
    const int writes_per_thread = 25;
    std::vector<std::thread> threads;
    
    // Launch multiple threads writing to WAL
    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&wal, t, writes_per_thread]() {
            for (int i = 0; i < writes_per_thread; ++i) {
                wal.append("PUT thread" + std::to_string(t) + "_key" + std::to_string(i) + " value");
            }
        });
    }
    
    // Wait for all threads to complete
    for (auto& thread : threads) {
        thread.join();
    }
    
    wal.flush();
    
    // Verify total number of entries
    std::ifstream file(test_file_);
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
    }
    EXPECT_EQ(count, num_threads * writes_per_thread);
}

// Test WAL reset functionality
TEST_F(WALTest, ResetWAL) {
    WriteAheadLog wal(test_file_);
    
    // Write some data
    wal.append("PUT key1 value1");
    wal.append("PUT key2 value2");
    wal.flush();
    
    // Verify file has content
    EXPECT_GT(std::filesystem::file_size(test_file_), 0);
    
    // Reset WAL
    wal.reset();
    
    // Verify file is empty or recreated
    EXPECT_TRUE(std::filesystem::exists(test_file_));
    EXPECT_EQ(std::filesystem::file_size(test_file_), 0);
}

// Test WAL with large batch sizes
TEST_F(WALTest, LargeBatchSize) {
    WriteAheadLog wal(test_file_);
    
    // Write a large number of entries to test batch behavior
    const int large_count = 1000;
    for (int i = 0; i < large_count; ++i) {
        wal.append("PUT large_key_" + std::to_string(i) + " large_value_" + std::to_string(i));
    }
    
    wal.flush();
    
    // Verify all entries are written
    std::ifstream file(test_file_);
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
    }
    EXPECT_EQ(count, large_count);
}

// Test WAL file operations edge cases
TEST_F(WALTest, FileOperationsEdgeCases) {
    // Test WAL with empty filename (should handle gracefully)
    EXPECT_NO_THROW(WriteAheadLog wal(""));
    
    // Test WAL with invalid path
    EXPECT_NO_THROW(WriteAheadLog wal("/invalid/path/test.log"));
}

// Test WAL append with various entry types
TEST_F(WALTest, VariousEntryTypes) {
    WriteAheadLog wal(test_file_);
    
    // Test different operation types
    wal.append("PUT key1 value1");
    wal.append("REMOVE key2");
    wal.append("PUT key3 value_with_spaces");
    wal.append("PUT key4 value\nwith\nnewlines");
    
    wal.flush();
    
    // Verify all entries are written
    std::ifstream file(test_file_);
    std::string line;
    std::vector<std::string> lines;
    while (std::getline(file, line)) {
        lines.push_back(line);
    }
    
    EXPECT_EQ(lines.size(), 4);
    EXPECT_TRUE(lines[0].find("PUT key1") != std::string::npos);
    EXPECT_TRUE(lines[1].find("REMOVE key2") != std::string::npos);
    EXPECT_TRUE(lines[2].find("PUT key3") != std::string::npos);
}

// Test WAL performance under stress
TEST_F(WALTest, StressTest) {
    WriteAheadLog wal(test_file_);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // High-frequency writes
    const int stress_count = 10000;
    for (int i = 0; i < stress_count; ++i) {
        wal.append("PUT stress_key_" + std::to_string(i) + " stress_value_" + std::to_string(i));
    }
    
    wal.flush();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should complete within reasonable time (adjust threshold as needed)
    EXPECT_LT(duration.count(), 5000); // 5 seconds max
    
    // Verify all entries written
    std::ifstream file(test_file_);
    std::string line;
    int count = 0;
    while (std::getline(file, line)) {
        count++;
    }
    EXPECT_EQ(count, stress_count);
}
