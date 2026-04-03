# MarketPulse - Real-Time Stock Monitoring and Insight Engine
# Advanced Makefile with all modules

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g -O2
CFLAGS += -Wno-newline-eof -Wno-unused-parameter -Wno-format
CFLAGS += -Wno-tautological-compare -Wno-unused-function
CFLAGS += -I/opt/homebrew/opt/openssl@3/include
CFLAGS += -Iinclude

# Linker flags
LDFLAGS = -L/opt/homebrew/opt/openssl@3/lib
LDLIBS = -lssl -lcrypto

# Directories
SRC_DIR = src
OBJ_DIR = obj
INC_DIR = include
CONFIG_DIR = config
DATA_DIR = data
LOGS_DIR = logs

# Target
TARGET = ./marketpulse

# Source files (all modules)
SOURCES = $(SRC_DIR)/main.c \
          $(SRC_DIR)/cli.c \
          $(SRC_DIR)/network.c \
          $(SRC_DIR)/parser.c \
          $(SRC_DIR)/monitor.c \
          $(SRC_DIR)/alert.c \
          $(SRC_DIR)/ai.c \
          $(SRC_DIR)/utils.c \
          $(SRC_DIR)/ipc.c \
          $(SRC_DIR)/master.c \
          $(SRC_DIR)/daemon.c \
          $(SRC_DIR)/logger.c \
          $(SRC_DIR)/config.c \
          $(SRC_DIR)/system.c

# Object files
OBJECTS = $(SOURCES:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Header files
HEADERS = $(INC_DIR)/marketpulse.h \
          $(INC_DIR)/ipc.h \
          $(INC_DIR)/daemon.h \
          $(INC_DIR)/logger.h \
          $(INC_DIR)/config.h

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR)
	@mkdir -p $(CONFIG_DIR)
	@mkdir -p $(DATA_DIR)
	@mkdir -p $(LOGS_DIR)

# Link
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS) $(LDLIBS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS)
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -rf $(OBJ_DIR)
	rm -f $(TARGET)
	@echo "Clean complete."

# Clean everything including data
distclean: clean
	rm -rf $(DATA_DIR)/*.db
	rm -rf $(LOGS_DIR)/*.log
	rm -f $(CONFIG_DIR)/marketpulse.json
	rm -f /tmp/marketpulse.pid
	rm -f /tmp/marketpulse.log

# Install (copy to /usr/local/bin)
install: $(TARGET)
	@echo "Installing to /usr/local/bin..."
	cp $(TARGET) /usr/local/bin/marketpulse
	@echo "Install complete."

# Uninstall
uninstall:
	rm -f /usr/local/bin/marketpulse

# Run tests
test: $(TARGET)
	@echo "Running basic tests..."
	$(TARGET) --help
	$(TARGET) --version
	@echo "Tests complete."

# Run with sample stocks
run: $(TARGET)
	$(TARGET) watch AAPL MSFT GOOGL

# Run single stock
single: $(TARGET)
	$(TARGET) AAPL

# Run in daemon mode
daemon: $(TARGET)
	$(TARGET) daemon watch AAPL MSFT GOOGL

# Stop daemon
stop:
	$(TARGET) daemon stop

# Check daemon status
status:
	$(TARGET) daemon status

# Debug build
debug: CFLAGS += -DDEBUG -g3 -O0
debug: clean all

# Release build
release: CFLAGS += -DNDEBUG -O3
release: clean all

# Show help
help:
	@echo "MarketPulse Makefile"
	@echo ""
	@echo "Targets:"
	@echo "  all       - Build the project (default)"
	@echo "  clean     - Remove build artifacts"
	@echo "  distclean - Remove all generated files"
	@echo "  install   - Install to /usr/local/bin"
	@echo "  uninstall - Remove from /usr/local/bin"
	@echo "  test      - Run basic tests"
	@echo "  run       - Run with sample stocks"
	@echo "  single    - Fetch single stock (AAPL)"
	@echo "  daemon    - Run in daemon mode"
	@echo "  stop      - Stop daemon"
	@echo "  status    - Check daemon status"
	@echo "  debug     - Build with debug symbols"
	@echo "  release   - Build optimized release"
	@echo "  help      - Show this help"

.PHONY: all clean distclean install uninstall test run single daemon stop status debug release help directories