#include "GraphTools.h"
#include "../utils.h"

/*
 * Generates a directed weighted graph divided into `comps` strongly connected
 * components of equal size. Each component forms a directed cycle, and every
 * vertex is connected to all vertices in later components. This preserves the
 * separate SCCs while creating many outgoing edges between them.
 *
 * Requires: n > 0, comps > 0, and n % comps == 0.
 */

float** WorstCaseGraph(int n, int comps, double outgoingProb, double internalProb)
{
    float** graph = allocate_graph(n);
    int k = n/comps;    // število vozlišč v vsakem ciklu

    /*
     * Create one directed cycle inside each SCC.
     * This guarantees that every component is strongly connected.
     */
    for (int i = 0; i < comps; ++i) {
        int first = i * k;
        int end = (i + 1) * k;

        for (int a = first; a < end - 1; ++a) {
            graph[a][a + 1] = frand();
        }

        graph[end - 1][first] = frand();
    }

    /*
     * Add additional random edges inside each SCC.
     */
    for (int i = 0; i < comps; ++i) {
        int first = i * k;
        int end = (i + 1) * k;

        for (int a = first; a < end; ++a) {
            for (int b = first; b < end; ++b) {
                if (a == b) {
                    continue;  // no self-loops
                }
                if (graph[a][b] == INFINITY &&
                    drand() < internalProb) {
                    graph[a][b] = frand();
                    }
            }
        }
    }

    /*
     * Add edges from each SCC to later SCCs with the given probability.
     *
     * Edges only go forward, so separate SCCs cannot merge into one SCC.
     */
    for (int i = 0; i < comps; ++i) {
        int first = i * k;
        int end = (i + 1) * k;

        for (int a = first; a < end; ++a) {
            for (int b = end; b < n; ++b) {
                if (drand() < outgoingProb) {
                    graph[a][b] = frand();
                }
            }
        }
    }

    //permuteGraph(graph, n);
    return graph;
}

// Export csv
void csv_WorstCase(int run, double** results, int algCount, GraphData data, float outProb, float inProb){
    FILE *csvFile = fopen("../results/worstCase_data.csv", "a");
    for (int i = 0; i < algCount; ++i) {
        fprintf(csvFile, "%0.3lf, ", results[run][i]);
    }
    fprintf(csvFile, "%.3f, %.1f, ", outProb, inProb);
    writeGraphDataToCSV(csvFile, &data);
    fclose(csvFile);
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

    int sizes[] = {2048};
    int m[] = {2, 4, 16, 128};
    double outProb[] = {0.001, 0.01, 0.1, 0.5, 1.0};
    double inProb[] = {0.1, 0.5, 1.0};

    // .csv header -----------------------------------------------------------------------------------------------------
	FILE *csvFile = fopen("../results/worstCase_data.csv", "w");
    for (int i = 0; i < algCount; ++i) {
        fprintf(csvFile, "%s,", algNames[i]);
    }
    fprintf(csvFile, "outProb, inProb, ");
    writeGraphDataHeader(csvFile);
	fclose(csvFile);
    // -----------------------------------------------------------------------------------------------------------------

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        for (int s = 0; s < sizeof(m) / sizeof(m[0]); s++)
        {
            printf("k =  %d \n", m[s]);
            for (int out = 0; out < sizeof(outProb) / sizeof(outProb[0]); out++)
            {
                printf("outProb =  %.3f \n", outProb[out]);
                for (int in = 0; in < sizeof(inProb) / sizeof(inProb[0]); in++)
                {
                    printf("inProb = %0.1f \n===========\n", inProb[in]);
                    double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
                    for (int run=0; run<TESTRUNS; run++)
                    {
                        // Generate graph.
                        float** graph = WorstCaseGraph(n, m[s], outProb[out], inProb[in]);
                        // Get graph data
                        GraphData data = getGraphData(graph, n);
                        int mStar = 0;
                        results[run] = RunTest(graph, n, algs, &mStar);

                        csv_WorstCase(run, results, algCount, data, outProb[out], inProb[in]);

                        freeGraphData(&data);
                        freeGraph(graph, n);
                    }
                    double** avgResults = ComputeAvg(results, algCount, TESTRUNS);

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
    }
}