//
// Created by Dani Zugan on 28. 5. 25.
//

#include "GraphTools.h"

// Allocate an n x n matrix initialized with INFINITY, diagonal set to 0.0
float** allocate_graph(int n) {
    float** graph = malloc(n * sizeof(float*));
    for (int i = 0; i < n; ++i) {
        graph[i] = malloc(n * sizeof(float));
        for (int j = 0; j < n; ++j) {
            graph[i][j] = (i == j) ? 0.0f : INFINITY;
        }
    }
    return graph;
}

// Free the allocated graph
void free_graph(float** graph, int n) {
    for (int i = 0; i < n; ++i)
        free(graph[i]);
    free(graph);
}

// Add edge with random direction
void add_random_directed_edge(float** graph, int u, int v) {
    if (rand() % 2 == 0)
        graph[u][v] = frand();
    else
        graph[v][u] = frand();
}

// Generate Barabási–Albert graph with n nodes and m edges per new node
float** generate_BA_graph(int n, int m) {
    if (m < 1 || m >= n) return NULL;

    float** graph = allocate_graph(n);

    int max_edges = n * m * 2;
    int* target_list = malloc(max_edges * sizeof(int));
    int target_size = 0;

    // Fully connect the first m nodes
    for (int i = 0; i < m; ++i) {
        for (int j = i + 1; j < m; ++j) {
            add_random_directed_edge(graph, i, j);
            target_list[target_size++] = i;
            target_list[target_size++] = j;
        }
    }

    // Add remaining nodes
    for (int node = m; node < n; ++node) {
        int edges_added = 0;
        int* connected = calloc(n, sizeof(int));  // prevent duplicate targets

        while (edges_added < m) {
            int rand_index = rand() % target_size;
            int target = target_list[rand_index];

            if (target == node || connected[target]) continue;

            add_random_directed_edge(graph, node, target);
            target_list[target_size++] = node;
            target_list[target_size++] = target;
            connected[target] = 1;
            edges_added++;
        }
        free(connected);
    }
    free(target_list);
    return graph;
}

// Export csv
void csv_BA(int run, double** results, int algCount, char** algNames, int m, int V, int E, int sccV, int sccE){

    for (int i = 0; i < algCount; ++i) {
        FILE *csvFile = fopen("../results/BA_data.csv", "a");
        fprintf(csvFile, "%s, %0.3lf, %d, %d, %d, %d, %d\n", algNames[i], results[run][i], V, E, m, sccV, sccE);
        fclose(csvFile);
    }
}

// Run test
void Run_BA_Test(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }
	printf("Algorithm count: %d\n", algCount);
    printf("Running BA test...\n");
    int sizes[] = {2048, 4096};
    int m[] = {2, 3, 4, 5};

	FILE *csvFile = fopen("../results/BA_data.csv", "w");
	fprintf(csvFile, "algorithm, time, V, E, m, sccV, sccE\n");
	fclose(csvFile);

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        for (int s = 0; s < sizeof(m) / sizeof(m[0]); s++)
        {
            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {

                // Generate graph.
                float** graph = generate_BA_graph(n, m[s]);
            	int graphEdges = countEdges(graph, n);

                int mStar = 0;
                results[run] = RunTest(graph, n, algs, &mStar);

            	int largestSCCSize = 0;
            	int* largestSCC = findLargestSCC(graph, n, &largestSCCSize);
            	float** SCCgraph = extractSubgraph(graph, largestSCC, largestSCCSize);
            	int SCCedges = countEdges(SCCgraph, largestSCCSize);

            	csv_BA(run, results, algCount, algNames, m[s], n, graphEdges, largestSCCSize, SCCedges);

            	freeGraph(graph, n);
            	freeGraph(SCCgraph, largestSCCSize);
            	free(largestSCC);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            printf("m =  %d \n===========\n", m[s]);
            printResults(avgResults, algCount, algNames);

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
        }
    }
}

