#ifndef UTILS_H
#define UTILS_H

#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

bool isfinite_cust(float n);
double drand(); // random double
float frand(); // random float
void shuffle(int *array, int n);
float** floydWarshall(float **graph, int n); // Floyd-Warshall algorithm
void FW(float** W, int n); // Floyd-Warshall algorithm overwrite W matrix
float** copy2DArray(float** source, int n);
void freeGraph(float** array, int rows);
void free2DInt(int** array, int n);
void print2DGraph(float** graph, int n);
void print2DInt(int** array, int n);
void printIntArray(int* array, int n);
float** createGraph(int size);
float** createGraphAndFill(int size);
int** create2DInt(int n, int init); // same as createGraph but for int, init specifies malloc or calloc
float** extractSubgraph(float** graph, int* component, int componentSize);
int* allocateIntArray(int size, int initialValue);
bool graphCheck (float** W, float** Y, int n); // checks if two graphs have the same values
void permuteGraph(float **graph, int N); // random permutation of a graph
int randInRange(int a, int b);
int compareInts(const void* a, const void* b);
bool contains(int* arr, int size, int value);
char** getFilenamesInFolder(const char* folderPath, int* fileCount);

bool** create2DBoolArray(int rows, int cols);
void free2DBoolArray(bool** array, int rows);
void print2DBoolArray(bool** matrix, int rows, int cols);

double elapsed_seconds(struct timespec start, struct timespec end);

typedef struct SimpleList SimpleList;
struct SimpleList {
    short label;
    SimpleList *next;
};

struct ArrayList {
    int* array;
    int size;
};




#ifdef __cplusplus
}
#endif

#endif //UTILS_H
