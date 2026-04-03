# Multithreaded Key-Value Store with Persistence in C++

A high-performance in-memory key-value store built in C++ supporting concurrent client connections, persistence, and key expiration.

---

## 🚀 Features

* Multithreaded server handling multiple clients concurrently
* Thread pool for efficient request processing
* TCP socket-based client-server communication
* In-memory key-value storage
* Persistence using append-only log (AOF)
* Crash recovery by replaying log file
* Time-To-Live (TTL) support for key expiration
* Background cleanup thread for expired keys
* Log compaction to optimize storage
* Custom C++ client for interaction

---

## 🛠️ Tech Stack

* C++
* POSIX Sockets
* Multithreading (`std::thread`, `mutex`, `condition_variable`)
* STL (`unordered_map`, `queue`)
* Linux / WSL

---

## 📂 Project Structure

```
.
├── server.cpp
├── client.cpp
├── data.txt
├── README.md
```

---

## ⚙️ How to Build

### Compile Server

```bash
g++ server.cpp -o server -pthread
```

### Compile Client

```bash
g++ client.cpp -o client
```

---

## ▶️ How to Run

### Start Server

```bash
./server
```

### Run Client (in another terminal)

```bash
./client
```

---

## 💻 Supported Commands

| Command            | Description      |
| ------------------ | ---------------- |
| SET key value      | Store value      |
| GET key            | Retrieve value   |
| EXPIRE key seconds | Set TTL          |
| DEL key            | Delete key       |
| COMPACT            | Optimize storage |

---

## 🧠 Key Concepts Implemented

* Thread Pool (Producer-Consumer Pattern)
* Mutex-based Synchronization
* Condition Variables
* Append-Only Logging (AOF)
* Lazy & Active Expiration
* Log Compaction

---

## 🔄 Example

```
SET name Nagesh
GET name
EXPIRE name 5
```

---

## 🚀 Future Improvements

* Distributed replication
* Binary protocol support
* Persistent snapshotting
* Performance benchmarking

---

## 👨‍💻 Author

Nagesh Gund
