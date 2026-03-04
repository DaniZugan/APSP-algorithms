#ifndef UNIFORMPATHS_H
#define UNIFORMPATHS_H

#include <boost/heap/pairing_heap.hpp>
#include <algorithm>
#include <cfloat>
//#include <math.h>
#include "../utils.h"

float** UniformPathsAlgorithm(float** graph, int n);

typedef struct Tree Tree;

extern float** global_W;
extern int** global_Tie;

struct Tree {
    short nextSibling;
    short firstChild;
    SimpleList* markers;
};

struct UPComparator
{
    bool operator() (const std::pair<std::pair<short, short>, float >& lhs, const std::pair<std::pair<short, short>, float >& rhs) const
    {
        float c = lhs.second - rhs.second;
        if (c > FLT_EPSILON)
        {
            return true;
        }
        else if (c < -FLT_EPSILON)
        {
            return false;
        }
        else
            return global_Tie[lhs.first.first][lhs.first.second] > global_Tie[rhs.first.first][rhs.first.second];
    }
};
typedef boost::heap::pairing_heap< std::pair<std::pair<short, short>, float >, boost::heap::compare<UPComparator> > UPheap;

#endif //UNIFORMPATHS_H
