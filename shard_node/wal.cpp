#include "wal.hpp"
#include <iostream>

WriteAheadLog::WriteAheadLog(const std::string& filename) 
    : logFileName(filename)  {
    walStream.open(filename, std::ios::app);
    if (!walStream.is_open()) {
        throw std::runtime_error("Failed to open WAL file: " + filename);
    }
    
    // Start batch writer thread
    shutdownFlag = false;
    batchWriterThread = std::thread(&WriteAheadLog::batchWriterLoop, this);
}

WriteAheadLog::~WriteAheadLog() {
    shutdownFlag = true;
    batchCondition.notify_all();
    
    if (batchWriterThread.joinable()) {
        batchWriterThread.join();
    }
    
    if (walStream.is_open()) {
        walStream.close();
    }
}

void WriteAheadLog::writeToFile(const std::string& entry) {
    std::lock_guard<std::mutex> lock(logMutex);
    if (walStream.is_open()) {
        walStream << entry << std::endl;
        walStream.flush(); // Ensure data is written to disk
    }
}

void WriteAheadLog::append(const std::string& entry) {
    writeToFile(entry);
}

void WriteAheadLog::appendBatch(const std::string& entry) {
    if (shutdownFlag) return;
    
    std::unique_lock<std::mutex> lock(batchMutex);
    batchBuffer.push_back(entry);
    
    // Trigger write if batch is full
    if (batchBuffer.size() >= BATCH_SIZE) {
        batchCondition.notify_one();
    }
}

void WriteAheadLog::flush() {
    std::lock_guard<std::mutex> lock(logMutex);
    if (walStream.is_open()) {
        walStream.flush();
    } else {
        throw std::runtime_error("WAL stream is not open.");
    }
}

void WriteAheadLog::reset() {
    std::lock_guard<std::mutex> lock(logMutex);
    walStream.close();
    walStream.open(logFileName, std::ios::trunc);
}

void WriteAheadLog::writeBatchToFile(const std::vector<std::string>& batch) {
    std::lock_guard<std::mutex> lock(logMutex);
    for (const auto& entry : batch) {
        walStream << entry << std::endl;
    }
    walStream.flush();
}

void WriteAheadLog::batchWriterLoop() {
    while (!shutdownFlag) {
        std::unique_lock<std::mutex> lock(batchMutex);
        
        // Wait for batch to fill or timeout
        batchCondition.wait_for(lock, std::chrono::milliseconds(BATCH_TIMEOUT_MS), 
            [this] { return batchBuffer.size() >= BATCH_SIZE || shutdownFlag; });
        
        if (!batchBuffer.empty()) {
            std::vector<std::string> currentBatch;
            currentBatch.swap(batchBuffer);
            lock.unlock();
            
            writeBatchToFile(currentBatch);
        }
    }
    
    // Flush remaining entries on shutdown
    std::lock_guard<std::mutex> lock(batchMutex);
    if (!batchBuffer.empty()) {
        writeBatchToFile(batchBuffer);
    }
}
