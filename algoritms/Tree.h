
#ifndef TREE_H
#define TREE_H

typedef struct SimpleTree SimpleTree;
typedef struct SimpleStack SimpleStack;
typedef struct TreeStack TreeStack;

struct SimpleTree {
    int label;

    SimpleTree* nextSibling;
    SimpleTree* firstChild;
};

struct SimpleStack {
    int size;
    int* container;
    int maxSize;
};

struct TreeStack {
    int size;
    SimpleTree** container;
};

float** FastHourglassHalf(float** graph, int n);
void ModTree(float** W, int n);
float** NullFunction(float** graph, int n);

#endif //TREE_H
