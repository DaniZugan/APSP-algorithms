
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

// structs for VertexRelax ---------------------------------------------------------------------------------------------

typedef struct {
    //int* id;            // SCC index for each vertex

    //int* firstIn;       // First vertex in each SCC
    //int* nextIn;        // Next vertex in the same SCC
    //int count;           // Total number of SCCs

    int** tailHead;         // Heads of outgoing edges for each tail
    int* tailOut;           // Count of heads of outgoing edges for each tail

    int* firstTail;         // First tail in each SCC
    int* nextTail;          // Next tail in this

    bool** isTail;           // row specifies component, column the vertex and boolean if this vertex is tail in this component
    bool** isHead;           // row specifies component, column the vertex and boolean if this vertex is head in this component
    int* tailCount;         // number of tails in each component

    int** heads;            // list of heads: row specifies component and headCount length
    int* headCount;         // number of heads in each component


} Bridges;


// Adjacency -----------------------------------------------------------------------------------------------------------

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
float** SCCvertex(float** graph, int N);
float** SCCvertexAdaptive(float** graph, int N);

SCCResult findSCCs(float** graph, int n);
SCCResult findSCCs_addEdges(float** graph, int n);

void InitAdjList(int N);
void FreeAdjList();

#ifdef __cplusplus
}
#endif

#endif //SCC_H
