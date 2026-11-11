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
    // UniformPathsAlgorithm, HiddenPathsAlgorithm, floydWarshall, DijkstraAPSPc, FastHourglassHalf, Toro, SCCedge, SCCvertex, SCCvertexAdaptive, SCCvertexHourglass, SCCvertexBridgeHG
    // "Uniform", "Hidden", "FW", "Dijkstra", "Tree", "Toroslu", "SCCedge", "SCCvertex", "SCCadaptive", "Vertex-SCC-hourglass", "Vertex-SCC-bridgeHG"
    // RelaxCounter rc;
    AlgorithmFP algs[] = {FastHourglassHalf, SCCedge, SCCvertexAdaptive, SCCvertexNoTree, SCCedgeOrVertex, DijkstraAPSPc, UniformPathsAlgorithm, HiddenPathsAlgorithm, floydWarshall, NULL}; // vedno dodaj null na koncu!
    char* algNames[] = {"Tree", "Edge-SCC", "Vertex-SCC", "VertexNaive-SCC", "Adaptive-SCC", "Dijkstra", "Uniform", "Hidden", "FW", "NULL"};

    //RunTurkTest(algs, algNames);          // ErdosRenyi
    //RunTurk2Test(algs, algNames);         // TurkTest with extra data (sccV, sccE, wccV, wccE)
    //RunN_1Test(algs, algNames);           // CompGraph
    //RunlgNTest(algs, algNames);           // LogGraph
    //RunSparseTest(algs, algNames);        // Cycle + ErdosRenyi
    //RunSCCsTest(algs, algNames);          // SCCs with density and probability of outgoing edges
    //RunCompleteTest(algs, algNames, 0);   // Complete graphs
    RunGnutellaTest(algs, algNames);      // Gnutella real-world networks
    //RunTxtTest(algs, algNames);           // Reading txt file to create graphs
    //Run_BA_Test(algs, algNames);          // Barabasi-Albert
    //Run_WorstCase_Test(algs, algNames);     // SCCs with outgoing edges between them
    //Run_RandomSizeSCC_Test(algs, algNames);
    Run_StarSCC_Test(algs, algNames);     // like RandomSizeSCC, pairs joined by 1 tail -> all heads or all tails -> 1 head
    Run_EqualSizeSCC_Test(algs, algNames); // like RandomSizeSCC, all SCCs of equal size -> results/equalSizeSCC_data.csv
    Run_SNAP_Test(algs, algNames);        // SNAP real-world graphs from SNAP-graphs/
    //Export_SCCGraph(32, 4, 0.1, 0.3, 0.5, 1, "../results/Viz/SCC/graph.csv"); // n, k, outProb, inProb, coin, star -> draw_graph.py

    /*
    int n = 16;
    float** graph = WorstCaseGraph(n, 4, 1.0, 0.1);
    print2DGraph(graph, n);
    freeGraph(graph,n);
    */

    return 0;
}
