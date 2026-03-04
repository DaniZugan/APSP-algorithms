
#ifndef RUN_ALGORITHM_H
#define RUN_ALGORITHM_H

#define TESTRUNS 5 // Number of runs (on different random graph instances), returns averaged result.

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef struct {
    char *name;
    double time;
} Entry;

typedef float**(*AlgorithmFP)(float **, int);

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

#ifdef __cplusplus
}
#endif

#endif //RUN_ALGORITHM_H
