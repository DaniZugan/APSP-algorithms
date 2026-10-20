#include <iostream>
#include <boost/algorithm/string.hpp>  // Boost string utilities

#include "Dijkstra.h"
#include "algoritms/HiddenPaths.h"
#include "algoritms/UniformPaths.h"
#include "algoritms/Tree.h"
#include "algoritms/SCC.h"
#include "algoritms/Toroslu.h"
#include "algoritms/DijkstraAPSP.h"
#include "graphGenerators.h"
#include "utils.h"
#include "run_algorithm.h"
#include "GraphTools/GraphTools.h"

using namespace std;

int main() {
    srand(time(NULL));
    // UniformPathsAlgorithm, HiddenPathsAlgorithm, floydWarshall, DijkstraAPSPc, FastHourglassHalf, Toro, SCCalgo, SCCvertex, SCCvertexAdaptive
    // "Uniform", "Hidden", "FW", "Dijkstra", "Tree", "Toroslu", "SCCedge", "SCCvertex", "SCCadaptive"
    // RelaxCounter rc;
    AlgorithmFP algs[] = {FastHourglassHalf, SCCalgo, SCCvertex, SCCvertexAdaptive, NULL}; // vedno dodaj null na koncu!
    char* algNames[] = {"Tree", "SCC-edge", "SCC-vertex", "SCC-vertexAdaptive", "NULL"};

    //RunTurkTest(algs, algNames);          // ErdosRenyi
    //RunTurk2Test(algs, algNames);         // TurkTest with extra data (sccV, sccE, wccV, wccE)
    //RunN_1Test(algs, algNames);           // CompGraph
    //RunlgNTest(algs, algNames);           // LogGraph
    //RunSparseTest(algs, algNames);        // Cycle + ErdosRenyi
    //RunSCCsTest(algs, algNames);          // SCCs with density and probability of outgoing edges
    //RunCompleteTest(algs, algNames, 0);   // Complete graphs
    //RunGnutellaTest(algs, algNames);      // Gnutella real-world networks
    //RunTxtTest(algs, algNames);           // Reading txt file to create graphs
    //Run_BA_Test(algs, algNames);          // Barabasi-Albert
    //Run_WorstCase_Test(algs, algNames);     // SCCs with outgoing edges between them
    Run_RandomSizeSCC_Test(algs, algNames);


    int n = 16;
    float** graph = WorstCaseGraph(n, 4, 1.0, 0.1);
    print2DGraph(graph, n);
    freeGraph(graph,n);
    
    return 0;
}
