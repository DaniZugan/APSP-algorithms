#include "graphGenerators.h"
#include "utils.h"

#include <stdlib.h>
#include <tgmath.h>
#include <time.h>
#include <string.h>


// random graph from Toroslu paper, similar as Erdos-Renyi where: |E| = |N|^e
float** TurkGraph(int n, double e) {     // n = number of vertices, n^e = number of edges

    // similar as SparseGraph but without cycle for connectivity
    int edges = (int) round(pow(n,e));
    //printf("Edges: %d\n", edges);

    float** graph = (float**)malloc(sizeof(float*)*n);

    for (int i = 0; i < n; ++i) {
        graph[i] = (float*)malloc(sizeof(float)*n);
        for (int j = 0; j < n; ++j) {
            if (i == j) graph[i][j] = 0;
            else graph[i][j] = INFINITY;
        }
    }

    int* list = (int*)malloc(sizeof(int)*n*(n-1));

    int listSize = 0;
    for (int i=0; i<n; i++)
    {
        for (int j=0; j<n; j++)
        {
            if (i != j)
            {
                list[listSize++] = i*n + j;
            }
        }
    }

    // Permute edge list
    shuffle(list, n*(n-1));
    int e_count = 0;
    for (int newEdge = 0; newEdge < edges; newEdge++)
    {
        int j=list[newEdge]%n;
        int i=(list[newEdge] - j)/n;
        graph[i][j] = frand();
        e_count++;
    }

    free(list);
    //printf("Edges: %d\n", e_count);
    return graph;

}

// graph for unit testing for SCCs, N has to be 14
float** SCCGraph() {
    int n = 14; // Number of vertices
    float **graph = (float **)malloc(n * sizeof(float *));
    for (int i = 0; i < n; i++) {
        graph[i] = (float *)malloc(n * sizeof(float));
        for (int j = 0; j < n; j++) {
            if (i == j) graph[i][j] = 0;
            else graph[i][j] = INFINITY;
        }
    }
    // Example graph

    graph[0][2] = 1;
    graph[1][0] = 2;
    graph[2][1] = 3;

    graph[3][8] = 5;
    graph[8][4] = 4;
    graph[4][5] = 2;
    graph[5][6] = 1;
    graph[6][9] = 3;
    graph[5][6] = 8;
    graph[9][7] = 4;
    graph[7][3] = 1;
    graph[4][9] = 1;
    graph[4][3] = 9;
    graph[6][5] = 7;

    graph[12][13] = 5;
    graph[13][12] = 1;

    // outgoing edges
    graph[1][11] = 7;
    graph[5][10] = 2;
    graph[8][10] = 3;
    graph[10][11] = 2;
    graph[10][12] = 9;
    graph[11][13] = 2;


    return graph;
}

// 2. graph for unit testing for SCCs, N has to be 14
float** SCCGraph2() {
    int n = 14; // Number of vertices
    float **graph = (float **)malloc(n * sizeof(float *));
    for (int i = 0; i < n; i++) {
        graph[i] = (float *)malloc(n * sizeof(float));
        for (int j = 0; j < n; j++) {
            if (i == j) graph[i][j] = 0;
            else graph[i][j] = INFINITY;
        }
    }
    // Applying the permutation mapping with random weights
    graph[5][8] = frand();  // 0 -> 1  (Now 5 -> 8)
    graph[8][12] = frand(); // 1 -> 2  (Now 8 -> 12)
    graph[12][5] = frand(); // 2 -> 0  (Now 12 -> 5)
    graph[12][10] = frand(); // 2 -> 4  (Now 12 -> 10)
    graph[8][7] = frand();  // 1 -> 3  (Now 8 -> 7)
    graph[7][10] = frand(); // 3 -> 4  (Now 7 -> 10)
    graph[10][6] = frand(); // 4 -> 5  (Now 10 -> 6)
    graph[6][11] = frand(); // 5 -> 6  (Now 6 -> 11)
    //graph[5][11] = frand(); // 0 -> 6 (Removed edge remains removed)
    graph[6][7] = frand();  // 5 -> 3  (Now 6 -> 7)
    graph[11][3] = frand(); // 6 -> 7  (Now 11 -> 3)
    graph[3][11] = frand(); // 7 -> 6  (Now 3 -> 11)
    graph[3][9] = frand();  // 7 -> 8  (Now 3 -> 9)
    graph[2][11] = frand(); // 9 -> 6  (Now 2 -> 11)
    graph[1][4] = frand();  // 10 -> 11 (Now 1 -> 4)
    graph[4][2] = frand();  // 11 -> 9  (Now 4 -> 2)
    graph[13][0] = frand(); // 12 -> 13 (Now 13 -> 0)
    graph[2][1] = frand();  // 9 -> 10  (Now 2 -> 1)

    return graph;
}

// help functions for LogGraph -----------------------------------------------------------------------------------------
void AddRandomEdges(float** graph, int n, int num_edges_to_add) {
    int added_edges = 0;

    while (added_edges < num_edges_to_add) {
        int u = rand() % n;
        int v = rand() % n;

        // Preveri ali ta povezava že obstaja
        if (u != v && graph[u][v] == INFINITY) {
            graph[u][v] = frand();
            added_edges++;
        }
    }
}
// puts values of matrix inside graph from vertices start to end.
void putGraph(int start, int end, float** graph, float** matrix) {
    for (int i = start; i < end; ++i) {
        for (int j = start; j < end; ++j) {
            graph[i][j] = matrix[i-start][j-start];
        }
    }
}
void distribute_vertices(int N, int **lists, int num_lists, int *list_sizes) {

    for (int i = 0; i < num_lists; ++i) {
        list_sizes[i] = 0;
    }

    for (int i = 0; i < N; ++i) {
        int list_index = rand() % num_lists;
        lists[list_index][list_sizes[list_index]] = i;
        list_sizes[list_index]++;
    }
}
// creates a directed cycle and ads M random edges
float** CycleX(int n, int M){
    float** cycle = (float**)malloc(sizeof(float*)*n);
    for (int i = 0; i < n; ++i) {
        cycle[i] = (float*)malloc(sizeof(float)*n);
        for (int j = 0; j < n; ++j) {
            if (i == j) cycle[i][j] = 0;
            else cycle[i][j] = INFINITY;
        }
    }
    for (int i = 0; i < n; i++) {
        // Connect each vertex to the next, the last connects to the first
        int nextVertex = (i + 1) % n;
        cycle[i][nextVertex] = frand();
    }
    //printf("%d", n);
    AddRandomEdges(cycle, n, M);
    permuteGraph(cycle, n);

    return cycle;
}
// Master thesis LogGraph
float** GeneratelgNGraph(int n, int edges) {
    if (n > edges) {
        printf("Not enough edges for lgN graph");
        return NULL;
    }
    int components = (int)(log2(n));
    int m = edges - n; // število povezav, ki je treba dodati v cikle


    int **lists = malloc(components * sizeof(int *));
    int *list_sizes = malloc(n * sizeof(int));
    for (int i = 0; i < components; ++i) {
        lists[i] = malloc(n * sizeof(int)); // Allocate with max possible size
    }

    distribute_vertices(n, lists, components, list_sizes);

    /*for (int i = 0; i < components; ++i) {
        printf("List %d: ", i);
        for (int j = 0; j < list_sizes[i]; ++j) {
            printf("%d ", lists[i][j]);
        }
        printf("\n");
    }*/

    float** graph = (float**)malloc(sizeof(float*)*n);
    for (int i = 0; i < n; ++i) {
        graph[i] = (float*)malloc(sizeof(float)*n);
        for (int j = 0; j < n; ++j) {
            if (i == j) graph[i][j] = 0;
            else graph[i][j] = INFINITY;
        }
    }

    int start = 0;
    int arcs; // number of edges added in each component

    for (int i = 0; i < components; i++) {

        int componentSize = list_sizes[i];
        if (i != components-1) {
            arcs = m/components;
        } else arcs = m - (components-1)* (m/components);

        float** comp = CycleX(componentSize, arcs);

        putGraph(start, start + componentSize, graph, comp);
        start += componentSize;

        for (int c = 0; c < componentSize; ++c) {
            free(comp[c]);
        }
        free(comp);
    }


    // Free the allocated memory
    for (int i = 0; i < components; ++i) {
        free(lists[i]);
    }
    free(lists);
    free(list_sizes);

    permuteGraph(graph, n);
    return graph;
}

// ---------------------------------------------------------------------------------------------------------------------
// Prufer code graph (needed for CompGraph)
float** PruferCodeGraph(int vertices) {
    int m = vertices - 2;
    int* prufer = (int*)malloc(m * sizeof(int));
    //printf("Prufer sequence: ");
    for (int i = 0; i < m; ++i) {
        prufer[i] = 1 + rand()%vertices;
        //printf("%d, ", prufer[i]);
    }
    //printf("\n");

    float** graph = (float**)malloc(vertices * sizeof(float*));
    for (int i = 0; i < vertices; ++i) {
        graph[i] = (float*)malloc(vertices * sizeof(float));
        for (int j = 0; j < vertices; ++j) {
            if (i == j) graph[i][j] = 0;
            else graph[i][j] = INFINITY;
        }
    }
    int* vertex_set = (int*)malloc(vertices * sizeof(int));
    for (int i = 0; i < vertices; i++)
        vertex_set[i] = 0;

    for (int i = 0; i < m; i++)
        vertex_set[prufer[i] - 1] += 1;

    // Find the smallest label not present in prufer[].
    int j = 0;
    for (int i = 0; i < vertices - 2; i++) {
        for (j = 0; j < vertices; j++) {
            // If j+1 is not present in prufer set
            if (vertex_set[j] == 0) {
                // Remove from Prufer set.
                vertex_set[j] = -1;
                // printf("(%d, %d) ", (j + 1), prufer[i]);
                if(frand()>0.5) graph[prufer[i]-1][j] = frand();
                else graph[j][prufer[i]-1] = frand();
                vertex_set[prufer[i] - 1]--;

                break;
            }
        }
    }

    j = 0;
    // For the last element
    int y = 0;
    int x = 0;
    for (int i = 0; i < vertices; i++) {
        if (vertex_set[i] == 0 && j == 0) {
            // printf("(%d, ", (i + 1));
            y = i;
            j++;
        }
        else if (vertex_set[i] == 0 && j == 1) {
            // printf("%d)\n", (i + 1));
            x = i;
        }
    }

    if(frand()>0.5) graph[y][x] = frand();
    else graph[x][y] = frand();

    // printf("x: %d\n", x);
    free(prufer);
    free(vertex_set);
    return graph;
}
// Master thesis CompGraph
float** N1Graph(int vertices, float ratio, float alpha) { // creates a graph B (Sparse graph) and graph A (Tree) and adds edges to A to that |E| = |V| - 1
    float** graph = (float**)malloc(vertices * sizeof(float*));
    for (int i = 0; i < vertices; ++i) {
        graph[i] = (float*)malloc(vertices * sizeof(float));
        for (int j = 0; j < vertices; ++j) {
            if (i == j) graph[i][j] = 0;
            else graph[i][j] = INFINITY;
        }
    }
    int verticesA = (int) vertices*ratio; // število vozlišč v grafu A
    int verticesB = vertices - verticesA;  // število vozlišč v grafu B

    float** subgraphA = PruferCodeGraph(verticesA);
    float** subgraphB = TurkGraph(verticesB, alpha);

    int edgesB = powf(verticesB, alpha); // število povezav v grafu B
    int edgesToAdd = verticesB - edgesB; // število povezav, ki jih je treba dodati v graf A

    AddRandomEdges(subgraphA, verticesA, edgesToAdd);

    for (int i = 0; i < verticesA; ++i) {
        for (int j = 0; j < verticesA; ++j) {
            graph[i][j] = subgraphA[i][j];
        }
    }

    for (int i = verticesA; i < vertices; ++i) {
        for (int j = verticesA; j <vertices; ++j) {
            graph[i][j] = subgraphB[i-verticesA][j-verticesA];
        }
    }

    free(subgraphB);
    free(subgraphA);

    permuteGraph(graph, vertices);
    return graph;

}

// ---------------------------------------------------------------------------------------------------------------------
// Graph of log2(N) SCCs
int randomOutEdgesNaturalCut4() {
    float t = frand();
    //printf("t = %f\n", t);
    if (t < 16.0f/31.0f) return 0; // 16/31 (51.6%) - 0 edges
    if (t < 24.0f/31.0f) return 1; // 8/31 (25.8%) - 1 edge
    if (t < 28.0f/31.0f) return 2; // 4/31 (12.9%) - 2 edges
    if (t < 30.0f/31.0f) return 3; // 2/31 (6.5%) - 3 edges
    return 4;                      // 1/31 (3.2%) - 4 edges
}
// edges can be very high (experimental)
int randOutEdgesNaturalCut(int edges) {
    float p = frand();
    printf("%f\n", p);
    for (int i = edges; i >= 0; --i) {
        //printf("%f ", 1/pow(2, i));
        if (p < 1/pow(2, i)) {
            //printf("i: %d\n", i);
            return i;
        }
    }
    //printf("\n");
}

//
float alphaRandEdges() {
    float t = frand();
    if (t < 16.0f/31.0f) return 0;      // 16/31 (51.6%)
    if (t < 24.0f/31.0f) return 0.3;    // 8/31 (25.8%)
    if (t < 28.0f/31.0f) return 0.5;    // 4/31 (12.9%)
    if (t < 30.0f/31.0f) return 0.7;    // 2/31 (6.5%)
    return 0.9;                         // 1/31 (3.2%)
}

bool* selectFirstAlphaEdges(int len, float alpha) {
    int select = (int) round(len*alpha);
    int* indices = (int*)malloc(len * sizeof(int));
    bool* arr = (bool*)calloc(len, sizeof(bool));
    for (int i = 0; i < len; ++i) {
        indices[i] = i;
        arr[i] = false;
    }
    shuffle(indices, len);
    for (int i = 0; i < select; ++i) {
        arr[indices[i]] = true;
    }
    return arr;
}

// 0-4 edges between each pair of component
float** sccGraph(int N, float density) {
    float** graph = createGraphAndFill(N);
    int K = (int)(log2(N));
    srand(time(NULL)); // Seed the random number generator
    int* bounds = (int*)malloc((K+1) * sizeof(int));
    bounds[0] = 0;
    bounds[K] = N;
    int bound;
    // create random bounds from 1 to N
    for (int i = 1; i < K; i++) {
        do {
            bound = randInRange(1, N);
        } while (contains(bounds, K+1, bound));
        bounds[i] = bound;
    }
    qsort(bounds, K, sizeof(int), compareInts);
    // create cycles
    for (int i = 0; i < K; i++) {
        for (int j = bounds[i]; j < bounds[i+1]-1; ++j) {
            graph[j][j+1] = frand();
        }
        if (bounds[i+1]-1 != bounds[i]) { // if component contains only one vertex prevent self loop
            graph[bounds[i+1]-1][bounds[i]] = frand(); // connect last vertex in the component to the first to create a cycle
        }
    }
    // add density
    for (int i = 0; i < K; ++i) {
        int size = bounds[i+1] - bounds[i];
        if (size < 3) continue;
        int edgesToAdd = pow(size, density);
        int remaining = pow(size, 2) - 2*size; // possible edges to add to the SCC
        if (remaining <= edgesToAdd) { // more edges to add than possible so we just return complete graph
            for (int j = bounds[i]; j < bounds[i+1]; ++j) {
                for (int k = bounds[i]; k < bounds[i+1]; ++k) {
                    if (j != k) {
                        graph[j][k] = frand();
                    }
                }
            }
        } else {
            float** subgraph = createGraph(size);
            for (int j = 0; j < size; ++j) {
                for (int k = 0; k < size; ++k) {
                    subgraph[j][k] = graph[j+bounds[i]][k+bounds[i]];
                }
            }
            AddRandomEdges(subgraph, size, edgesToAdd);
            for (int j = 0; j < size; ++j) {
                for (int k = 0; k < size; ++k) {
                    graph[j+bounds[i]][k+bounds[i]] = subgraph[j][k];
                }
            }
            freeGraph(subgraph, size);
        }
    }
    // add edges between SCC
    for (int i = 0; i < K; ++i) {
        for (int j = i+1; j < K; ++j) {
            int ee = randomOutEdgesNaturalCut4();
            for (int k = 0; k < ee; ++k) {
                int u = randInRange(bounds[i], bounds[i+1]-1);
                int v = randInRange(bounds[j], bounds[j+1]-1);
                //printf("i:%d, j:%d, (%d, %d)\n", i, j, u, v);
                graph[u][v] = frand();
            }
        }
    }

    permuteGraph(graph, N);
    free(bounds);
    return graph;
}

// Alfa edges between each pair of component (based on N)
// I create an array with N elements and create K bounds. Then I create cycle in each component and add edges to get desired density.
// At the end I add edges between each pair of SCCs
float** sccGraphAlfa(int N, float density) {
    float** graph = createGraphAndFill(N);
    int K = 10;
    srand(time(NULL)); // Seed the random number generator
    int* bounds = (int*)malloc((K+1) * sizeof(int));
    bounds[0] = 0;
    bounds[K] = N;
    int bound;
    // create random bounds from 1 to N
    for (int i = 1; i < K; i++) {
        do {
            bound = randInRange(1, N);
        } while (contains(bounds, K+1, bound));
        bounds[i] = bound;
    }
    qsort(bounds, K, sizeof(int), compareInts);
    // create cycles
    for (int i = 0; i < K; i++) {
        for (int j = bounds[i]; j < bounds[i+1]-1; ++j) {
            graph[j][j+1] = frand();
        }
        if (bounds[i+1]-1 != bounds[i]) { // if component contains only one vertex prevent self loop
            graph[bounds[i+1]-1][bounds[i]] = frand(); // connect last vertex in the component to the first to create a cycle
        }
    }
    // add density
    for (int i = 0; i < K; ++i) {
        int size = bounds[i+1] - bounds[i];
        if (size < 3) continue;
        int edgesToAdd = pow(size, density);
        int remaining = pow(size, 2) - 2*size; // possible edges to add to the SCC
        if (remaining <= edgesToAdd) { // more edges to add than possible so we just return complete graph
            for (int j = bounds[i]; j < bounds[i+1]; ++j) {
                for (int k = bounds[i]; k < bounds[i+1]; ++k) {
                    if (j != k) {
                        graph[j][k] = frand();
                    }
                }
            }
        } else {
            float** subgraph = createGraph(size);
            for (int j = 0; j < size; ++j) {
                for (int k = 0; k < size; ++k) {
                    subgraph[j][k] = graph[j+bounds[i]][k+bounds[i]];
                }
            }
            AddRandomEdges(subgraph, size, edgesToAdd);
            for (int j = 0; j < size; ++j) {
                for (int k = 0; k < size; ++k) {
                    graph[j+bounds[i]][k+bounds[i]] = subgraph[j][k];
                }
            }
            freeGraph(subgraph, size);
        }
    }

    // add edges between SCC
    for (int i = 0; i < K; ++i) {
        for (int j = i+1; j < K; ++j) {
            int size1 = bounds[i+1] - bounds[i];    // size of component i
            int size2 = bounds[j+1] - bounds[j];    // size of component j
            int size = size1*size2;                 // number of possible edges between components
            float alpha = 0.5f * ((float)size1 + (float)size2) / (float)size;
            //printf("alpha = %.4f, size1 = %d, size2 = %d, size = %d\n", alpha, size1, size2, size);
            //printf("OUTedges = %.1f\n", size*alpha);
            bool* select = selectFirstAlphaEdges(size, alpha);
            int ind = 0;
            for (int k = bounds[i]; k < bounds[i+1]; ++k) {
                for (int l = bounds[j]; l < bounds[j+1]; ++l) {
                    if (select[ind]) graph[k][l] = frand();
                    ind++;
                }
            }
            free(select);
        }
    }
    //permuteGraph(graph, N);
    free(bounds);
    return graph;
}

// ---------------------------------------------------------------------------------------------------------------------
float** GenerateCompleteGraph(int n, int unit)
{
    float** graph = (float**)malloc(sizeof(float*)*n);
    for (int i=0; i<n; i++)
    {
        graph[i] =  (float*)malloc(sizeof(float)*n);
        for (int j=0; j<n; j++)
        {
            if (i != j)
            {
                if (unit)
                    graph[i][j] = 1;
                else
                    graph[i][j] = frand();
            }
            else
                graph[i][j] = 0;
        }
    }

    return graph;
}

// From txt file with data in #
Graf GenerateGraphGnutella(char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
    }
    char line[256];
    int N = -1;

    while (fgets(line, sizeof(line), file)) {
        if (line[0] != '#')
            break;

        char* found = strstr(line, "Nodes: ");
        if (found) {
            sscanf(found + 7, "%d", &N); // Skip "Nodes: "
        }
    }
    if (N <= 0) {
        fprintf(stderr, "Error: Could not determine number of nodes\n");
    }

    //printf("Number of nodes = %d\n", N);

    float** graph = createGraph(N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i != j) graph[i][j] = INFINITY;
            else graph[i][j] = 0;
        }
    }

    int src, dest;
    do {
        if (line[0] == '#' || strlen(line) < 2) continue; // skip empty or comment
        if (sscanf(line, "%d %d", &src, &dest) == 2) {
            graph[src][dest] = frand();
        }
    } while (fgets(line, sizeof(line), file));

    fclose(file);
    Graf graf = {graph, N};
    return graf;
}

// From simple txt file
Graf GenerateGraphTxt(char* filename) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
    }
    int max = 0;
    int num;
    while (fscanf(file, "%d", &num) == 1) {
        if (num > max) {
            max = num;
        }
    }
    if (max <= 0) {
        fprintf(stderr, "Error: Could not determine number of nodes\n");
    }
    int N = max; // vertex zero is also a vertex and
    float** graph = createGraph(N);
    for (int i = 0; i < N; ++i) {
        for (int j = 0; j < N; ++j) {
            if (i != j) graph[i][j] = INFINITY;
            else graph[i][j] = 0;
        }
    }
    int u, v;
    while (fscanf(file, "%d %d", &u, &v) == 2) {
        if (u >= 0 && v >= 0 && u < N && v < N) {
            graph[u][v] = frand();  // mark the edge u → v
        }
    }

    fclose(file);
    Graf graf = {graph, N};
    return graf;
}