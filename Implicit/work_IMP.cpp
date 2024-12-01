#include <iostream>
#include <ctime>
#include <cmath>
#include <fstream>
#include <omp.h>
#include <cstdlib>
using namespace std;

// Function to allocate memory for a matrix
float** allocate_matrix(int N) {
    float** matrix = (float**)malloc(N * sizeof(float*));
    for (int i = 0; i < N; i++) {
        matrix[i] = (float*)malloc(N * sizeof(float));
    }
    return matrix;
}

// Function to free the memory of a matrix
void free_matrix(float** matrix, int N) {
    for (int i = 0; i < N; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

bool checkSym(float** M, int N) {
    for (int i = 0; i < N; ++i) {
        for (int j = i + 1; j < N; ++j) {
            if (M[i][j] != M[j][i]) {
                return false;
            }
        }
    }
    return true;
}

void matTransposeImp(float** M, float** T, int N) {
    if (!checkSym(M,N)) {
        // Transpose the matrix M into T
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j+=8) {
                // Touch the next row to prefetch (hardware prefetcher can load cache lines)
                volatile float touch = M[i][j + 4 < N ? j + 4 : j];

                T[j][i] = M[i][j];      //unrolled without conditions because known that j+3 won't be a NULL value if dimension==2^n
                T[j+1][i] = M[i][j+1];
                T[j+2][i] = M[i][j+2];
                T[j+3][i] = M[i][j+3];
                T[j+4][i] = M[i][j+4];
                T[j+5][i] = M[i][j+5];
                T[j+6][i] = M[i][j+6];
                T[j+7][i] = M[i][j+7];
            }
        }
        
    }
}

void init_matrix(float** M, int N) {
    // Seed the random number generator
    srand(static_cast<unsigned int>(time(0)));
    
    // Initialize matrix M with random floating-point values
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            M[i][j] = static_cast<float>(rand()) / RAND_MAX; // Random value in [0.0, 1.0]
        }
    }
}

int compare (const void * a, const void * b){return ( *(int*)a - *(int*)b );}

void printValues(double* results, FILE *file, int n_times, int N, int n_threads, int exec){
    qsort(results, n_times, sizeof(double), compare);
    for(int i=((n_times/10)*3); i<((n_times/10)*7); i++){
        fprintf(file, "Implicit,%d,%d,%.9f,%d\n", n_threads , N, results[i], exec);
    }

}

int main(int argc, char* argv[]) {
    // Controllo degli argomenti
    if (argc != 5) {
        cerr << "Usage: " << argv[0] << " <matrix_exponential> <num_threads> <num_times>" << endl;
        return 1;
    }

    // Lettura della dimensione della matrice da argv
    int N = pow(2,atoi(argv[1]));
    if (N < 2) {
        cerr << "Matrix exponential must be a positive integer between 4 and 12" << endl;
        return 1;
    }

    int n_threads=atoi(argv[2]);
    if (n_threads < 1 || n_threads > 64) {
        cerr << "The number of threads must be a positive integer between 1 and 64" << endl;
        return 1;
    }
    omp_set_num_threads(n_threads);

    int n_times = atoi(argv[3]);
    if (n_times < 1) {
        cerr << "The number of times must be a positive integer" << endl;
        return 1;
    }
    double *uresults= (double*)malloc(n_times * sizeof(double));

    int exec = atoi(argv[4]);

    // Seed the random number generator
    srand(static_cast<unsigned int>(time(0)));

    // Allocate memory for matrices
    float** M = allocate_matrix(N);
    float** T = allocate_matrix(N);

    // Inizialization of the matrix
    init_matrix(M, N);

    // Opening of the file in append mode
    const char *result="result.csv";
	FILE *file = fopen(result, "a");
	if (file == NULL) {
		cerr << "Error opening file";
        free_matrix(M, N);
        free_matrix(T, N);    
        return(1);
        
    }

    double time_taken, s, e;
    for(int i=0; i<n_times; i++){
        //Matrix transposition
        s = omp_get_wtime();
        matTransposeImp(M,T,N);
        e = omp_get_wtime();
        time_taken = e-s;
        uresults[i]=time_taken;
    }

    printValues(uresults, file, n_times, N, n_threads, exec);
    free(uresults);

    // Free allocated memory
    free_matrix(M, N);
    free_matrix(T, N);


    fclose(file);

    return 0;
}
