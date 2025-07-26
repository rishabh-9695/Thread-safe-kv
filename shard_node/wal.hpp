#pragma once

#include <string>
#include <fstream>
#include <mutex>


class WriteAheadLog {
    private:
        std::string logFileName;
        std::ofstream walStream;
        std::mutex logMutex; // Mutex for thread-safe access
    public:
        WriteAheadLog(const std::string& filename);

        ~WriteAheadLog();
        void append(const std::string& entry);
        void flush();
        void reset();
};