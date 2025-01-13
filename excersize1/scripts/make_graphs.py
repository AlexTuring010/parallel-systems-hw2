import os
import pandas as pd
import matplotlib.pyplot as plt

# Ρυθμίσεις directories
SCRIPT_DIR = os.path.dirname(os.path.realpath(__file__))
PROJECT_DIR = os.path.abspath(os.path.join(SCRIPT_DIR, ".."))
DATA_DIR = os.path.join(PROJECT_DIR, "data")
GRAPHS_DIR = os.path.join(DATA_DIR, "graphs")

# Δημιουργία φακέλου για γραφήματα αν δεν υπάρχει
os.makedirs(GRAPHS_DIR, exist_ok=True)

# Αναζήτηση των δεδομένων
data_files = [f for f in os.listdir(DATA_DIR) if f.startswith("collected_data_for_") and f.endswith(".csv")]

# Δημιουργία γραφημάτων
for file in data_files:
    file_path = os.path.join(DATA_DIR, file)
    df = pd.read_csv(file_path)

    # Εξαγωγή διάστασης απο το όνομα του αρχείου
    dimension = file.split("_for_")[1].split("_")[0]

    # Δημιουργία γραφήματος
    plt.figure(figsize=(10, 6))
    plt.plot(df['threads'], df['time'], marker='o', label=f"n = {dimension}")
    plt.title(f"Time vs Threads for n = {dimension}", fontsize=16)
    plt.xlabel("Number of Threads", fontsize=14)
    plt.ylabel("Time", fontsize=14)
    plt.grid(True, linestyle='--', alpha=0.7)
    plt.legend(fontsize=12)

    # Αποθήκευση γραφήματος
    graph_filename = f"graph_for_{dimension}_grid.png"  # Adjusted filename to use dimension
    graph_path = os.path.join(GRAPHS_DIR, graph_filename)
    plt.savefig(graph_path, bbox_inches='tight')
    plt.close()

    print(f"Graph saved: {graph_path}")

print("All graphs have been generated and saved in ./data/graphs/")
