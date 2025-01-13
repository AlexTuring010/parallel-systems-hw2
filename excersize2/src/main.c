#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <omp.h>
#include "timer.h"

// Prints the upper triangular matrix A and vector b
void print_A_and_b(long double** A, long double* b, int n) {
    printf("\nUpper Triangular Matrix (A) and Vector (b):\n");
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            if (j < i) {
                printf("  0 "); // Zero for elements below the diagonal
            } else {
                printf(" %2Lf ", A[i][j]); // Print upper triangular elements
            }
        }
        printf(" | %Lf\n", b[i]); // Print the corresponding b value
    }
    printf("\n");
}

// Prints the solution vector x
void print_result(long double* x, int n) {
    printf("Solution vector x:\n\n");
    for (int i = 0; i < n; i++) {
        printf("%Lf\n", x[i]);
    }
    printf("\n");
}

long double max(long double a, long double b) {
    return (a > b) ? a : b;
}

// Compares two long doubles for approximate equality
int are_doubles_equal(long double a, long double b) {
    // Define a relative tolerance that adapts to the size of the numbers
    long double tolerance = max(fabsl(a), fabsl(b)) * 1e-6;  // You can adjust the factor (1e-6) based on your precision needs

    return fabsl(a - b) <= tolerance;  // If the difference is smaller than or equal to the tolerance, consider them equal
}

// Compares two vectors for equality
int are_vectors_equal(long double* x1, long double* x2, int n) {
    for (int i = 0; i < n; i++) {
        if (!are_doubles_equal(x1[i], x2[i])) {
            printf("x1 = %Lf x2 = %Lf\n", x1[i], x2[i]);
            return 0;
        }
    }
    return 1;
}

// Serial implementation of back substitution
void back_substitution_row_serial(long double** A, long double* b, long double* x, int n) {
    // Solves Ax = b for an upper triangular matrix A
    for (int row = n - 1; row >= 0; row--) {
        x[row] = b[row];
        for (int col = row + 1; col < n; col++) {
            x[row] -= A[row][col] * x[col];
        }
        x[row] /= A[row][row];
    }
}

void back_substitution_row_parallel(long double** A, long double* b, long double* x, int n) {
    long double xi = b[n-1];
    #pragma omp parallel
    {
        for (int row = n - 1; row >= 0; row--) {            
            // Compute the contributions from other variables in parallel
            #pragma omp for schedule(runtime) reduction(-:xi)
            for (int col = row + 1; col < n; col++) {
                xi -= A[row][col] * x[col];
            }
            
            // Update x[row] and xi using a single thread
            #pragma omp single
            {
                // Had to make sure those two are paired in a single thread
                // else there was possible race condition that could occur
                x[row] = xi / A[row][row];
                if(row > 0) xi = b[row - 1];
            }
            // Implicit barrier is placed here
        }
    }
}

// Serial implementation of back substitution (column-wise)
void back_substitution_column_serial(long double** A, long double* b, long double* x, int n) {
    for (int row = 0; row < n; row++) {
        x[row] = b[row];
    }
    for (int col = n - 1; col >= 0; col--) {
        x[col] /= A[col][col];
        for (int row = 0; row < col; row++) {
            x[row] -= A[row][col] * x[col];
        }
    }
}

// Parallel implementation of back substitution (column-wise)
void back_substitution_column_parallel(long double** A, long double* b, long double* x, int n) {
    // Initialize the solution vector
    #pragma omp parallel for
    for (int row = 0; row < n; row++) {
        x[row] = b[row];
    }

    #pragma omp parallel
    {
        for (int col = n - 1; col >= 0; col--) {
            // This is used to avoid the need of synchronization that would make overhead
            long double xj = x[col] / A[col][col];

            // We will let one thread do the update of the real x[col]
            #pragma omp single nowait
            x[col] = xj;
            
            // Update rows below the current column in parallel
            #pragma omp for schedule(runtime)
            for (int row = 0; row < col; row++) {
                x[row] -= A[row][col] * xj;
            }
        }
    }
}

// Main function
int main(int argc, char* argv[]) {
    if (argc != 5) {
        printf("Usage: %s <matrix_dimension> <algorithm_type> <method_type> <num_of_threads>\n", argv[0]);
        printf("algorithm_type: serial or parallel\n");
        printf("method_type: row or column\n");
        return 1;
    }

    // Parse command-line arguments
    int n = atoi(argv[1]);
    char* algorithm_type = argv[2];
    char* method_type = argv[3];
    int num_threads = atoi(argv[4]);

    omp_set_num_threads(num_threads);

    // Allocate memory for matrix A, vector b, and solution vectors x and expected_x
    long double** A = (long double**)malloc(n * sizeof(long double*));
    for (int i = 0; i < n; i++) {
        A[i] = (long double*)malloc(n * sizeof(long double));
    }
    long double* b = (long double*)malloc(n * sizeof(long double));
    long double* x = (long double*)malloc(n * sizeof(long double));

#ifdef TEST_MODE
    long double* expected_x = (long double*)malloc(n * sizeof(long double));
#endif

    // Initialize the upper triangular matrix and vector b
    // srand(time(NULL));
    for (int i = 0; i < n; i++) {
        for (int j = i; j < n; j++) {
            A[i][j] = rand() % 2 + 1;
        }
        b[i] = rand() % 2 + 1;
    }

#ifdef TEST_MODE
    // Compute the expected solution using the serial implementation
    back_substitution_row_serial(A, b, expected_x, n);
#endif

    // Measure execution time
    long double start, end;
    GET_TIME(start);

    // Perform back substitution using the chosen method
    if (strcmp(method_type, "row") == 0) {
        if(strcmp(algorithm_type, "parallel") == 0){
            back_substitution_row_parallel(A, b, x, n);
        } else if(strcmp(algorithm_type, "serial") == 0){
            back_substitution_row_serial(A, b, x, n);
        } else{
            printf("Invalid algorithm type. Use \"parallel\" or \"serial\".\n");
        }
    } else if (strcmp(method_type, "column") == 0) {
        if(strcmp(algorithm_type, "parallel") == 0){
            back_substitution_column_parallel(A, b, x, n);
        } else if(strcmp(algorithm_type, "serial") == 0){
            back_substitution_column_serial(A, b, x, n);
        } else{
            printf("Invalid algorithm type. Use \"parallel\" or \"serial\".\n");
        }
    } else {
        printf("Invalid method type. Use \"row\" or \"column\".\n");
        return 1;
    }

    GET_TIME(end);

#ifdef TEST_MODE
    // Verify the result
    if (!are_vectors_equal(x, expected_x, n)) {
        printf("Error: Computed result does not match the expected result.\n");
    }
#endif

    // Used it during first stages of testing, but now that Im confident of my program I turned this 
    // off because solving twice was making the program slower during the bigger tests which is annoying

    // print_A_and_b(A, b, n);
    // print_result(x, n);
    printf("Execution Time: %.12Lf seconds\n", end - start);

    // Free allocated memory
    for (int i = 0; i < n; i++) {
        free(A[i]);
    }
    free(A);
    free(b);
    free(x);
#ifdef TEST_MODE
    free(expected_x);
#endif

    return 0;
}
