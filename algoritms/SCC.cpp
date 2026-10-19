#include "SCC.h"
#include "../graphGenerators.h"
#include <cstring>
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

static float** AdaptiveBridgeTree(const SCCResult Scc, const Bridges B, float** W, const int n, int** last) {
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


