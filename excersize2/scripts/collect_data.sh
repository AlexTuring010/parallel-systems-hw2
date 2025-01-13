#!/bin/bash

# Δυναμικό path για το script και τα άλλα directories
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXECUTABLE="$PROJECT_DIR/bin/main"
DATA_DIR="$PROJECT_DIR/data"

# Πινακες για δοκιμές
DIMENSIONS_LIST=(64 1024 4096 8192 16384)
THREADS_LIST=(1 2 4 8 16)

# Εκτέλεση δοκιμών για κάθε κατηγορία darts
for dimension in "${DIMENSIONS_LIST[@]}"; do
    # Δημιουργία αρχείου CSV για κάθε αριθμό dimension
    DATA_FILE="$DATA_DIR/collected_data_for_${dimension}_size.csv"

    rm -f "$DATA_FILE"
    echo "dimension,threads,row_time,column_time" > "$DATA_FILE"

    # Εκτέλεση των δοκιμών για την συγκεκριμένη διάσταση πίνακα
    for threads in "${THREADS_LIST[@]}"; do
        TOTAL_ROW_TIME=0
        TOTAL_COLUMN_TIME=0

        algorithm_type="parallel"
        if [ "$threads" -eq 1 ]; then
            algorithm_type="serial"
        fi
        
        # Επαναλαμβάνω την εκτέλεση 4 φορές για να υπολογίσω τον μέσο χρόνο
        for _ in {1..4}; do
            # Εκτέλεση του προγράμματος
            OUTPUT1=$("$EXECUTABLE" "$dimension" "$algorithm_type" "row" "$threads") 
            OUTPUT2=$("$EXECUTABLE" "$dimension" "$algorithm_type" "column" "$threads") 

            # Ανάλυση αποτελεσμάτων με χρήση grep και awk
            ROW_TIME=$(echo "$OUTPUT1" | grep "Execution Time:" | awk '{print $3}')
            COLUMN_TIME=$(echo "$OUTPUT2" | grep "Execution Time:" | awk '{print $3}')
    
            # Άθροιση των χρόνων
            TOTAL_ROW_TIME=$(echo "$TOTAL_ROW_TIME + $ROW_TIME" | bc)
            TOTAL_COLUMN_TIME=$(echo "$TOTAL_COLUMN_TIME + $COLUMN_TIME" | bc)
        done

        # Υπολογισμός μέσου όρου χρόνου
        AVERAGE_ROW_TIME=$(echo "$TOTAL_ROW_TIME / 4" | bc -l)
        AVERAGE_COLUMN_TIME=$(echo "$TOTAL_COLUMN_TIME / 4" | bc -l)
    
        # Εισαγωγή στο CSV για την συγκεκριμένη διάσταση πίνακα
        echo "$dimension,$threads,$AVERAGE_ROW_TIME,$AVERAGE_COLUMN_TIME" >> "$DATA_FILE"
    done

    echo "Tests completed for dimension ${dimension}"
done

echo "Data has been collected and stored inside the ./data/ directory"
