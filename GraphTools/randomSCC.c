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

// Posebne vrednosti v comps[], ki so odvisne od n.
#define COMPS_SQRT_N (-1)   // floor(sqrt(n))
#define COMPS_LOG_N  (-2)   // floor(log2(n))

static int resolveComps(int comps, int n)
{
    int k = comps;
    if (comps == COMPS_SQRT_N) {
        k = 1;
        while ((long long)(k + 1) * (k + 1) <= n) k++;
    } else if (comps == COMPS_LOG_N) {
        k = 0;
        while ((1LL << (k + 1)) <= n) k++;
    }
    if (k < 1) k = 1;
    if (k > n) k = n;
    return k;
}

// Posebna vrednost v componentProb[], ki je odvisna od števila komponent k.
#define COMP_PROB_2_K (-1.0)  // 2/k: pričakovano k - 1 povezanih parov komponent, kot drevo

static double resolveCompProb(double componentProb, int k)
{
    if (componentProb == COMP_PROB_2_K)
        return k > 2 ? 2.0 / k : 1.0;
    return componentProb;
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
// Appends component sizes and edge counts between components to <prefix>sizes.csv and <prefix>outs.csv in
// results/Viz/SCC (drawn by run2.py). prefix is "" for RandomSizeSCC and "star_" for StarSCC.
static void csv_export(struct SCCdata D, const char* prefix) {
    char sizesPath[128], outsPath[128];
    snprintf(sizesPath, sizeof(sizesPath), "../results/Viz/SCC/%ssizes.csv", prefix);
    snprintf(outsPath, sizeof(outsPath), "../results/Viz/SCC/%souts.csv", prefix);

    FILE* csvSizes = fopen(sizesPath, "a");
    for (int i = 0; i < D.k-1; ++i) {
        fprintf(csvSizes, "%d, ", D.sizes[i]);
    }
    fprintf(csvSizes, "%d\n", D.sizes[D.k-1]);
    fclose(csvSizes);

    FILE* csvOut = fopen(outsPath, "a");
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

/*
 * With equal != 0, the boundaries are i*n/comps, so all components have n/comps vertices (sizes differ by at
 * most one when comps does not divide n).
 *
 * With star != 0, outgoingProb is ignored and a selected pair i < j gets, with equal probability, either
 * edges from the tail of component i to every vertex of component j (one tail, all heads), or
 * edges from every vertex of component i to one random vertex of component j (all tails, one head).
 * The tail of a component is chosen once, so it is the same for all its pairs.
 */
static float** SCCGraph(int n, int comps, double outgoingProb, double internalProb, double coin,
                        struct SCCdata* SCC, int star, int equal)
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

    if (equal) {
        /* Enako velike komponente. */
        for (int i = 1; i < comps; ++i) {
            cuts[i] = (int)((long long)i * n / comps);
        }
    } else {
        for (int i = 0; i < comps - 1; ++i) {
            cuts[i + 1] = candidates[i];
        }

        qsort(cuts + 1, (size_t)(comps - 1),
              sizeof(int), compareIntegers);
    }

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

    /* Za vsako komponento enkrat izberemo tail, ki ga uporabimo pri vseh njenih parih. */
    int* tail = malloc((size_t)comps * sizeof(int));
    for (int i = 0; i < comps; ++i) {
        tail[i] = cuts[i] + rand() % (cuts[i + 1] - cuts[i]);
    }

    /* Za vsak par komponent vržemo kovanec. */
    for (int i = 0; i < comps; ++i) {
        for (int j = i + 1; j < comps; ++j) {
            if (drand() >= coin) {
                continue;
            }

            if (star) {
                if (rand() % 2) {
                    /* En tail v komponenti i, vsi headi v komponenti j. */
                    int a = tail[i];
                    for (int b = cuts[j]; b < cuts[j + 1]; ++b) {
                        graph[a][b] = frand();
                        SCC->outE[i][j]++;
                    }
                } else {
                    /* Vsi taili v komponenti i, en head v komponenti j. */
                    int b = cuts[j] + rand() % (cuts[j + 1] - cuts[j]);
                    for (int a = cuts[i]; a < cuts[i + 1]; ++a) {
                        graph[a][b] = frand();
                        SCC->outE[i][j]++;
                    }
                }
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

    free(tail);
    free(cuts);

    /* Premešamo vozlišča, da komponente niso strnjeni bloki v matriki. */
    permuteGraph(graph, n);
    return graph;
}

float** RandomSizeSCCGraph(int n, int comps, double outgoingProb, double internalProb, double coin, struct SCCdata* SCC)
{
    return SCCGraph(n, comps, outgoingProb, internalProb, coin, SCC, 0, 0);
}

// Like RandomSizeSCCGraph, but every selected pair of components is joined by one tail -> all heads or by
// all tails -> one head (chosen at random for each pair).
float** StarSCCGraph(int n, int comps, double internalProb, double coin, struct SCCdata* SCC)
{
    return SCCGraph(n, comps, 0.0, internalProb, coin, SCC, 1, 0);
}

// Writes one generated graph as an edge list (from,to,weight) to path, for results/Viz/SCC/draw_graph.py.
// star = 0: RandomSizeSCCGraph with outgoingProb, star = 1: StarSCCGraph (outgoingProb is ignored).
void Export_SCCGraph(int n, int comps, double outgoingProb, double internalProb, double coin, int star,
                     const char* path)
{
    struct SCCdata D = create_scc_data(comps);
    float** graph = SCCGraph(n, comps, outgoingProb, internalProb, coin, &D, star, 0);
    FILE* f = fopen(path, "w");
    if (graph == NULL || f == NULL) {
        if (f) fclose(f);
        if (graph) freeGraph(graph, n);
        free_scc_data(D);
        return;
    }
    fprintf(f, "from,to,weight\n");
    for (int i = 0; i < n; ++i)
        for (int j = 0; j < n; ++j)
            if (i != j && isfinite_cust(graph[i][j]))
                fprintf(f, "%d,%d,%.4f\n", i, j, graph[i][j]);
    fclose(f);
    freeGraph(graph, n);
    free_scc_data(D);
}

// Export results of one run to CSV.
static void csv_SCCRow(const char* path, int run, double** results, int algCount,
                       GraphData data, double outProb, double inProb,
                       double componentProb, int comps, int equal)
{
    FILE* csvFile = fopen(path, "a");

    if (csvFile == NULL) {
        perror(path);
        return;
    }

    for (int i = 0; i < algCount; ++i) {
        fprintf(csvFile, "%.3lf, ", results[run][i]);
    }

    fprintf(csvFile, "%.3f, %.3f, %.3f, %d, %d, ",
            outProb, inProb, componentProb, comps, equal);

    writeGraphDataToCSV(csvFile, &data);
    fclose(csvFile);
}

// Run tests on graphs with randomly sized SCCs. With star, pairs of components are joined as in StarSCCGraph,
// with equal all SCCs have the same size.
static void runSCCTest(AlgorithmFP* algs, char** algNames, int star, int equal)
{
    const char* path = star ? "../results/starSCC_data.csv"
                     : equal ? "../results/equalSizeSCC_data.csv" : "../results/randomSizeSCC_data.csv";
    int algCount = 0;
    while (algs[algCount] != NULL) {
        algCount++;
    }

    printf("Algorithm count: %d\n", algCount);
    printf(star ? "Running Star SCC test...\n"
                : equal ? "Running Equal Size SCC test...\n" : "Running Random Size SCC test...\n");

    int sizes[] = {4096, 8192};
    int comps[] = {2, 4, COMPS_LOG_N, COMPS_SQRT_N};

    double outProb[] = {0.001, 0.01, 0.5, 1.0};
    size_t outCount = star ? 1 : sizeof(outProb) / sizeof(outProb[0]); // star graphs do not use outProb
    double inProb[] = {1.0};
    double componentProb[] = {COMP_PROB_2_K, 1.0};

    // Create the CSV header, overwriting previous results.
    FILE* csvFile = fopen(path, "w");

    // Empty the files for the visualisation (run2.py), overwriting previous graphs.
    const char* vizPrefix = star ? "star_" : equal ? "equal_" : "";
    {
        char vizPath[128];
        snprintf(vizPath, sizeof(vizPath), "../results/Viz/SCC/%ssizes.csv", vizPrefix);
        FILE* csvSizes = fopen(vizPath, "w");
        if (csvSizes) fclose(csvSizes);
        snprintf(vizPath, sizeof(vizPath), "../results/Viz/SCC/%souts.csv", vizPrefix);
        FILE* csvOut = fopen(vizPath, "w");
        if (csvOut) fclose(csvOut);
    }


    if (csvFile == NULL) {
        perror(path);
        return;
    }

    for (int i = 0; i < algCount; ++i) {
        fprintf(csvFile, "%s,", algNames[i]);
    }

    fprintf(csvFile, "outProb, inProb, componentProb, comps, equalSizes, ");
    writeGraphDataHeader(csvFile);
    fclose(csvFile);

    for (size_t z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];

        for (size_t s = 0; s < sizeof(comps) / sizeof(comps[0]); ++s) {
            int k = resolveComps(comps[s], n);

            for (size_t out = 0; out < outCount; ++out) {

                for (size_t in = 0;
                     in < sizeof(inProb) / sizeof(inProb[0]); ++in) {

                    for (size_t cp = 0;
                         cp < sizeof(componentProb) /
                              sizeof(componentProb[0]); ++cp) {
                        double compProb = resolveCompProb(componentProb[cp], k);

                        printf("N = %d, comps = %d, %s sizes\n", n, k, equal ? "equal" : "random");
                        if (star)
                            printf("inProb = %.3f, componentProb = %.3f\n===========\n",
                                   inProb[in], compProb);
                        else
                            printf("outProb = %.3f, inProb = %.3f, "
                                   "componentProb = %.3f\n===========\n",
                                   outProb[out], inProb[in],
                                   compProb);

                        double** results =
                            malloc(sizeof(double*) * TESTRUNS);

                        if (results == NULL) {
                            fprintf(stderr,
                                    "Cannot allocate results array.\n");
                            return;
                        }

                        for (int run = 0; run < TESTRUNS; ++run) {
                            // Generate a new graph for each run.
                            struct SCCdata SCCviz = create_scc_data(k);
                            float** graph = SCCGraph(n, k, star ? 0.0 : outProb[out], inProb[in], compProb,
                                                     &SCCviz, star, equal);

                            csv_export(SCCviz, vizPrefix);

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

                            // outProb is written as -1 for star graphs, so the CSV has the same columns
                            csv_SCCRow(
                                path, run, results, algCount, data,
                                star ? -1.0 : outProb[out], inProb[in],
                                compProb, k, equal
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

// Like Run_RandomSizeSCC_Test, but all SCCs have the same size. Results go to results/equalSizeSCC_data.csv.
void Run_EqualSizeSCC_Test(AlgorithmFP* algs, char** algNames)
{
    runSCCTest(algs, algNames, 0, 1);
}

void Run_RandomSizeSCC_Test(AlgorithmFP* algs, char** algNames)
{
    runSCCTest(algs, algNames, 0, 0);
}

void Run_StarSCC_Test(AlgorithmFP* algs, char** algNames)
{
    runSCCTest(algs, algNames, 1, 0);
}
