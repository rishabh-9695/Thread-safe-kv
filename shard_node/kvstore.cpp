#include "kvstore.hpp"
#include "wal.hpp"

#include <fstream>
#include <iostream>
#include <sstream>
#include <thread>

//
// KVStore::Value definitions
//
KVStore::Value::Value() : value(""), expiration(std::nullopt) {}

KVStore::Value::Value(const std::string& val)
    : value(val), expiration(std::nullopt) {}

KVStore::Value::Value(const std::string& val, std::chrono::steady_clock::time_point exp)
    : value(val), expiration(exp) {}

bool KVStore::Value::isExpired() const {
    return expiration.has_value() &&
           std::chrono::steady_clock::now() >= expiration.value();
}

//
// Constructors
//
KVStore::KVStore() = default;

KVStore::KVStore(const std::string& logFile)
    : wal(std::make_unique<WriteAheadLog>(logFile)) {
    snapshotFileName = logFile + ".snapshot";
    if (!snapshotFileName.empty()) {
        loadSnapshot(snapshotFileName);
    }
    recoverFromWAL(logFile);
}

void KVStore::startBackgroundThreads() {
    stopFlag.store(false);
    cleaner = std::thread([this]() {
    std::unique_lock<std::mutex> lock(cleanerMutex);
        while (!stopFlag.load()) {
            if (cleanerCV.wait_for(lock, std::chrono::seconds(1)) == std::cv_status::timeout) {
                cleanup_expired_keys();
            }
        }
    });

    snapshotThread = std::thread([this]() {
        std::unique_lock<std::mutex> lock(snapshotMutex);
        while (!stopFlag.load()) {
            if (snapshotCV.wait_for(lock, std::chrono::seconds(snapshotIntervalSeconds)) == std::cv_status::timeout) {
                snapshot(snapshotFileName);
            }
        }
    });

}

//
// Public API methods
//
std::unique_ptr<KVStore> KVStore::create(const std::string& logFile) {
    auto kvstore = std::unique_ptr<KVStore>(new KVStore(logFile));
    kvstore->startBackgroundThreads();
    return kvstore;
}
void KVStore::put(const std::string& key, const std::string& value) {
    std::unique_lock lock(mutex);
    Value val(value);
    store[key] = std::move(val);
    if (wal)
        wal->appendBatch("PUT " + key + " " + value);
}

void KVStore::put(const std::string& key, const std::string& value, int ttl_ms) {
    auto expiration = std::chrono::steady_clock::now() + std::chrono::milliseconds(ttl_ms);
    std::unique_lock lock(mutex);
    Value val(value, expiration);
    store[key] = std::move(val);
    if (wal)
        wal->appendBatch("PUT " + key + " " + value + " " + std::to_string(ttl_ms));
}

std::optional<std::string> KVStore::get(const std::string& key) {
    std::shared_lock lock(mutex);
    auto it = store.find(key);
    if (it != store.end() && !it->second.isExpired()) {
        return it->second.value;
    }
    return std::nullopt;
}

void KVStore::remove(const std::string& key) {
    std::unique_lock lock(mutex);
    if (wal)
        wal->appendBatch("REMOVE " + key);
    store.erase(key);
}

//
// Internal methods
//

void KVStore::recoverFromWAL(const std::string& filename) {
    std::ifstream infile(filename);
    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string op, key, value;
        if (!(iss >> op >> key)) continue;

        if (op == "PUT") {
            iss >> value;
            std::unique_lock lock(mutex);
            Value val(value);
            store[key] = std::move(val);
        } else if (op == "PUT_TTL") {
            long long expiry_epoch;
            if (iss >> value >> expiry_epoch) {
                auto expiry_time = std::chrono::steady_clock::time_point{
                    std::chrono::milliseconds(expiry_epoch)
                };
                std::unique_lock lock(mutex);
                Value val(value, expiry_time);
                store[key] = std::move(val);
            } else {
                std::cerr << "[WAL Recovery] Bad PUT_TTL line: " << line << "\n";
            }
        } else if (op == "REMOVE") {
            std::unique_lock lock(mutex);
            store.erase(key);
        }
    }
}

void KVStore::snapshot(const std::string& filename) {
    // Reduced verbosity - only log snapshots occasionally
    // std::cout << "Creating snapshot: " << filename << "\n";
    std::string tmpFilename = filename + ".tmp";
    std::ofstream out(tmpFilename);
    if (!out.is_open()) {
        throw std::runtime_error("Failed to open snapshot file: " + tmpFilename);
    }
    {
        std::shared_lock lock(mutex);
        for (const auto& [key, val] : store) {
            if (val.isExpired()) continue;

            out << key << '\t' << val.value << '\t';
            if (val.expiration.has_value()) {
                out << std::chrono::duration_cast<std::chrono::milliseconds>(
                        val.expiration->time_since_epoch()).count();
            } else {
                out << -1;
            }
            out << '\n';
        }
    }
    out.close();
    std::rename(tmpFilename.c_str(), filename.c_str());

    if (wal) {
        wal->reset();  // Truncate WAL after a snapshot
    }
}

void KVStore::loadSnapshot(const std::string& filename) {
    std::ifstream infile(filename);
    if (!infile.is_open()) {
        // Snapshot file does not exist, starting with empty store - removed logging for performance
        return; // Gracefully handle missing snapshot file
    }

    std::string line;
    while (std::getline(infile, line)) {
        std::istringstream iss(line);
        std::string key, value;
        long long expiry_epoch = -1;
        if (!(iss >> key >> value >> expiry_epoch)) continue;

        if (expiry_epoch != -1) {
            auto expiration = std::chrono::steady_clock::time_point{
                std::chrono::milliseconds(expiry_epoch)
            };
            std::unique_lock lock(mutex);
            store[key] = Value(value, expiration);
        } else {
            std::unique_lock lock(mutex);
            store[key] = Value(value);
        }
    }
}

void KVStore::cleanup_expired_keys() {
    std::unique_lock lock(mutex);
    for (auto it = store.begin(); it != store.end(); ) {
        if (it->second.isExpired()) {
            it = store.erase(it);
        } else {
            ++it;
        }
    }
}

void KVStore::shutdown() {
    if (wal) {
        wal->flush();
    }
    // Optionally, save a snapshot before shutdown
    if (!snapshotFileName.empty()) {
        std::shared_lock lock(mutex);
        snapshot(snapshotFileName);
    }
    {
        std::unique_lock lock(mutex);
        stopFlag.store(true);
    }
    snapshotCV.notify_all(); // Notify background threads to stop
    cleanerCV.notify_all(); // Notify cleaner thread to stop
    if (cleaner.joinable()) {
        cleaner.join();
    }
    if (snapshotThread.joinable()) {
        snapshotThread.join();
    }
    

};

KVStore::~KVStore() {
    shutdown();
}