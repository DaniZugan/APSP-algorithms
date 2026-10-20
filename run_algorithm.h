
#ifndef RUN_ALGORITHM_H
#define RUN_ALGORITHM_H

#define TESTRUNS 1 // Number of runs (on different random graph instances), returns averaged result.

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <stdio.h>

typedef struct {
    char *name;
    double time;
} Entry;

typedef struct {
    int vertices;
    int edges;
    double density;

    int outgoingEdges;
    int tailCount;
    int headCount;

    int wccCount;
    int* wccHistogram;
    int largestWCCVertices;
    int largestWCCEdges;

    int sccCount;
    int* sccHistogram;
    int largestSCCVertices;
    int largestSCCEdges;
} GraphData;

typedef float**(*AlgorithmFP)(float **, int);

// Time limit in seconds for one algorithm on one instance. A run that exceeds it is killed and its time is
// reported as MAX_INSTANCE_TIME.
#define MAX_INSTANCE_TIME 60.0

typedef enum {
    RUN_OK = 0,      // finished, output correct (or not checked)
    RUN_WRONG = 1,   // finished, check found errors
    RUN_TIMEOUT = 2, // killed after the time limit, time = limit
    RUN_CRASH = 3    // child process died (segfault, out of memory, ...), time = -1
} RunStatus;

// Checks an output matrix and returns the number of errors. Runs in the child process, so it may only read ctx.
typedef int (*CheckFP)(float** mat, int n, void* ctx);

// Shared n x n matrix that a child process can write its result into (row-major, unmap with FreeSharedMatrix)
float* CreateSharedMatrix(int n);
void FreeSharedMatrix(float* matrix, int n);

RunStatus RunAlgorithmTimed(float** graph, int n, AlgorithmFP alg, double limit, double* time,
                            CheckFP check, void* ctx, float* out, int* errors);
const char* RunStatusName(RunStatus status);

int countEdges(float** graph, int n);
float** RunAlgorithmReturnMatrix(float** graph, int n, double* time, AlgorithmFP alg);
int RunAlgorithm(float** graph, int n, double* time, AlgorithmFP alg, int checkOutput, float** reference);
double* RunTest(float** graph, int n, AlgorithmFP* algs, int* mStar);
double** ComputeAvg(double** results, int size, int totalRuns);
int* graphHistogram(float**graph, int n, bool print);
int* findLargestSCC(float** graph, int V, int* largestSCCSize);
float** extractMaxComp(float** graph, int* histogram, int n, int* max);
void printResults(double** results, int algCount, char** algNames);
void RunTurkTest(AlgorithmFP* algs, char** algNames);
void RunTurk2Test(AlgorithmFP* algs, char** algNames);
void RunlgNTest(AlgorithmFP* algs, char** algNames);
void RunN_1Test(AlgorithmFP* algs, char** algNames);
void RunSparseTest(AlgorithmFP* algs, char** algNames);
void RunSCCsTest(AlgorithmFP* algs, char** algNames);
void RunCompleteTest(AlgorithmFP* algs, char** algNames, int unit);
void RunGnutellaTest(AlgorithmFP* algs, char** algNames);
void RunTxtTest(AlgorithmFP* algs, char** algNames);

GraphData getGraphData(float** graph, int n);
void writeGraphDataToCSV(FILE *csvFile, const GraphData *data);
void writeGraphDataHeader(FILE *csvFile);
void freeGraphData(GraphData* data);

#ifdef __cplusplus
}
#endif

#endif //RUN_ALGORITHM_H
