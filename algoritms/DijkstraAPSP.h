
#ifndef DIJKSTRAAPSP_H
#define DIJKSTRAAPSP_H

typedef struct {
    int vertex;
    float distance;
} HeapNode;

typedef struct {
    HeapNode* nodes;
    int* pos;
    int size;
    int capacity;
} MinHeap;


float** DijkstraAPSP(float** graph, int n);
float** DijkstraWithAdjListAPSP(float** graph, int n);
float** DijkstraWthAdjListAPSP(float** graph, int n);

#endif //DIJKSTRAAPSP_H
