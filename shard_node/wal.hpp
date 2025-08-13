#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <atomic>
#include <chrono>
#include <thread>

class WriteAheadLog {
    private:
        std::string logFileName;
        std::ofstream walStream;
        std::mutex logMutex; // Mutex for thread-safe access
        
        // Batch WAL components
        std::vector<std::string> batchBuffer;
        std::mutex batchMutex;
        std::condition_variable batchCondition;
        std::atomic<bool> shutdownFlag{false};
        std::thread batchWriterThread;
        
        static constexpr size_t BATCH_SIZE = 100; // Write after 50 operations
        static constexpr int BATCH_TIMEOUT_MS = 10; // Or after 10ms
        
        // Internal methods
        void writeToFile(const std::string& entry);
        void writeBatchToFile(const std::vector<std::string>& batch);
        void batchWriterLoop();
        
    public:
        WriteAheadLog(const std::string& filename);

        ~WriteAheadLog();
        void append(const std::string& entry);
        void appendBatch(const std::string& entry);
        void flush();
        void reset();
};