#include "GraphTools.h"
#include "../utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define INF 1e30f

// -------- Tarjan on adjacency matrix --------
static float **G;         // adjacency (weights; INF = no edge)
static int N;

static int *dfn, *low, *onstack, *stack, top, time_idx;
static int *comp, comp_cnt;

static void tarjan_dfs(int u) {
    dfn[u] = low[u] = ++time_idx;
    stack[top++] = u;
    onstack[u] = 1;

    for (int v = 0; v < N; ++v) {
        if (G[u][v] >= INF/2 || u == v) continue; // no edge or self-loop handled by dfn/low init
        if (!dfn[v]) {
            tarjan_dfs(v);
            if (low[v] < low[u]) low[u] = low[v];
        } else if (onstack[v]) {
            if (dfn[v] < low[u]) low[u] = dfn[v];
        }
    }

    if (low[u] == dfn[u]) {
        // pop an SCC
        for (;;) {
            int w = stack[--top];
            onstack[w] = 0;
            comp[w] = comp_cnt;
            if (w == u) break;
        }
        comp_cnt++;
    }
}

static int compute_scc(float **g, int n, int **out_comp) {
    G = g; N = n;
    dfn = (int*)calloc(N, sizeof(int));
    low = (int*)calloc(N, sizeof(int));
    onstack = (int*)calloc(N, sizeof(int));
    stack = (int*)malloc(N * sizeof(int));
    comp  = (int*)malloc(N * sizeof(int));
    top = time_idx = comp_cnt = 0;

    for (int u = 0; u < N; ++u)
        if (!dfn[u]) tarjan_dfs(u);

    free(dfn); free(low); free(onstack); free(stack);
    *out_comp = comp;            // caller owns comp
    return comp_cnt;
}

// -------- Find boundary vertices across SCCs --------
void print_scc_boundaries(float **g, int n) {
    int *C = NULL;
    int K = compute_scc(g, n, &C);   // K = #SCCs, C[u] = component id in [0..K-1]

    // Marks per vertex
    char *is_exit = (char*)calloc(n, 1);
    char *is_entry= (char*)calloc(n, 1);

    // Mark endpoints of inter-SCC edges
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            if (g[u][v] < INF/2 && C[u] != C[v]) {
                is_exit[u] = 1;   // u has an outgoing to another SCC
                is_entry[v] = 1;  // v has an incoming from another SCC
            }
        }
    }

    // Print, grouped by SCC
    for (int s = 0; s < K; ++s) {
        printf("SCC %d:\n", s);
        printf("  exits (to other SCCs): ");
        int first = 1;
        for (int u = 0; u < n; ++u) if (C[u] == s && is_exit[u]) {
            if (!first) printf(", ");
            printf("%d", u);
            first = 0;
        }
        if (first) printf("(none)");
        printf("\n");

        printf("  entries (from other SCCs): ");
        first = 1;
        for (int u = 0; u < n; ++u) if (C[u] == s && is_entry[u]) {
            if (!first) printf(", ");
            printf("%d", u);
            first = 0;
        }
        if (first) printf("(none)");
        printf("\n");
    }

    free(is_exit); free(is_entry); free(C);
}

void FW2scc(float **g, int n) {
    int *C = NULL;
    int K = compute_scc(g, n, &C);   // K = #SCCs, C[u] = component id in [0..K-1]

   int* inters = malloc(n * sizeof(int));
   int size = 0;
    bool add_v = true;
    bool add_u = true;

    // Mark endpoints of inter-SCC edges
    for (int u = 0; u < n; ++u) {
        for (int v = 0; v < n; ++v) {
            if (g[u][v] < 1 && C[u] != C[v]) {
                //printf("u: %d, v: %d\n", u,v);
                for (int i = 0; i < size; ++i) {
                    if (inters[i] == v) add_v = false;
                }
                if (add_v) inters[size++] = v;
                for (int i = 0; i < size; ++i) {
                    if (inters[i] == u) add_u = false;
                }
                if (add_u) inters[size++] = u;
            }
            add_u = true;
            add_v = true;
        }
    }
    printf("Size: %d\n", size);
    //for (int u = 0; u < size; ++u) {
        //printf(" %d", inters[u]);
    //} printf("\n");
    // FW in both comps
    for (int k = 0; k < n/2; ++k) {
        for (int i = 0; i < n/2; ++i) {
            for (int j = 0; j < n/2; ++j) {
                if (g[i][j] > g[i][k] + g[k][j]) {
                    g[i][j] = g[i][k] + g[k][j];
                }
            }
        }
    }
    for (int k = n/2; k < n; ++k) {
        for (int i = n/2; i < n; ++i) {
            for (int j = n/2; j < n; ++j) {
                if (g[i][j] > g[i][k] + g[k][j]) {
                    g[i][j] = g[i][k] + g[k][j];
                }
            }
        }
    }
    // FW between comps
    for (int a = 0; a < size; ++a) {
        int k = inters[a];
        for (int i = 0; i < n/2; ++i) {
            for (int j = n/2; j < n; ++j) {
                if (g[i][j] > g[i][k] + g[k][j]) {
                    g[i][j] = g[i][k] + g[k][j];
                }
            }
        }
    }
    free(C);
}

// graf z dvema SCC. nA = št. vozlišč v prvi komponenti, nB št. vozlišč v drugi komponenti in mAB št. povezav med njima
float **two_scc_graph(int nA, int nB, int mAB) {
    int n = nA + nB;
    float **g = allocate_graph(n);

    // Make A strongly connected: a single directed cycle
    for (int i = 0; i < nA; ++i) {
        g[i][(i + 1) % nA] = frand();
    }

    // Make B strongly connected: a single directed cycle
    for (int j = 0; j < nB; ++j) {
        int u = nA + j;
        int v = nA + ((j + 1) % nB);
        g[u][v] = frand();
    }

    // A -> B "bridge" edges (few, forward only)
    for (int e = 0; e < mAB; ++e) {
        int x = rand() % nA;         // node in A
        int y = nA + (rand() % nB);  // node in B
        g[x][y] = frand();
    }

    return g;
}


