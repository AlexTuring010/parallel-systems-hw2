#!/bin/bash

# Δυναμικό path για το script και τα άλλα directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXECUTABLE="$PROJECT_DIR/bin/game_of_life"
DATA_DIR="$PROJECT_DIR/data"
OUTPUT_FILE="$DATA_DIR/data_for_animation.txt"

# Ensure output file exists and is empty
> "$OUTPUT_FILE"

# Grid size is fixed at 30
GRID_SIZE=30

# Run for generations 1 to 1000
for GENERATION in {1..100}
do
    # Run the Game of Life program with the current generation
    echo "Running generation $GENERATION"
    mpiexec -n 4 "$EXECUTABLE" "$GENERATION" "$GRID_SIZE" >> "$OUTPUT_FILE"
    

    # Add a separator line after each generation output
    echo "==============================" >> "$OUTPUT_FILE"
done

echo "All generations have been processed and saved to $OUTPUT_FILE."
