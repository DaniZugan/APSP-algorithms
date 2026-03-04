#include "GraphTools.h"

float** generate_regular_graph(int N, int k) {
    if (N<1 || k>N || k<0) {
        printf("Invalid input for regular graph\n");
        return NULL;
    }
    float** graph = (float**)malloc(N*sizeof(float*));
    if (k % 2 == 0) {
        // even k: connect i to i+1..i+k/2 (mod n)
        int r = k / 2;
        for (int i = 0; i < N; i++) {
            for (int t = 1; t <= r; t++) {
                int j = (i + t) % N;
                graph[i][j] = 1.0f;
                //add_edge(G, i, j, 1.0f);
            }
        }
    } else {
        int r = (k - 1) / 2;
        for (int i = 0; i < N; i++) {
            for (int t = 1; t <= r; t++) {
                int j = (i + t) % N;
                graph[i][j] = 1.0f;
            }
        }
        for (int i = 0; i < N/2; i++) {
            graph[i][i + N/2] = 1.0f;

        }
    }


    return graph;
}
