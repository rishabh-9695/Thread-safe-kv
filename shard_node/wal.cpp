#include "wal.hpp"
#include <iostream>

WriteAheadLog::WriteAheadLog(const std::string& filename) 
    : logFileName(filename)  {
    walStream.open(filename, std::ios::app);
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
    std::cout<<"Appending entry to WAL: " << entry << std::endl;
    if (walStream.is_open()) {
        walStream << entry << std::endl;
    } else {
        throw std::runtime_error("WAL stream is not open.");
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
    if (walStream.is_open()) {
        walStream.close();
        walStream.open(logFileName, std::ios::trunc); // Truncate the file
        if (!walStream.is_open()) {
            throw std::runtime_error("Failed to reset WAL file: " + logFileName);
        }
    }
}