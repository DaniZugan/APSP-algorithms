#include "utils.h"

bool isfinite_cust(float n) { /*return n < INFTY_CMP;*/return isfinite(n); }
double drand() { return ((double)rand())/((double)RAND_MAX); }
float frand() {	return (float)drand(); }


// Floyd-Warshall algorithm --------------------------------------------------------------------------------------------
float** floydWarshall(float **graph, int n) {
    float** W = copy2DArray(graph, n);
    for (int k=0; k<n; k++)
    {
        for (int i=0; i<n; i++)
        {
            if (W[i][k] == INFINITY)
                continue;
            for (int j=0; j<n; j++)
            {
                if (W[i][k] + W[k][j] < W[i][j])
                {
                    W[i][j] = W[i][k] + W[k][j];
                    //last[i][j] = last[k][j];
                }
            }
        }
    }
    return W;
} // new write
void FW(float** W, int n) {
    for (int k=0; k<n; k++)
    {
        for (int i=0; i<n; i++)
        {
            if (W[i][k] == INFINITY)
                continue;
            for (int j=0; j<n; j++)
            {

                if (W[i][k] + W[k][j] < W[i][j])
                {
                    W[i][j] = W[i][k] + W[k][j];
                    //printf("W[%d][%d] = %f via k = %d \n", i, j, W[i][j], k);
                    //last[i][j] = last[k][j];
                }
            }
        }
    }
} // overwrite

// Integer Array--------------------------------------------------------------------------------------------------------
int* allocateIntArray(int size, int initialValue) {
    int* array = (int*)malloc(size * sizeof(int));
    for (int i = 0; i < size; ++i) {
        array[i] = initialValue;
    }
    return array;
}
void printIntArray(int* array, int n) {
    for (int i = 0; i < n; ++i) {
        printf("%d ", array[i]);
    }
    printf("\n");
}
void shuffle(int *array, int n)
{
    if (n > 1)
    {
        for (int i=n-1; i>=1; i--)
        {
            int j = (int)(drand()*((double)i));
            int t = array[i];
            array[i] = array[j];
            array[j] = t;
        }
    }
}

// 2D Integer Array-----------------------------------------------------------------------------------------------------
int** create2DInt(int n, int init) { // 0 for calloc and 1 for malloc
    if (init == 0) {
        int** arr = (int**)calloc(n, sizeof(int*)); // Allocate row pointers
        if (!arr) {
            printf("Memory allocation failed for row pointers!\n");
            exit(1);
        }

        for (int i = 0; i < n; i++) {
            arr[i] = (int*)calloc(n, sizeof(int)); // Allocate columns
            if (!arr[i]) {
                printf("Memory allocation failed for row %d!\n", i);
                exit(1);
            }
        }
        return arr;
    }
    else {
        int** array = (int**)malloc(n * sizeof(int*)); // Allocate rows
        if (!array) {
            perror("Memory allocation failed for rows");
            exit(EXIT_FAILURE);
        }
        for (int i = 0; i < n; i++) {
            array[i] = (int*)malloc(n * sizeof(int)); // Allocate columns
            if (!array[i]) {
                perror("Memory allocation failed for columns");
                exit(EXIT_FAILURE);
            }
        }
        return array;
    }
}
void free2DInt(int** array, int n) {
    if (!array) return; // Check if array is already NULL

    for (int i = 0; i < n; i++) {
        free(array[i]); // Free each row
    }
    free(array); // Free the main array pointer
}
void print2DInt(int** array, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%d ", array[i][j]);
        }
        printf("\n");
    }
}
float** copy2DArray(float** source, int n) {
    float** copy = (float**)malloc(sizeof(float*) * n);
    for (int i = 0; i < n; i++) {
        copy[i] = (float*)malloc(sizeof(float) * n);
        // Copy each element from the source array
        for (int j = 0; j < n; j++) {
            copy[i][j] = source[i][j];
        }
    }
    return copy;
}

// 2D Boolean Array-----------------------------------------------------------------------------------------------------
bool** create2DBoolArray(int rows, int cols) {
    if (rows <= 0 || cols <= 0) return NULL;

    // 1. Allocate array of row pointers
    bool** matrix = malloc(rows * sizeof(bool*)); // Notice sizeof(bool*)
    if (!matrix) return NULL;

    // 2. Allocate each individual row
    for (int i = 0; i < rows; i++) {
        matrix[i] = calloc(cols, sizeof(bool)); // Sets all cells to 'false'

        // Safety check: cleanup previously allocated rows if malloc fails mid-way
        if (!matrix[i]) {
            for (int j = 0; j < i; j++) {
                free(matrix[j]);
            }
            free(matrix);
            return NULL;
        }
    }

    return matrix;
}
void free2DBoolArray(bool** matrix, int rows) {
    if (!matrix) return;

    for (int i = 0; i < rows; i++) {
        free(matrix[i]); // Free each row
    }
    free(matrix);        // Free outer row pointers
}
void print2DBoolArray(bool** matrix, int rows, int cols) {
    if (!matrix) {
        printf("(null matrix)\n");
        return;
    }

    printf("2D Bool Array (%dx%d):\n", rows, cols);

    for (int i = 0; i < rows; i++) {
        printf("Row %2d: ", i); // Row label

        for (int j = 0; j < cols; j++) {
            // Prints 1 for true, 0 for false
            printf("%d ", matrix[i][j] ? 1 : 0);
        }

        printf("\n");
    }
}

// Graph (float 2D Array)-----------------------------------------------------------------------------------------------
float** createGraph(int size) {
    // Allocate memory for a square 2D array (size x size)
    float** array = (float**)malloc(size * sizeof(float*));

    // Allocate memory for each row
    for (int i = 0; i < size; i++) {
        array[i] = (float*)malloc(size * sizeof(float));
    }
    return array;
}
void freeGraph(float** array, int rows) {
    if (array != NULL) {
        // Free each row
        for (int i = 0; i < rows; i++) {
            free(array[i]); // Free the memory for each row
        }
        // Free the array of pointers
        free(array);
    }
}
float** createGraphAndFill(int size) {
    // Allocate memory for a square 2D array (size x size)
    float** array = (float**)malloc(size * sizeof(float*));
    // Allocate memory for each row
    for (int i = 0; i < size; i++) {
        array[i] = (float*)malloc(size * sizeof(float));
        for (int j = 0; j < size; j++) {
            if (i == j) array[i][j] = 0;
            else array[i][j] = INFINITY;
        }
    }
    return array;
}
void print2DGraph(float** graph, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            printf("%.2f ", graph[i][j]);
        }
        printf("\n");
    }
}
float** extractSubgraph(float** graph, int* component, int componentSize) {
    float** subgraph = (float**)malloc(componentSize * sizeof(float*));
    for (int i = 0; i < componentSize; i++) {
        subgraph[i] = (float*)malloc(componentSize * sizeof(float));
        for (int j = 0; j < componentSize; ++j) {
            subgraph[i][j] = graph[component[i]][component[j]];
        }
    }
    return subgraph;
}
void permuteGraph(float **graph, int N) {

    float **permutedGraph = (float **)malloc(N * sizeof(float *));
    for (int i = 0; i < N; i++) {
        permutedGraph[i] = (float *)malloc(N * sizeof(float));
    }

    int *permutation = (int *)malloc(N * sizeof(int));
    for (int i = 0; i < N; i++) {
        permutation[i] = i;
    }
    for (int i = N - 1; i > 0; i--) {
        int j = rand() % (i + 1);
        int temp = permutation[i];
        permutation[i] = permutation[j];
        permutation[j] = temp;
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            permutedGraph[i][j] = graph[permutation[i]][permutation[j]];
        }
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            graph[i][j] = permutedGraph[i][j];
        }
    }
    freeGraph(permutedGraph, N);
    free(permutation);
}
bool graphCheck (float** W, float** Y, int n) {
    bool check = true;
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            if (fabs(Y[i][j] - W[i][j]) >= 0.0001) {
                printf("%.2f vs %.2f: [%d,%d]\n", W[i][j], Y[i][j], i, j);
                check = false;
            }
        }
    }
    return check;
} // checks if two graphs are the same


// including a and b
int randInRange(int a, int b) {
    return a + rand() % (b - a + 1);
}

// Comparison function for ascending order
int compareInts(const void* a, const void* b) {
    return (*(int*)a - *(int*)b);
}

bool contains(int* arr, int size, int value) {
    for (int i = 0; i < size; i++) {
        if (arr[i] == value) {
            return true;
        }
    }
    return false;
}

// Check if filename ends with ".txt"
int endsWithTxt(const char* filename) {
    size_t len = strlen(filename);
    return len >= 4 && strcmp(filename + len - 4, ".txt") == 0;
}

// Returns a NULL-terminated array of file names and sets fileCount
char** getFilenamesInFolder(const char* folderPath, int* fileCount) {
    DIR* dir = opendir(folderPath);
    if (!dir) {
        perror("Failed to open directory");
        *fileCount = 0;
        return NULL;
    }

    struct dirent* entry;
    int capacity = 10;
    int count = 0;

    char** filenames = malloc(capacity * sizeof(char*));
    if (!filenames) {
        perror("Memory allocation failed");
        closedir(dir);
        *fileCount = 0;
        return NULL;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_REG && endsWithTxt(entry->d_name)) {  // only regular files
            if (count >= capacity) {
                capacity *= 2;
                filenames = realloc(filenames, capacity * sizeof(char*));
                if (!filenames) {
                    perror("Memory reallocation failed");
                    closedir(dir);
                    *fileCount = 0;
                    return NULL;
                }
            }
            filenames[count] = strdup(entry->d_name);  // copy filename
            count++;
        }
    }

    closedir(dir);
    *fileCount = count;
    return filenames;
}

double elapsed_seconds(struct timespec start, struct timespec end)
{
    return (double)(end.tv_sec - start.tv_sec)
         + (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}







