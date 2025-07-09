# 🧱 Distributed Key-Value Store – Final Blueprint (2-Week Build Plan)

This project takes a basic in-memory `KVStore` implementation and evolves it into a **distributed, self-healing, auto-sharded, and highly available key-value database**, built from scratch in modern C++. The goal is to create a system that demonstrates expert-level skills in **C++ concurrency, distributed architecture, fault tolerance, persistence, and observability**.

---

## 🚩 Problem Statement

> **Build a fault-tolerant, horizontally scalable, multi-threaded, auto-sharding distributed key-value store** with persistence and observability, capable of surviving node crashes, scaling on load, and maintaining consistency.

---

## ✅ High-Level Requirements

* 🧠 **Thread-safe in-memory KV engine** (with support for GET, PUT, DELETE)
* 🔀 **Partitioning & Consistent Hashing** to distribute keys across shards
* 🚦 **Auto-sharding**: Automatically split hot partitions based on size or access
* 🔁 **Asynchronous Replication** (Leader-Follower) for durability
* 🧱 **Write-Ahead Log (WAL)** and **snapshotting** for persistence and crash recovery
* 💥 **Self-healing** (failover detection, follower promotion, heartbeat monitor)
* 🌐 **Custom TCP protocol** or optional gRPC/HTTP API
* 📊 **Observability**: metrics, logging, and request tracing

---

## 📐 Architecture Diagram

```
                       +--------------------+
                       |  Client / CLI Tool |
                       +---------+----------+
                                 |
                                 v
                       +--------------------+
                       |   Router / Proxy   |
                       |  (Consistent Hash) |
                       +----+--------+------+
                            |        |
                            v        v
                   +--------+     +---------+
                   | Shard A |     | Shard B |
                   +---+----+     +----+-----+
                       |               |
               +-------v------+   +----v------+
               | WAL + Memory |   | WAL + Mem |
               | Replication  |   | Replica   |
               +--------------+   +-----------+
```

---

## 📁 Folder Structure (Monorepo, CMake-based)

```bash
kvstore/
├── CMakeLists.txt
├── README.md
├── src/
│   ├── main.cpp                # Server entry point
│   ├── kv_store/               # Core KV engine
│   │   ├── kv_store.hpp
│   │   ├── kv_store.cpp
│   │   ├── wal.hpp             # Write-ahead log
│   │   └── snapshot.hpp
│   ├── partition/              # Shard abstraction
│   │   ├── partition.hpp
│   │   └── partition.cpp
│   ├── router/                 # Consistent hashing & routing
│   │   ├── router.hpp
│   │   └── router.cpp
│   ├── replication/            # Leader-Follower logic
│   │   ├── replicator.hpp
│   │   └── replicator.cpp
│   ├── networking/             # TCP server, protocol parsing
│   │   ├── server.hpp
│   │   ├── protocol.hpp
│   │   └── connection_handler.cpp
│   ├── auto_sharder/           # Monitoring + splitting logic
│   │   ├── monitor.hpp
│   │   └── auto_sharder.cpp
│   ├── heartbeat/              # Node liveness and failover
│   │   ├── heartbeat.hpp
│   │   └── failover.cpp
│   └── observability/          # Metrics, logging, tracing
│       ├── metrics.hpp
│       ├── logger.hpp
│       └── tracing.hpp
├── include/                    # Shared headers (if needed)
├── tests/                      # Unit and integration tests
│   └── test_kv.cpp
├── scripts/                    # CLI client, launch helpers
└── docs/                       # Diagrams, usage examples
```

---

## 📅 2-Week Build Plan (3–4 hrs/day)

| Day | Milestone                                                         |
| --- | ----------------------------------------------------------------- |
| 1   | Set up repo, CMake, and single-threaded KVStore class             |
| 2   | Add thread safety: `shared_mutex`, `lock_guard`, thread pool      |
| 3   | Add basic partition abstraction + routing by hash or range        |
| 4   | Implement `ShardRouter` with dynamic partition split on threshold |
| 5   | Build WAL + snapshotting for crash recovery                       |
| 6   | Add background monitoring thread (auto-sharder)                   |
| 7   | Add simple TCP server (custom protocol parser)                    |
| 8   | Build client CLI for GET/PUT over socket                          |
| 9   | Add replication manager + write-forwarding to followers           |
| 10  | Build heartbeat monitor + leader election stub                    |
| 11  | Add metrics/logging and test WAL recovery                         |
| 12  | Integration testing: multi-shard, replication, crash recovery     |
| 13  | Polish CLI, optimize thread pools, add config support             |
| 14  | Final polish: README, diagrams, stress test scripts               |

---

## 🎯 Optional Stretch Goals

*
