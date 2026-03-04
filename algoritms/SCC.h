
#ifndef SCC_H
#define SCC_H

#ifdef __cplusplus
extern "C" {  // Ensure C linkage when included from C++
#endif

// To hold the result of Tarjan algorithm
typedef struct {
    int* component;         // SCC index for each vertex
    int* nextInComponent;   // Next vertex in the same SCC
    int* firstInComponent;  // First vertex in each SCC
    int sccCount;           // Total number of SCCs
} SCCResult;

// Outgoing edges (a = tail vertex, b = head vertex, nxt = next edge in the same component)
typedef struct {
    int a;
    int b;
    int nxt;
} tEdge;

typedef struct {
    int sEdge;
    tEdge* edge;
    int sReach;
    int* SCCreach;
} tSCCdesc;

// start and degree of a vertex in E array
typedef struct {
    int start;
    int deg;
} idxE;

// Adjacency list for graph (no weights)
typedef struct {
    int** edges;
    int* deg;
    int size;
} GraphAdjList;
// Adjacency matrix for graph (no weights)
typedef struct {
    int** matrix;
    int size;
} GraphAdjMatrix;
// Adj. list and matrix for graph
typedef struct {
    GraphAdjList seznam;
    GraphAdjMatrix matrika;
} GraphDual;

float** SCCalgo(float** graph, int N);
float** SCCalgo2(float** graph, int N);
SCCResult findSCCs(float** graph, int n);
void InitAdjList(int N);
void FreeAdjList();

#ifdef __cplusplus
}
#endif

#endif //SCC_H
