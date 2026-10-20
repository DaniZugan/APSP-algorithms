#include "GraphTools.h"
#include "../utils.h"
#include "../graphGenerators.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SNAP_FOLDER "../SNAP-graphs"
#define SNAP_CSV "../results/SNAP_data.csv"

// Graphs with more vertices are skipped: graph, W and last take ~12*n^2 bytes (n = 20000 -> 4.8 GB)
#define SNAP_MAX_NODES 20000

// Number of random source rows checked with Dijkstra for every algorithm, 0 checks all rows
#define SNAP_CHECK_ROWS 0

// Pomožne funkcije ----------------------------------------------------------------------------------------------------
static int compareLongs(const void* a, const void* b)
{
    long x = *(const long*)a;
    long y = *(const long*)b;
    return (x > y) - (x < y);
}

// Index of id in the sorted array ids (ids are unique)
static int findId(const long* ids, int n, long id)
{
    int lo = 0, hi = n - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        if (ids[mid] < id) lo = mid + 1;
        else if (ids[mid] > id) hi = mid - 1;
        else return mid;
    }
    return -1;
}

/*
 * Reads a SNAP edge list ("# ..." comment lines, then "from to" per line).
 * SNAP node ids are not contiguous (e.g. 1001, 9304045), so they are remapped
 * to 0..n-1 in increasing order. Self loops are ignored and every edge gets a
 * random weight from frand().
 *
 * Returns a graph with size 0 if the file cannot be read or has more than
 * maxNodes vertices (then *skippedNodes holds its vertex count).
 */
static Graf ReadSnapGraph(const char* filename, int maxNodes, int* skippedNodes)
{
    Graf graf = {NULL, 0};
    *skippedNodes = 0;

    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open SNAP file");
        return graf;
    }

    int capacity = 1024, m = 0;
    long* from = malloc(capacity * sizeof(long));
    long* to = malloc(capacity * sizeof(long));

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        if (line[0] == '#') continue;
        long a, b;
        if (sscanf(line, "%ld %ld", &a, &b) != 2) continue;
        if (m == capacity) {
            capacity *= 2;
            from = realloc(from, capacity * sizeof(long));
            to = realloc(to, capacity * sizeof(long));
        }
        from[m] = a;
        to[m] = b;
        m++;
    }
    fclose(file);

    // Remap ids to 0..n-1
    long* ids = malloc(2 * (size_t)m * sizeof(long) + 1);
    for (int i = 0; i < m; ++i) {
        ids[2*i] = from[i];
        ids[2*i + 1] = to[i];
    }
    qsort(ids, 2 * (size_t)m, sizeof(long), compareLongs);
    int n = 0;
    for (int i = 0; i < 2*m; ++i)
        if (n == 0 || ids[i] != ids[n - 1])
            ids[n++] = ids[i];

    if (n > maxNodes) {
        *skippedNodes = n;
    }
    else if (n > 0) {
        float** graph = allocate_graph(n);
        for (int i = 0; i < m; ++i) {
            int u = findId(ids, n, from[i]);
            int v = findId(ids, n, to[i]);
            if (u != v)
                graph[u][v] = frand();
        }
        graf.matrix = graph;
        graf.size = n;
    }

    free(ids);
    free(from);
    free(to);
    return graf;
}

// Graph in compressed sparse row form, used by the Dijkstra check
typedef struct {
    int n;
    int* start;     // edges of u are adj[start[u] .. start[u+1])
    int* adj;
    float* w;
} CSR;

static CSR toCSR(float** graph, int n)
{
    CSR g;
    g.n = n;
    g.start = calloc((size_t)n + 1, sizeof(int));
    int m = 0;
    for (int u = 0; u < n; ++u) {
        g.start[u] = m;
        for (int v = 0; v < n; ++v)
            if (u != v && isfinite(graph[u][v])) m++;
    }
    g.start[n] = m;
    g.adj = malloc(((size_t)m + 1) * sizeof(int));
    g.w = malloc(((size_t)m + 1) * sizeof(float));
    for (int u = 0; u < n; ++u) {
        int e = g.start[u];
        for (int v = 0; v < n; ++v)
            if (u != v && isfinite(graph[u][v])) {
                g.adj[e] = v;
                g.w[e] = graph[u][v];
                e++;
            }
    }
    return g;
}

static void freeCSR(CSR* g)
{
    free(g->start);
    free(g->adj);
    free(g->w);
}

// Binary min-heap of (distance, vertex) with lazy deletion
typedef struct {
    float d;
    int v;
} HeapItem;

static void heapPush(HeapItem* h, int* size, float d, int v)
{
    int i = (*size)++;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h[p].d <= d) break;
        h[i] = h[p];
        i = p;
    }
    h[i].d = d;
    h[i].v = v;
}

static HeapItem heapPop(HeapItem* h, int* size)
{
    HeapItem top = h[0];
    HeapItem last = h[--(*size)];
    int i = 0;
    while (1) {
        int c = 2*i + 1;
        if (c >= *size) break;
        if (c + 1 < *size && h[c + 1].d < h[c].d) c++;
        if (last.d <= h[c].d) break;
        h[i] = h[c];
        i = c;
    }
    h[i] = last;
    return top;
}

// Single-source shortest paths from s into dist (heap needs room for m + 1 items)
static void dijkstraRow(const CSR* g, int s, float* dist, HeapItem* heap)
{
    for (int v = 0; v < g->n; ++v) dist[v] = INFINITY;
    dist[s] = 0;
    int size = 0;
    heapPush(heap, &size, 0, s);
    while (size > 0) {
        HeapItem it = heapPop(heap, &size);
        if (it.d > dist[it.v]) continue;
        for (int e = g->start[it.v]; e < g->start[it.v + 1]; ++e) {
            int v = g->adj[e];
            float nd = it.d + g->w[e];
            if (nd < dist[v]) {
                dist[v] = nd;
                heapPush(heap, &size, nd, v);
            }
        }
    }
}

/*
 * Checks the APSP matrix row by row against Dijkstra, so no n x n reference
 * matrix is needed. Returns the number of wrong rows.
 */
static int checkWithDijkstra(const CSR* g, float** mat, int rows)
{
    int n = g->n;
    float* dist = malloc(n * sizeof(float));
    HeapItem* heap = malloc(((size_t)g->start[n] + 1) * sizeof(HeapItem));
    int checked = (rows <= 0 || rows > n) ? n : rows;
    int wrongRows = 0;

    for (int r = 0; r < checked; ++r) {
        int s = (checked == n) ? r : rand() % n;
        dijkstraRow(g, s, dist, heap);
        for (int d = 0; d < n; ++d) {
            if (d == s) continue;
            float a = mat[s][d];
            float b = dist[d];
            if (isfinite(a) != isfinite(b) || (isfinite(a) && fabs(a - b) >= 0.01)) {
                wrongRows++;
                break;
            }
        }
    }

    free(dist);
    free(heap);
    return wrongRows;
}

// CheckFP for RunAlgorithmTimed, ctx is the CSR graph
static int snapCheck(float** mat, int n, void* ctx)
{
    return checkWithDijkstra((const CSR*)ctx, mat, SNAP_CHECK_ROWS);
}

// ---------------------------------------------------------------------------------------------------------------------

// Runs all algorithms on every .txt graph in SNAP-graphs and checks the results with Dijkstra.
// Every run is limited to MAX_INSTANCE_TIME seconds; a run that exceeds it gets the limit as its time.
// The status column is ok, WRONG, TIMEOUT or CRASH (time -1).
void Run_SNAP_Test(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    while (algs[algCount] != NULL) {
        algCount++;
    }

    int fileCount = 0;
    char** files = getFilenamesInFolder(SNAP_FOLDER, &fileCount);
    if (!files || fileCount == 0) {
        printf("No .txt files found in %s\n", SNAP_FOLDER);
        free(files);
        return;
    }

    printf("SNAP test -> algorithm count: %d, graph count: %d\n", algCount, fileCount);

    // Create the CSV header, overwriting previous results.
    FILE* csvFile = fopen(SNAP_CSV, "w");
    if (csvFile == NULL) {
        perror("Cannot create SNAP_data.csv");
        return;
    }
    fprintf(csvFile, "graph,");
    for (int i = 0; i < algCount; ++i)
        fprintf(csvFile, "%s,", algNames[i]);
    for (int i = 0; i < algCount; ++i)
        fprintf(csvFile, "%s status,", algNames[i]);
    writeGraphDataHeader(csvFile);
    fclose(csvFile);

    double* times = malloc(algCount * sizeof(double));
    RunStatus* status = malloc(algCount * sizeof(RunStatus));

    for (int f = 0; f < fileCount; ++f) {
        char filename[512];
        snprintf(filename, sizeof(filename), "%s/%s", SNAP_FOLDER, files[f]);

        for (int run = 0; run < TESTRUNS; ++run) {
            int skippedNodes = 0;
            Graf graf = ReadSnapGraph(filename, SNAP_MAX_NODES, &skippedNodes);
            if (skippedNodes > 0) {
                printf("%s: skipped, %d vertices > SNAP_MAX_NODES (%d)\n",
                       files[f], skippedNodes, SNAP_MAX_NODES);
                break;
            }
            if (graf.size == 0) {
                printf("%s: no edges read, skipped\n", files[f]);
                break;
            }

            float** graph = graf.matrix;
            int n = graf.size;
            printf("%s: N = %d\n===========\n", files[f], n);

            GraphData data = getGraphData(graph, n);
            CSR csr = toCSR(graph, n);

            for (int i = 0; i < algCount; ++i) {
                int wrongRows = 0;
                status[i] = RunAlgorithmTimed(graph, n, algs[i], MAX_INSTANCE_TIME, &times[i],
                                              snapCheck, &csr, NULL, &wrongRows);

                printf("%-20s %8.3f s  %s\n", algNames[i], times[i], RunStatusName(status[i]));
                if (status[i] == RUN_WRONG)
                    printf("  %d rows differ from Dijkstra\n", wrongRows);
            }

            csvFile = fopen(SNAP_CSV, "a");
            if (csvFile != NULL) {
                fprintf(csvFile, "%s,", files[f]);
                for (int i = 0; i < algCount; ++i)
                    fprintf(csvFile, "%.3lf,", times[i]);
                for (int i = 0; i < algCount; ++i)
                    fprintf(csvFile, "%s,", RunStatusName(status[i]));
                writeGraphDataToCSV(csvFile, &data);
                fclose(csvFile);
            }

            freeCSR(&csr);
            freeGraphData(&data);
            freeGraph(graph, n);
        }
    }

    free(times);
    free(status);
    for (int i = 0; i < fileCount; ++i)
        free(files[i]);
    free(files);
}
