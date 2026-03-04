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
    // UniformPathsAlgorithm, HiddenPathsAlgorithm, floydWarshall, DijkstraAPSPc, FastHourglassHalf, Toro, SCCalgo, SCCalgo2
    // "Uniform", "Hidden", "FW", "Dijkstra", "Tree", "Toroslu", "SCCedge", "SCCvertex"
    //RelaxCounter rc;
    AlgorithmFP algs[] = {floydWarshall, NULL}; // vedno dodaj null na koncu!
    char* algNames[] = {"FW", "NULL"};

    RunTurkTest(algs, algNames);          // ErdosRenyi
    //RunTurk2Test(algs, algNames);         // TurkTest with extra data (sccV, sccE, wccV, wccE)
    //RunN_1Test(algs, algNames);           // CompGraph
    //RunlgNTest(algs, algNames);           // LogGraph
    //RunSparseTest(algs, algNames);        // Cycle + ErdosRenyi
    //RunSCCsTest(algs, algNames);          // SCCs with density and probability of outgoing edges
    //RunCompleteTest(algs, algNames, 0);   // Complete graphs
    //RunGnutellaTest(algs, algNames);      // Gnutella real-world networks
    //RunTxtTest(algs, algNames);           // Reading txt file to create graphs
    //Run_BA_Test(algs, algNames);          // Barabasi-Albert
    //Run_WorstCase_Test(algs, algNames);   // 2 SCCs with maximum outgoing edges between them


    return 0;
}
