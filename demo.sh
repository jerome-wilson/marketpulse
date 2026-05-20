#!/bin/bash
#
# MarketPulse Demo Script
# Interactive demonstration of all features
#
# Usage:
#   ./demo.sh           # Interactive menu
#   ./demo.sh all       # Run all demos automatically
#   ./demo.sh <number>  # Run specific demo
#

# ═══════════════════════════════════════════════════════════════════════════
# COLORS
# ═══════════════════════════════════════════════════════════════════════════
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
MAGENTA='\033[0;35m'
CYAN='\033[0;36m'
WHITE='\033[1;37m'
BOLD='\033[1m'
RESET='\033[0m'

# ═══════════════════════════════════════════════════════════════════════════
# ASCII ART BANNER
# ═══════════════════════════════════════════════════════════════════════════
show_banner() {
    clear
    echo ""
    echo -e "${GREEN}${BOLD}"
    echo "    ███╗   ███╗ █████╗ ██████╗ ██╗  ██╗███████╗████████╗"
    echo "    ████╗ ████║██╔══██╗██╔══██╗██║ ██╔╝██╔════╝╚══██╔══╝"
    echo "    ██╔████╔██║███████║██████╔╝█████╔╝ █████╗     ██║   "
    echo "    ██║╚██╔╝██║██╔══██║██╔══██╗██╔═██╗ ██╔══╝     ██║   "
    echo "    ██║ ╚═╝ ██║██║  ██║██║  ██║██║  ██╗███████╗   ██║   "
    echo "    ╚═╝     ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚═╝  ╚═╝╚══════╝   ╚═╝   "
    echo -e "${RESET}"
    echo -e "${RED}${BOLD}"
    echo "    ██████╗ ██╗   ██╗██╗     ███████╗███████╗"
    echo "    ██╔══██╗██║   ██║██║     ██╔════╝██╔════╝"
    echo "    ██████╔╝██║   ██║██║     ███████╗█████╗  "
    echo "    ██╔═══╝ ██║   ██║██║     ╚════██║██╔══╝  "
    echo "    ██║     ╚██████╔╝███████╗███████║███████╗"
    echo "    ╚═╝      ╚═════╝ ╚══════╝╚══════╝╚══════╝"
    echo -e "${RESET}"
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -e "${YELLOW}${BOLD}    📈 Real-Time Stock Monitoring & AI Insight Engine 📉${RESET}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo ""
    echo -e "${WHITE}    System Programming Project | fork() • socket() • signal() • mmap()${RESET}"
    echo ""
}

# ═══════════════════════════════════════════════════════════════════════════
# HELPER FUNCTIONS
# ═══════════════════════════════════════════════════════════════════════════
press_enter() {
    echo ""
    echo -e "${YELLOW}Press Enter to continue...${RESET}"
    read -r
}

run_command() {
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -e "${GREEN}${BOLD}$ $1${RESET}"
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo ""
    eval "$1"
}

# ═══════════════════════════════════════════════════════════════════════════
# DEMO FUNCTIONS
# ═══════════════════════════════════════════════════════════════════════════

demo_single_stock() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 1: Single Stock Fetch + AI Insights (Indian Stock)     ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Fetching real-time stock data for TCS (BSE) with AI analysis${RESET}"
    echo -e "${YELLOW}System calls: socket(), connect(), SSL_read(), SSL_write()${RESET}"
    echo -e "${CYAN}AI: Groq API (Llama 3.3) provides trading insights automatically${RESET}"
    run_command "./marketpulse TCS.BSE"
    press_enter
}

demo_indian_market() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 2: Indian Market (NIFTY50)                             ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Monitoring NIFTY50 stocks (BSE/NSE)${RESET}"
    echo -e "${YELLOW}Features: INR currency, IST timezone, Indian market hours${RESET}"
    echo ""
    echo -e "${CYAN}Controls: [Q] Quit  [S] Sort  [I] Market Overview  [P] Pause${RESET}"
    echo -e "${RED}Press Q to exit when ready...${RESET}"
    echo ""
    echo -e "${GREEN}Starting NIFTY50 watch mode...${RESET}"
    sleep 2
    
    ./marketpulse watch nifty50 2>/dev/null || true
    
    press_enter
}

demo_sp500() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 3: S&P 500 Watch Mode                                  ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Monitoring top S&P 500 stocks (AAPL, MSFT, GOOGL, etc.)${RESET}"
    echo -e "${YELLOW}System calls: fork(), pipe(), waitpid(), select()${RESET}"
    echo ""
    echo -e "${CYAN}Controls: [Q] Quit  [S] Sort  [I] Market Overview  [P] Pause${RESET}"
    echo -e "${RED}Press Q to exit when ready...${RESET}"
    echo ""
    echo -e "${GREEN}Starting S&P 500 watch mode...${RESET}"
    sleep 2
    
    ./marketpulse watch sp500 2>/dev/null || true
    
    press_enter
}

demo_top_movers() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 4: Top Market Movers                                   ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Displaying top gainers and losers${RESET}"
    echo -e "${YELLOW}System calls: fork() for parallel fetching${RESET}"
    echo ""
    echo -e "${CYAN}Select market: 1 for US, 2 for India${RESET}"
    echo ""
    ./marketpulse top
    press_enter
}

demo_ai_insight() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 5: AI Stock Analysis                                   ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}AI-powered stock analysis using Groq (Llama 3.3)${RESET}"
    echo -e "${YELLOW}Features: Technical analysis, RSI, Moving Averages, AI Commentary${RESET}"
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo ""
    echo -e "${WHITE}${BOLD}  Select a stock to analyze:${RESET}"
    echo ""
    echo -e "  ${GREEN}🇮🇳 Indian Stocks (BSE/NSE)${RESET}"
    echo -e "     ${CYAN}1)${RESET} RELIANCE.BSE  - Reliance Industries"
    echo -e "     ${CYAN}2)${RESET} TCS.BSE       - Tata Consultancy"
    echo -e "     ${CYAN}3)${RESET} INFY.BSE      - Infosys"
    echo -e "     ${CYAN}4)${RESET} HDFCBANK.BSE  - HDFC Bank"
    echo ""
    echo -e "  ${GREEN}🇺🇸 US Stocks (NYSE/NASDAQ)${RESET}"
    echo -e "     ${CYAN}5)${RESET} AAPL          - Apple Inc."
    echo -e "     ${CYAN}6)${RESET} NVDA          - NVIDIA Corp"
    echo -e "     ${CYAN}7)${RESET} MSFT          - Microsoft"
    echo -e "     ${CYAN}8)${RESET} GOOGL         - Alphabet (Google)"
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -n "  Enter choice (1-8): "
    read -r stock_choice
    
    case $stock_choice in
        1) run_command "./marketpulse insight RELIANCE.BSE" ;;
        2) run_command "./marketpulse insight TCS.BSE" ;;
        3) run_command "./marketpulse insight INFY.BSE" ;;
        4) run_command "./marketpulse insight HDFCBANK.BSE" ;;
        5) run_command "./marketpulse insight AAPL" ;;
        6) run_command "./marketpulse insight NVDA" ;;
        7) run_command "./marketpulse insight MSFT" ;;
        8) run_command "./marketpulse insight GOOGL" ;;
        *) 
            echo -e "${YELLOW}Invalid choice. Running default (NVDA)...${RESET}"
            run_command "./marketpulse insight NVDA"
            ;;
    esac
    press_enter
}

demo_alert() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 6: Price Alert System                                  ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Setting up price alerts with signal handling${RESET}"
    echo -e "${YELLOW}System calls: signal(), SIGALRM, fork()${RESET}"
    echo ""
    echo -e "${CYAN}Command: ./marketpulse alert TSLA 200${RESET}"
    echo -e "${RED}(Alert monitoring runs continuously - press Ctrl+C to stop)${RESET}"
    echo ""
    echo -e "${GREEN}Simulating alert trigger...${RESET}"
    echo ""
    echo -e "${YELLOW}Alert would monitor TSLA and trigger when price crosses \$200${RESET}"
    press_enter
}

demo_system_status() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 7: System Status & Introspection                       ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Displaying system status and resource usage${RESET}"
    echo -e "${YELLOW}System calls: getrusage(), shmget(), semget()${RESET}"
    run_command "./marketpulse status"
    press_enter
}

demo_help() {
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 8: Help & Available Commands                           ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    run_command "./marketpulse --help"
    press_enter
}

# ═══════════════════════════════════════════════════════════════════════════
# MENU
# ═══════════════════════════════════════════════════════════════════════════
show_menu() {
    show_banner
    echo -e "${WHITE}${BOLD}  Select a demo to run:${RESET}"
    echo ""
    echo -e "  ${GREEN}1)${RESET} 🇮🇳 Single Stock + AI      ${CYAN}Fetch price with AI insights${RESET}"
    echo -e "  ${GREEN}2)${RESET} 🇮🇳 NIFTY50 Watch          ${CYAN}Monitor Indian market${RESET}"
    echo -e "  ${GREEN}3)${RESET} 🇺🇸 S&P 500 Watch          ${CYAN}Monitor US market${RESET}"
    echo -e "  ${GREEN}4)${RESET} 📊 Top Movers              ${CYAN}Gainers & losers${RESET}"
    echo -e "  ${GREEN}5)${RESET} 🤖 AI Stock Analysis       ${CYAN}Deep AI insights${RESET}"
    echo -e "  ${GREEN}6)${RESET} 🚨 Price Alerts            ${CYAN}Signal-based alerts${RESET}"
    echo -e "  ${GREEN}7)${RESET} ⚙️  System Status           ${CYAN}Resource monitoring${RESET}"
    echo -e "  ${GREEN}8)${RESET} ❓ Help                    ${CYAN}All commands${RESET}"
    echo ""
    echo -e "  ${YELLOW}A)${RESET} Run ALL demos sequentially"
    echo -e "  ${RED}Q)${RESET} Quit"
    echo ""
    echo -e "${CYAN}━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━${RESET}"
    echo -n "  Enter choice: "
}

run_all_demos() {
    demo_single_stock
    demo_indian_market
    demo_sp500
    demo_top_movers
    # Skip interactive AI insight in auto mode - run default
    show_banner
    echo -e "${MAGENTA}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${MAGENTA}${BOLD}║  DEMO 5: AI Stock Analysis                                   ║${RESET}"
    echo -e "${MAGENTA}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}AI-powered stock analysis using Groq (Llama 3.3)${RESET}"
    run_command "./marketpulse insight NVDA"
    press_enter
    
    demo_alert
    demo_system_status
    demo_help
    
    show_banner
    echo -e "${GREEN}${BOLD}╔══════════════════════════════════════════════════════════════╗${RESET}"
    echo -e "${GREEN}${BOLD}║  ✅ All demos completed!                                     ║${RESET}"
    echo -e "${GREEN}${BOLD}╚══════════════════════════════════════════════════════════════╝${RESET}"
    echo ""
    echo -e "${WHITE}Thank you for watching the MarketPulse demonstration!${RESET}"
    echo ""
}

# ═══════════════════════════════════════════════════════════════════════════
# MAIN
# ═══════════════════════════════════════════════════════════════════════════

# Check if marketpulse binary exists
if [ ! -f "./marketpulse" ]; then
    echo -e "${RED}Error: ./marketpulse not found. Run 'make' first.${RESET}"
    exit 1
fi

# Handle command line arguments
if [ "$1" == "all" ]; then
    run_all_demos
    exit 0
elif [ "$1" == "1" ]; then
    demo_single_stock
    exit 0
elif [ "$1" == "2" ]; then
    demo_indian_market
    exit 0
elif [ "$1" == "3" ]; then
    demo_sp500
    exit 0
elif [ "$1" == "4" ]; then
    demo_top_movers
    exit 0
elif [ "$1" == "5" ]; then
    demo_ai_insight
    exit 0
elif [ "$1" == "6" ]; then
    demo_alert
    exit 0
elif [ "$1" == "7" ]; then
    demo_system_status
    exit 0
elif [ "$1" == "8" ]; then
    demo_help
    exit 0
fi

# Interactive menu loop
while true; do
    show_menu
    read -r choice
    
    case $choice in
        1) demo_single_stock ;;
        2) demo_indian_market ;;
        3) demo_sp500 ;;
        4) demo_top_movers ;;
        5) demo_ai_insight ;;
        6) demo_alert ;;
        7) demo_system_status ;;
        8) demo_help ;;
        [Aa]) run_all_demos ;;
        [Qq]) 
            echo ""
            echo -e "${GREEN}Goodbye! 👋${RESET}"
            echo ""
            exit 0 
            ;;
        *)
            echo -e "${RED}Invalid choice. Please try again.${RESET}"
            sleep 1
            ;;
    esac
done