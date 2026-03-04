

#ifndef GRAPHGENERATORS_H
#define GRAPHGENERATORS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef struct Graf {
   float** matrix;
   int size;
} Graf;



float** TurkGraph(int n, double e);
float** SCCGraph();
float** SCCGraph2();
float** GeneratelgNGraph(int n, int edges);
float** N1Graph(int vertices, float ratio, float alpha);
float** CycleX(int n, int M);
float** sccGraph(int N, float density);
float** GenerateCompleteGraph(int n, int unit);
float** sccGraphAlfa(int N, float density);
Graf GenerateGraphGnutella(char* filename);
Graf GenerateGraphTxt(char* filename);



#ifdef __cplusplus
}
#endif

#endif //GRAPHGENERATORS_H
