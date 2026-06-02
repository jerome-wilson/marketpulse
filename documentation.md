# MARKETPULSE

## AI-Powered Real-Time Stock Monitoring & Insight Engine

### SYSTEMS PROGRAMMING PROJECT REPORT

**Done By:**

Jerome Wilson
2025sl70021
---

## TABLE OF CONTENTS

1. [ABSTRACT](#1-abstract)
2. [INTRODUCTION](#2-introduction)
3. [PROBLEM STATEMENT](#3-problem-statement)
4. [PROJECT OBJECTIVES](#4-project-objectives)
5. [SYSTEM ARCHITECTURE](#5-system-architecture)
6. [TECHNOLOGIES USED](#6-technologies-used)
7. [AI COMPONENTS](#7-ai-components)
8. [FEATURES AND FUNCTIONALITIES](#8-features-and-functionalities)
9. [CORE CONCEPTS](#9-core-concepts)
10. [PROJECT WORKFLOW](#10-project-workflow)
11. [CHALLENGES](#11-challenges)
12. [PROJECT AND AI](#12-project-and-ai)
13. [FUTURE ENHANCEMENTS](#13-future-enhancements)
14. [APPLICATIONS](#14-applications)
15. [CONCLUSION](#15-conclusion)

---

## 1. ABSTRACT

MarketPulse is an AI-powered real-time stock monitoring and insight engine developed as a comprehensive systems programming project. The application is specifically designed for developers, students, and retail investors who require a lightweight, efficient, terminal-based tool to track stock markets without the overhead of expensive graphical applications, subscription-based services, or browser-based platforms that disrupt workflow. In today's fast-paced financial markets, access to real-time stock information is crucial for making informed investment decisions, yet traditional stock monitoring tools present significant barriers including high costs (professional terminals like Bloomberg cost upwards of $20,000 annually), complex setup requirements, and the need to switch contexts away from terminal-based development environments.

MarketPulse addresses these challenges by providing a terminal-native application that fetches real-time stock prices from the Finnhub financial API, monitors multiple stocks simultaneously using parallel processing with fork() and pipe() system calls, detects price movements and triggers alerts using Unix signal handling mechanisms (SIGINT, SIGALRM, SIGCHLD), and generates intelligent AI-powered trading insights using the Groq API with the Llama 3.3 70B Versatile model. The parallel fetching architecture reduces data retrieval time by approximately 10x compared to sequential fetching, enabling responsive real-time monitoring of multiple stocks. The AI integration provides automatic trend assessment (Bullish/Bearish/Neutral), support and resistance level identification, trading suggestions, and market sentiment analysis without requiring users to leave the terminal environment.

The project combines network programming (socket(), connect(), SSL/TLS encryption via OpenSSL), process management (fork(), pipe(), waitpid() for parallel execution and inter-process communication), signal handling (signal(), sigaction(), alarm() for graceful shutdown and timer-based alerts), memory-mapped files (mmap(), msync() for persistent price history), terminal control (tcgetattr(), tcsetattr() for raw mode keyboard input), and I/O multiplexing (select() for non-blocking user interaction) into a single integrated platform. This comprehensive implementation demonstrates advanced system programming concepts including process creation and synchronization, inter-process communication, network socket programming, signal-driven event handling, and memory-mapped I/O, while providing practical trading value through real-time market data, visual sparkline charts, color-coded trend indicators, and AI-generated insights. The application supports both US markets (S&P 500 stocks via NYSE/NASDAQ) and Indian markets (NIFTY50 stocks via NSE/BSE) with automatic currency detection and timezone-aware market status display.

---

## 2. INTRODUCTION

### 2.1 Introduction to the Project

Stock market monitoring is essential for investors, traders, and financial analysts. However, existing tools present significant challenges:

**For Developers and System Administrators:**
- Working primarily in terminals, switching to browser-based tools breaks workflow
- No way to monitor stocks while coding or deploying
- Existing CLI tools require manual JSON parsing

**For Retail Investors:**
- Professional tools like Bloomberg Terminal cost $20,000+/year
- Free tools are cluttered with ads and unnecessary features
- No real-time alerts for price movements

**For Students:**
- Learning system programming concepts through practical applications
- Understanding process management, IPC, and network programming
- Gaining exposure to AI integration in systems software

MarketPulse was developed to eliminate these barriers by introducing a terminal-native stock monitoring solution with AI-assisted insights and real-time alerts.

---

## 3. PROBLEM STATEMENT

### 3.1 Problem Statement of the Project

Traditional stock monitoring solutions are not designed with developers and terminal users as a primary concern.

**Major Limitations Include:**

#### Accessibility Challenges
- Heavy dependence on graphical interfaces
- Expensive subscription requirements
- Complex setup and configuration
- No terminal-based solutions

#### Technical Challenges
- Sequential data fetching is slow for multiple stocks
- API rate limiting from financial data providers
- Complex SSL/TLS handshaking for secure connections
- No persistent price history across sessions

#### User Experience Challenges
- Raw JSON output is hard to read
- No visual indicators for trends
- No keyboard controls for interaction
- Blocking I/O prevents responsive UI

MarketPulse aims to solve these issues by enabling users to monitor stocks directly from the terminal with parallel fetching, AI insights, and real-time alerts.

---

## 4. PROJECT OBJECTIVES

### 4.1 Objectives of the Project

**Primary Objective:**
To develop an AI-assisted stock monitoring system that demonstrates core system programming concepts while providing real trading value to developers, students, and retail investors.

**Secondary Objectives:**
- Provide real-time stock price fetching from external APIs
- Enable multi-stock monitoring with parallel processing
- Offer AI-powered technical analysis and trading insights
- Implement price alert system with signal handling
- Support both US (S&P 500) and Indian (NIFTY50) markets
- Maintain persistent price history using memory-mapped files
- Create intuitive terminal UI with keyboard controls

---

## 5. SYSTEM ARCHITECTURE

### 5.1 Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         USER (CLI)                               │
│              ./marketpulse watch AAPL MSFT GOOGL                │
└───────────────────────────┬─────────────────────────────────────┘
                            │
                            ▼
┌─────────────────────────────────────────────────────────────────┐
│                      CLI PARSER (cli.c)                          │
│                 Command interpretation and routing               │
└───────────────────────────┬─────────────────────────────────────┘
                            │
            ┌───────────────┼───────────────┐
            │               │               │
            ▼               ▼               ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│    NETWORK    │  │    MONITOR    │  │     ALERT     │
│    MODULE     │  │    ENGINE     │  │    ENGINE     │
│               │  │               │  │               │
│  socket()     │  │   fork()      │  │  signal()     │
│  connect()    │  │   pipe()      │  │  sigaction()  │
│  SSL_*()      │  │   waitpid()   │  │  alarm()      │
└───────┬───────┘  └───────┬───────┘  └───────┬───────┘
        │                  │                  │
        ▼                  ▼                  ▼
┌───────────────┐  ┌───────────────┐  ┌───────────────┐
│  DATA PARSER  │  │    HISTORY    │  │      AI       │
│               │  │    MODULE     │  │    MODULE     │
│  JSON parsing │  │   mmap()      │  │  Groq API     │
│  Data extract │  │   msync()     │  │  Llama 3.3    │
└───────┬───────┘  └───────┬───────┘  └───────┬───────┘
        │                  │                  │
        └──────────────────┼──────────────────┘
                           │
                           ▼
┌─────────────────────────────────────────────────────────────────┐
│                    TERMINAL DISPLAY (utils.c)                    │
│                                                                  │
│  tcgetattr() / tcsetattr() - Raw terminal mode                  │
│  select() - Non-blocking keyboard input                         │
│  ANSI escape codes - Colors and formatting                      │
└─────────────────────────────────────────────────────────────────┘
```

### 5.2 Layer Description

**Input Layer:**
- User CLI commands
- Keyboard input during monitoring

**Processing Layer:**
- CLI parsing and command routing
- Network communication with APIs
- Parallel stock fetching with fork/pipe
- Signal-based alert monitoring

**Data Layer:**
- JSON parsing for API responses
- Memory-mapped price history
- AI insight generation

**Output Layer:**
- Terminal UI with colors and charts
- Sparkline price visualization
- Text-to-terminal rendering

---

## 6. TECHNOLOGIES USED

### 6.1 Technologies Used

| Technology | Purpose |
|------------|---------|
| C Programming | Core implementation |
| Linux/macOS System Calls | Process and file management |
| OpenSSL | HTTPS/TLS encryption |
| Finnhub API | Real-time stock data |
| Groq API | AI-powered insights |
| Llama 3.3 Model | Natural language analysis |
| fork() | Process creation for parallel fetching |
| pipe() | Inter-process communication |
| waitpid() | Process synchronization |
| socket() | Network socket creation |
| connect() | TCP connection establishment |
| signal() / sigaction() | Signal handling |
| alarm() | Timer-based alerts |
| mmap() / msync() | Memory-mapped file persistence |
| select() | I/O multiplexing |
| tcgetattr() / tcsetattr() | Terminal raw mode |
| mkfifo() | Named pipes for streaming |
| ANSI Escape Codes | Terminal colors and formatting |

MarketPulse extensively utilizes Linux process management concepts including fork-exec architecture, IPC mechanisms, and signal handling.

---

## 7. AI COMPONENTS

### 7.1 AI Integration Overview

MarketPulse integrates artificial intelligence through the Groq API, utilizing the Llama 3.3 70B Versatile model for intelligent stock analysis and trading insights.

### 7.2 Single Stock AI Analysis

When fetching a single stock, MarketPulse automatically generates AI-powered insights.

**Example:**
```
Command: ./marketpulse AAPL

AI Commentary:
AAPL shows bullish momentum with price above key moving averages.
Support at $185, resistance at $195. Consider holding or adding
on dips below $187.
```

**Features:**
- Trend assessment (Bullish/Bearish/Neutral)
- Support and resistance levels
- Trading suggestions
- Automatic with every fetch - no extra command needed

### 7.3 Market Overview AI

Press 'I' in watch mode to get AI-powered market analysis.

**Features:**
- Overall market sentiment
- Top performers analysis
- Actionable trading advice
- Market-wide insights across all watched stocks

### 7.4 Technical Analysis

MarketPulse performs technical analysis including:
- Moving Averages (MA5, MA10)
- RSI (Relative Strength Index)
- Volatility percentage
- Golden Cross / Death Cross signal detection

### 7.5 Deep Analysis Mode

```
Command: ./marketpulse insight NVDA
```

Provides comprehensive technical analysis with detailed AI commentary and signal detection.

---

## 8. FEATURES AND FUNCTIONALITIES

### 8.1 Single Stock Fetch

```bash
./marketpulse AAPL
```

**Features:**
- Real-time price from Finnhub API
- Price change (absolute and percentage)
- Open, High, Low, Previous Close
- Automatic AI-powered insights
- Market status (OPEN/CLOSED)

### 8.2 Multi-Stock Watch Mode

```bash
./marketpulse watch AAPL MSFT GOOGL
```

**Features:**
- Parallel fetching using fork() and pipe()
- Real-time updates every 5 seconds
- Sparkline charts showing price history
- Color-coded trend indicators (green/red)
- Keyboard controls (Q, S, P, +, -)

### 8.3 Index Monitoring

```bash
./marketpulse watch sp500    # US S&P 500 stocks
./marketpulse watch nifty50  # Indian NIFTY50 stocks
```

**Features:**
- Predefined stock groups
- Automatic currency detection ($ or ₹)
- Timezone-aware market status
- Regional market hours detection

### 8.4 Price Alert System

```bash
./marketpulse alert AAPL 190
```

**Features:**
- Signal-based threshold monitoring
- Visual terminal flash alerts
- Audio bell notifications
- Bidirectional threshold crossing detection
- Graceful shutdown with Ctrl+C

### 8.5 Top Market Movers

```bash
./marketpulse movers
```

**Features:**
- Interactive market selection (US or India)
- Top 5 Gainers and Top 5 Losers
- Market sentiment analysis
- AI-generated market commentary

### 8.6 Keyboard Controls

| Key | Action |
|-----|--------|
| Q | Quit monitoring |
| S | Sort by change percentage |
| I | Show AI market insights |
| P | Pause/Resume updates |
| + | Faster refresh rate |
| - | Slower refresh rate |
| Esc | Close insights panel |

### 8.7 Visual Features

- Color-coded price changes (green for up, red for down)
- Sparkline charts using Unicode block elements (▁▂▃▄▅▆▇█)
- Trend arrows (▲/▼)
- Market status indicators
- Clean, readable table format

---

## 9. CORE CONCEPTS

### 9.1 Core Linux Concepts Implemented

**Process Management:**
- `fork()` - Create child processes for parallel stock fetching
- `waitpid()` - Synchronize parent-child processes, prevent zombies
- `exit()` - Terminate child processes

**Inter-Process Communication:**
- `pipe()` - Unidirectional data channel between processes
- `mkfifo()` - Named pipes for external tool integration

**Network Programming:**
- `socket()` - Create network communication endpoint
- `connect()` - Establish TCP connection to API servers
- `getaddrinfo()` - DNS resolution

**Signal Handling:**
- `signal()` - Basic signal handling (SIGPIPE)
- `sigaction()` - Advanced signal handling (SIGINT, SIGALRM, SIGCHLD)
- `alarm()` - Timer-based periodic price checking
- `kill()` - Send signals to background processes

**Memory Management:**
- `mmap()` - Memory-mapped files for persistent price history
- `msync()` - Synchronize memory to disk

**Terminal Control:**
- `tcgetattr()` - Get terminal attributes
- `tcsetattr()` - Set terminal to raw mode for immediate key response

**I/O Multiplexing:**
- `select()` - Non-blocking keyboard input checking

---

## 10. PROJECT WORKFLOW

### 10.1 Project Workflow

**Step 1: Command Input**
User enters command (e.g., `./marketpulse watch AAPL MSFT`)

**Step 2: CLI Parsing**
CLI parser interprets command and extracts symbols

**Step 3: Terminal Setup**
Terminal switched to raw mode using tcsetattr()

**Step 4: Network Connection**
For each stock:
- socket() creates TCP endpoint
- connect() establishes connection
- SSL_connect() performs TLS handshake

**Step 5: Parallel Fetching**
- fork() creates child process for each stock
- Each child fetches independently via HTTPS
- pipe() transfers data back to parent
- waitpid() synchronizes all children

**Step 6: Data Processing**
- JSON response parsed to extract price data
- Price added to mmap'd history file
- msync() persists to disk

**Step 7: AI Analysis (Optional)**
- Stock data sent to Groq API
- Llama 3.3 generates trading insights
- Response parsed and formatted

**Step 8: Display Rendering**
- Clear screen with ANSI codes
- Render table with colors
- Draw sparkline charts
- Show trend indicators

**Step 9: Keyboard Handling**
- select() checks for input without blocking
- Handle Q/S/P/+/- keys
- Loop back to Step 5 after refresh interval

**Step 10: Cleanup**
- Restore terminal settings
- Close mmap'd files
- Exit gracefully

---

## 11. CHALLENGES

### 11.1 Challenges Faced During Development

#### Challenge 1: High Latency in Sequential Fetching

**Problem:**
Fetching 10 stocks sequentially took 10-15 seconds due to:
- DNS resolution
- TCP 3-way handshake
- SSL/TLS handshake
- HTTP request/response

**Solution:**
Implemented parallel fetching using fork() and pipe():
```c
for (int i = 0; i < stock_count; i++) {
    pipe(pipes[i]);
    pids[i] = fork();
    if (pids[i] == 0) {
        // Child fetches one stock
        fetch_stock_quote(symbols[i], &stock);
        write(pipes[i][1], &stock, sizeof(stock));
        exit(0);
    }
}
```
**Result:** 10 stocks now fetch in ~1-2 seconds.

#### Challenge 2: API Rate Limiting

**Problem:**
Finnhub API limits requests to 60 calls/minute. With 10 stocks refreshing every 5 seconds, we exceeded the limit.

**Solution:**
- Staggered requests with usleep() between fetches
- Implemented token bucket rate limiter
- Added simulated data fallback when quota exhausted
- Caching of recent responses

#### Challenge 3: SSL/TLS Complexity

**Problem:**
Financial APIs require HTTPS. In C, this requires:
- OpenSSL library initialization
- SSL context creation
- Certificate verification
- Proper error handling (SSL errors don't set errno)

**Solution:**
Created robust SSL wrapper functions with proper initialization, error handling, and cleanup to prevent memory leaks.

#### Challenge 4: JSON Parsing Without Libraries

**Problem:**
C has no built-in JSON parser. External libraries add dependencies.

**Solution:**
Built custom lightweight JSON parser using string matching:
```c
double extract_json_double(const char *json, const char *key) {
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":", key);
    char *pos = strstr(json, search);
    if (pos) {
        pos += strlen(search);
        return atof(pos);
    }
    return 0.0;
}
```

#### Challenge 5: Terminal Raw Mode and Keyboard Input

**Problem:**
Default terminal mode is line-buffered - user must press Enter for input. This prevents responsive keyboard controls.

**Solution:**
Used tcsetattr() to switch to raw mode and select() for non-blocking input:
```c
raw.c_lflag &= ~(ICANON | ECHO);
tcsetattr(STDIN_FILENO, TCSANOW, &raw);
```

---

## 12. PROJECT AND AI

### 12.1 Project Journey and AI Usage

A significant part of MarketPulse's development involved iterative design, debugging, optimization, and feature enhancement using AI-assisted development workflows.

### 12.2 AI Tools Used

#### Cline (AI Development Assistant)
Used throughout the development process for:
- Architecture planning and module design
- Code generation and implementation
- Debugging complex issues
- Documentation generation

#### Groq API (Runtime AI)
Integrated into the application for:
- Real-time stock analysis
- Trading insight generation
- Market sentiment analysis

### 12.3 AI Assistance Used For

**System Design:**
- Architecture planning
- Module decomposition
- System call selection
- Data flow design

**Feature Development:**
- Network module implementation
- Parallel fetching with fork/pipe
- Signal-based alert system
- Memory-mapped history
- Terminal UI rendering

**Debugging:**
- Fork-related issues
- SSL/TLS handshaking problems
- JSON parsing edge cases
- Race conditions in IPC
- Terminal mode restoration

**User Experience Enhancements:**
- Sparkline chart rendering
- Color-coded output
- Keyboard control implementation
- AI insight formatting

**Documentation and Presentation:**
- Technical explanations
- Project documentation
- Presentation content
- Architecture diagrams

### 12.4 Developer Contributions

The final implementation, integration, testing, debugging, and feature customization were performed by the developer. AI assistance was used as a tool to accelerate development and improve code quality, but all design decisions, testing, and final implementation were done by the developer.

---

## 13. FUTURE ENHANCEMENTS

### 13.1 Future Enhancements

**Planned Features:**

#### WebSocket Support
- True real-time streaming instead of polling
- Lower latency price updates
- Reduced API calls

#### Portfolio Tracking
- Track owned stocks
- Calculate profit/loss
- Portfolio performance metrics

#### Historical Data Analysis
- Fetch historical price data
- Backtesting capabilities
- Long-term trend analysis

#### Multiple AI Models
- GPT-4 integration
- Claude integration
- Model comparison

#### Mobile Notifications
- Push notifications via services
- SMS alerts for critical prices
- Email notifications

#### Database Integration
- SQLite for long-term history
- Query historical data
- Export capabilities

#### Options Chain Analysis
- Options pricing data
- Greeks calculation
- Options strategy suggestions

#### Cryptocurrency Support
- Bitcoin, Ethereum monitoring
- Crypto exchange integration
- 24/7 market support

---

## 14. APPLICATIONS

### 14.1 Real-World Applications

**Retail Investors:**
Quick terminal-based market monitoring without expensive subscriptions.

**Day Traders:**
Real-time alerts and AI insights for trading decisions.

**Developers:**
Monitor stocks while coding without leaving the terminal.

**Students:**
Learning system programming concepts through practical application.

**Quant Analysts:**
Building automated trading systems and analysis tools.

**Financial Educators:**
Teaching market concepts with live data.

**DevOps Engineers:**
Monitoring market during deployments and on-call shifts.

---

## 15. CONCLUSION

### 15.1 Conclusion

MarketPulse successfully demonstrates how system programming concepts can be combined with modern AI to create a powerful, practical application.

**Key Achievements:**
- Implemented 15+ system calls (fork, pipe, socket, signal, mmap, select, etc.)
- Parallel processing with fork/pipe architecture reduces fetch time by 10x
- Signal-based alert system with graceful shutdown
- AI integration using Groq API with Llama 3.3 for intelligent trading insights
- Dual market support (US and India) with automatic currency/timezone detection
- Intuitive terminal-based UI with keyboard controls and sparkline charts
- Persistent price history using memory-mapped files

The project transforms traditional stock monitoring into an intelligent, terminal-native experience that demonstrates the power of combining low-level system programming with modern AI capabilities.

**"System programming becomes truly powerful when it solves real-world problems."**

---

**Project Repository:**
[MarketPulse GitHub Repository](https://github.com/jerome-wilson/marketpulse)

**Technologies:** C, Linux System Calls, OpenSSL, Groq AI, Llama 3.3

**Key System Calls:** fork() | pipe() | socket() | signal() | mmap() | select()

---

*Jerome Wilson*
*Systems Programming Project - 2026*