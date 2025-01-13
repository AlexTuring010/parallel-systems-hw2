# Compiler and flags
CC = gcc
CFLAGS = -fopenmp -Wall -O0 -Iincludes
LDFLAGS = -fopenmp -lm

# Directories and files
SRC = ./src/main.c
BIN_DIR = ./bin
TARGET = $(BIN_DIR)/main
TEST_TARGET = $(BIN_DIR)/main_test

# Default target to build the program
all: $(TARGET)

# Rule to compile the source file into the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

# Rule to compile the program with TEST_MODE defined
test: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -DTEST_MODE -o $(TEST_TARGET) $(SRC)

# Clean up build files
clean:
	rm -rf $(BIN_DIR)/*

# Phony targets
.PHONY: all clean test
