#include "GraphTools.h"
#include "../utils.h"
// comps ciklov z maksimalno izhodnih povezav
float** WorstCaseGraph(int n, int comps) {
    float** graph = allocate_graph(n);
    int k = n/comps;    // število vozlišč v vsakem ciklu
    // cikli
    for (int i = 0; i < comps; ++i) {
        for (int a = i*k; a < (i+1)*k - 1; ++a) {
            graph[a][a+1] = frand();
        }
        graph[(i+1)*k - 1][i*k] = frand();
    }
    // izhodne povezave
    for (int i = 0; i < comps; ++i) {
        for (int a = i*k; a < (i+1)*k; ++a) {
            for (int b = (i+1)*k; b < n; ++b) {
                graph[a][b] = frand();
            }
        }
    }
    //permuteGraph(graph, n);
    return graph;
}

// Export csv
void csv_WorstCase(int run, double** results, int algCount, char** algNames, int m, int V, int E, int sccV, int sccE){

    for (int i = 0; i < algCount; ++i) {
        FILE *csvFile = fopen("../results/worstCase_data.csv", "a");
        fprintf(csvFile, "%s, %0.3lf, %d, %d, %d, %d, %d\n", algNames[i], results[run][i], V, E, m, sccV, sccE);
        fclose(csvFile);
    }
}

// Run test
void Run_WorstCase_Test(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }
	printf("Algorithm count: %d\n", algCount);
    printf("Running Worst Case test...\n");
    int sizes[] = {512, 1024};
    int m[] = {2};

	FILE *csvFile = fopen("../results/worstCase_data.csv", "w");
	fprintf(csvFile, "algorithm, time, n, m, k, sccV, sccE\n");
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
                float** graph = WorstCaseGraph(n, m[s]);
            	int graphEdges = countEdges(graph, n);

                int mStar = 0;
                results[run] = RunTest(graph, n, algs, &mStar);

            	int largestSCCSize = 0;
            	int* largestSCC = findLargestSCC(graph, n, &largestSCCSize);
            	float** SCCgraph = extractSubgraph(graph, largestSCC, largestSCCSize);
            	int SCCedges = countEdges(SCCgraph, largestSCCSize);

            	csv_WorstCase(run, results, algCount, algNames, m[s], n, graphEdges, largestSCCSize, SCCedges);

            	freeGraph(graph, n);
            	freeGraph(SCCgraph, largestSCCSize);
            	free(largestSCC);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            printf("k =  %d \n===========\n", m[s]);
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


/*
    for (int i = 0; i < comps; ++i) {
        for (int a = i*k; a < (i+1)*k; ++a) {
            for (int b = i*k; b < n; ++b) {
                printf("%d, %d \n", a, b);
                if (a == b) continue;
                graph[a][b] = frand();
                //printf("graph[%d][%d] = %f\n", a, b, graph[a][b]);
            }
        }
    }
    */