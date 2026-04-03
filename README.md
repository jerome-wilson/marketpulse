# MarketPulse - Real-Time Stock Monitoring and Insight Engine

A publication-quality Linux/macOS system program in C that demonstrates advanced system programming concepts including shared memory IPC, semaphores, multi-process architecture, daemon mode, event-driven design, fault tolerance, and AI-driven behavior.

## 🎯 Project Overview

MarketPulse is a terminal-based stock monitoring tool that fetches real-time stock prices from the Finnhub API, monitors multiple stocks simultaneously, detects price changes, triggers alerts, and generates AI-based insights.

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                         USER INTERFACE                               │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐ │
│  │   status    │  │   stats     │  │  simulate   │  │   watch     │ │
│  │  command    │  │  command    │  │  -failure   │  │  command    │ │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘ │
└─────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       MASTER PROCESS                                 │
│  ┌─────────────────────────────────────────────────────────────┐    │
│  │  • Configuration management (JSON config, hot reload)        │    │
│  │  • Worker lifecycle management (spawn, monitor, restart)     │    │
│  │  • Signal handling (SIGUSR1=reload, SIGUSR2=debug)          │    │
│  │  • Event-driven loop with select()                          │    │
│  │  • Rate limiting (token bucket algorithm)                   │    │
│  └─────────────────────────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────────────────┘
                                    │ fork()
        ┌───────────────────────────┼───────────────────────────┐
        │                           │                           │
        ▼                           ▼                           ▼
┌───────────────┐           ┌───────────────┐           ┌───────────────┐
│    Worker     │           │    Worker     │           │    Worker     │
│     AAPL      │           │     MSFT      │           │     GOOGL     │
│  ┌─────────┐  │           │  ┌─────────┐  │           │  ┌─────────┐  │
│  │ Fetch   │  │           │  │ Fetch   │  │           │  │ Fetch   │  │
│  │ Parse   │  │           │  │ Parse   │  │           │  │ Parse   │  │
│  │ Update  │  │           │  │ Update  │  │           │  │ Update  │  │
│  └─────────┘  │           │  └─────────┘  │           │  └─────────┘  │
└───────┬───────┘           └───────┬───────┘           └───────┬───────┘
        │                           │                           │
        └───────────────────────────┼───────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────┐
│                       SHARED MEMORY (IPC)                            │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐      │
│  │  Stock Data     │  │  Worker Status  │  │  Message Queue  │      │
│  │  ┌───────────┐  │  │  ┌───────────┐  │  │  ┌───────────┐  │      │
│  │  │ symbol    │  │  │  │ pid       │  │  │  │ type      │  │      │
│  │  │ price     │  │  │  │ status    │  │  │  │ sender    │  │      │
│  │  │ change    │  │  │  │ heartbeat │  │  │  │ timestamp │  │      │
│  │  │ timestamp │  │  │  │ errors    │  │  │  │ payload   │  │      │
│  │  └───────────┘  │  │  └───────────┘  │  │  └───────────┘  │      │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘      │
│                                                                      │
│  ┌─────────────────┐  ┌─────────────────────────────────────────┐   │
│  │  System Stats   │  │  Semaphores (Reader-Writer Lock)        │   │
│  │  ┌───────────┐  │  │  ┌─────────┐ ┌─────────┐ ┌─────────┐   │   │
│  │  │ requests  │  │  │  │ mutex   │ │ readers │ │ writers │   │   │
│  │  │ latency   │  │  │  └─────────┘ └─────────┘ └─────────┘   │   │
│  │  │ uptime    │  │  └─────────────────────────────────────────┘   │
│  │  └───────────┘  │                                                │
│  └─────────────────┘                                                │
└─────────────────────────────────────────────────────────────────────┘
```

## 🔧 System Programming Concepts Demonstrated

### 1. Inter-Process Communication (IPC)

| System Call | File | Purpose |
|-------------|------|---------|
| `shmget()` | ipc.c | Create shared memory segment |
| `shmat()` | ipc.c | Attach shared memory to process |
| `shmdt()` | ipc.c | Detach shared memory |
| `shmctl()` | ipc.c | Control/destroy shared memory |
| `semget()` | ipc.c | Create semaphore set |
| `semop()` | ipc.c | Semaphore wait/signal operations |
| `semctl()` | ipc.c | Initialize/destroy semaphores |

### 2. Reader-Writer Lock Implementation

```c
/* Reader-Writer Lock using Semaphores */
int rwlock_read_lock(int semid) {
    sem_wait(semid, SEM_MUTEX);      // Lock mutex
    read_count++;
    if (read_count == 1)
        sem_wait(semid, SEM_WRITERS); // First reader blocks writers
    sem_signal(semid, SEM_MUTEX);    // Unlock mutex
}

int rwlock_write_lock(int semid) {
    sem_wait(semid, SEM_WRITERS);    // Exclusive access
}
```

### 3. Process Control

| System Call | File | Purpose |
|-------------|------|---------|
| `fork()` | master.c, daemon.c | Create child processes |
| `waitpid()` | master.c | Wait for child process termination |
| `exit()` | master.c | Terminate process |
| `getpid()` | various | Get process ID |
| `kill()` | master.c, daemon.c | Send signals to processes |

### 4. Signal Handling

| Signal | Handler | Purpose |
|--------|---------|---------|
| `SIGINT` | master.c | Graceful shutdown (Ctrl+C) |
| `SIGTERM` | master.c | Termination request |
| `SIGCHLD` | master.c | Child process status change |
| `SIGUSR1` | master.c | **Reload configuration** |
| `SIGUSR2` | master.c | **Toggle debug mode** |
| `SIGALRM` | alert.c | Alert check timer |

### 5. Daemon Mode (Double Fork)

```c
int daemonize(void) {
    pid_t pid;
    
    /* First fork - exit parent */
    pid = fork();
    if (pid > 0) _exit(0);
    
    /* Create new session */
    setsid();
    
    /* Second fork - prevent controlling terminal */
    pid = fork();
    if (pid > 0) _exit(0);
    
    /* Redirect file descriptors */
    chdir("/");
    umask(0);
    close_all_fds();
    redirect_stdio();
    
    /* Create PID file */
    create_pid_file(getpid());
}
```

### 6. I/O Multiplexing

| System Call | File | Purpose |
|-------------|------|---------|
| `select()` | master.c | Event-driven I/O |
| `FD_SET/FD_ZERO` | master.c | File descriptor set management |

### 7. Network Programming

| System Call | File | Purpose |
|-------------|------|---------|
| `socket()` | network.c | Create TCP socket |
| `connect()` | network.c | Connect to server |
| `read()/write()` | network.c | Data transfer |
| SSL functions | network.c | HTTPS encryption |

## 📁 Project Structure

```
marketpulse/
├── include/
│   ├── marketpulse.h      # Main header with definitions
│   ├── ipc.h              # Shared memory & semaphore definitions
│   ├── daemon.h           # Daemon mode definitions
│   ├── logger.h           # Logging system definitions
│   └── config.h           # Configuration definitions
├── src/
│   ├── main.c             # Entry point
│   ├── cli.c              # Command line parser
│   ├── network.c          # HTTPS client (sockets + SSL)
│   ├── parser.c           # Custom JSON parser
│   ├── monitor.c          # Watch mode implementation
│   ├── alert.c            # Alert system with signals
│   ├── ai.c               # AI insights (moving averages, trends)
│   ├── utils.c            # Utility functions
│   ├── ipc.c              # Shared memory & semaphore implementation
│   ├── master.c           # Multi-process architecture controller
│   ├── daemon.c           # Daemon mode implementation
│   ├── logger.c           # Structured logging system
│   ├── config.c           # Configuration file parser
│   └── system.c           # Status, stats, failure simulation
├── config/                # Configuration files
├── data/                  # Database storage
├── logs/                  # Log files
├── Makefile               # Build configuration
└── README.md              # This file
```

## 🚀 Building and Running

### Prerequisites

- macOS or Linux
- GCC compiler
- OpenSSL library
- Finnhub API key

### Build

```bash
cd marketpulse
make clean
make
```

### Usage

```bash
# Single stock quote
./marketpulse AAPL

# Watch multiple stocks
./marketpulse watch AAPL MSFT GOOGL

# Watch preset groups
./marketpulse watch sp500
./marketpulse watch tech
./marketpulse watch nifty50    # 🇮🇳 Indian stocks (NEW!)

# Set price alert
./marketpulse alert AAPL 250

# System introspection
./marketpulse status

# Performance metrics
./marketpulse stats

# Failure simulation (fault tolerance demo)
./marketpulse simulate-failure

# Daemon control
./marketpulse daemon start
./marketpulse daemon stop
./marketpulse daemon status
```

### Signal Control (Daemon Mode)

```bash
# Reload configuration
kill -USR1 $(cat /tmp/marketpulse.pid)

# Toggle debug mode
kill -USR2 $(cat /tmp/marketpulse.pid)

# Graceful shutdown
kill -TERM $(cat /tmp/marketpulse.pid)
```

## 🇮🇳 Indian Stock Market Support (NEW!)

MarketPulse now supports monitoring Indian stocks from NIFTY50!

### NIFTY50 Preset
```bash
./marketpulse watch nifty50
```

### Stocks Included
| Symbol | Company |
|--------|---------|
| RELIANCE.BSE | Reliance Industries |
| TCS.BSE | Tata Consultancy Services |
| HDFCBANK.BSE | HDFC Bank |
| INFY.BSE | Infosys |
| ICICIBANK.BSE | ICICI Bank |
| HINDUNILVR.BSE | Hindustan Unilever |
| SBIN.BSE | State Bank of India |
| BHARTIARTL.BSE | Bharti Airtel |
| KOTAKBANK.BSE | Kotak Mahindra Bank |
| ITC.BSE | ITC Limited |

### Features
- **Currency**: Prices displayed in ₹ (INR)
- **API**: Uses Alpha Vantage API for Indian stocks
- **Formatting**: Large numbers shown with Lakhs (L) and Crores (Cr)
- **Auto-detection**: Symbols ending with `.BSE`, `.NS`, `.NSE`, `.BO` are automatically routed to Alpha Vantage

### Alternative Commands
```bash
./marketpulse watch nifty      # Same as nifty50
./marketpulse watch india      # Same as nifty50
./marketpulse watch bse        # Same as nifty50
```

### Watch Specific Indian Stocks
```bash
./marketpulse watch RELIANCE.BSE TCS.BSE INFY.BSE
```

### API Note
Alpha Vantage free tier has a limit of 25 requests/day. For production use, get your own API key at [alphavantage.co](https://www.alphavantage.co/) and update `ALPHAVANTAGE_API_KEY` in `include/marketpulse.h`.

---

## 🔑 Key Features

### 1. Shared Memory IPC with Reader-Writer Lock
- Stock data shared between master and worker processes
- **Reader-writer locks using semaphores** (multiple readers, single writer)
- Message queue for inter-process communication
- System statistics tracking

### 2. Multi-Level Process Architecture
- Master process controls overall operation
- Worker processes fetch individual stock data
- **Fault tolerance with automatic worker restart**
- Health monitoring via heartbeats

### 3. Event-Driven Architecture
- Uses `select()` for I/O multiplexing
- Non-blocking signal handling
- Efficient CPU usage

### 4. Daemon Mode
- Standard double-fork technique
- PID file management
- Log file redirection
- Signal-based control

### 5. Failure Simulation Mode
```
./marketpulse simulate-failure

=== MarketPulse Failure Simulation Mode ===

This mode tests system resilience by simulating:
  1. Worker process crashes
  2. Network failures
  3. API response delays
  4. Random failures

Operation 1: FAILED (simulated)
  → System detecting failure...
  → Initiating recovery...
  → Recovered

=== Simulation Results ===
  Successful operations: 8
  Failed operations:     2
  Recovery rate:         100%
```

### 6. Exponential Backoff Retry
```c
int fetch_with_retry(const char *symbol, ...) {
    while (attempt < max_retries) {
        if (!rate_limiter_acquire()) {
            sleep(1);  // Rate limited
            continue;
        }
        
        int result = fetch_stock_quote(symbol, ...);
        if (result == 0) return 0;  // Success
        
        // Exponential backoff: delay = base * 2^attempt
        int delay = base_delay * (1 << attempt);
        usleep(delay * 1000);
        attempt++;
    }
}
```

### 7. API Rate Limiting (Token Bucket)
```c
typedef struct {
    int tokens;
    int max_tokens;
    int refill_rate;
    time_t last_refill;
} RateLimiter;

int rate_limiter_acquire(void) {
    refill_tokens();
    if (tokens > 0) {
        tokens--;
        return 1;  // Acquired
    }
    return 0;  // Rate limited
}
```

### 8. AI-Driven Adaptive Polling
```c
int get_adaptive_poll_interval(double volatility) {
    if (volatility > 5.0) {
        // High volatility - poll more frequently
        return base_interval / 2;
    } else if (volatility < 2.0) {
        // Low volatility - poll less frequently
        return base_interval * 2;
    }
    return base_interval;
}
```

### 9. System Introspection
```
./marketpulse status

╔══════════════════════════════════════════════════════════════╗
║           MarketPulse System Status                          ║
╚══════════════════════════════════════════════════════════════╝

[Shared Memory]
  Status:        Active
  Size:          12288 bytes
  Master PID:    12345
  Uptime:        3600 seconds

[Workers]
  Active:        4
  Worker 0:      PID 12346 (AAPL) - running
  Worker 1:      PID 12347 (MSFT) - running

[Rate Limiter]
  Tokens:        58 / 60
  Refill rate:   1/sec
```

### 10. Performance Metrics
```
./marketpulse stats

╔══════════════════════════════════════════════════════════════╗
║           MarketPulse Performance Metrics                    ║
╚══════════════════════════════════════════════════════════════╝

[Request Statistics]
  Total requests:     1250
  Successful:         1245
  Failed:             5
  Success rate:       99.6%

[Performance]
  Avg response time:  120.5 ms
  Requests/min:       12.5

[System Health]
  Health score:       95%
  Status:             ● Healthy
```

## 📊 Sample Output

```
╔══════════════════════════════════════════════════════════════╗
║           MarketPulse - Stock Monitoring Engine              ║
╚══════════════════════════════════════════════════════════════╝

──────────────────────────────────────────────────────────────────
 Stock: AAPL (Apple Inc)
──────────────────────────────────────────────────────────────────

  Price:      $249.94 ▼
  Change:     -4.29 (-1.69%)
  Open:       $252.62
  High:       $254.94
  Low:        $249.00
  Prev Close: $254.23

  Time:       16:57:54
  Market:     CLOSED

──────────────────────────────────────────────────────────────────
```

## 📚 System Calls Summary

| Category | System Calls Used |
|----------|-------------------|
| **IPC** | `shmget`, `shmat`, `shmdt`, `shmctl`, `semget`, `semop`, `semctl` |
| **Process** | `fork`, `waitpid`, `exit`, `getpid`, `setsid` |
| **Signals** | `signal`, `sigaction`, `kill`, `alarm` |
| **I/O** | `open`, `read`, `write`, `close`, `dup2`, `select` |
| **Network** | `socket`, `connect`, `send`, `recv` |
| **File** | `stat`, `fstat`, `unlink`, `chdir`, `umask` |
| **Time** | `time`, `sleep`, `usleep`, `gettimeofday` |

## 🎓 Educational Value

This project demonstrates:

1. **IPC Mechanisms**: Shared memory with semaphore synchronization
2. **Reader-Writer Lock**: Advanced concurrency control
3. **Process Management**: Multi-process architecture with fault tolerance
4. **Signal Handling**: Comprehensive signal-based control system
5. **Daemon Creation**: Standard Unix daemon pattern
6. **Event-Driven Design**: Using select() for efficient I/O
7. **Network Programming**: TCP sockets with SSL/TLS
8. **Fault Tolerance**: Failure simulation and recovery
9. **Rate Limiting**: Token bucket algorithm
10. **AI Integration**: Adaptive behavior based on market conditions

## 📝 License

Educational project for Systems Programming course.

## 👤 Author

Systems Programming Project - MarketPulse