# parallel-systems-hw2

Two **OpenMP** exercises with full performance studies — serial vs. parallel runtimes across grid sizes and thread counts, plus a deep comparison of OpenMP scheduling strategies on an irregularly-loaded numerical kernel.

Second homework of the **Parallel Programming** course at the University of Athens (Department of Informatics & Telecommunications). Solo project.

## Exercise 1 — Conway's Game of Life

A classical parallelization: each generation computes the next from the current grid by inspecting 8-neighbourhoods. Serial baseline + an OpenMP-parallel version compared across grid sizes (512, 1024) and thread counts.

```bash
cd excersize1
make
./bin/game_of_life <generations> <grid_size> <serial|parallel> <num_threads>
./scripts/collect_data.sh        # sweep sizes × threads, dump CSV
python scripts/make_graphs.py     # plots
python scripts/make_animation.py  # game_of_life_animation.mp4
```

Performance data and runtime graphs ship in `excersize1/data/`. The animation MP4 visualizes a single run for sanity.

## Exercise 2 — Back substitution for upper-triangular Ax = b

This is the more interesting one because the workload is **inherently irregular**. In back substitution:

- Row `n-1` requires no inner work.
- Row `n-2` does 1 multiply-add.
- Row 0 does `n-1` multiply-adds.

Static block scheduling assigns the late (light) rows to one thread and the early (heavy) rows to another → load imbalance. Dynamic / guided scheduling reassigns chunks at runtime → much better balance. The exercise runs both **row-oriented** and **column-oriented** implementations under five OpenMP scheduling clauses and measures.

```bash
cd excersize2
make
./bin/main <matrix_dimension> <serial|parallel> <row|column> <num_threads>

# scheduling is OpenMP-controlled:
export OMP_SCHEDULE="dynamic, 64"
./bin/main 4096 parallel row 8

./scripts/collect_data.sh        # full sweep
python scripts/make_graphs.py     # per-scheduling-strategy plots
```

Pre-collected data sweeps ship under `excersize2/data/`:

| Subfolder | OpenMP clause |
|---|---|
| `auto_scheduling/` | `schedule(auto)` — let the runtime pick |
| `static_scheduling/` | `schedule(static)` — equal blocks per thread |
| `static_1_scheduling/` | `schedule(static, 1)` — round-robin per iteration |
| `dynamic_scheduling/` | `schedule(dynamic)` — work-stealing chunks |
| `guided_scheduling/` | `schedule(guided)` — exponentially shrinking chunks |

For each, runtime data is collected for grid sizes 64, 1024, 4096, 8192, and 16384. Plotted graphs in each `graphs/` subfolder show speedup vs. thread count per grid size.

The **row-oriented** version exhibits more cache-friendly access patterns; the **column-oriented** version exposes the load-imbalance more sharply. Both are checked for numerical agreement against the serial baseline using a relative-tolerance comparator (`are_doubles_equal`, `are_vectors_equal`).

## What's interesting

- **The matched-pair design.** Running both row and column versions surfaces the cache-locality vs. scheduling-overhead trade-off cleanly. With static scheduling the row version usually wins on cache; with dynamic/guided the column version sometimes catches up because the dispatch overhead is amortized differently.
- **`static, 1` is sometimes faster than `static`** for the irregular back-substitution workload — the round-robin assignment happens to balance the work better than equal-block partitioning, which is counterintuitive but well-documented.
- **Reproducible data collection.** `collect_data.sh` runs the full sweep deterministically and dumps CSV; the Python plotting scripts consume those CSVs to regenerate graphs. Parameter changes don't require manual re-graphing.

## Artefacts

- **`excersize1/excersize1.pdf`**, **`excersize2/excersize2.pdf`** — assignment briefs.
- **`excersize1/data/graphs/game_of_life_animation.mp4`** — visualization of a single Game of Life run.

## License

[MIT](LICENSE) — applies to my own code in this repo. Assignment-distributed materials retain their original course copyright.
