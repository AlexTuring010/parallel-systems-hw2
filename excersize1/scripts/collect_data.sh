#!/bin/bash

# Δυναμικό path για το script και τα άλλα directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXECUTABLE="$PROJECT_DIR/bin/game_of_life"
DATA_DIR="$PROJECT_DIR/data"

# Πινακες για δοκιμές
DIMENSIONS_LIST=(512 1024)
THREADS_LIST=(1 2 4 8 16 32 64 100 128)

# Εκτέλεση δοκιμών για κάθε κατηγορία darts
for dimension in "${DIMENSIONS_LIST[@]}"; do
    # Δημιουργία αρχείου CSV για κάθε αριθμό dimension
    DATA_FILE="$DATA_DIR/collected_data_for_${dimension}_size.csv"

    rm -f "$DATA_FILE"
    echo "dimension,threads,time" > "$DATA_FILE"

    # Εκτέλεση των δοκιμών για την συγκεκριμένη διάσταση πίνακα
    for threads in "${THREADS_LIST[@]}"; do
        TOTAL_TIME=0
        GENERATIONS=1000

        # Επαναλαμβάνω την εκτέλεση 4 φορές για να υπολογίσω τον μέσο χρόνο
        for _ in {1..1}; do
            # Εκτέλεση του προγράμματος
            OUTPUT=$(mpiexec -f "$PROJECT_DIR/machines" -n "$threads" "$EXECUTABLE" "$GENERATIONS" "$dimension")

            # Άθροιση των χρόνων
            TIME=$(echo "$OUTPUT" | grep "Execution Time:" | awk '{print $3}')  

            # Ανάλυση αποτελεσμάτων με χρήση grep και awk
            TOTAL_TIME=$(echo "$TOTAL_TIME + $TIME" | bc)  
        done

        # Υπολογισμός μέσου όρου χρόνου
        AVARAGE_TIME=$(echo "scale=2; $TOTAL_TIME / 1" | bc)
    
        # Εισαγωγή στο CSV για την συγκεκριμένη διάσταση πίνακα
        echo "$dimension,$threads,$AVARAGE_TIME" >> "$DATA_FILE"
    done

    echo "Tests completed for dimension ${dimension}"
done

echo "Data has been collected and stored inside the ./data/ directory"
