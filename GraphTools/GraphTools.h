
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
    void Run_RandomSizeSCC_Test(AlgorithmFP* algs, char** algNames);
    float** WorstCaseGraph(int n, int comps, double outgoingProb, double internalProb);
    float **two_scc_graph(int nA, int nB, int mAB);
    void print_scc_boundaries(float **g, int n);
    void FW2scc(float **g, int n);

    struct SCCdata {
        int k;          // Število komponent
        int* sizes;     // Velikosti komponent
        int** outE;     // Matrika izhodnih povezav
    };

#ifdef __cplusplus
}
#endif


#endif //GRAPHTOOLS_H
