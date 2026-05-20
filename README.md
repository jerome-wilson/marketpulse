# MarketPulse 📈

<div align="center">

```
███╗   ███╗ █████╗ ██████╗ ██╗  ██╗███████╗████████╗
████╗ ████║██╔══██╗██╔══██╗██║ ██╔╝██╔════╝╚══██╔══╝
██╔████╔██║███████║██████╔╝█████╔╝ █████╗     ██║   
██║╚██╔╝██║██╔══██║██╔══██╗██╔═██╗ ██╔══╝     ██║   
██║ ╚═╝ ██║██║  ██║██║  ██║██║  ██╗███████╗   ██║   
╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝   ╚═╝   

██████╗ ██╗   ██╗██╗     ███████╗███████╗
██╔══██╗██║   ██║██║     ██╔════╝██╔════╝
██████╔╝██║   ██║██║     ███████╗█████╗  
██╔═══╝ ██║   ██║██║     ╚════██║██╔══╝  
██║     ╚██████╔╝███████╗███████║███████╗
╚═╝      ╚═════╝ ╚══════╝╚══════╝╚══════╝
```

**Real-Time Stock Monitoring & AI Insight Engine**

[![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)](https://en.wikipedia.org/wiki/C_(programming_language))
[![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)](https://www.linux.org/)
[![macOS](https://img.shields.io/badge/macOS-000000?style=for-the-badge&logo=apple&logoColor=white)](https://www.apple.com/macos/)

*A System Programming Project demonstrating fork(), socket(), signal(), mmap(), and more*

</div>

---

## 🎯 Overview

MarketPulse is a terminal-based stock monitoring application that fetches real-time stock prices, provides AI-powered insights, and demonstrates key system programming concepts. Built entirely in C, it runs on Linux and macOS.

### Key Features

- 📊 **Real-time stock quotes** from Finnhub API
- 👁️ **Multi-stock watch mode** with live updates
- 🤖 **AI-powered analysis** using Groq (Llama 3.3)
- 🚨 **Price alerts** with signal handling
- 🇮🇳 **Indian market support** (NIFTY50/BSE)
- 📈 **Technical indicators** (RSI, Moving Averages, Volatility)
- 🔄 **Parallel fetching** using fork() and pipe()
- 💾 **Persistent history** using mmap()

---

## 🛠️ System Programming Concepts

| Concept | System Call | Usage in MarketPulse |
|---------|-------------|---------------------|
| Process Control | `fork()`, `waitpid()` | Parallel stock fetching |
| Inter-Process Communication | `pipe()` | Data transfer between processes |
| Network Programming | `socket()`, `connect()` | API requests to Finnhub |
| Signal Handling | `signal()`, `SIGINT`, `SIGALRM` | Graceful shutdown, alerts |
| I/O Multiplexing | `select()` | Non-blocking keyboard input |
| Memory Mapping | `mmap()`, `msync()` | Persistent price history |
| Named Pipes | `mkfifo()` | Live JSON streaming |
| Terminal Control | `tcgetattr()`, `tcsetattr()` | Raw mode for keyboard |
| SSL/TLS | `SSL_connect()`, `SSL_read()` | Secure HTTPS connections |

---

## 📦 Installation

### Prerequisites

- GCC compiler
- OpenSSL development libraries
- Make

### macOS
```bash
brew install openssl@3
```

### Ubuntu/Debian
```bash
sudo apt-get install libssl-dev build-essential
```

### Build
```bash
git clone https://github.com/jerome-wilson/marketpulse.git
cd marketpulse
make
```

---

## 🚀 Usage

### Quick Start
```bash
./marketpulse AAPL              # Fetch single stock
./marketpulse watch AAPL MSFT   # Watch multiple stocks
./marketpulse insight NVDA      # AI analysis
./demo.sh                       # Interactive demo
```

### All Commands

| Command | Description |
|---------|-------------|
| `./marketpulse <SYMBOL>` | Fetch single stock quote |
| `./marketpulse watch <SYMBOLS...>` | Monitor multiple stocks |
| `./marketpulse watch sp500` | Monitor S&P 500 top 10 |
| `./marketpulse watch nifty50` | Monitor NIFTY50 (Indian) |
| `./marketpulse insight <SYMBOL>` | AI-powered stock analysis |
| `./marketpulse alert <SYMBOL> <PRICE>` | Set price alert |
| `./marketpulse top` | Show top market movers |
| `./marketpulse status` | System status |
| `./marketpulse stats` | Performance metrics |
| `./marketpulse stream` | Live JSON stream (mkfifo) |
| `./marketpulse --help` | Show all commands |

### Watch Mode Controls

| Key | Action |
|-----|--------|
| `Q` | Quit |
| `S` | Sort by change % |
| `I` | Show market insights |
| `P` | Pause/Resume |
| `+` | Faster refresh |
| `-` | Slower refresh |
| `Esc` | Close insights panel |

---

## 🎬 Demo Script

Run the interactive demo for presentations:

```bash
./demo.sh           # Interactive menu
./demo.sh all       # Run all demos automatically
./demo.sh 1         # Run specific demo (1-8)
```

### Demo Menu
1. Single Stock Fetch
2. Multi-Stock Watch Mode
3. Top Market Movers
4. AI Stock Analysis
5. Price Alert System
6. System Status
7. Indian Market (NIFTY50)
8. Help & Commands

---

## 🤖 AI Integration

MarketPulse uses **Groq API** (Llama 3.3) for AI-powered insights:

### Individual Stock Analysis
```bash
./marketpulse insight AAPL
```
Output includes:
- Technical analysis (Trend, Momentum, RSI)
- Moving averages (MA5, MA10)
- Golden Cross / Death Cross signals
- AI commentary with trading suggestions

### Market Overview
Press `I` in watch mode to see:
- Overall market sentiment
- Top gainers and losers
- AI market commentary

### API Key Setup
```bash
# Option 1: Environment variable
export GROQ_API_KEY="your-key-here"

# Option 2: Config file
echo "your-key-here" > config/groq_key
```

Get your free API key at: https://console.groq.com

---

## 📊 Technical Indicators

| Indicator | Description |
|-----------|-------------|
| **MA(5)** | 5-period moving average |
| **MA(10)** | 10-period moving average |
| **RSI(14)** | Relative Strength Index |
| **Volatility** | Price volatility percentage |
| **Golden Cross** | MA5 crosses above MA10 (bullish) |
| **Death Cross** | MA5 crosses below MA10 (bearish) |

---

## 🇮🇳 Indian Market Support

Monitor NIFTY50 stocks with INR currency:

```bash
./marketpulse watch nifty50
```

Supported stocks:
- RELIANCE.BSE
- TCS.BSE
- HDFCBANK.BSE
- INFY.BSE
- ICICIBANK.BSE
- And more...

---

## 📁 Project Structure

```
marketpulse/
├── src/
│   ├── main.c          # Entry point
│   ├── cli.c           # Command line parser
│   ├── network.c       # Socket & SSL networking
│   ├── parser.c        # JSON parsing
│   ├── monitor.c       # Watch mode & display
│   ├── alert.c         # Price alert system
│   ├── ai.c            # Technical analysis
│   ├── ai_insights.c   # Groq AI integration
│   ├── history.c       # mmap price history
│   ├── stream.c        # mkfifo live stream
│   ├── system.c        # System status
│   └── utils.c         # Utility functions
├── include/
│   └── marketpulse.h   # Main header
├── config/
│   └── groq_key        # API key (gitignored)
├── demo.sh             # Interactive demo script
├── Makefile            # Build configuration
└── README.md           # This file
```

---

## 🔧 Configuration

### API Keys

| API | Purpose | Config |
|-----|---------|--------|
| Finnhub | Stock quotes | Built-in (demo key) |
| Groq | AI insights | `config/groq_key` |

### Environment Variables

```bash
export GROQ_API_KEY="your-groq-key"
export GEMINI_API_KEY="your-gemini-key"  # Alternative
```

---

## 📝 Example Output

### Single Stock
```
╔══════════════════════════════════════════════════════════════╗
║           MarketPulse - Stock Monitoring Engine              ║
╚══════════════════════════════════════════════════════════════╝

──────────────────────────────────────────────────────────────────
 Stock: AAPL (Apple Inc)
──────────────────────────────────────────────────────────────────

  Price:      $189.42 ▲
  Change:     +1.23 (+0.65%)
  Open:       $188.50
  High:       $190.10
  Low:        $187.80
  Prev Close: $188.19
```

### AI Insight
```
═══ AI Insight for NVDA ═══

  📈 Trend:      Bullish
  ⚡ Momentum:   Strong
  📊 MA(5):      $875.20
  📊 MA(10):     $862.15
  📈 Volatility: 2.3%
  📊 RSI(14):    68.5
  ✨ Signal:     Golden Cross (MA5 above MA10)

  🤖 NVDA shows strong bullish momentum with consistent gains.
     Consider holding positions with a stop-loss at $850.
```

---

## 🧪 Testing

```bash
# Build
make clean && make

# Test single stock
./marketpulse AAPL

# Test watch mode
./marketpulse watch AAPL MSFT GOOGL

# Test AI insight
./marketpulse insight NVDA

# Run demo
./demo.sh
```

---

## 📄 License

This project is created for educational purposes as part of a System Programming course.

---

## 👨‍💻 Author

**Jerome Wilson**

System Programming Project - 2024

---

<div align="center">

**⭐ Star this repo if you found it helpful! ⭐**

</div>