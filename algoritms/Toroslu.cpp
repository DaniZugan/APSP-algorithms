#include "Toroslu.h"

#include <cmath>
#include <cstdlib>

float** Toro(float** graph,  int N) {
    int MAXN = N; // max of vertices

    float** A = (float**)malloc(sizeof(float*)*N);
    for (int i=0; i<N; i++)
    {
        A[i] = (float*)malloc(sizeof(float)*N);
        for (int j=0; j<N; j++)
        {
            A[i][j] = graph[i][j];
        }
    }
    // int INF = 9999; // old infinity
    int* inc = (int*)malloc(MAXN * sizeof(int));
    int* outc = (int*)malloc(MAXN * sizeof(int)); //int inc[MAXN], outc[MAXN]; counts of in/out edges

    int** inlist = (int**)malloc(sizeof(int*)*MAXN);
    int** outlist = (int**)malloc(sizeof(int*)*MAXN);
    for (int i=0; i<MAXN; i++)
    {
        inlist[i] = (int*)malloc(sizeof(int)*MAXN);
        outlist[i] = (int*)malloc(sizeof(int)*MAXN);
    }
    // int inlist[MAXN][MAXN], outlist[MAXN][MAXN];
    int i, j, k, kk;
    int mininxout, mink;
    int* select_k = (int*)malloc(sizeof(int) * MAXN);

    // choose the "best" k
    for (i = 0; i < N; i++)
        inc[i] = 0, outc[i] = 0, select_k[i] = 0;
    // Generate initial inlist and outlist for each vertex
    for (i = 0; i < N; i++)
        for (j = 0; j < N; j++) {
            if (i != j && A[i][j] != INFINITY) // (A[i][j] != 0) && (A[i][j] < INFTY)
                inc[j]++, outc[i]++, inlist[j][inc[j] - 1] = i, outlist[i][outc[i] - 1] = j;
        }
    // The outer loop
    for (kk = 0; kk < N; kk++) {
        // choose the "best" k
        mink = -1;
        mininxout = 2 * N * N;
        for (k = 0; k < N; k++) {
            if ((select_k[k] == 0) && (inc[k] * outc[k] < mininxout)) {
                mink = k;
                mininxout = inc[k] * outc[k];
            }
        }
        k = mink; // "best" k

        select_k[k] = 1; // remove selected vertex
        // explore only useful relaxation attempts
        for (i = 0; i < inc[k]; i++)
            for (j = 0; j < outc[k]; j++) {
                if ((A[inlist[k][i]][k] + A[k][outlist[k][j]]) < A[inlist[k][i]][outlist[k][j]]) {
                    if (A[inlist[k][i]][outlist[k][j]] == INFINITY) {
                        outc[inlist[k][i]]++;
                        outlist[inlist[k][i]][outc[inlist[k][i]] - 1] = outlist[k][j];
                        inc[outlist[k][j]]++;
                        inlist[outlist[k][j]][inc[outlist[k][j]] - 1] = inlist[k][i];
                    }
                    A[inlist[k][i]][outlist[k][j]] = A[inlist[k][i]][k] + A[k][outlist[k][j]];
                }
            }
    }

    for (int z = 0; z < N; z++) {
        free(inlist[z]);
        free(outlist[z]);
    }
    free(inlist);
    free(outlist);
    free(inc);
    free(outc);
    free(select_k);

    return A;
}
