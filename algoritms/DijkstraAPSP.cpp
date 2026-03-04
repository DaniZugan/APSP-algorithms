#include "DijkstraAPSP.h"
#include "../utils.h"
#include "SCC.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <float.h>

// adjencency matrix
static int* E;
static idxE* V;

MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    heap->nodes = (HeapNode*)malloc(capacity * sizeof(HeapNode));
    heap->pos = (int*)malloc(capacity * sizeof(int));
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void swapHeapNodes(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void minHeapify(MinHeap* heap, int idx) {
    int smallest = idx;
    int left = 2 * idx + 1;
    int right = 2 * idx + 2;

    if (left < heap->size && heap->nodes[left].distance < heap->nodes[smallest].distance)
        smallest = left;

    if (right < heap->size && heap->nodes[right].distance < heap->nodes[smallest].distance)
        smallest = right;

    if (smallest != idx) {
        heap->pos[heap->nodes[smallest].vertex] = idx;
        heap->pos[heap->nodes[idx].vertex] = smallest;
        swapHeapNodes(&heap->nodes[smallest], &heap->nodes[idx]);
        minHeapify(heap, smallest);
    }
}

HeapNode extractMin(MinHeap* heap) {
    if (heap->size == 0)
        return (HeapNode){-1, INFINITY};

    HeapNode root = heap->nodes[0];
    HeapNode lastNode = heap->nodes[heap->size - 1];
    heap->nodes[0] = lastNode;
    heap->pos[root.vertex] = heap->size - 1;
    heap->pos[lastNode.vertex] = 0;
    heap->size--;
    minHeapify(heap, 0);
    return root;
}

void decreaseKey(MinHeap* heap, int v, float dist) {
    int i = heap->pos[v];
    heap->nodes[i].distance = dist;

    while (i && heap->nodes[i].distance < heap->nodes[(i - 1) / 2].distance) {
        heap->pos[heap->nodes[i].vertex] = (i - 1) / 2;
        heap->pos[heap->nodes[(i - 1) / 2].vertex] = i;
        swapHeapNodes(&heap->nodes[i], &heap->nodes[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

bool isEmpty(MinHeap* heap) {
    return heap->size == 0;
}

void insertHeap(MinHeap* heap, int v, float dist) {
    int i = heap->size;
    heap->size++;
    heap->nodes[i].vertex = v;
    heap->nodes[i].distance = dist;
    heap->pos[v] = i;
    decreaseKey(heap, v, dist);
}

void Dijkstra(int start, float* dist, bool* solved, float** weights, int n) {
    MinHeap* minHeap = createMinHeap(n);
    for (int i = 0; i < n; i++) {
        dist[i] = INFINITY;
        solved[i] = false;
        insertHeap(minHeap, i, INFINITY);
    }
    dist[start] = 0;
    decreaseKey(minHeap, start, 0);

    while (!isEmpty(minHeap)) {
        HeapNode minNode = extractMin(minHeap);
        int u = minNode.vertex;
        solved[u] = true;

        for (int i = 0; i < n; i++) {
            if (!solved[i] && weights[u][i] != INFINITY) {
                float newDist = dist[u] + weights[u][i];
                // Only update if the new distance is smaller
                if (newDist < dist[i]) {
                    dist[i] = newDist;
                    decreaseKey(minHeap, i, dist[i]);
                }
            }
        }
    }
    free(minHeap->nodes);
    free(minHeap->pos);
    free(minHeap);
}

int createAdjList(float** graph, int N) {
    InitAdjList(N);
    int edges = 0;
    for (int i = 0; i < N; i++) {
        V[i].start = edges;
        for (int j = 0; j < N; j++) {
            if (i != j & graph[i][j] != INFINITY) {
                //Union(i, j);
                E[edges] = j;
                edges++;
            }
        }
        V[i].deg = edges - V[i].start;
    }
    return edges;
}

// improved
void DijkstraWithAdjList(int start, float* dist, bool* solved, float** graph, int n) {
    MinHeap* minHeap = createMinHeap(n);
    bool* inserted = (bool*)malloc(n * sizeof(bool));

    // Initialize distances and minHeap
    for (int i = 0; i < n; i++) {
        inserted[i] = false;
        solved[i] = false;
        dist[i] = INFINITY;
        //insertHeap(minHeap, i, INFINITY);
    }
    dist[start] = 0;
    //decreaseKey(minHeap, start, 0);
    insertHeap(minHeap, start, 0);
    inserted[start] = true;
    while (!isEmpty(minHeap)) {
        HeapNode minNode = extractMin(minHeap);
        int u = minNode.vertex;
        solved[u] = true;

        // Iterate over neighbors in the adjacency list for node u
        for (int i = 0; i < V[u].deg; i++) {
            int v = E[V[u].start + i];  // Neighbor v
            if (!solved[v]) {
                float newDist = dist[u] + graph[u][v];
                if (!inserted[v]) {                    // == INFINITY, special relaxation
                    dist[v] = newDist;
                    insertHeap(minHeap, v, dist[v]);
                    inserted[v] = true;
                }
                else if (newDist < dist[v]) {
                    dist[v] = newDist;
                    decreaseKey(minHeap, v, dist[v]);
                }
            }
        }
    }
    free(inserted);
    // Cleanup heap
    free(minHeap->nodes);
    free(minHeap->pos);
    free(minHeap);
}
void DijkstraWthAdjList(int start, float* dist, bool* solved, float** graph, int n) {
    MinHeap* minHeap = createMinHeap(n);

    // Initialize distances and minHeap
    for (int i = 0; i < n; i++) {
        dist[i] = INFINITY;
        solved[i] = false;
        insertHeap(minHeap, i, INFINITY);
    }
    dist[start] = 0;
    decreaseKey(minHeap, start, 0);

    while (!isEmpty(minHeap)) {
        HeapNode minNode = extractMin(minHeap);
        int u = minNode.vertex;
        solved[u] = true;

        // Iterate over neighbors in the adjacency list for node u
        for (int i = 0; i < V[u].deg; i++) {
            int v = E[V[u].start + i];  // Neighbor v
            if (!solved[v] && graph[u][v] != INFINITY) {
                float newDist = dist[u] + graph[u][v];
                if (newDist < dist[v]) {
                    dist[v] = newDist;
                    decreaseKey(minHeap, v, dist[v]);
                }
            }
        }
    }

    // Cleanup heap
    free(minHeap->nodes);
    free(minHeap->pos);
    free(minHeap);
}
//improved
float** DijkstraWithAdjListAPSP(float** graph, int n) {
    float** W = copy2DArray(graph, n);
    int z = createAdjList(W, n);
    //print2DGraph(W, n);
    float* opt = (float*)malloc(n * sizeof(float));
    bool* solved = (bool*)malloc(n * sizeof(bool));

    for (int i = 0; i < n; i++) {
        DijkstraWithAdjList(i, opt, solved, graph, n);
        for (int j = 0; j < n; j++) {
            W[i][j] = opt[j];
        }
    }
    free(opt);
    free(solved);
    FreeAdjList();
    return W;
}
float** DijkstraWthAdjListAPSP(float** graph, int n) {
    float** W = copy2DArray(graph, n);
    int z = createAdjList(W, n);
    //print2DGraph(W, n);
    float* opt = (float*)malloc(n * sizeof(float));
    bool* solved = (bool*)malloc(n * sizeof(bool));

    for (int i = 0; i < n; i++) {
        DijkstraWithAdjList(i, opt, solved, graph, n);
        for (int j = 0; j < n; j++) {
            W[i][j] = opt[j];
        }
    }
    free(opt);
    free(solved);
    FreeAdjList();
    return W;
}

float** DijkstraAPSP(float** graph, int n) {
    float** W = copy2DArray(graph, n);
    float* opt = (float*)malloc(n * sizeof(float));
    bool* solved = (bool*)malloc(n * sizeof(bool));
    for (int i = 0; i < n; i++) {
        Dijkstra(i, opt, solved, graph, n);
        for (int j = 0; j < n; j++) {
            W[i][j] = opt[j];
        }
    }
    free(opt);
    free(solved);

    return W;
}

void ModDijkstra(float** graph, int n) {
    float* opt = (float*)malloc(n * sizeof(float));
    bool* solved = (bool*)malloc(n * sizeof(bool));

    for (int i = 0; i < n; i++) {
        DijkstraWithAdjList(i, opt, solved, graph, n);
        for (int j = 0; j < n; j++) {
            graph[i][j] = opt[j];
        }
    }
    free(opt);
    free(solved);
}
