#include "GraphTools.h"
#include "../utils.h"

#include <stdlib.h>

// Pomožne funkcije ----------------------------------------------------------------------------------------------------
static int compareIntegers(const void* a, const void* b)
{
    int x = *(const int*)a;
    int y = *(const int*)b;
    return (x > y) - (x < y);
}

// Funkcija za alokacijo pomnilnika znotraj strukture
static struct SCCdata create_scc_data(int comps) {
    struct SCCdata D;

    // Število komponent
    D.k = comps;

    // Velikost vsake komponente
    D.sizes = calloc((size_t)comps, sizeof(int));

    // Število izhodnih povezav za vsak par komponent
    D.outE = malloc((size_t)comps * sizeof(int*));
    for (int i = 0; i < comps; ++i) {
        D.outE[i] = calloc((size_t)comps, sizeof(int));
    }

    return D;
}

// Funkcija za sprostitev pomnilnika strukture
static void free_scc_data(struct SCCdata D) {
    free(D.sizes);
    for (int i = 0; i < D.k; ++i) {
        free(D.outE[i]);
    }
    free(D.outE);
}

// csv izvoz
static void csv_export(struct SCCdata D) {
    FILE* csvSizes = fopen("../results/Viz/SCC/sizes.csv", "a");
    for (int i = 0; i < D.k-1; ++i) {
        fprintf(csvSizes, "%d, ", D.sizes[i]);
    }
    fprintf(csvSizes, "%d\n", D.sizes[D.k-1]);
    fclose(csvSizes);

    FILE* csvOut = fopen("../results/Viz/SCC/outs.csv", "a");
    for (int i = 0; i < D.k; ++i) {
        for (int j = 0; j < D.k-1; ++j) {
            fprintf(csvOut, "%d, ", D.outE[i][j]);
        }
        fprintf(csvOut, "%d", D.outE[i][D.k-1]);
        fprintf(csvOut, "\n");
    }
    fprintf(csvOut, "\n");
    fclose(csvOut);
}

// ---------------------------------------------------------------------------------------------------------------------

/*
 * Creates a weighted directed graph with n vertices and exactly comps
 * strongly connected components (SCCs).
 *
 * Randomly selects comps - 1 distinct boundaries between vertices
 * and sorts them. The resulting intervals define components of random
 * sizes. Each component contains at least one vertex; sizes may repeat.
 *
 * Within each component containing more than one vertex, creates a
 * directed cycle to ensure strong connectivity. Each additional missing
 * edge between distinct vertices in the same component is added with
 * probability internalProb.
 *
 * For each pair of components i < j, selects the pair with probability
 * componentProb. If selected, each possible edge from component i to
 * component j is added with probability outgoingProb. A selected pair
 * may therefore still have no direct edge between its components.
 *
 * Edges between components only point towards later components,
 * preventing separate SCCs from merging. Edge weights are generated
 * by frand().
 *
 * Parameters:
 *   n             - total number of vertices, n >= 1
 *   comps         - number of SCCs, 1 <= comps <= n
 *   outgoingProb  - probability of each edge between a selected SCC pair
 *   internalProb  - probability of each additional edge within an SCC
 *   componentProb - probability of selecting an SCC pair for edge addition
 *
 * All probabilities must be in [0, 1].
 * Returns the graph matrix, or NULL for invalid parameters
 * or a detected memory allocation failure.
 */

float** RandomSizeSCCGraph(int n, int comps, double outgoingProb, double internalProb, double coin, struct SCCdata* SCC)
{
    if (n < 1 || comps < 1 || comps > n ||
        !(outgoingProb >= 0.0 && outgoingProb <= 1.0) ||
        !(internalProb >= 0.0 && internalProb <= 1.0)) {
        return NULL;
    }

    int* candidates = malloc((size_t)n * sizeof(int));
    int* cuts = malloc(((size_t)comps + 1) * sizeof(int));

    if (candidates == NULL || cuts == NULL) {
        free(candidates);
        free(cuts);
        return NULL;
    }

    /* Možne meje med vozlišči. */
    for (int i = 0; i < n - 1; ++i) {
        candidates[i] = i + 1;
    }

    /* Naključno izberemo comps - 1 različnih mej. */
    for (int i = 0; i < comps - 1; ++i) {
        int j = i + rand() % (n - 1 - i);

        int tmp = candidates[i];
        candidates[i] = candidates[j];
        candidates[j] = tmp;
    }

    cuts[0] = 0;
    cuts[comps] = n;

    for (int i = 0; i < comps - 1; ++i) {
        cuts[i + 1] = candidates[i];
    }

    qsort(cuts + 1, (size_t)(comps - 1),
          sizeof(int), compareIntegers);

    free(candidates);

    float** graph = allocate_graph(n);

    if (graph == NULL) {
        free(cuts);
        return NULL;
    }

    /* Povezave znotraj vsake komponente. */
    for (int i = 0; i < comps; ++i) {
        int first = cuts[i];
        int end = cuts[i + 1];
        SCC->sizes[i] = end - first;
        /* Cikel zagotovi močno povezanost. */
        if (end - first > 1) {
            for (int a = first; a < end - 1; ++a) {
                graph[a][a + 1] = frand();
            }

            graph[end - 1][first] = frand();
        }

        /* Dodatne naključne notranje povezave. */
        for (int a = first; a < end; ++a) {
            for (int b = first; b < end; ++b) {
                if (a != b &&
                    graph[a][b] == INFINITY &&
                    drand() < internalProb) {
                    graph[a][b] = frand();
                }
            }
        }
    }

    /* Za vsak par komponent vržemo kovanec. */
    for (int i = 0; i < comps; ++i) {
        for (int j = i + 1; j < comps; ++j) {
            if (drand() >= coin) {
                continue;
            }

            /* Povezave samo iz komponente i v komponento j. */
            for (int a = cuts[i]; a < cuts[i + 1]; ++a) {
                for (int b = cuts[j]; b < cuts[j + 1]; ++b) {
                    if (drand() < outgoingProb) {
                        graph[a][b] = frand();
                        SCC->outE[i][j]++;
                    }
                }
            }
        }
    }

    free(cuts);
    return graph;
}

// Export results of one run to CSV.
void csv_RandomSizeSCC(int run, double** results, int algCount,
                       GraphData data, double outProb, double inProb,
                       double componentProb)
{
    FILE* csvFile = fopen("../results/randomSizeSCC_data.csv", "a");

    if (csvFile == NULL) {
        perror("Cannot open randomSizeSCC_data.csv");
        return;
    }

    for (int i = 0; i < algCount; ++i) {
        fprintf(csvFile, "%.3lf, ", results[run][i]);
    }

    fprintf(csvFile, "%.3f, %.3f, %.3f, ",
            outProb, inProb, componentProb);

    writeGraphDataToCSV(csvFile, &data);
    fclose(csvFile);
}

// Run tests on graphs with randomly sized SCCs.
void Run_RandomSizeSCC_Test(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    while (algs[algCount] != NULL) {
        algCount++;
    }

    printf("Algorithm count: %d\n", algCount);
    printf("Running Random Size SCC test...\n");

    int sizes[] = {2048};
    int comps[] = {4, 16, 128};

    double outProb[] = {0.001, 0.001, 0.01, 0.01, 0.1, 0.1, 0.5, 0.5, 1.0, 1.0};
    double inProb[] = {0.1, 0.5, 1.0};
    double componentProb[] = {0.1, 0.5, 0.6, 1.0};

    // Create the CSV header, overwriting previous results.
    FILE* csvFile = fopen("../results/randomSizeSCC_data.csv", "w");

    // Create the CSV header, overwriting previous visualisations.
    FILE* csvSizes = fopen("../results/Viz/SCC/sizes.csv", "w");
    FILE* csvOut = fopen("../results/Viz/SCC/outs.csv", "w");
    fclose(csvSizes);
    fclose(csvOut);


    if (csvFile == NULL) {
        perror("Cannot create randomSizeSCC_data.csv");
        return;
    }

    for (int i = 0; i < algCount; ++i) {
        fprintf(csvFile, "%s,", algNames[i]);
    }

    fprintf(csvFile, "outProb, inProb, componentProb, ");
    writeGraphDataHeader(csvFile);
    fclose(csvFile);

    for (size_t z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];

        for (size_t s = 0; s < sizeof(comps) / sizeof(comps[0]); ++s) {
            for (size_t out = 0;
                 out < sizeof(outProb) / sizeof(outProb[0]); ++out) {

                for (size_t in = 0;
                     in < sizeof(inProb) / sizeof(inProb[0]); ++in) {

                    for (size_t cp = 0;
                         cp < sizeof(componentProb) /
                              sizeof(componentProb[0]); ++cp) {

                        printf("N = %d, comps = %d\n", n, comps[s]);
                        printf("outProb = %.3f, inProb = %.3f, "
                               "componentProb = %.3f\n===========\n",
                               outProb[out], inProb[in],
                               componentProb[cp]);

                        double** results =
                            malloc(sizeof(double*) * TESTRUNS);

                        if (results == NULL) {
                            fprintf(stderr,
                                    "Cannot allocate results array.\n");
                            return;
                        }

                        for (int run = 0; run < TESTRUNS; ++run) {
                            // Generate a new graph for each run.
                            struct SCCdata SCCviz = create_scc_data(comps[s]);
                            float** graph = RandomSizeSCCGraph(
                                n, comps[s], outProb[out], inProb[in],
                                componentProb[cp], &SCCviz
                            );

                            csv_export(SCCviz);

                            free_scc_data(SCCviz);

                            if (graph == NULL) {
                                fprintf(stderr,
                                        "Graph generation failed.\n");

                                for (int f = 0; f < run; ++f) {
                                    free(results[f]);
                                }
                                free(results);
                                return;
                            }

                            GraphData data = getGraphData(graph, n);


                            int mStar = 0;
                            results[run] =
                                RunTest(graph, n, algs, &mStar);

                            csv_RandomSizeSCC(
                                run, results, algCount, data,
                                outProb[out], inProb[in],
                                componentProb[cp]
                            );

                            freeGraphData(&data);
                            freeGraph(graph, n);
                        }

                        // Compute and print averages for this combination.
                        double** avgResults =
                            ComputeAvg(results, algCount, TESTRUNS);

                        printResults(avgResults, algCount, algNames);

                        for (int f = 0; f < TESTRUNS; ++f) {
                            free(results[f]);
                        }
                        free(results);

                        for (int f = 0; f < algCount; ++f) {
                            free(avgResults[f]);
                        }
                        free(avgResults);
                    }
                }
            }
        }
    }
}
