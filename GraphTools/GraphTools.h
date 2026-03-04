
#ifndef GRAPHTOOLS_H
#define GRAPHTOOLS_H

#include "../utils.h"
#include "../run_algorithm.h"

#ifdef __cplusplus
extern "C" {
#endif
    float** allocate_graph(int n);
    void Run_BA_Test(AlgorithmFP* algs, char** algNames);
    void Run_WorstCase_Test(AlgorithmFP* algs, char** algNames);
    float** WorstCaseGraph(int n, int comps);
    float **two_scc_graph(int nA, int nB, int mAB);
    void print_scc_boundaries(float **g, int n);
    void FW2scc(float **g, int n);

#ifdef __cplusplus
}
#endif


#endif //GRAPHTOOLS_H
