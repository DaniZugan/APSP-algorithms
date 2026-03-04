#include <stdio.h>
#include <algorithm>
//#include <math.h>
#include <list>
#include <utility>
#include <boost/heap/pairing_heap.hpp>

#include "utils.h"
#include "Dijkstra.h"

struct DijkstraComparator
{
    bool operator() (const std::pair<short, float> & lhs, const std::pair<short, float> & rhs) const
    {
        return lhs.second > rhs.second;
    }
};
typedef boost::heap::pairing_heap< std::pair<short, float>, boost::heap::compare<DijkstraComparator> > dijkstraHeap;

int DijkstraC(int s, float* opt, bool* solved, std::pair<short,float>** sparse_graph, int n, short* firstVertexOnPath = NULL)
{
	dijkstraHeap::handle_type* handles = new dijkstraHeap::handle_type[n];
	for (int i=0; i<n; i++)
	{
		opt[i] = INFINITY;
		solved[i] = false;
	}
	if (firstVertexOnPath != NULL)
	{
		for (int i=0; i<n; i++)
		{
			firstVertexOnPath[i] = -1;
		}
	}
	dijkstraHeap pq;

	handles[s] = pq.push(std::make_pair(s, 0.0f));
	opt[s] = 0.0f;

	while (true)
	{
		if (pq.empty())
			break;
		// Find minimum
		std::pair<short, float> min = pq.top();
		pq.pop();
		if (!isfinite_cust(min.second)) // Infinite
			break; // We are done.
		short minVertex = min.first;
		solved[minVertex] = true;

		// Relax minimum
		for (int i=0; sparse_graph[minVertex][i].first != -1; i++)
		{
			int target = sparse_graph[minVertex][i].first;
			float cost = sparse_graph[minVertex][i].second;
			
			if (solved[target])
				continue;
			else if (cost + min.second + FLT_EPSILON < opt[target]) // FP intricacies, necessary for FastAPSP to avoid non-direct paths of same length
			{
				if (firstVertexOnPath != NULL) // Only required by fastAPSP
				{
					if (minVertex == s)
						firstVertexOnPath[target] = target;

					else
						firstVertexOnPath[target] = firstVertexOnPath[minVertex];
				}
				float prevDistance = opt[target];
				opt[target] = cost + min.second;
				if (!isfinite_cust(prevDistance)) // was inf., add to pq
				{
					handles[target] = pq.push(std::make_pair(target, opt[target]));
				}
				else // already in pq, decrease key
				{	
					pq.update(handles[target], std::make_pair(target, opt[target]));
				}
			}
		}
	}
	delete[] handles;
	return 0;
}

float** DijkstraAPSPc(float** graph, int n)
{
	float** W = copy2DArray(graph, n);
	
	float* opt = new float[n];
	bool* solved = new bool[n];
	// Create sparse representation
	std::pair<short,float>** sparse_graph = new std::pair<short,float>*[n];
	
	for (int i=0; i<n; i++)
	{
		// first count outgoing edges
		short outdeg = 0;
		for (int j=0; j<n; j++)
		{
			if (isfinite_cust(graph[i][j])) // An actual edge
				outdeg++;
		}
		
		sparse_graph[i] = new std::pair<short,float>[outdeg+1]; // last one is sentry
		int c=0;
		for (int j=0; j<n; j++)
		{
			if (isfinite_cust(graph[i][j]))
			{
				sparse_graph[i][c++] = ( std::make_pair((short)j, graph[i][j]));
			}
		}
		sparse_graph[i][outdeg] = std::make_pair(-1, 0);
	}

	for (int i=0; i<n; i++)
	{
		DijkstraC(i, opt, solved, sparse_graph, n);
		for (int j=0; j<n; j++)
		{
			W[i][j] = opt[j];
		}
	}
		
	
	delete[] opt;
	delete[] solved;
	for (int i=0; i<n; i++)
		delete[] sparse_graph[i];
	delete[] sparse_graph;

	return W;
}