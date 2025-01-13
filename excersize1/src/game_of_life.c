#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <timer.h>

typedef enum{
    DEAD,
    ALIVE
} CellState;

// Function to initialize the full board at root
void initialize_full_board(int grid_size, CellState *board) {
    for (int i = 0; i < grid_size; i++) {
        for (int j = 0; j < grid_size; j++) {
            board[i * grid_size + j] = rand() % 2; // Randomly ALIVE or DEAD
        }
    }
}

void update_independent_rows(CellState* board, CellState* newBoard, int rows, int columns);
void update_dependent_rows(CellState* board, CellState* newBoard, int rows, int columns);
int count_alive_neighbors(int x, int y, CellState *board, int columns, int rows);

int main(int argc, char **argv){
    long double start, end;

    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if(rank == 0){
        GET_TIME(start);
    }

    if(argc != 3){
        if(rank == 0){
            printf("Usage: %s <generations> <grid_size>\n", argv[0]);
        }
        MPI_Finalize();
        return 1;
    }

    int generations = atoi(argv[1]);
    int grid_size = atoi(argv[2]);

    int rows_per_process = grid_size / size;
    int extra_rows = grid_size % size;
    int local_rows = rows_per_process + (rank < extra_rows ? 1 : 0);
    
    // Αφού η διεργασία 0 αρχικοποιήσει τον πλήρη πίνακα, σκοπεύω να χρησιμοποιήσω τη συνάρτηση MPI_Scatterv 
    // για να διαμοιράσω τις γραμμές στις διεργασίες. 
    // Η συγκεκριμένη συνάρτηση δέχεται ορίσματα όπως τα sendcounts και displs, 
    // τα οποία καθορίζουν πόσα στοιχεία θα λάβει κάθε διεργασία και από ποια θέση του πίνακα θα ξεκινήσει 
    // η λήψη αυτών των στοιχείων.
    int *sendcounts = malloc(size * sizeof(int));
    int * displs = malloc(size * sizeof(int));

    int offset = 0;
    for(int i = 0; i < size; i++){
        int rows = rows_per_process + (i < extra_rows? 1 : 0);
        sendcounts[i] = rows * grid_size;
        displs[i] = offset;
        offset += sendcounts[i];
    }

    // Για να διευκολυνθεί η μεταφορά δεδομένων μεταξύ διεργασιών, αποφάσισα να χρησιμοποιήσω 
    // έναν μονοδιάστατο πίνακα αντί για τους κλασικούς δυσδιάστατους πίνακες με δείκτες. 
    // Το πλέγμα θα αποθηκεύεται σε συνεχόμενες θέσεις του μονοδιάστατου πίνακα. 
    // Έτσι, για να διαβάσεις την τιμή του κελιού στη θέση (i, j), μπορείς να γράψεις: 
    // flatFullBoard[i * grid_size + j]
    CellState *flatFullBoard = NULL;

    // Κάθε διεργασία (συμπεριλαμβανομένης της διεργασίας 0) θα διαχειρίζεται ένα υποσύνολο γραμμών του πίνακα. 
    // Για την παραγωγή της επόμενης γενιάς σε κάθε φάση, οι γειτονικές διεργασίες πρέπει να επικοινωνούν μεταξύ τους, 
    // ανταλλάσσοντας τις ακριανές γραμμές. 
    // Για αυτόν τον λόγο, στον πίνακα flatLocalBoard, εκτός από τις τοπικές γραμμές της διεργασίας, 
    // διατηρώ χώρο στην αρχή και στο τέλος του πίνακα για 2 επιπλέον γραμμές (για αυτό και το local_rows + 2). 
    // Αυτές οι γραμμές ονομάζονται halo rows και είναι οι γραμμές που πρέπει να ενημερώνονται σε κάθε φάση, 
    // λαμβάνοντάς τες από τις γειτονικές διεργασίες.
    CellState *flatLocalBoard = malloc(grid_size * (local_rows + 2) * sizeof(CellState));
    
    // Ένα πρόβλημα που προέκυψε είναι ότι για την πρώτη διεργασία δεν υπάρχει
    // πραγματική top halo γραμμή και για την τελευταία διεργασία δεν υπάρχει
    // bottom halo γραμμή. Για να το λύσω, σκέφτηκα να αφήσω αυτές τις γραμμές να υπάρχουν,
    // αρχικοποιώντας τες με νεκρά κελιά, ώστε να μην επηρεάζουν το αποτέλεσμα.
    if(rank == size-1){
        for(int j = 0; j < grid_size; j++){
            flatLocalBoard[grid_size * (local_rows + 1) + j] = DEAD;
        }
    } else if(rank == 0){
        for(int j = 0; j < grid_size; j++){
            flatLocalBoard[j] = DEAD;
        }
    }
    
    // Μόνο η διεργασία 0 φτιάχνει το αρχικό πλέγμα
    if (rank == 0){
        flatFullBoard = malloc(grid_size * grid_size * sizeof(CellState));
        initialize_full_board(grid_size, flatFullBoard);
    }

    // Και είμαι έτοιμος να στείλω τις γραμμές στις διεργασίες
    MPI_Scatterv(
        flatFullBoard,              // Ο πίνακας δεδομένων ο οποίος θα σταλεί
        sendcounts,                 // Πίνακας με το πόσα στοιχεία θα διαβάσει η κάθε διεργασία
        displs,                     // Πίνακας με τα displacements για την κάθε διεργασία
        MPI_INT,                    // Ο τύπος δεδομένων που στέλνονται
        &flatLocalBoard[grid_size], // Receive buffer, βάζω index [grid_size] καθώς στην αρχή του πίνακα η πρώτη γραμμή είναι halo row
        local_rows * grid_size ,    // Πόσα στοιχεία περιμένει να παραλάβει η διεργασία
        MPI_INT,                    // Ο τύπος δεδομένων που θα παραλάβειι
        0,                          // Ποια διεργασία στέλνει τα δεδομένα
        MPI_COMM_WORLD              // Το κανάλη επικοινωνίας που θα χρησιμοποιηθεί
    );

    // Χρειαζόμαστε έναν πίνακα όπου θα αποθηκεύεται η επόμενη γενιά σε κάθε φάση του αλγορίθμου. 
    // Η στρατηγική είναι ότι, αφού υπολογιστεί το πλέγμα της επόμενης γενιάς, το πλέγμα της προηγούμενης 
    // δεν είναι πλέον απαραίτητο. Συνεπώς, μπορούμε να κάνουμε swap τους δείκτες: 
    // στη νέα φάση, η μεταβλητή newBoard θα δείχνει στον παλιό πίνακα, ενώ ο τωρινός πίνακας θα δείχνει 
    // στο νέο πλέγμα που έχει υπολογιστεί. Έτσι, ο αλγόριθμος μπορεί να συνεχίσει με τον υπολογισμό 
    // της επόμενης γενιάς.
    CellState *newBoard = malloc(grid_size * (local_rows + 2) * sizeof(CellState));

    for (int gen = 0; gen < generations; gen++){
        // Σε αυτό το σημείο ενημερώνω τις halo γραμμές. Για να το πετύχω αυτό, χρειάζεται να ανταλλαχθούν δεδομένα
        // μεταξύ γειτονικών διεργασιών. Χρησιμοποιώ τις συναρτήσεις MPI_Isend και MPI_Irecv, οι οποίες είναι non-blocking,
        // πράγμα που σημαίνει ότι δεν μπλοκάρουν την εκτέλεση του προγράμματος και επιτρέπουν την επικοινωνία να εκτελείται
        // στο παρασκήνιο. Αυτό με βοηθάει να αποφύγω race conditions, ενώ ταυτόχρονα μου δίνει τη δυνατότητα να αρχίσω την
        // επεξεργασία των γραμμών που δεν εξαρτώνται από δεδομένα άλλων διεργασιών, καθώς η επικοινωνία συνεχίζεται παράλληλα.
        // Όταν ολοκληρωθεί η επικοινωνία, περιμένω με την MPI_Waitall για να ολοκληρωθούν οι μεταφορές των halo γραμμών και
        // στη συνέχεια προχωρώ στην επεξεργασία των γραμμών που εξαρτώνται από αυτές.
        MPI_Request requests[4];
        int req_count = 0;
        
        // Στέλνω την πάνω γραμμή στην προηγούμενη διεργασίας
        // Λαμβάνω την κάτω γραμμή της προηγούμενης διεργασίας 
        if (rank > 0) { 
            // Αν τα indices σου φαίνονται μπερδεμένα, θυμήσου ότι στο index 0 αποθηκεύεται η halo γραμμή, η οποία αντιστοιχεί
            // στην κάτω γραμμή της προηγούμενης διεργασίας. Επομένως, η πραγματική πρώτη γραμμή της διεργασίας ξεκινά από το index 1,
            // και επειδή οι πίνακες είναι flat (μονοδιάστατοι), αυτό το index αντιστοιχεί στη θέση [grid_size * 1].
            MPI_Isend(&flatLocalBoard[grid_size], grid_size, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
            MPI_Irecv(&flatLocalBoard[0], grid_size, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
        }
        
        // Στέλνω την κάτω γραμμή στην επόμενη διεργασία
        // Λαμβάνω την πάνω γραμμή της επόμενης διεργασίας
        if (rank < size - 1) { 
            MPI_Isend(&flatLocalBoard[grid_size * local_rows], grid_size, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
            MPI_Irecv(&flatLocalBoard[grid_size * (local_rows + 1)], grid_size, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &requests[req_count++]);
        }
        
        // Δεν ήταν απαραίτητο να σπάσω την ενημέρωση του πλέγματος σε δύο μέρη, καθώς θα μπορούσα να περιμένω να ολοκληρωθούν
        // οι επικοινωνίες και να ολοκληρώσω την ενημέρωση με μία μόνο διαδικασία. Παρόλα αυτά, αποφάσισα να δοκιμάσω να το
        // σπάσω, καθώς θεωρητικά πιστεύω ότι αυτό μπορεί να βελτιώσει την απόδοση.
        update_independent_rows(flatLocalBoard, newBoard, local_rows, grid_size);

        // Περιμένω να ολοκληρωθεί η επικοινωνία
        MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);

        update_dependent_rows(flatLocalBoard, newBoard, local_rows, grid_size);

        CellState *temp = flatLocalBoard;
        flatLocalBoard = newBoard;
        newBoard = temp;
    }

    // Και τώρα συλλέγω στην διεργασία 0 τις γραμμές ώστε να φτιάξω το
    // τελικό ολόκληρο πλέγμα της τελευταίας γενιάς
    MPI_Gatherv(
        &flatLocalBoard[grid_size],  // Η κάθε διεργασία στέλνει τις γραμμές της (όχι τις halo) 
        local_rows * grid_size,      // Αριθμός των δεδομένων που θα στείλει η κάθε διεργασία
        MPI_INT,                     // Τύπος δεδομένων που θα στείλει
        flatFullBoard,               // Που θα ληφθούν τα δεδομένα
        sendcounts,                  // Πόσα δεδομένα περιμένεις να ληφθούν από κάθε διεργασία
        displs,                      // Offsets ώστε να τοποθετηθούν στις σωτές θέσεις στον πίνακα
        MPI_INT,                     // Τύπος δεδομένων που θα ληφθούν
        0,                           // Ποια διεργασία θα τα λάβει
        MPI_COMM_WORLD               // Το κανάλη επικοινωνίας που θα χρησιμοποιηθεί
    );


    if(rank == 0){
        GET_TIME(end);
    }

    // Τυπώνω το τελικό πλέγμα
    if (rank == 0){
#ifdef PRINT_GRID
        for (int i = 0; i < grid_size; i++) {
            for (int j = 0; j < grid_size; j++) {
                printf("%c", flatFullBoard[i * grid_size + j] == ALIVE ? 'O' : '.');
            }
            printf("\n");
        }
#endif
#ifndef PRINT_GRID
        printf("\nExecution Time: %.12Lf seconds\n", end - start);
#endif
    }


    // Frees και τα λοιπα για να τερματίσει το πρόγραμμα όμορφα
    if (rank == 0){
        free(flatFullBoard);
    }

    free(flatLocalBoard);
    free(newBoard);
    free(sendcounts);
    free(displs);

    MPI_Finalize();
    return 0;
}

// Ενημερώνει μόνο τις γραμμές που δεν εξαρτώνται από δεδομένα άλλων διεργασιών
void update_independent_rows(CellState* board, CellState* newBoard, int rows, int columns){
    for(int i = 2; i < rows; i++){
        for(int j = 0; j < columns; j++){
            int alive_neighbors = count_alive_neighbors(j, i, board, columns, rows+2);
            newBoard[i * columns + j] = (board[i * columns + j] == ALIVE) ? (alive_neighbors == 2 || alive_neighbors == 3) : (alive_neighbors == 3);
        }
    }
}

// Ενημερώνει τις γραμμές που εξαρτώνται από δεδομένα άλλων διεργασιών.
// Είναι σημαντικό όλες οι halo γραμμές να έχουν ενημερωθεί και όλες οι επικοινωνίες να έχουν ολοκληρωθεί.
// Ακόμα και το να στείλεις τις δικές σου γραμμές χωρίς να περιμένεις μπορεί να δημιουργήσει πρόβλημα,
// καθώς δεν είναι εγγυημένο ότι οι γραμμές έχουν αντιγραφεί στους buffers και το να προσπαθήσεις να τις τροποποιήσεις
// πριν ολοκληρωθεί η αποστολή ενδέχεται να προκαλέσει ασυμβατότητες ή λάθη.
void update_dependent_rows(CellState* board, CellState* newBoard, int rows, int columns){
    int i = 1;
    for(int j = 0; j < columns; j++){
        int alive_neighbors = count_alive_neighbors(j, i, board, columns, rows+2);
        newBoard[i * columns + j] = (board[i * columns + j] == ALIVE) ? (alive_neighbors == 2 || alive_neighbors == 3) : (alive_neighbors == 3);
    }
    i = rows;
    for(int j = 0; j < columns; j++){
        int alive_neighbors = count_alive_neighbors(j, i, board, columns, rows+2);
        newBoard[i * columns + j] = (board[i * columns + j] == ALIVE) ? (alive_neighbors == 2 || alive_neighbors == 3) : (alive_neighbors == 3);
    }
}

// Ο κλασικός αλγόριθμος που χρησιμοποίησα και για την εργασία 2
int count_alive_neighbors(int x, int y, CellState *board, int columns, int rows) {
    int count = 0;
    for (int dx = -1; dx <= 1; dx++){
        for (int dy = -1; dy <= 1; dy++){
            if (dx == 0 && dy == 0) continue;
            int nx = x + dx;
            int ny = y + dy;
            if (nx >= 0 && nx < columns && ny >= 0 && ny < rows){
                count += (board[ny * columns + nx] == ALIVE);
            }
        }
    }
    return count;
}