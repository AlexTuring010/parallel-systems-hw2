# Compiler and flags
CC = mpicc
CFLAGS = -Wall -O0 -Iincludes
LDFLAGS = 

# Directories and files
SRC = ./src/game_of_life.c
BIN_DIR = ./bin
TARGET = $(BIN_DIR)/game_of_life

# Default target to build the program
all: $(BIN_DIR) $(TARGET)

# Rule to create the bin directory
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# Rule to compile the source file into the executable
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET) $(SRC)

# Rule to compile the program with TEST_MODE defined
print_grid: $(SRC)
	$(CC) $(CFLAGS) $(LDFLAGS) -DPRINT_GRID -o $(TARGET) $(SRC)

# Clean up build files
clean:
	rm -rf $(BIN_DIR)/*

# Phony targets
.PHONY: all clean
