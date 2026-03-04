#include "UniformPaths.h"
#include "../utils.h"

float** global_W = nullptr;
int** global_Tie = nullptr;

float** UniformPathsAlgorithm(float** graph, int n)
{
	clock_t timeTotal = clock();
	clock_t timePQ;
	double timePQTotal = 0.0;

	float** W = copy2DArray(graph, n);

	bool** solved = new bool*[n];
	Tree** trees = new Tree*[n];
	short** firstEdgeUsed = new short*[n];
	short** parent = new short*[n];
	int** tieBreaker = new int*[n];

	global_W = W;
	global_Tie = tieBreaker;

	SimpleList** markers = new SimpleList*[n];

	UPheap::handle_type** handles = new UPheap::handle_type*[n];
	UPheap pq;
	for (int i=0; i<n; i++)
	{
		tieBreaker[i] = new int[n];
		solved[i] = new bool[n];
		trees[i] = new Tree[n];
		markers[i] = new SimpleList[n];
		firstEdgeUsed[i] = new short[n];
		parent[i] = new short[n]; // the penultimate node on the path
		handles[i] = new UPheap::handle_type[n];
		for (int j=0; j<n; j++)
		{
			tieBreaker[i][j] = 0;
			solved[i][j] = false;

			trees[i][j].nextSibling = -1;
			trees[i][j].firstChild = -1;
			trees[i][j].markers = NULL;

			markers[i][j].label = i; // stays fixed
			markers[i][j].next = NULL;

			firstEdgeUsed[i][j] = -1;
			parent[i][j] = -1;

			// Add the direct edges to the PQ
			if (i != j && isfinite_cust(graph[i][j])) // direct edge
			{
				tieBreaker[i][j] = i+n*j;
				W[i][j] = graph[i][j];
				firstEdgeUsed[i][j] = j;
				parent[i][j] = i;
				timePQ = clock();
				handles[i][j] = pq.push(std::make_pair(std::make_pair(i,j), W[i][j]));
				timePQTotal += (double)(clock() - timePQ);
			}
		}
		solved[i][i] = true;
	}
	// End of initialization

	// Main loop
	while (!pq.empty())
	{
		timePQ = clock();
		std::pair<std::pair<short,short>, float > min = pq.top();
		pq.pop();
		timePQTotal += (double)(clock() - timePQ);

		short i=min.first.first;
		short j=min.first.second;
		solved[i][j] = true;

		// Add the target to our tree
		trees[i][j].nextSibling = trees[i][parent[i][j]].firstChild;
		trees[i][parent[i][j]].firstChild = j;

		// Handle all the marked nodes, notify them we have a new candidate
		SimpleList* entry = trees[i][parent[i][j]].markers;
		while (entry != NULL)
		{
			short v = entry->label;

			if (!solved[v][j])
			{
				if (W[v][i] + W[i][j] + FLT_EPSILON < W[v][j] || (W[v][i] + W[i][j] <= W[v][j] + FLT_EPSILON &&
					(std::max(tieBreaker[v][i], tieBreaker[i][j]) < tieBreaker[v][j])))
				{

					if (isfinite_cust(W[v][j])) // exists in PQ
					{
						W[v][j] = W[v][i] + W[i][j];
						tieBreaker[v][j] = std::max(tieBreaker[v][i], tieBreaker[i][j]);
						timePQ = clock();
						pq.update(handles[v][j], std::make_pair(std::make_pair(v,j), W[v][j]));
						timePQTotal += (double)(clock() - timePQ);
					}
					else // add to PQ
					{
						W[v][j] = W[v][i] + W[i][j];
						tieBreaker[v][j] = std::max(tieBreaker[v][i], tieBreaker[i][j]);
						timePQ = clock();
						handles[v][j] = pq.push(std::make_pair(std::make_pair(v,j), W[v][j]));
						timePQTotal += (double)(clock() - timePQ);
					}


					firstEdgeUsed[v][j] = firstEdgeUsed[v][i];
					parent[v][j] = parent[i][j];
				}
			}

			entry = entry->next;
		}

		short neighbor = firstEdgeUsed[i][j];
		// Mark the node in our neighbor's tree, and explore it further.
		markers[i][j].next = trees[neighbor][j].markers;
		trees[neighbor][j].markers = &(markers[i][j]);

		// Now explore the children
		short t = trees[neighbor][j].firstChild;
		while (t != -1)
		{
			if (!solved[i][t])
			{
				if (W[i][neighbor] + W[neighbor][t] + FLT_EPSILON < W[i][t] || (W[i][neighbor] + W[neighbor][t] <= W[i][t] + FLT_EPSILON &&
					(std::max(tieBreaker[i][neighbor], tieBreaker[neighbor][t]) < tieBreaker[i][t])))
				{
					if (isfinite_cust(W[i][t])) // exists in PQ
					{
						W[i][t] = W[i][neighbor] + W[neighbor][t];
						tieBreaker[i][t] = std::max(tieBreaker[i][neighbor], tieBreaker[neighbor][t]);
						timePQ = clock();
						pq.update(handles[i][t], std::make_pair(std::make_pair(i,t), W[i][t]));
						timePQTotal += (double)(clock() - timePQ);
					}
					else // add to PQ
					{
						W[i][t] = W[i][neighbor] + W[neighbor][t];
						tieBreaker[i][t] = std::max(tieBreaker[i][neighbor], tieBreaker[neighbor][t]);
						timePQ = clock();
						handles[i][t] = pq.push(std::make_pair(std::make_pair(i,t), W[i][t]));
						timePQTotal += (double)(clock() - timePQ);
					}


					firstEdgeUsed[i][t] = neighbor;
					parent[i][t] = j;
				}
			}

			t = trees[neighbor][t].nextSibling;
		}
	}
	timeTotal = clock() - timeTotal;
	//#ifdef DBG
	/*	printf("######UP TIMERS######\n");
		printf("Total: %f\n", ((double)timeTotal)/(double)CLOCKS_PER_SEC);
		printf("PQ: %.2f%%\n", (timePQTotal/((double)timeTotal))*100.0);
		printf("#####################\n\n");*/
	//#endif

	// Clean up
	for (int i=0; i<n; i++)
	{
		delete[] solved[i];
		delete[] trees[i];
		delete[] markers[i];
		delete[] firstEdgeUsed[i];
		delete[] parent[i];
		delete[] handles[i];
		delete[] tieBreaker[i];
	}
	delete[] solved;
	delete[] trees;
	delete[] markers;
	delete[] firstEdgeUsed;
	delete[] parent;
	delete[] handles;
	delete[] tieBreaker;

	return W;
}