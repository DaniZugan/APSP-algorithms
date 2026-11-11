#include "GraphTools.h"
#include "../utils.h"
#include "../graphGenerators.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SNAP_FOLDER "../SNAP-graphs"
#define SNAP_CSV "../results/SNAP_data.csv"

// Graphs with more vertices are skipped: graph, W and last take ~12*n^2 bytes (n = 20000 -> 4.8 GB,
// n = 27770 (Cit-HepTh) -> 9.3 GB, which swaps on an 8 GB machine). Above ~46000 vertices n*n overflows
// int in the SCC algorithms (InitAdjList), so the limit must stay below that.
#define SNAP_MAX_NODES 46000

// Number of random source rows checked with Dijkstra for every algorithm, 0 checks all rows
#define SNAP_CHECK_ROWS 0

// 1: bitcoin graphs (file name contains "bitcoin") use their trust ratings as weights,
// rating -10 -> weight 1, rating 10 -> weight 0, linear in between. 0: random weights as for the other graphs.
#define SNAP_BITCOIN_RATINGS 1

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

static int compareStrings(const void* a, const void* b)
{
    return strcmp(*(char* const*)a, *(char* const*)b);
}

// All regular files in folder except hidden ones (.DS_Store, ...), sorted by name
static char** listSnapFiles(const char* folder, int* count)
{
    *count = 0;
    DIR* dir = opendir(folder);
    if (!dir) {
        perror("Failed to open SNAP folder");
        return NULL;
    }

    int capacity = 16;
    char** names = malloc(capacity * sizeof(char*));
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG || entry->d_name[0] == '.') continue;
        if (*count == capacity) {
            capacity *= 2;
            names = realloc(names, capacity * sizeof(char*));
        }
        names[(*count)++] = strdup(entry->d_name);
    }
    closedir(dir);

    qsort(names, *count, sizeof(char*), compareStrings);
    return names;
}

// Reads one line into *buf (grown as needed), without the trailing newline. Returns 0 at end of file.
static int readLine(FILE* file, char** buf, size_t* capacity)
{
    size_t len = 0;
    int ch;
    while ((ch = fgetc(file)) != EOF && ch != '\n') {
        if (len + 1 >= *capacity) {
            *capacity *= 2;
            *buf = realloc(*buf, *capacity);
        }
        (*buf)[len++] = (char)ch;
    }
    if (ch == EOF && len == 0) return 0;
    if (len > 0 && (*buf)[len - 1] == '\r') len--;
    (*buf)[len] = '\0';
    return 1;
}

// Hash table that gives every user name a number (wiki-RfA uses names instead of ids)
typedef struct {
    char** keys;
    long* ids;
    size_t capacity;
    size_t count;
} NameTable;

static unsigned long hashName(const char* s)
{
    unsigned long h = 5381;
    while (*s) h = h * 33 + (unsigned char)*s++;
    return h;
}

static void nameTableInsert(NameTable* t, char* key, long id)
{
    size_t i = hashName(key) & (t->capacity - 1);
    while (t->keys[i] != NULL) i = (i + 1) & (t->capacity - 1);
    t->keys[i] = key;
    t->ids[i] = id;
}

static long nameId(NameTable* t, const char* name)
{
    size_t i = hashName(name) & (t->capacity - 1);
    while (t->keys[i] != NULL) {
        if (strcmp(t->keys[i], name) == 0) return t->ids[i];
        i = (i + 1) & (t->capacity - 1);
    }

    if (2 * (t->count + 1) > t->capacity) { // keep the table at most half full
        char** oldKeys = t->keys;
        long* oldIds = t->ids;
        size_t oldCapacity = t->capacity;
        t->capacity *= 2;
        t->keys = calloc(t->capacity, sizeof(char*));
        t->ids = malloc(t->capacity * sizeof(long));
        for (size_t j = 0; j < oldCapacity; ++j)
            if (oldKeys[j] != NULL) nameTableInsert(t, oldKeys[j], oldIds[j]);
        free(oldKeys);
        free(oldIds);
    }

    long id = (long)t->count++;
    nameTableInsert(t, strdup(name), id);
    return id;
}

static void freeNameTable(NameTable* t)
{
    for (size_t i = 0; i < t->capacity; ++i)
        free(t->keys[i]);
    free(t->keys);
    free(t->ids);
}

typedef struct {
    long* from;
    long* to;
    float* w; // weight of the edge, negative means a random weight from frand()
    int m;
    int capacity;
} EdgeList;

static void addEdge(EdgeList* e, long a, long b, float w)
{
    if (e->m == e->capacity) {
        e->capacity *= 2;
        e->from = realloc(e->from, e->capacity * sizeof(long));
        e->to = realloc(e->to, e->capacity * sizeof(long));
        e->w = realloc(e->w, e->capacity * sizeof(float));
    }
    e->from[e->m] = a;
    e->to[e->m] = b;
    e->w[e->m] = w;
    e->m++;
}

// Trust rating -10..10 (soc-sign-bitcoin*) -> weight 1..0: distrust is a long edge, full trust a free one
static float ratingWeight(long rating)
{
    if (rating < -10) rating = -10;
    if (rating > 10) rating = 10;
    return (float)(10 - rating) / 20.0f;
}

/*
 * Reads a SNAP graph. Supported formats (decided line by line):
 *   - edge list "from to ..." separated by spaces, tabs or commas, "#" comment lines
 *     (Wiki-Vote, Cit-HepTh, email-Eu-core, soc-sign-bitcoin*.csv; extra columns are ignored)
 *   - wikiElec: "U <id> ..." starts an election, every "V <vote> <id> ..." is an edge voter -> candidate
 *   - wiki-RfA: "SRC:<name>" then "TGT:<name>" is an edge source -> target, names are numbered
 *     (votes with an empty SRC are anonymous and ignored)
 * Node ids are not contiguous (e.g. 1001, 9304045), so they are remapped to 0..n-1 in increasing order.
 * Self loops are ignored and every edge gets a random weight from frand(). With SNAP_BITCOIN_RATINGS the
 * bitcoin graphs ("SOURCE,TARGET,RATING,TIME") instead get ratingWeight(RATING) in [0, 1].
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

    EdgeList edges = {malloc(1024 * sizeof(long)), malloc(1024 * sizeof(long)), malloc(1024 * sizeof(float)), 0, 1024};
    int useRatings = SNAP_BITCOIN_RATINGS && strstr(filename, "bitcoin") != NULL;
    NameTable names = {calloc(1024, sizeof(char*)), malloc(1024 * sizeof(long)), 1024, 0};

    size_t lineCapacity = 1024;
    char* line = malloc(lineCapacity);
    long candidate = -1; // wikiElec: candidate of the current election
    long source = -1;    // wiki-RfA: source of the current vote

    while (readLine(file, &line, &lineCapacity)) {
        if (line[0] == '#' || line[0] == '\0') continue;

        if (strncmp(line, "SRC:", 4) == 0) {
            source = (line[4] != '\0') ? nameId(&names, line + 4) : -1;
            continue;
        }
        if (strncmp(line, "TGT:", 4) == 0) {
            if (source != -1 && line[4] != '\0')
                addEdge(&edges, source, nameId(&names, line + 4), -1.0f);
            source = -1;
            continue;
        }
        if (strchr(line, ':') && line[0] >= 'A' && line[0] <= 'Z' && line[1] >= 'A' && line[1] <= 'Z')
            continue; // other wiki-RfA fields (VOT:, RES:, YEA:, DAT:, TXT:)

        long a, b, rating;
        if (line[0] == 'U' && (line[1] == ' ' || line[1] == '\t')) {
            if (sscanf(line + 1, "%ld", &a) == 1) candidate = a;
            continue;
        }
        if (line[0] == 'V' && (line[1] == ' ' || line[1] == '\t')) {
            if (candidate != -1 && sscanf(line + 1, "%ld %ld", &b, &a) == 2) // b = vote, a = voter
                addEdge(&edges, a, candidate, -1.0f);
            continue;
        }

        for (char* c = line; *c; ++c)
            if (*c == ',') *c = ' ';
        int read = sscanf(line, "%ld %ld %ld", &a, &b, &rating);
        if (read >= 2)
            addEdge(&edges, a, b, (useRatings && read == 3) ? ratingWeight(rating) : -1.0f);
    }
    fclose(file);
    free(line);
    freeNameTable(&names);

    long* from = edges.from;
    long* to = edges.to;
    float* w = edges.w;
    int m = edges.m;

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
                graph[u][v] = (w[i] >= 0.0f) ? w[i] : frand();
        }
        graf.matrix = graph;
        graf.size = n;
    }

    free(ids);
    free(from);
    free(to);
    free(w);
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

// Runs all algorithms on every graph file in SNAP-graphs and checks the results with Dijkstra.
// Every run is limited to SNAP_INSTANCE_TIME seconds; a run that exceeds it gets the limit as its time.
// The status column is ok, WRONG, TIMEOUT or CRASH (time -1).
void Run_SNAP_Test(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    while (algs[algCount] != NULL) {
        algCount++;
    }

    int fileCount = 0;
    char** files = listSnapFiles(SNAP_FOLDER, &fileCount);
    if (!files || fileCount == 0) {
        printf("No graph files found in %s\n", SNAP_FOLDER);
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
                // algs[0] (Tree) gets the full limit, the others a multiple of its time
                double limit = i == 0 ? SNAP_INSTANCE_TIME
                                      : RelativeLimit(status[0] == RUN_TIMEOUT ? SNAP_INSTANCE_TIME : times[0],
                                                      SNAP_INSTANCE_TIME);
                status[i] = RunAlgorithmTimed(graph, n, algs[i], limit, &times[i],
                                              snapCheck, &csr, NULL, &wrongRows);
                if (status[i] == RUN_TIMEOUT)
                    times[i] = SNAP_INSTANCE_TIME;

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
