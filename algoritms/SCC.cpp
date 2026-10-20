#include "SCC.h"
#include "../graphGenerators.h"
#include <cstring>
#include <cstdint>
#include <ctime>
#include <cstdio>

#include "../utils.h"
#include "Tree.h"


tEdge* EE;
int ee;
int* firstEE;

// adjacency list
static int* E;
static idxE* V;

// initialise adj. list
void InitAdjList(int N) {
    E = (int*)malloc(N*N * sizeof(int));
    V = (idxE*)malloc(N * sizeof(idxE));
}

void addEdge(int a, int b) {
    E[V[a].start + V[a].deg] = b;
    V[a].deg++;
}
void addtEdge(int a, int b) {
    EE[ee].a = a;
    EE[ee].b = b;
    EE[ee].nxt = -1;
    ee++;
}
void tEdgesToSCCs(int* sccComponent, int sccCount) {
    // connect EE edges to the component
    firstEE = allocateIntArray(sccCount, -1);
    for (int i = 0; i < ee; ++i) {
        int comp = sccComponent[EE[i].a];
        EE[i].nxt = firstEE[comp];
        firstEE[comp] = i;
    }
}



GraphAdjList createGraphAdjList(int size) {
    int** edges = create2DInt(size, 1);
    int* deg = (int*)calloc(size, sizeof(int));
    int n = size;
    GraphAdjList graph = {edges, deg, n};
    return graph;
}
void addEdgeAdjList(int tail, int head, GraphAdjList G) {
    G.edges[tail][G.deg[tail]] = head;
    G.deg[tail]++;
}
void FreeSCCsGraph(GraphAdjList graph) {
    free2DInt(graph.edges, graph.size);
    free(graph.deg);
}

void PrintAdjList(GraphAdjList graph) {
    for (int i = 0; i < graph.size; ++i) {
        for (int j = 0; j < graph.deg[i]; ++j) {

        }
    }
}

int** tEdgesToCondenseGraph(int* sccComponent, int sccCount) {
    GraphAdjList graph = createGraphAdjList(sccCount);
    int** G = create2DInt(sccCount, 0);
    firstEE = allocateIntArray(sccCount, -1);
    for (int i = 0; i < ee; ++i) {
        int comp = sccComponent[EE[i].a];
        EE[i].nxt = firstEE[comp];
        firstEE[comp] = i;
        int headComp = sccComponent[EE[i].b];
           // če še ni povezave
        G[comp][headComp]++;      // dodaj povezavo v matriko sosednosti
    }
    //GraphDual graf = {graph, G};
    return G;
}
void FreeGraphDual(GraphDual graph) {
    free2DInt(graph.matrika.matrix, graph.matrika.size);
    free2DInt(graph.seznam.edges, graph.seznam.size);
    free(graph.seznam.deg);
}

void tarjanDFS_addEdges(int v, float** graph, int n, int* ids, int* low, int* stack, int* stackTop,
               bool* onStack, int* sccComponent, int* nextInComponent, int* firstInComponent,
               int* id, int* sccCount) {
    ids[v] = low[v] = (*id)++;
    stack[(*stackTop)++] = v;
    onStack[v] = true;
    V[v].start = v * n;
    V[v].deg = 0;
    for (int w = 0; w < n; ++w) {
        if (graph[v][w] == INFINITY && v != w) continue; // Skip if no edge exists
        addEdge(v, w); // add edge to the adjacency list
        if (ids[w] == -1) { // If w is not visited
            tarjanDFS_addEdges(w, graph, n, ids, low, stack, stackTop, onStack, sccComponent, nextInComponent, firstInComponent, id, sccCount);
            if (!onStack[w]) {
                addtEdge(v, w);
            }

            if (low[w] < low[v]) {
                low[v] = low[w];
            }
        } else if (onStack[w]) { // If w is in the stack
            if (ids[w] < low[v]) {
                low[v] = ids[w];
            }
        } else addtEdge(v, w); // outgoing edge
    }


    if (ids[v] == low[v]) { // Found the root of an SCC
        // Pop nodes and build the SCC linked list
        int w = stack[--(*stackTop)];
        onStack[w] = false;
        sccComponent[w] = *sccCount;
        firstInComponent[*sccCount] = w; // Always set first element

        int prev = w;

        while (w != v) {
            w = stack[--(*stackTop)];
            onStack[w] = false;
            sccComponent[w] = *sccCount;
            nextInComponent[prev] = w; // Link previous to current
            prev = w;
        }

        nextInComponent[prev] = -1; // Last node in SCC points to -1
        (*sccCount)++;
    }

    /*if (ids[v] == low[v]) { // Found the root of an SCC
        int w, sccSize = 0;
        int* scc = (int*)malloc(n * sizeof(int));

        do {
            w = stack[--(*stackTop)];
            onStack[w] = false;
            sccComponent[w] = *sccCount;
            scc[sccSize++] = w;
        } while (w != v);

        // Update nextInComponent and firstInComponent arrays for this SCC
        firstInComponent[*sccCount] = scc[0];
        for (int i = 0; i < sccSize; ++i) {
            nextInComponent[scc[i]] = (i + 1 < sccSize) ? scc[i + 1] : -1;
        }

        (*sccCount)++;
        free(scc);
    } */
}

SCCResult findSCCs_addEdges(float** graph, int n) {
    int* ids = allocateIntArray(n, -1);
    int* low = allocateIntArray(n, 0);
    int* stack = (int*)malloc(n * sizeof(int));
    int stackTop = 0;
    bool* onStack = (bool*)malloc(n * sizeof(bool));
    memset(onStack, false, n * sizeof(bool));

    EE = (tEdge *)malloc(n*(n-1) * sizeof(tEdge));
    ee = 0;

    int* sccComponent = allocateIntArray(n, -1);
    int* nextInComponent = allocateIntArray(n, -1);
    int* firstInComponent = allocateIntArray(n, -1);

    int id = 0, sccCount = 0;
    V[0].start = 0;
    V[0].deg = 0;
    for (int i = 0; i < n; ++i) {
        if (ids[i] == -1) {
            tarjanDFS_addEdges(i, graph, n, ids, low, stack, &stackTop, onStack, sccComponent, nextInComponent, firstInComponent, &id, &sccCount);
        }
    }
    free(ids);
    free(low);
    free(stack);
    free(onStack);

    //tEdgesToSCCs(sccComponent, sccCount); // connect tEdges with SCCs

    // Create and return the result struct
    SCCResult result = {sccComponent, nextInComponent, firstInComponent, sccCount};
    return result;
}

void extractSCC(float** graph, float** SCC, int* component, int componentSize) {
    for (int i = 0; i < componentSize; i++) {
        for (int j = 0; j < componentSize; ++j) {
            SCC[i][j] = graph[component[i]][component[j]];
        }
    }
}
void mapBackSCC(float** W, float** SCC, int* component, int componentSize) {
    for (int i = 0; i < componentSize; ++i) {
        for (int j = 0; j < componentSize; ++j) {
            W[component[i]][component[j]] = SCC[i][j];
        }
    }
}
void APSPseparately(float** WCCgraph, int n, SCCResult res) {
    int size = 1 + n - res.sccCount; // max possible size of a SCC
    int *comp = (int *)malloc(n * sizeof(int)); // to store vertices in current SCC
    float** SCCgraph = createGraph(size);
    for (int K = 0; K < res.sccCount; ++K) {
        int vertex = res.firstInComponent[K];
        int sccSize = 0;
        while (vertex != -1) {
            comp[sccSize++] = vertex;
            vertex = res.nextInComponent[vertex];
        }
        extractSCC(WCCgraph, SCCgraph, comp, sccSize);
        ModTree(SCCgraph, sccSize);
        mapBackSCC(WCCgraph, SCCgraph, comp, sccSize);
    }
    free(comp);
    freeGraph(SCCgraph, size);
}

// relax via: all vertices in K + outgoing edge (u, v) + all vertices in L
void relaxOutEdges(SCCResult SCCs, int L, int K, int u, int v, float** W) {
    int w = SCCs.firstInComponent[L];
    while (w != -1) {   // loop through every vertex in component L
        int x = SCCs.firstInComponent[K];
        while (x != -1) {   // loop through every vertex in component K
            float dist = W[x][u] + W[u][v] + W[v][w];
            if (dist < W[x][w]) {
                //printf("x: %d, (%d, %d), w: %d. old value: %0.0f, new value: %0.0f\n", x, u, v, w, W[x][w], dist);
                W[x][w] = dist; // relaxation
            }
            x = SCCs.nextInComponent[x];
        }
        w = SCCs.nextInComponent[w];
    }
}
void relaxOutEdges2(SCCResult SCCs, int inter, int src, int dest, float** W) {
    int i = SCCs.firstInComponent[inter];
    while (i != -1) {           // loop through every vertex in component inter
        int s = SCCs.firstInComponent[src];
        while (s != -1) {       // loop through every vertex in component src
            int d = SCCs.firstInComponent[dest];
            while (d != -1) {   // loop through every vertex in component dest
                float dist = W[s][i] + W[i][d];
                if (dist < W[s][d]) {
                    W[s][d] = dist; // relaxation
                }
                d = SCCs.nextInComponent[d];
            }
            s = SCCs.nextInComponent[s];
        }
        i = SCCs.nextInComponent[i];
    }
}

// process outgoing edges for shortest paths between SCCs
void processOutgoingEdges(SCCResult SCCs, float** W, float** graph) {
    int** reach = create2DInt(SCCs.sccCount, 1); // reachability for between SCCs
    for (int K = 0; K < SCCs.sccCount; ++K) {
        reach[K][K] = 1;
        int e = firstEE[K];
        while (e != -1) { // for each outgoing edge
            int u = EE[e].a;
            int v = EE[e].b;
            if (graph[u][v] <= W[u][v]) { // lahko obstaja krajša pot od a do b kot je sama povezava
                int C = SCCs.component[v];
                for (int L = C; L >= 0; --L) { // Iterate through components in reverse order
                    if (reach[C][L] == 1) {
                        // If component L is reachable from component C
                        reach[K][L] = 1; // Mark as reachable
                        relaxOutEdges(SCCs, L, K, u, v, W); // Perform relaxation
                    }
                }
            }
            e = EE[e].nxt;
        }
    }
    free2DInt(reach, SCCs.sccCount);
}

// process outgoing edges for shortest paths between SCCs (relaxing via vertices)
void processOnlyInters(SCCResult SCCs, float** W, float** graph) {
    int** reach = create2DInt(SCCs.sccCount, 1); // reachability for between SCCs
    for (int K = 0; K < SCCs.sccCount; ++K) { // for each SCC (K = source SCC)
        reach[K][K] = 1;
        int e = firstEE[K];
        while (e != -1) { // for each outgoing edge
            int tail = EE[e].a;     // tail
            int head = EE[e].b;     // head
            int source = SCCs.firstInComponent[K];
            while (source != -1) { // for vertex in source SCC
                float dist = W[source][tail] + W[tail][head]; // find intermediate shortest paths from sources to heads
                if (W[source][head] > dist) {
                    W[source][head] = W[source][tail] + W[tail][head];
                }
                source = SCCs.nextInComponent[source];
            }
            e = EE[e].nxt;
        }
    }
    free2DInt(reach, SCCs.sccCount);
}

void processEdgesBetweenSCCs(SCCResult SCCs, float** W, int** reach) {
    for (int S = 1; S < SCCs.sccCount; ++S) {
            for (int D = S-1; D >= 0; D--) {
                if (reach[S][D] == 1) {
                    for (int K = S; K >= D; --K) {
                        if (reach[S][K] == 1 && reach[K][D] == 1) {
                            relaxOutEdges2(SCCs, K, S, D, W);
                        }
                    }
                }
            }
    }
}

void betweenSCCsByVertex(SCCResult SCCs, float** W) {
   for (int K = 0; K < SCCs.sccCount; ++K) {

   }
}

void FreeAdjList() {
    free(E);
    free(V);
}
void freeSCCs(SCCResult res) {
    free(res.component);
    free(res.nextInComponent);
    free(res.firstInComponent);
}

// for adjecency matrix and for .csv export
void tarjanDFS(int v, float** graph, int n, int* ids, int* low, int* stack, int* stackTop,
               bool* onStack, int* sccComponent, int* nextInComponent, int* firstInComponent,
               int* id, int* sccCount) {
    ids[v] = low[v] = (*id)++;
    stack[(*stackTop)++] = v;
    onStack[v] = true;

    for (int w = 0; w < n; ++w) {
        if (graph[v][w] == INFINITY) continue; // Skip if no edge exists

        if (ids[w] == -1) { // If w is not visited
            tarjanDFS(w, graph, n, ids, low, stack, stackTop, onStack, sccComponent, nextInComponent, firstInComponent, id, sccCount);
            if (low[w] < low[v]) {
                low[v] = low[w];
            }
        } else if (onStack[w]) { // If w is in the stack
            if (ids[w] < low[v]) {
                low[v] = ids[w];
            }
        }
    }

    if (ids[v] == low[v]) { // Found the root of an SCC
        int w, sccSize = 0;
        int* scc = (int*)malloc(n * sizeof(int));

        do {
            w = stack[--(*stackTop)];
            onStack[w] = false;
            sccComponent[w] = *sccCount;
            scc[sccSize++] = w;
        } while (w != v);

        // Update nextInComponent and firstInComponent arrays for this SCC
        firstInComponent[*sccCount] = scc[0];
        for (int i = 0; i < sccSize; ++i) {
            nextInComponent[scc[i]] = (i + 1 < sccSize) ? scc[i + 1] : -1;
        }

        (*sccCount)++;
        free(scc);
    }
}

SCCResult findSCCs(float** graph, int n) {
    int* ids = allocateIntArray(n, -1);
    int* low = allocateIntArray(n, 0);
    int* stack = (int*)malloc(n * sizeof(int));
    int stackTop = 0;
    bool* onStack = (bool*)malloc(n * sizeof(bool));
    memset(onStack, false, n * sizeof(bool));

    int* sccComponent = allocateIntArray(n, -1);
    int* nextInComponent = allocateIntArray(n, -1);
    int* firstInComponent = allocateIntArray(n, -1);

    int id = 0, sccCount = 0;

    for (int i = 0; i < n; ++i) {
        if (ids[i] == -1) {
            tarjanDFS(i, graph, n, ids, low, stack, &stackTop, onStack, sccComponent, nextInComponent, firstInComponent, &id, &sccCount);
        }
    }

    free(ids);
    free(low);
    free(stack);
    free(onStack);

    // Create and return the result struct
    SCCResult result = {sccComponent, nextInComponent, firstInComponent, sccCount};
    return result;
}

int** BooleanReach(int** reach, int n) {
    for (int k = 0; k < n; ++k)
        for (int i = 0; i < n; ++i)
            for (int j = 0; j < n; ++j)
                reach[i][j] = reach[i][j] || (reach[i][k] && reach[k][j]);
    for (int i = 0; i < n; ++i) {
        reach[i][i] = 1;
    }
    return reach;
}

// relax by edge
float** SCCalgo(float** graph, int N){
    float** W = copy2DArray(graph, N);
    //rc_reset(&rc);
    InitAdjList(N);                                 // adjacency list
    SCCResult SCCs = findSCCs_addEdges(W, N);       // find SCCs with outgoing edges
    tEdgesToSCCs(SCCs.component, SCCs.sccCount);    // map outgoing edges with tail components

    if (SCCs.sccCount>1) {                      // if we have at least two components
        APSPseparately(W, N, SCCs);             // APSP separately
        processOutgoingEdges(SCCs, W, graph);   // Relax via outgoing edges
    }
    else ModTree(W, N);                         // else just Tree algo in the whole graph

    free(EE); free(firstEE); FreeAdjList(); freeSCCs(SCCs);
    return W;
}

// relax by vertices (naive)
float** SCCalgo2(float** graph, int N){
    float** W = copy2DArray(graph, N);

    InitAdjList(N);
    SCCResult SCCs = findSCCs_addEdges(W, N);
    int** SCCsGraph = tEdgesToCondenseGraph(SCCs.component, SCCs.sccCount);
    //print2DInt(SCCsGraph, SCCs.sccCount);
    int** reach = BooleanReach(SCCsGraph, SCCs.sccCount);
    //print2DInt(reach, SCCs.sccCount);
    APSPseparately(W, N, SCCs);
    processEdgesBetweenSCCs(SCCs, W, reach);
    //free2DInt(reach, SCCs.sccCount);
    free2DInt(SCCsGraph, SCCs.sccCount);
    free(EE); free(firstEE); FreeAdjList(); freeSCCs(SCCs);
    return W;
}

// VertexRelax for second paper ----------------------------------------------------------------------------------------
// Bridge is for information about tail and head vertices for every component
Bridges createBridges(int N, int C) {

    int** tailHead = create2DInt(N, 1);
    int* tailOut = allocateIntArray(N,0);

    int* firstTail = allocateIntArray(N,-1);
    int* nextTail = allocateIntArray(N,-1);

    bool** isTail = create2DBoolArray(C, N);
    bool** isHead = create2DBoolArray(C, N);

    int* tailCount = allocateIntArray(C, 0);

    int** heads = create2DInt(N, 1);
    int* headCount = allocateIntArray(C, 0);

    Bridges B = {tailHead, tailOut, firstTail, nextTail, isTail, isHead, tailCount, heads, headCount};
    return B;
}
void freeBridges(Bridges B, int N, int C) {
    // 1. Free 2D int array (tailHead)
    if (B.tailHead != NULL) {
        for (int i = 0; i < N; i++) {
            free(B.tailHead[i]); // Free inner int arrays
        }
        free(B.tailHead);        // Free outer pointer array
    }

    // 2. Free 1D vertex arrays
    free(B.tailOut);
    free(B.firstTail);
    free(B.nextTail);

    // 3. Free 2D boolean matrices (isTail & isHead)
    if (B.isTail != NULL) {
        for (int i = 0; i < C; i++) {
            free(B.isTail[i]);
        }
        free(B.isTail);
    }

    if (B.isHead != NULL) {
        for (int i = 0; i < C; i++) {
            free(B.isHead[i]);
        }
        free(B.isHead);
    }

    // 4. Free 1D component arrays
    free(B.tailCount);
    for (int i = 0; i < C; i++) {
        free(B.heads[i]);
    }
    free(B.heads);
    free(B.headCount);
}

static void add(const int tail, const int head, Bridges* B, const int* scc) {
    int comp = scc[tail];           // find tail's component

    B->tailHead[tail][B->tailOut[tail]] = head; // add head to the matrix
    B->tailOut[tail]++;               // increase counter for heads

    if (!B->isTail[comp][tail])         // if we find this tail for the first time mark it and increase the counter
    {
        B->isTail[comp][tail] = true;
        B->tailCount[comp]++;

        int prev = B->firstTail[comp];   // find first tail in this component
        B->firstTail[comp] = tail;       // set current tail as first one
        B->nextTail[tail] = prev;        // and point to previous first as next
    }

    if (!B->isHead[comp][head])         // if we find this head for the first time mark it and increase the counter
    {
        B->isHead[comp][head] = true;
        B->heads[comp][B->headCount[comp]] = head;
        B->headCount[comp]++;
    }
}

static Bridges tEdgesToVertexRelaxSets(const int* sccComponent, const int sccCount, const int N) {
    Bridges B = createBridges(N, sccCount);

    for (int i = 0; i < ee; ++i) {
        int t = EE[i].a;    // tail vertex
        int h = EE[i].b;    // head vertex
        add(t, h, &B, sccComponent);
    }

    return B;
}

static void mapBack(float** W, float** SCC, int** last, int** SCClast, const int* component, const int componentSize) {
    for (int i = 0; i < componentSize; ++i) {
        for (int j = 0; j < componentSize; ++j) {
            W[component[i]][component[j]] = SCC[i][j];
            last[component[i]][component[j]] = component[SCClast[i][j]];
        }
    }
}

static void constructOutTree(SimpleTree* trees, int k, int t, SCCResult scc, float** W, int** last) {

    int* reach = (int*)malloc(t * sizeof(int));
    int reachSize = 0;
    trees[k].nextSibling = NULL; // root may be a tail in C_t, which is not reset below
    trees[k].firstChild = NULL;
    for (int i = 0; i < t; ++i) // set up tree for every vertex in reachable components
    {
        int d = scc.firstInComponent[i];
        if (isfinite_cust(W[k][d]))
        {
            reach[reachSize++] = i; // add component to the reach array
            while (d != -1)
            {
                trees[d].nextSibling = NULL;
                trees[d].firstChild = NULL;
                d = scc.nextInComponent[d];
            }
        }
    }
    for (int r = 0; r < reachSize; ++r) // set up tree
    {
        int comp = reach[r];
        int d = scc.firstInComponent[comp];
        while (d != -1) {
            if (d != k) // Nothing to do for d=h
            {
                int parent = last[k][d];
                // For a tail root the path can go through another tail of C_t. Such d is not in this tree, which is
                // fine, since that tail is a pivot itself and covers d.
                if (parent != k && scc.component[parent] >= t)
                {
                    d = scc.nextInComponent[d];
                    continue;
                }
                if (trees[parent].firstChild == NULL)
                    trees[parent].firstChild = &(trees[d]);
                else
                {
                    trees[d].nextSibling = trees[parent].firstChild;
                    trees[parent].firstChild = &(trees[d]);
                }
            }
            d = scc.nextInComponent[d];
        }
    }
    free(reach);
}

static int buildDFS(SimpleTree* outTrees, int k, TreeStack* ts, int* dfs, int* idfs, int* skip) {
    SimpleTree* toVisit = outTrees[k].firstChild;
    while (toVisit != NULL)
    {
        //add it to our stack to explore
        TreeStack_push(ts, toVisit);

        toVisit = toVisit->nextSibling;
    }
    // Now traverse as DFS
    int c = 0;
    while (ts->size != 0)
    {
        toVisit = TreeStack_top(ts);
        TreeStack_pop(ts);
        dfs[c] = toVisit->label;
        idfs[toVisit->label] = c;
        if (ts->size > 0)
            skip[c] = TreeStack_top(ts)->label; // next entry on stack is where we have to skip
        else
            skip[c] = -1; // skip to end
        c++;
        toVisit = toVisit->firstChild;
        while (toVisit != NULL)
        {
            //add it to our stack to explore
            TreeStack_push(ts, toVisit);
            toVisit = toVisit->nextSibling;
        }
    }

    // Now fix the skip array, since it uses vertex ids, but needs dfs offsets
    for (int v=0; v<c; v++)
    {
        if (skip[v] == -1)
            skip[v] = c;
        else
            skip[v] = idfs[skip[v]];
    }

    return c;
}

static void separatelyTree(float** WCCgraph, int n, SCCResult scc, int** last) {
    int size = 1 + n - scc.sccCount; // max possible size of a SCC
    int *comp = (int *)malloc(n * sizeof(int)); // to store vertices in current SCC
    float** SCCgraph = createGraph(size);
    for (int K = 0; K < scc.sccCount; ++K) {
        int vertex = scc.firstInComponent[K];
        int sccSize = 0;
        while (vertex != -1) {
            comp[sccSize++] = vertex;
            vertex = scc.nextInComponent[vertex];
        }
        extractSCC(WCCgraph, SCCgraph, comp, sccSize);      // get component subgraph
        int** SCClast = VertexTree(SCCgraph, sccSize);                // solve APSP and get last of subgraph
        mapBack(WCCgraph, SCCgraph, last, SCClast , comp, sccSize); // put results back to the main W and last
        free2DInt(SCClast, sccSize);
    }
    free(comp);
    freeGraph(SCCgraph, size);
}

static float** BridgeTree(const SCCResult Scc, const Bridges B, float** W, const int n, int** last) {
    // Initialization the same as in Tree algorithm
    //int** last = (int**)malloc(sizeof(int*)*n); // Last node on path (without destination)
    int* dfs = (int*)malloc(sizeof(int)*(n-1)); // DFS-order permutation of the vertices
    int* skip = (int*)malloc(sizeof(int)*(n-1)); // Skip array to skip subtrees in the DFS traversal
    int* idfs = (int*)malloc(sizeof(int)*n); // inverse DFS permutation, used when building skip
    float* Wh = (float*)malloc(sizeof(float)*(n-1)); // W[k][] distances rearranged to fit permutation order


    SimpleTree* outTrees = (SimpleTree*)malloc(sizeof(SimpleTree)*n);
    for (int i=0; i<n; i++)
        outTrees[i].label = i;

    TreeStack ts;
    TreeStack_create(&ts, n);

    // Loop from last to first component
    for (int t = 1; t < Scc.sccCount; ++t)
    {
        clock_t start = clock();
        // Bridge step -------------------------------------------------------------------------------------------------
        int k = B.firstTail[t];
        while (k != -1) // for all pivot vertices
        {
            int s = Scc.firstInComponent[t];
            while (s != -1) // for all sources
            {
                float Wsk = W[s][k];
                for (int i = 0; i < B.tailOut[k]; ++i) // for all heads of this tail (destinations)
                {
                    int h = B.tailHead[k][i];
                    if (Wsk + W[k][h] < W[s][h]) {
                        W[s][h] = Wsk + W[k][h];
                        last[s][h] = last[k][h];
                    }
                }
                s = Scc.nextInComponent[s];
            }
            k = B.nextTail[k];
        }
        clock_t end = clock();
        double aTime = static_cast<double>(end - start) / CLOCKS_PER_SEC;

        start = clock();
        // Reduced-relaxation step -------------------------------------------------------------------------------------
        for (int i = 0; i < B.headCount[t]; ++i)
        {
            int h = B.heads[t][i];              // pivots
            constructOutTree(outTrees, h, t, Scc, W, last);
            int c = buildDFS(outTrees, h, &ts, dfs, idfs, skip);

            for (int v=0; v<c; v++)
                Wh[v] = W[h][dfs[v]];

            int s = Scc.firstInComponent[t];
            while (s != -1)                     // sources
            {
                float Wsh = W[s][h];
                if (!isfinite_cust(Wsh))
                {
                    printf("Something wrong because all heads should be reachable by destinations in SCCvertex.\n");
                    printf("s: %d, h: %d", s, h);
                    s = Scc.nextInComponent[s];
                    continue;
                }

                float* Ws = W[s];

                for (int v = 0; v < c;)
                {
                    int d = dfs[v];             // destinations
                    if (Wsh + Wh[v] < Ws[d])
                    {
                        Ws[d] = Wsh + Wh[v];
                        last[s][d] = last[h][d];

                        v++;
                    }
                    else v = skip[v]; // we can skip the subtree since we didn't improve our distance
                }

                s = Scc.nextInComponent[s];
            }
        }
        end = clock();
        //double bTime = static_cast<double>(end - start) / CLOCKS_PER_SEC;
        //printf("Time bridge/red-relax: %.2f ratio\n", aTime/bTime);
    }

    free(dfs);
    free(idfs);
    free(skip);
    free(Wh);
    free(outTrees);
    TreeStack_destroy(&ts);

    return W;
    }

// Buffers for the out-tree DFS traversal, reused for every pivot
typedef struct {
    SimpleTree* outTrees;
    TreeStack ts;
    int* dfs;     // DFS-order permutation of the vertices
    int* skip;    // Skip array to skip subtrees in the DFS traversal
    int* idfs;    // inverse DFS permutation, used when building skip
    float* Wp;    // W[p][] distances rearranged to fit permutation order
    int* headIdx; // index of a head in B.heads[t]
} TreeBuffers;

static void createTreeBuffers(TreeBuffers* tb, const int n) {
    tb->outTrees = (SimpleTree*)malloc(sizeof(SimpleTree)*n);
    for (int i=0; i<n; i++)
        tb->outTrees[i].label = i;
    TreeStack_create(&tb->ts, n);
    tb->dfs = (int*)malloc(sizeof(int)*n);
    tb->skip = (int*)malloc(sizeof(int)*n);
    tb->idfs = (int*)malloc(sizeof(int)*n);
    tb->Wp = (float*)malloc(sizeof(float)*n);
    tb->headIdx = (int*)malloc(sizeof(int)*n);
}

static void freeTreeBuffers(TreeBuffers* tb) {
    free(tb->outTrees);
    TreeStack_destroy(&tb->ts);
    free(tb->dfs);
    free(tb->skip);
    free(tb->idfs);
    free(tb->Wp);
    free(tb->headIdx);
}

// Builds the out-tree of pivot p over C^(<t) in DFS order and returns its size
static int pivotTree(const int p, const int t, const SCCResult Scc, float** W, int** last, TreeBuffers* tb) {
    constructOutTree(tb->outTrees, p, t, Scc, W, last);
    int c = buildDFS(tb->outTrees, p, &tb->ts, tb->dfs, tb->idfs, tb->skip);
    for (int v=0; v<c; v++)
        tb->Wp[v] = W[p][tb->dfs[v]];
    return c;
}

// W[s][d] = min(W[s][d], W[s][p] + W[p][d]) for d in the out-tree of p (built by pivotTree),
// skipping subtrees whose root did not improve
static void relaxSource(const int s, const int p, const int c, float** W, int** last, const TreeBuffers* tb) {
    float Wsp = W[s][p];
    if (s == p || !isfinite_cust(Wsp))
        return;

    const float* Wp = tb->Wp;
    const int* dfs = tb->dfs;
    const int* skip = tb->skip;
    float* Ws = W[s];
    for (int v = 0; v < c;)
    {
        int d = dfs[v];             // destinations
        if (Wsp + Wp[v] < Ws[d])
        {
            Ws[d] = Wsp + Wp[v];
            last[s][d] = last[p][d];
            v++;
        }
        else v = skip[v]; // we can skip the subtree since we didn't improve our distance
    }
}

// Reduced relaxation from all sources in C_t via pivot p to all destinations reachable from p
static void relaxViaPivot(const int p, const int t, const SCCResult Scc, float** W, int** last, TreeBuffers* tb) {
    int c = pivotTree(p, t, Scc, W, last, tb);
    if (c == 0)
        return;
    for (int s = Scc.firstInComponent[t]; s != -1; s = Scc.nextInComponent[s]) // sources
        relaxSource(s, p, c, W, last, tb);
}

// Paper order: bridge step (sources -> heads over Out(C_t)), then reduced relaxation from sources via heads
static void bridgeThenHeads(const int t, const SCCResult Scc, const Bridges B, float** W, int** last, TreeBuffers* tb) {
    // Bridge step: W[s][h] = min(W[s][h], W[s][k] + w(k,h)) -------------------------------------------------------
    int k = B.firstTail[t];
    while (k != -1) // for all tails
    {
        int s = Scc.firstInComponent[t];
        while (s != -1) // for all sources
        {
            float Wsk = W[s][k];
            for (int i = 0; i < B.tailOut[k]; ++i) // for all heads of this tail
            {
                int h = B.tailHead[k][i];
                if (Wsk + W[k][h] < W[s][h]) {
                    W[s][h] = Wsk + W[k][h];
                    last[s][h] = last[k][h];
                }
            }
            s = Scc.nextInComponent[s];
        }
        k = B.nextTail[k];
    }

    // Reduced-relaxation step: from all sources via heads to all reachable destinations ----------------------------
    for (int i = 0; i < B.headCount[t]; ++i)
        relaxViaPivot(B.heads[t][i], t, Scc, W, last, tb);
}

#ifndef TREE_MIN_TAILS
#define TREE_MIN_TAILS 4 // heads with at most this many tails are relaxed without building their out-tree
#endif

// Swapped order: tails get distances to C^(<t) through their outgoing edges, then reduced relaxation via tails
static void tailsThenBridge(const int t, const SCCResult Scc, const Bridges B, float** W, int** last, TreeBuffers* tb) {
    // Step 1: W[k][d] = min(W[k][d], w(k,h) + W[h][d]) ------------------------------------------------------------
    // Edges are grouped by head, so the out-tree of every head is built once and walked for each of its tails
    const int H = B.headCount[t];
    int* start = (int*)calloc(H + 1, sizeof(int)); // tails of head i are inTails[start[i] .. start[i+1])
    for (int i = 0; i < H; ++i)
        tb->headIdx[B.heads[t][i]] = i;
    for (int k = B.firstTail[t]; k != -1; k = B.nextTail[k])
        for (int i = 0; i < B.tailOut[k]; ++i)
            start[tb->headIdx[B.tailHead[k][i]] + 1]++;
    for (int i = 0; i < H; ++i)
        start[i + 1] += start[i];

    int* pos = (int*)malloc(sizeof(int)*(H + 1));
    memcpy(pos, start, sizeof(int)*(H + 1));
    int* inTails = (int*)malloc(sizeof(int)*(start[H] + 1));
    for (int k = B.firstTail[t]; k != -1; k = B.nextTail[k])
        for (int i = 0; i < B.tailOut[k]; ++i)
            inTails[pos[tb->headIdx[B.tailHead[k][i]]]++] = k;

    for (int i = 0; i < H; ++i)
    {
        int h = B.heads[t][i];
        if (start[i + 1] - start[i] > TREE_MIN_TAILS) // the out-tree pays off only when many tails share this head
        {
            int c = pivotTree(h, t, Scc, W, last, tb);
            for (int j = start[i]; j < start[i + 1]; ++j)
                relaxSource(inTails[j], h, c, W, last, tb);
            continue;
        }

        const float* Wh = W[h];
        for (int j = start[i]; j < start[i + 1]; ++j)
        {
            int k = inTails[j];
            float* Wk = W[k];
            float Wkh = Wk[h];
            for (int c = 0; c <= Scc.component[h]; ++c) // only components below h can be reachable from h
            {
                int d = Scc.firstInComponent[c];
                if (!isfinite_cust(Wh[d]))
                    continue;
                for (; d != -1; d = Scc.nextInComponent[d])
                {
                    if (Wkh + Wh[d] < Wk[d])
                    {
                        Wk[d] = Wkh + Wh[d];
                        last[k][d] = last[h][d];
                    }
                }
            }
        }
    }
    free(inTails);
    free(pos);
    free(start);

    // Step 2: from all sources via tails to all reachable destinations ----------------------------------------------
    for (int k = B.firstTail[t]; k != -1; k = B.nextTail[k])
        relaxViaPivot(k, t, Scc, W, last, tb);
}

// Number of vertices in the components marked in bits
static long long bitsSize(const uint64_t* bits, const int words, const long long* compSize) {
    long long size = 0;
    for (int w = 0; w < words; ++w)
    {
        uint64_t x = bits[w];
        while (x)
        {
            size += compSize[w*64 + __builtin_ctzll(x)];
            x &= x - 1;
        }
    }
    return size;
}

// For every component C_t (from last to first) picks the cheaper order. Both orders only touch destinations that
// are reachable from the pivot, so the cost uses R(v) = number of vertices in C^(<t) reachable from v:
//   bridge -> heads: |C_t|*|Out(C_t)| + sum_{h in H_t} (t + (|C_t|+1)*R(h))
//   tails -> bridge: sum_{h in H_t} step1(h) + sum_{k in I_t} (t + |C_t|*R(k))
// where t + R(p) is the cost of building (or scanning without a tree) the reach of pivot p, and step1(h) follows
// tailsThenBridge: deg(h)*(t + R(h)) without the out-tree, (t + R(h)) + deg(h)*R(h) with it.
// R(k) is the union over the heads of k, computed from component-level reachability bitsets.
static float** AdaptiveBridgeTree(const SCCResult Scc, const Bridges B, float** W, const int n, int** last) {
    const int C = Scc.sccCount;
    const int words = (C + 63) / 64;

    long long* compSize = (long long*)calloc(C, sizeof(long long));
    for (int v = 0; v < n; ++v)
        compSize[Scc.component[v]]++;

    uint64_t* reachBits = (uint64_t*)calloc((size_t)C*words, sizeof(uint64_t)); // components reachable from component c
    long long* reachSize = (long long*)malloc(sizeof(long long)*C);              // vertices reachable from component c
    uint64_t* tailBits = (uint64_t*)malloc(sizeof(uint64_t)*words);
    int* inDeg = (int*)calloc(n, sizeof(int));                                     // number of tails of a head in C_t

    TreeBuffers tb;
    createTreeBuffers(&tb, n);

    for (int t = 0; t < C; ++t)
    {
        uint64_t* bitsT = reachBits + (size_t)t*words;
        long long size = compSize[t];
        long long out = 0, headsCost = 0, tailsCost = 0;

        for (int k = B.firstTail[t]; k != -1; k = B.nextTail[k])
        {
            memset(tailBits, 0, sizeof(uint64_t)*words);
            for (int i = 0; i < B.tailOut[k]; ++i)
            {
                int h = B.tailHead[k][i];
                inDeg[h]++;
                const uint64_t* bitsH = reachBits + (size_t)Scc.component[h]*words;
                for (int w = 0; w < words; ++w)
                    tailBits[w] |= bitsH[w];
            }
            tailsCost += t + size*bitsSize(tailBits, words, compSize);
            out += B.tailOut[k];
        }

        for (int i = 0; i < B.headCount[t]; ++i)
        {
            int h = B.heads[t][i];
            int hc = Scc.component[h];
            long long R = reachSize[hc];
            headsCost += t + (size + 1)*R;
            if (inDeg[h] > TREE_MIN_TAILS)
                tailsCost += t + R + inDeg[h]*R;
            else
                tailsCost += inDeg[h]*(t + R);
            inDeg[h] = 0;

            const uint64_t* bitsH = reachBits + (size_t)hc*words;
            for (int w = 0; w < words; ++w)
                bitsT[w] |= bitsH[w];
        }
        headsCost += size*out;

        if (out > 0)
        {
            if (headsCost <= tailsCost)
                bridgeThenHeads(t, Scc, B, W, last, &tb);
            else
                tailsThenBridge(t, Scc, B, W, last, &tb);
        }

        bitsT[t / 64] |= (uint64_t)1 << (t % 64);
        reachSize[t] = bitsSize(bitsT, words, compSize);
    }

    freeTreeBuffers(&tb);
    free(inDeg);
    free(tailBits);
    free(reachSize);
    free(reachBits);
    free(compSize);
    return W;
    }

// algorithm
float** SCCvertex(float** graph, int n) {

    float** W = copy2DArray(graph, n);
    int** last = (int**)malloc(sizeof(int*)*n); // Last node on path (without destination)
    for (int i=0; i<n; i++)
    {
        last[i] = (int*)malloc(sizeof(int)*n);
        for (int j=0; j<n; j++)
        {
            last[i][j] = i;
        }
    }

    InitAdjList(n);
    SCCResult SCCs = findSCCs_addEdges(W, n);

    clock_t start = clock();
    Bridges B = tEdgesToVertexRelaxSets(SCCs.component, SCCs.sccCount, n);
    clock_t end = clock();
    double elapsed =
    static_cast<double>(end - start) / CLOCKS_PER_SEC;
    //printf("B time: %.6f seconds\n", elapsed);

    start = clock();
    separatelyTree(W, n , SCCs, last); // solve APSP in each component with Tree
    end = clock();
    elapsed =
    static_cast<double>(end - start) / CLOCKS_PER_SEC;
    //printf("Sepa time: %.6f seconds\n", elapsed);

    start = clock();
    BridgeTree(SCCs, B, W, n, last); // APSP between the SCCs
    end = clock();
    elapsed =
    static_cast<double>(end - start) / CLOCKS_PER_SEC;
    //printf("Inter SCC time: %.6f seconds\n", elapsed);

    freeBridges(B, n , SCCs.sccCount);
    FreeAdjList(); freeSCCs(SCCs);
    free2DInt(last, n);

    return W;
    }

// SCCvertex, but each component picks the cheaper order of the two steps (bridge -> heads or tails -> bridge)
float** SCCvertexAdaptive(float** graph, int n) {

    float** W = copy2DArray(graph, n);
    int** last = (int**)malloc(sizeof(int*)*n); // Last node on path (without destination)
    for (int i=0; i<n; i++)
    {
        last[i] = (int*)malloc(sizeof(int)*n);
        for (int j=0; j<n; j++)
        {
            last[i][j] = i;
        }
    }

    InitAdjList(n);
    SCCResult SCCs = findSCCs_addEdges(W, n);
    Bridges B = tEdgesToVertexRelaxSets(SCCs.component, SCCs.sccCount, n);

    separatelyTree(W, n , SCCs, last); // solve APSP in each component with Tree
    AdaptiveBridgeTree(SCCs, B, W, n, last); // APSP between the SCCs

    freeBridges(B, n , SCCs.sccCount);
    FreeAdjList(); freeSCCs(SCCs);
    free2DInt(last, n);

    return W;
    }
