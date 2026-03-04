#include "HiddenPaths.h"
#include "../utils.h"

struct HiddenPathsComparator
{
    bool operator() (const std::pair<std::pair<short, short>, float> & lhs, const std::pair<std::pair<short, short>, float> & rhs) const
    {
        return lhs.second > rhs.second;
    }
};
typedef boost::heap::pairing_heap< std::pair<std::pair<short, short>, float>, boost::heap::compare<HiddenPathsComparator> > HPheap;

float** HiddenPathsAlgorithm(float** graph, int n)
{
	timespec start;
	timespec end;

	timespec totalStart;
	timespec totalEnd;

	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &totalStart);

	double timeTotal = 0;
	double timePQTotal = 0.0;

	float** W = copy2DArray(graph, n);

	bool** solved = new bool*[n];
	bool** direct = new bool*[n]; // stores if the path is an edge

	short** incomingOpt = new short*[n];
	short** outgoingOpt = new short*[n];
	short* incomingOptSize = new short[n];
	short* outgoingOptSize = new short[n];
	HPheap::handle_type** handles = new HPheap::handle_type*[n];
	HPheap pq;
	for (int i=0; i<n; i++)
	{
		solved[i] = new bool[n];
		direct[i] = new bool[n];

		incomingOpt[i] = new short[n];
		outgoingOpt[i] = new short[n];
		incomingOptSize[i] = 0;
		outgoingOptSize[i] = 0;
		handles[i] = new HPheap::handle_type[n];
		for (int j=0; j<n; j++)
		{
			solved[i][j] = false;
			direct[i][j] = true;

			// Add the direct edges to the PQ
			if (i != j && isfinite_cust(graph[i][j])) // direct edge
			{
				W[i][j] = graph[i][j];
				//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
				handles[i][j] = pq.push(std::make_pair(std::make_pair(i,j), W[i][j]));
				/*clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
				timePQTotal += (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds*/
			}
		}
		solved[i][i] = true;
	}
	// End of initialization

	// Main loop
	while (!pq.empty())
	{
		//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

		std::pair<std::pair<short,short>, float > min = pq.top();
		pq.pop();

		/*clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		timePQTotal += (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds*/

		short i=min.first.first;
		short j=min.first.second;
		solved[i][j] = true;

		// If edge was direct, add it to our list & inform neighbors
		if (direct[i][j])
		{
			outgoingOpt[i][outgoingOptSize[i]++] = j;

			// Inform all incoming nodes of the new target
			for (int c=0; c<incomingOptSize[i]; c++)
			{
				short v = incomingOpt[i][c];

				if (!solved[v][j])
				{
					if (W[v][i] + W[i][j] + FLT_EPSILON < W[v][j])
					{

						if (isfinite_cust(W[v][j])) // exists in PQ
						{
							W[v][j] = W[v][i] + W[i][j];

							//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

							pq.update(handles[v][j], std::make_pair(std::make_pair(v,j), W[v][j]));
							/*clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
							timePQTotal += (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds*/
						}
						else // add to PQ
						{
							W[v][j] = W[v][i] + W[i][j];
							//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);

							handles[v][j] = pq.push(std::make_pair(std::make_pair(v,j), W[v][j]));
							/*clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
							timePQTotal += (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds*/
						}

						direct[v][j] = false;
					}
				}
			}
		}

		// Add ourselves to the target's list
		incomingOpt[j][incomingOptSize[j]++] = i;

		// Explore our target's direct opt edges.
		for (int c=0; c<outgoingOptSize[j]; c++)
		{
			short t = outgoingOpt[j][c];
			if (!solved[i][t])
			{
				if (W[i][j] + W[j][t] + FLT_EPSILON < W[i][t])
				{
					if (isfinite_cust(W[i][t])) // exists in PQ
					{
						W[i][t] = W[i][j] + W[j][t];
						//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);


						pq.update(handles[i][t], std::make_pair(std::make_pair(i,t), W[i][t]));
						/*clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
						timePQTotal += (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds*/
					}
					else // add to PQ
					{
						W[i][t] = W[i][j] + W[j][t];
						//clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);


						handles[i][t] = pq.push(std::make_pair(std::make_pair(i,t), W[i][t]));
						/*clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
						timePQTotal += (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds*/
					}

					direct[i][t] = false;
				}
			}
		}
	}
	clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &totalEnd);
	timeTotal += (double)(totalEnd.tv_sec - totalStart.tv_sec) + (double)(totalEnd.tv_nsec - totalStart.tv_nsec)/1.0e9; // seconds
	#ifdef DBG
		printf("######HP TIMERS######\n");
		printf("Total: %f\n", ((double)timeTotal));
		printf("PQ: %.2f%%\n", (timePQTotal/((double)timeTotal))*100.0);
		printf("PQ Total: %f\n", ((double)timePQTotal));
		printf("#####################\n\n");
	#endif

	// Clean up
	for (int i=0; i<n; i++)
	{
		delete[] solved[i];
		delete[] incomingOpt[i];
		delete[] outgoingOpt[i];
		delete[] direct[i];
		delete[] handles[i];
	}
	delete[] solved;
	delete[] incomingOpt;
	delete[] outgoingOpt;
	delete[] incomingOptSize;
	delete[] outgoingOptSize;
	delete[] direct;
	delete[] handles;

	return W;
}