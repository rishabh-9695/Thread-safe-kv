#include "wal.hpp"

WriteAheadLog::WriteAheadLog(const std::string& filename) 
    : logFileName(filename), walStream(filename, std::ios::app) {
    if (!walStream.is_open()) {
        throw std::runtime_error("Failed to open WAL file: " + filename);
    }
}
WriteAheadLog::~WriteAheadLog() {
    if (walStream.is_open()) {
        walStream.close();
    }
}
void WriteAheadLog::append(const std::string& entry) {
    std::lock_guard<std::mutex> lock(logMutex); // Ensure thread-safe access
    if (walStream.is_open()) {
        walStream << entry << std::endl;
    } else {
        throw std::runtime_error("WAL stream is not open.");
    }
}