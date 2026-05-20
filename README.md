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
- 🇺🇸 **US market support** (S&P 500)
- 📈 **Technical indicators** (RSI, Moving Averages, Volatility)
- 🔄 **Parallel fetching** using fork() and pipe()
- 💾 **Persistent history** using mmap()
- 📊 **Top market movers** with AI insights

---

## 🛠️ System Programming Concepts

| Concept | System Call | Usage in MarketPulse |
|---------|-------------|---------------------|
| Process Control | `fork()`, `waitpid()` | Parallel stock fetching |
| Inter-Process Communication | `pipe()` | Data transfer between processes |
| Network Programming | `socket()`, `connect()` | API requests to Finnhub |
| Signal Handling | `signal()`, `sigaction()`, `SIGINT`, `SIGALRM` | Graceful shutdown, price alerts |
| I/O Multiplexing | `select()` | Non-blocking keyboard input |
| Memory Mapping | `mmap()`, `msync()` | Persistent price history |
| Named Pipes | `mkfifo()` | Live JSON streaming |
| Terminal Control | `tcgetattr()`, `tcsetattr()` | Raw mode for keyboard |
| SSL/TLS | `SSL_connect()`, `SSL_read()` | Secure HTTPS connections |
| Timer Alarms | `alarm()` | Periodic alert checking |
| Process Termination | `kill()` | Stop background alerts |

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
./marketpulse AAPL              # Fetch single stock with AI insights
./marketpulse watch AAPL MSFT   # Watch multiple stocks
./marketpulse insight NVDA      # Deep AI analysis
./marketpulse top               # Top market movers
./demo.sh                       # Interactive demo
```

### All Commands

| Command | Description |
|---------|-------------|
| `./marketpulse <SYMBOL>` | Fetch single stock quote with AI insights |
| `./marketpulse watch <SYMBOLS...>` | Monitor multiple stocks |
| `./marketpulse watch sp500` | Monitor S&P 500 top 10 |
| `./marketpulse watch nifty50` | Monitor NIFTY50 (Indian) |
| `./marketpulse insight <SYMBOL>` | Deep AI-powered stock analysis |
| `./marketpulse alert <SYMBOL> <PRICE>` | Set price alert |
| `./marketpulse top` | Show top market movers (US/India) |
| `./marketpulse status` | System status |
| `./marketpulse stats` | Performance metrics |
| `./marketpulse stream` | Live JSON stream (mkfifo) |
| `./marketpulse --help` | Show all commands |

### Watch Mode Controls

| Key | Action |
|-----|--------|
| `Q` | Quit |
| `S` | Sort by change % |
| `I` | Show market insights (AI) |
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
| # | Demo | Description |
|---|------|-------------|
| 1 | 🇮🇳 Single Stock + AI | Fetch TCS.BSE with AI insights |
| 2 | 🇮🇳 NIFTY50 Watch | Monitor Indian market |
| 3 | 🇺🇸 S&P 500 Watch | Monitor US market |
| 4 | 📊 Top Movers | Gainers & losers (select US/India) |
| 5 | 🤖 AI Stock Analysis | Deep AI insights (select stock) |
| 6 | 🚨 Price Alerts | Signal-based alert system |
| 7 | ⚙️ System Status | Resource monitoring |
| 8 | ❓ Help | All available commands |

---

## 🚨 Price Alert System

The alert system demonstrates **Unix signal handling** - a core system programming concept.

### Usage
```bash
./marketpulse alert TSLA 200    # Alert when TSLA crosses $200
```

### System Calls Used

| System Call | Purpose |
|-------------|---------|
| `signal()` / `sigaction()` | Handle SIGINT (Ctrl+C), SIGALRM (timer) |
| `alarm()` | Periodic price checking |
| `fork()` | Background monitoring |
| `kill()` | Stop background alerts |
| `waitpid()` | Reap zombie processes |

### How It Works

1. **Setup**: Signal handlers are registered for SIGINT, SIGALRM, SIGCHLD
2. **Monitor Loop**: Fetches price every 10 seconds
3. **Threshold Check**: Detects when price crosses threshold (up or down)
4. **Alert Trigger**: Visual flash + audio bell when triggered
5. **Continue Option**: Asks if you want to keep monitoring

### Features
- ✅ Visual terminal flash alerts
- ✅ Audio bell notifications (`\a`)
- ✅ Bidirectional threshold crossing
- ✅ Trend indicators (▲/▼)
- ✅ Graceful shutdown with Ctrl+C
- ✅ Background mode with `fork()`

### Sample Output
```
Alert Monitoring Started

  Symbol:    TSLA
  Threshold: $200.00
  Interval:  10 seconds

  [15:30:45] Current price: $178.90 (threshold: $200.00)
  [15:30:55] Check #1: $179.12 (below threshold) ▲
  [15:31:05] Check #2: $179.38 (below threshold) ▲

  🚨 PRICE ALERT 🚨
  Symbol:    TSLA
  Price:     $200.15
  Threshold: $200.00
  Status:    PRICE ABOVE THRESHOLD
```

---

## 📊 Top Market Movers

View top gainers and losers with AI-powered market analysis.

### Usage
```bash
./marketpulse top
```

### Features
- **Market Selection**: Choose between US (NYSE/NASDAQ) or India (NSE/BSE)
- **Top 5 Gainers**: Stocks with highest positive change
- **Top 5 Losers**: Stocks with highest negative change
- **Market Sentiment**: Bullish/Bearish/Mixed indicator
- **AI Commentary**: AI-generated market analysis

### Sample Output
```
╔══════════════════════════════════════════════════════════════╗
║          🇮🇳  Top Market Movers — India (NSE/BSE)          ║
╚══════════════════════════════════════════════════════════════╝

  🟢 TOP GAINERS                            🔴 TOP LOSERS
  ────────────────────────────────────  ────────────────────────────────────
  ICICIBANK.BSE  ₹  1275.02   +1.49% ▲  BHARTIARTL.BSE ₹  1649.69   -1.74% ▼
  TCS.BSE        ₹  4099.61   +1.19% ▲  HDFCBANK.BSE   ₹  1651.80   -1.22% ▼
  ...

  Market Sentiment: Mixed (-0.02% avg across 10 stocks)

  ═══ AI Market Analysis ═══
  🤖 The market is showing a mixed sentiment with 5 stocks rising and 5
  falling. Traders can consider buying into top gainers like ICICIBANK.BSE...
```

---

## 🤖 AI Integration

MarketPulse uses **Groq API** (Llama 3.3) for AI-powered insights:

### Single Stock Fetch (Automatic AI)
```bash
./marketpulse TCS.BSE
```
Now automatically includes AI insights with:
- Current trend assessment
- Support/resistance levels
- Trading suggestions

### Deep AI Analysis
```bash
./marketpulse insight AAPL
```
Output includes:
- Technical analysis (Trend, Momentum, RSI)
- Moving averages (MA5, MA10)
- Golden Cross / Death Cross signals
- Detailed AI commentary

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
- Free tier: 30 requests/minute, 14,400 requests/day

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
./marketpulse TCS.BSE
./marketpulse insight RELIANCE.BSE
```

### Supported Indian Stocks
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

---

## 🇺🇸 US Market Support

Monitor S&P 500 stocks with USD currency:

```bash
./marketpulse watch sp500
./marketpulse AAPL
./marketpulse insight NVDA
```

### Supported US Stocks
| Symbol | Company |
|--------|---------|
| AAPL | Apple Inc. |
| MSFT | Microsoft Corp |
| GOOGL | Alphabet Inc |
| AMZN | Amazon.com Inc |
| NVDA | NVIDIA Corp |
| META | Meta Platforms |
| TSLA | Tesla Inc |
| AMD | Advanced Micro Devices |
| INTC | Intel Corp |
| ORCL | Oracle Corp |

---

## 📁 Project Structure

```
marketpulse/
├── src/
│   ├── main.c          # Entry point, command routing
│   ├── cli.c           # Command line parser
│   ├── network.c       # Socket & SSL networking
│   ├── parser.c        # JSON parsing
│   ├── monitor.c       # Watch mode & display
│   ├── alert.c         # Price alert system (signal handling)
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
| Groq | AI insights | `config/groq_key` or `GROQ_API_KEY` env |

### Environment Variables

```bash
export GROQ_API_KEY="your-groq-key"
```

---

## 📝 Example Output

### Single Stock with AI
```
╔══════════════════════════════════════════════════════════════╗
║           MarketPulse - Stock Monitoring Engine              ║
╚══════════════════════════════════════════════════════════════╝

──────────────────────────────────────────────────────────────────
 Stock: TCS.BSE (Tata Consultancy)
──────────────────────────────────────────────────────────────────

  Price:      ₹4038.44 ▼
  Change:     -12.96 (-0.32%)
  Open:       ₹4044.92
  High:       ₹4078.82
  Low:        ₹3998.05
  Prev Close: ₹4051.40

  ═══ AI Insights ═══

  🤖 AI Commentary:
    TCS.BSE stock is currently experiencing a slight downtrend, with a
  minor 0.32% decline. Key support levels are at ₹3998 and ₹3950,
  while resistance levels are at ₹4078 and ₹4150. For traders, buy
  on dips near ₹3998 with a target of ₹4078.
```

### AI Insight
```
═══ AI Insight for NVDA ═══

  Stock: NVDA (NVIDIA Corp)
  Price: $875.20
  Change: +5.30 (+0.61%)

  ── Technical Analysis ──
  📈 Trend:      Bullish
  ⚡ Momentum:   Strong
  📊 MA(5):      $875.20
  📊 MA(10):     $862.15
  📈 Volatility: 2.3%
  📊 RSI(14):    68.5
  ✨ Signal:     Golden Cross (MA5 above MA10)

  ── AI Commentary ──
  🤖 NVDA shows strong bullish momentum with consistent gains.
     Consider holding positions with a stop-loss at $850.
```

---

## 🧪 Testing

```bash
# Build
make clean && make

# Test single stock (US)
./marketpulse AAPL

# Test single stock (India)
./marketpulse TCS.BSE

# Test watch mode
./marketpulse watch AAPL MSFT GOOGL

# Test AI insight
./marketpulse insight NVDA

# Test top movers
./marketpulse top

# Run demo
./demo.sh
```

---

## 📄 License

This project is created for educational purposes as part of a System Programming course.

---

## 👨‍💻 Author

**Jerome Wilson**

System Programming Project - 2026

---

<div align="center">

**⭐ Star this repo if you found it helpful! ⭐**

</div>