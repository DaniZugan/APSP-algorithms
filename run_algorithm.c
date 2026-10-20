
#include "run_algorithm.h"
#include "graphGenerators.h"
#include "utils.h"
#include "algoritms/SCC.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <tgmath.h>
#include <time.h>
#include <string.h>

#ifndef _WIN32
#include <errno.h>
#include <poll.h>
#include <signal.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

float** RunAlgorithmReturnMatrix(float** graph, int n, double* time, AlgorithmFP alg)
{
	#ifdef _WIN32
		clock_t t;
		t = clock();
	#else // assuming a UNIX type OS
		struct timespec start;
		struct timespec end;
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &start);
	#endif
	float** mat = alg(graph, n);
	#ifdef _WIN32
		t = clock() - t;
		(*time) = ((double)t)/((double)CLOCKS_PER_SEC);
	#else
		clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &end);
		(*time) = (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec)/1.0e9; // seconds
	#endif

	return mat;
}

int RunAlgorithm(float** graph, int n, double* time, AlgorithmFP alg, int checkOutput, float** reference)
{
	int ret = 0;
	float** mat = RunAlgorithmReturnMatrix(graph, n, time, alg);

	if (checkOutput)
	{
		for (int i=0; i<n; i++) // Check correctness of output
		{
			for (int j=0; j<n; j++)
			{
				if (i == j)
					continue;

				float a = mat[i][j];
				float b = reference[i][j];
				if ( fabs(a - b) >= 0.01 ) //0.0001
				{
					printf("Diff: %f %f \n", a, b);
					ret = -1;
				}
			}
		}
	}
	for (int i=0; i<n; i++)
	{
		free(mat[i]);
	}
	free(mat);

	return ret;
}

// Timed runs in a child process ----------------------------------------------------------------------------------------

const char* RunStatusName(RunStatus status)
{
	switch (status) {
		case RUN_OK: return "ok";
		case RUN_WRONG: return "WRONG";
		case RUN_TIMEOUT: return "TIMEOUT";
		default: return "CRASH";
	}
}

#ifndef _WIN32

float* CreateSharedMatrix(int n)
{
	void* p = mmap(NULL, (size_t)n * (size_t)n * sizeof(float), PROT_READ | PROT_WRITE,
				   MAP_SHARED | MAP_ANON, -1, 0);
	return (p == MAP_FAILED) ? NULL : (float*)p;
}

void FreeSharedMatrix(float* matrix, int n)
{
	if (matrix != NULL)
		munmap(matrix, (size_t)n * (size_t)n * sizeof(float));
}

static double monotonicNow(void)
{
	struct timespec t;
	clock_gettime(CLOCK_MONOTONIC, &t);
	return (double)t.tv_sec + (double)t.tv_nsec / 1.0e9;
}

// Writes the whole buffer, returns 0 on failure
static int writeAll(int fd, const void* buf, size_t size)
{
	const char* p = (const char*)buf;
	while (size > 0) {
		ssize_t w = write(fd, p, size);
		if (w < 0 && errno == EINTR) continue;
		if (w <= 0) return 0;
		p += w;
		size -= (size_t)w;
	}
	return 1;
}

// Reads size bytes, waiting until deadline (monotonic seconds, < 0 waits forever).
// Returns 1 when read, 0 on timeout, -1 when the writer closed the pipe (child died).
static int readWithDeadline(int fd, void* buf, size_t size, double deadline)
{
	char* p = (char*)buf;
	while (size > 0) {
		int timeoutMs = -1;
		if (deadline >= 0) {
			double left = deadline - monotonicNow();
			if (left <= 0) return 0;
			timeoutMs = (int)(left * 1000.0) + 1;
		}
		struct pollfd pfd = {fd, POLLIN, 0};
		int r = poll(&pfd, 1, timeoutMs);
		if (r < 0 && errno == EINTR) continue;
		if (r < 0) return -1;
		if (r == 0) continue; // loop re-checks the deadline

		ssize_t got = read(fd, p, size);
		if (got < 0 && errno == EINTR) continue;
		if (got <= 0) return -1;
		p += got;
		size -= (size_t)got;
	}
	return 1;
}

/*
 * Runs alg in a forked child process and kills it if it does not finish within limit seconds.
 * The child sees graph (and ctx) through copy-on-write, so nothing is copied. All memory the algorithm
 * allocates lives in the child and is released by the OS when the child exits or is killed, so a killed
 * run cannot leak memory or leave global state (e.g. the adjacency lists in SCC.cpp) broken in this process.
 *
 * The limit covers only the algorithm. The optional check (and copying the result into the shared matrix out)
 * runs after the time is reported and has no limit.
 *
 * time: CPU time of the algorithm, limit on timeout, -1 on crash. errors: result of check (0 if none).
 */
RunStatus RunAlgorithmTimed(float** graph, int n, AlgorithmFP alg, double limit, double* time,
							CheckFP check, void* ctx, float* out, int* errors)
{
	*errors = 0;
	*time = -1;

	int fd[2];
	if (pipe(fd) != 0) {
		perror("pipe");
		return RUN_CRASH;
	}

	fflush(stdout); // otherwise buffered output is printed by both processes
	fflush(stderr);
	pid_t pid = fork();
	if (pid < 0) {
		perror("fork");
		close(fd[0]);
		close(fd[1]);
		return RUN_CRASH;
	}

	if (pid == 0) { // child
		close(fd[0]);
		double t = 0;
		float** mat = RunAlgorithmReturnMatrix(graph, n, &t, alg);
		writeAll(fd[1], &t, sizeof(t)); // message 1: algorithm finished

		int e = check ? check(mat, n, ctx) : 0;
		if (out != NULL)
			for (int i = 0; i < n; i++)
				memcpy(out + (size_t)i * n, mat[i], (size_t)n * sizeof(float));
		freeGraph(mat, n);
		writeAll(fd[1], &e, sizeof(e)); // message 2: check done, out written

		close(fd[1]);
		fflush(stdout);
		fflush(stderr);
		_exit(0); // _exit, so the parent's atexit handlers and buffers are not run again
	}

	// parent
	close(fd[1]);
	RunStatus status;
	double t = 0;
	int r = readWithDeadline(fd[0], &t, sizeof(t), monotonicNow() + limit);
	if (r == 0) {
		kill(pid, SIGKILL);
		*time = limit;
		status = RUN_TIMEOUT;
	}
	else if (r < 0) {
		status = RUN_CRASH;
	}
	else {
		*time = t;
		int e = 0;
		if (readWithDeadline(fd[0], &e, sizeof(e), -1) == 1) {
			*errors = e;
			status = e ? RUN_WRONG : RUN_OK;
		}
		else status = RUN_CRASH; // died during the check
	}
	close(fd[0]);

	int wstatus;
	while (waitpid(pid, &wstatus, 0) < 0 && errno == EINTR) {} // always reap the child, also after kill
	if (status != RUN_TIMEOUT && WIFSIGNALED(wstatus))
		status = RUN_CRASH;
	if (status == RUN_CRASH)
		*time = -1;

	return status;
}

#else // no fork on Windows: run without a time limit

float* CreateSharedMatrix(int n)
{
	return (float*)malloc((size_t)n * (size_t)n * sizeof(float));
}

void FreeSharedMatrix(float* matrix, int n)
{
	free(matrix);
}

RunStatus RunAlgorithmTimed(float** graph, int n, AlgorithmFP alg, double limit, double* time,
							CheckFP check, void* ctx, float* out, int* errors)
{
	float** mat = RunAlgorithmReturnMatrix(graph, n, time, alg);
	*errors = check ? check(mat, n, ctx) : 0;
	if (out != NULL)
		for (int i = 0; i < n; i++)
			memcpy(out + (size_t)i * n, mat[i], (size_t)n * sizeof(float));
	freeGraph(mat, n);
	return *errors ? RUN_WRONG : RUN_OK;
}

#endif

// Compares an output with the reference matrix (ctx), prints the first few differences
static int compareWithReference(float** mat, int n, void* ctx)
{
	float** reference = (float**)ctx;
	int diffs = 0;
	for (int i=0; i<n; i++)
	{
		for (int j=0; j<n; j++)
		{
			if (i == j)
				continue;

			float a = mat[i][j];
			float b = reference[i][j];
			if ( fabs(a - b) >= 0.01 ) //0.0001
			{
				if (diffs < 10)
					printf("Diff [%d][%d]: %f %f \n", i, j, a, b);
				diffs++;
			}
		}
	}
	return diffs;
}

// Runs all algorithms with the MAX_INSTANCE_TIME limit. The first algorithm is the reference the others are checked
// against; if it does not finish, the others are still timed but not checked.
double* RunTest(float** graph, int n, AlgorithmFP* algs, int* mStar)
{
	int size = 0;
	for (int i=0; algs[i] != NULL; i++) {
		size++;
	}

	double* results = (double*)malloc(sizeof(double)*size);
	*mStar = 0;

	float* refData = CreateSharedMatrix(n); // the reference is computed in a child, which writes it here
	float** ref = NULL;
	int errors = 0;
	RunStatus status = RUN_CRASH;
	if (refData != NULL)
		status = RunAlgorithmTimed(graph, n, algs[0], MAX_INSTANCE_TIME, &(results[0]), NULL, NULL, refData, &errors);
	else
		printf("Cannot allocate the reference matrix\n");

	if (status == RUN_OK)
	{
		ref = (float**)malloc(sizeof(float*)*n); // row pointers into refData
		for (int i=0; i<n; i++)
			ref[i] = refData + (size_t)i * n;

		for (int i=0; i<n; i++)
		{
			for (int j=0; j<n; j++)
			{
				if (i == j)
					continue;
				else
				{
					if (fabs(ref[i][j] - graph[i][j]) <= 0.0001)
					{
						(*mStar)++;
					}
				}
			}
		}
	}
	else
	{
		if (refData == NULL)
			results[0] = -1;
		printf("Algorithm 0 (reference): %s, other algorithms are not checked\n", RunStatusName(status));
	}

	for (int i=1; i<size; i++)
	{
		status = RunAlgorithmTimed(graph, n, algs[i], MAX_INSTANCE_TIME, &(results[i]),
								   ref ? compareWithReference : NULL, ref, NULL, &errors);
		if (status == RUN_WRONG)
			printf("Algorithm %d: WRONG, %d entries differ from the reference\n", i, errors);
		else if (status != RUN_OK)
			printf("Algorithm %d: %s\n", i, RunStatusName(status));
	}

	free(ref);
	FreeSharedMatrix(refData, n);

	return results;
}

double** ComputeAvg(double** results, int size, int totalRuns)
{
	double** avgResults = (double**)malloc(sizeof(double*)*size);
	for (int b=0; b<size; b++)
	{
		avgResults[b] = (double*)malloc(sizeof(double)*3);
		avgResults[b][0] = 0.0;
		avgResults[b][1] = INFINITY;
		avgResults[b][2] = 0.0;
	}

	for (int b=0; b<totalRuns; b++)
	{
		for (int c=0; c<size; c++)
		{
			avgResults[c][0] += results[b][c];
			if (results[b][c] < avgResults[c][1])
				avgResults[c][1] = results[b][c];
			if (results[b][c] > avgResults[c][2])
				avgResults[c][2] = results[b][c];
		}
	}

	for (int b=0; b<size; b++)
	{
		avgResults[b][0] = avgResults[b][0] / ((double)totalRuns);
	}

	return avgResults;
}



// Tests ---------------------------------------------------------------------------------------------------------------
// export for TurkTest
void csvExport(double** results, int algCount, char** algNames, double factor, int n){
	FILE *csvFile = fopen("data.csv", "a");
	//fprintf(csvFile, "Name, N, e, time (s)\n");
	for (int i = 0; i < algCount; ++i) {
		fprintf(csvFile, "%s, %d, %.2lf, %.4lf\n", algNames[i], n, factor, results[i][0]);
	}
	fclose(csvFile);
}
void csvTurkHeader() {
	FILE *csvFile = fopen("data.csv", "w");
	fprintf(csvFile, "Name, N, e, time (s)\n");
	fclose(csvFile);
}
void RunTurkTest(AlgorithmFP* algs, char** algNames)
{
	printf("Erdos-Renyi Test \n");
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }

	csvTurkHeader();
    int sizes[] = {2048, 4096};
    double edgeFactors[] = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1};

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        for (int s = 0; s< sizeof(edgeFactors) / sizeof(edgeFactors[0]); s++)
        {
        	printf("e = %0.2f \n", edgeFactors[s]);
            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {

                // Generate graph.
                float** graph = TurkGraph(n, edgeFactors[s]);

                int mStar = 0;
                results[run] = RunTest(graph, n, algs, &mStar);

                /*
                FILE *csvFile = fopen("data.csv", "a");
                fprintf(csvFile, "Graph (e, N, m)\n");
                fprintf(csvFile, "%0.2f, %d, %0.0lf\n", edgeFactors[s], n, pow((double)n, edgeFactors[s]));
                fclose(csvFile);


                csvFile = fopen("data.csv", "a");
                fprintf(csvFile, "Algorithms (name, time)\n");
                for (int i = 0; i < algCount; ++i) {  // then fprintf results
                    fprintf(csvFile, "%s, %0.3lf\n", algNames[i], results[run][i]);
                }
                fprintf(csvFile, "\n");
                fclose(csvFile); */
                //for (int i = 0; i < algCount; ++i) { // print every recorded time
                    //printf("%s, %lf\n", algNames[i], results[run][i]);
                //}

                for (int i=0; i<n; i++)
                {
                    free(graph[i]);
                }
                free(graph);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
        	for (int i = 0; i < algCount; ++i) {
        		printf("%s, %.4lf\n", algNames[i], avgResults[i][0]);
        	}
            csvExport(avgResults, algCount, algNames, edgeFactors[s], n); // average results export to data.csv

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
        }
    }
}

// TurkTest with extra data (sccV, sccE, wccV, wccE) and functions
// count edges of a graph
int countEdges(float** graph, int n) {
	int edges = 0;
	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (graph[i][j] != INFINITY && i != j) edges++;
		}
	}
	return edges;
}
// dfs for graphHistogram
void DFSh(int node, bool* visited, int *componentSize, int n, float** graph) {
	visited[node] = true;
	(*componentSize)++;
	for (int i = 0; i < n; ++i) {
		if((graph[node][i] != INFINITY || graph[i][node] != INFINITY) && !visited[i]) {
			DFSh(i, visited, componentSize, n, graph);
		}
	}
}
// graph histogram for WCCs
int* graphHistogram(float**graph, int n, bool print){

	int* histogram = (int*)calloc(n+1, sizeof(int));

	bool* visited = (bool*)malloc(n * sizeof(bool));
	for (int i = 0; i < n; i++) {
		visited[i] = false;
	}

	for (int i = 0; i < n; ++i) {
		if (!visited[i]) {
			int componentSize = 0;
			DFSh(i, visited, &componentSize, n, graph);
			histogram[componentSize]++;
		}
	}

	if (print) {
		printf("Histogram of weakly connected components:\n");
		for (int i = n; i > 0; i--) {
			if (histogram[i] > 0) {
				printf("Size %d: %d components\n", i, histogram[i]);
			}
		}
	}

	free(visited);
	return histogram;
}
// returns number of WCCs
int WCCcount(int* histogram, int n) {
	int wcc = 0; // counter
	for (int i = 1; i <= n; ++i) {
		if (histogram[i] > 0) {
			wcc++;
		}
	}
	return wcc;
}
// dfs for extractMaxComp
void dfsW(float** graph, int n, int v, bool* visited, int* component, int* componentSize) {
	visited[v] = true;
	component[(*componentSize)++] = v;

	for (int i = 0; i < n; i++) {
		if (!visited[i] && (graph[v][i] != INFINITY || graph[i][v] != INFINITY)) {
			dfsW(graph, n, i, visited, component, componentSize);
		}
	}
}
float** extractMaxComp(float** graph, int* histogram, int n, int* max){

	for (int i = n; i > 0; i--) {
		if (histogram[i] > 0) {
			*max = i;
			break;
		}
	}

	bool* visited = (bool*)malloc(n * sizeof(bool));
	for (int i = 0; i < n; i++) {
		visited[i] = false;
	}

	int* component = (int*)malloc(n * sizeof(int));

	for (int i = 0; i < n; ++i) {
		if (!visited[i]) {           //count++;
			int componentSize = 0;
			int componentEdges = 0;
			dfsW(graph, n, i, visited, component, &componentSize);
			if(componentSize == *max) {
				float** subgraph = extractSubgraph(graph, component, componentSize);

				return subgraph;
				for (int j = 0; j < componentSize; j++) {
					free(subgraph[j]);
				}
				free(subgraph);
			}
		}
	}
	printf("Could not extract the largest component.");
	return 0;
}
// function for findLargestSCC
void SCChelp(int u, float** graph, int V, int* disc, int* low, int* stackMember, int* stack, int* top, int* time, int** sccVertices, int* sccSizes, int* numSCCs, int* largestSCCIndex, int* largestSCCSize) {
	disc[u] = low[u] = ++(*time);
	stack[++(*top)] = u;
	stackMember[u] = 1;

	for (int v = 0; v < V; v++) {
		if (graph[u][v] != INFINITY) {
			if (disc[v] == -1) {
				SCChelp(v, graph, V, disc, low, stackMember, stack, top, time, sccVertices, sccSizes, numSCCs, largestSCCIndex, largestSCCSize);
				low[u] = (low[u] < low[v]) ? low[u] : low[v];
			} else if (stackMember[v]) {
				low[u] = (low[u] < disc[v]) ? low[u] : disc[v];
			}
		}
	}

	if (low[u] == disc[u]) {
		int sccIndex = (*numSCCs)++;
		sccVertices[sccIndex] = (int*)malloc(V * sizeof(int));
		int sccSize = 0;

		while (stack[*top] != u) {
			int w = stack[(*top)--];
			stackMember[w] = 0;
			sccVertices[sccIndex][sccSize++] = w;
		}
		int w = stack[(*top)--];
		stackMember[w] = 0;
		sccVertices[sccIndex][sccSize++] = w;

		sccSizes[sccIndex] = sccSize;

		if (sccSize > *largestSCCSize) {
			*largestSCCSize = sccSize;
			*largestSCCIndex = sccIndex;
		}
	}
}
int* findLargestSCC(float** graph, int V, int* largestSCCSize) {
	// Utility variables
	int* disc = (int*)malloc(V * sizeof(int));
	int* low = (int*)malloc(V * sizeof(int));
	int* stackMember = (int*)malloc(V * sizeof(int));
	int* stack = (int*)malloc(V * sizeof(int));
	int** sccVertices = (int**)malloc(V * sizeof(int*)); // Store vertices for each SCC
	int* sccSizes = (int*)calloc(V + 1, sizeof(int)); // Track sizes of SCCs
	int time = 0, top = -1, numSCCs = 0;

	// Initialize arrays
	for (int i = 0; i < V; i++) {
		disc[i] = -1;
		low[i] = -1;
		stackMember[i] = 0;
	}

	int largestSCCIndex = -1;
	*largestSCCSize = 0;

	// Call DFS-based SCC utility for all unvisited nodes
	for (int i = 0; i < V; i++) {
		if (disc[i] == -1) {
			SCChelp(i, graph, V, disc, low, stackMember, stack, &top, &time, sccVertices, sccSizes, &numSCCs, &largestSCCIndex, largestSCCSize);
		}
	}

	// Extract the largest SCC
	int* largestSCC = (int*)malloc((*largestSCCSize) * sizeof(int));
	for (int i = 0; i < *largestSCCSize; i++) {
		largestSCC[i] = sccVertices[largestSCCIndex][i];
	}

	// Free memory for auxiliary data structures
	for (int i = 0; i < numSCCs; i++) {
		free(sccVertices[i]);
	}
	free(sccVertices);
	free(disc);
	free(low);
	free(stackMember);
	free(stack);
	free(sccSizes);

	return largestSCC;
}
// csv export
void csvViz(int run, double** results, int algCount, char** algNames, double factor, int V, int E, int wccV, int wccE, int sccV, int sccE){

	for (int i = 0; i < algCount; ++i) {
		FILE *csvFile = fopen("../results/Turkdata.csv", "a");
		fprintf(csvFile, "%s, %0.3lf, %d, %d, %0.2lf, %d, %d, %d, %d\n", algNames[i], results[run][i], V, E, factor, wccV, wccE, sccV, sccE);
		fclose(csvFile);
	}
}
void csvGeneric(char* filename, int run, double** results, int algCount, char** algNames, double factor, int V, int E, int wccV, int wccE, int sccV, int sccE){
	for (int i = 0; i < algCount; ++i) {
		FILE *csvFile = fopen(filename, "a");
		fprintf(csvFile, "%s, %0.3lf, %d, %d, %0.2lf, %d, %d, %d, %d\n", algNames[i], results[run][i], V, E, factor, wccV, wccE, sccV, sccE);
		fclose(csvFile);
	}
}
// for printResults
int compare(const void *a, const void *b) {
	double diff = ((Entry *)a)->time - ((Entry *)b)->time;
	return (diff > 0) - (diff < 0); // Returns -1, 0, or 1
}
void printResults(double** results, int algCount, char** algNames)
{

	Entry *entries = (Entry *)malloc(algCount * sizeof(Entry));
	for (int i=0; i<algCount; i++) {
		entries[i].name = algNames[i];
		entries[i].time = results[i][0];
		//printf("%s: %f ( min: %f max: %f ) \n", algNames[i], results[i][0], results[i][1], results[i][2]);
	}
	qsort(entries, algCount, sizeof(Entry), compare);
	//printf("Sorted entries by time:\n");
	for (int i = 0; i < algCount; i++) {
		printf("%s: %.3lf\n", entries[i].name, entries[i].time);
	}
	printf("\n");

	free(entries);
}

void getGraphDataAndWriteCsv(char* filename, float** graph, int n, int run, double** results, int algCount, char** algNames, double* edgeFactors, int z) {
	int graphEdges = countEdges(graph, n);

	int* histogram = graphHistogram(graph, n, false);
	int max = 0; // size of largest WCC

	float** WCCgraph = extractMaxComp(graph, histogram, n, &max);
	int WCCedges = countEdges(WCCgraph, max);

	int largestSCCSize = 0;
	int* largestSCC = findLargestSCC(graph, n, &largestSCCSize);
	float** SCCgraph = extractSubgraph(graph, largestSCC, largestSCCSize);
	int SCCedges = countEdges(SCCgraph, largestSCCSize);

	//csvViz(run, results, algCount, algNames, edgeFactors[z], n, graphEdges, max, WCCedges, largestSCCSize, SCCedges);
	csvGeneric(filename, run, results, algCount, algNames, edgeFactors[z], n, graphEdges, max, WCCedges, largestSCCSize, SCCedges);
	free(histogram);
	freeGraph(WCCgraph, max);
	freeGraph(SCCgraph, largestSCCSize);
	free(largestSCC);
}

void RunTurk2Test(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }
	printf("RunTurk2Test algorithm count: %d\n", algCount);
    int sizes[] = {2048, 4096};
    double edgeFactors[] = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0, 1.1};

	FILE *csvFile = fopen("../results/Turkdata.csv", "w");
	fprintf(csvFile, "algorithm, time, V, E, exponent, wccV, wccE, sccV, sccE\n");
	fclose(csvFile);

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        for (int s = 0; s< sizeof(edgeFactors) / sizeof(edgeFactors[0]); s++)
        {
            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {

                // Generate graph.
                float** graph = TurkGraph(n, edgeFactors[s]);
            	int graphEdges = countEdges(graph, n);

                int mStar = 0;
                results[run] = RunTest(graph, n, algs, &mStar);

            	int* histogram = graphHistogram(graph, n, false);
            	int max = 0; // size of largest WCC
            	float** WCCgraph = extractMaxComp(graph, histogram, n, &max);
            	int WCCedges = countEdges(WCCgraph, max);

            	int largestSCCSize = 0;
            	int* largestSCC = findLargestSCC(graph, n, &largestSCCSize);
            	float** SCCgraph = extractSubgraph(graph, largestSCC, largestSCCSize);
            	int SCCedges = countEdges(SCCgraph, largestSCCSize);

            	csvViz(run, results, algCount, algNames, edgeFactors[s], n, graphEdges, max, WCCedges, largestSCCSize, SCCedges);

                // FILE *csvFile = fopen("data.csv", "a");
                // fprintf(csvFile, "Graph (e, N, m)\n");
                // fprintf(csvFile, "%0.2f, %d, %0.0lf\n", edgeFactors[s], n, pow((double)n, edgeFactors[s]));
                // fclose(csvFile);
                // histExport(histogram, n);
                // graphHistSCC(graph, n, 0);
                //
                // csvFile = fopen("data.csv", "a");
                // fprintf(csvFile, "Algorithms (name, time)\n");
                // for (int i = 0; i < algCount; ++i) {  // then fprintf results
                //     fprintf(csvFile, "%s, %0.3lf\n", algNames[i], results[run][i]);
                // }
                // fprintf(csvFile, "\n");
                // fclose(csvFile);
                //analizeGraph(maxComp, max);
                //for (int i = 0; i < algCount; ++i) { // print every recorded time
                    //printf("%s, %lf\n", algNames[i], results[run][i]);
                //}

            	free(histogram);
            	freeGraph(graph, n);
            	freeGraph(WCCgraph, max);
            	freeGraph(SCCgraph, largestSCCSize);

            	free(largestSCC);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            double edges = pow((double)n, edgeFactors[s]);
            printf("Eksponent %0.2lf (%0.0f edges): \n===========\n", edgeFactors[s], edges);
            printResults(avgResults, algCount, algNames);

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
        }
    }
}


// GraphData -----------------------------------------------------------------------------------------------------------
static int countOutgoingEdges(float** graph, const int n, const int* sccComponent, int *tailCount, int *headCount, int C) {
	int outgoingEdges = 0;
	bool* tails = (bool*)calloc(n, sizeof(bool));
	bool** heads = (bool**)malloc(C * sizeof(bool*));
	for (int i = 0; i < C; i++)
		heads[i] = (bool*)calloc(n, sizeof(bool));

	*tailCount = 0;
	*headCount = 0;

	for (int i = 0; i < n; ++i) {
		for (int j = 0; j < n; ++j) {
			if (i != j && graph[i][j] != INFINITY && sccComponent[i] != sccComponent[j])
			{
				outgoingEdges++;
				if (!tails[i])
				{
					tails[i] = true;
					(*tailCount)++;
				}
				if (!heads[sccComponent[i]][j])
				{
					heads[sccComponent[i]][j] = true;
					(*headCount)++;
				}
			}
		}
	}

	free(tails);

	for (int i = 0; i < C; i++) free(heads[i]);
	free(heads);

	return outgoingEdges;
}

static int* SCChistogram(SCCResult SCCs, int n) {
	int* sizes = (int*)calloc(SCCs.sccCount, sizeof(int));
	int* histogram = (int*)calloc(n + 1, sizeof(int));

	for (int i = 0; i < n; ++i) {
		sizes[SCCs.component[i]]++;
	}

	for (int i = 0; i < SCCs.sccCount; ++i) {
		histogram[sizes[i]]++;
	}

	free(sizes);
	return histogram;
}

void writeGraphDataHeader(FILE *csvFile)
{
	fprintf(csvFile,
			"N,"
			"E,"
			"density(%%),"
			"outgoingEdges,"
			"tails,"
			"heads,"
			"wccCount,"
			"largestWCCVertices,"
			"largestWCCEdges,"
			"sccCount,"
			"largestSCCVertices,"
			"largestSCCEdges\n");
}

void writeGraphDataToCSV(FILE *csvFile, const GraphData *data)
{
	if (csvFile == NULL || data == NULL) {
		return;
	}

	fprintf(csvFile, "%d,%d,%.1lf,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
			data->vertices,
			data->edges,
			data->density,
			data->outgoingEdges,
			data->tailCount,
			data->headCount,
			data->wccCount,
			data->largestWCCVertices,
			data->largestWCCEdges,
			data->sccCount,
			data->largestSCCVertices,
			data->largestSCCEdges);
}

GraphData getGraphData(float** graph, int n) {
	GraphData data;
	data.vertices = n;
	data.edges = countEdges(graph, n);
	data.density = (n > 1) ? (double)data.edges / ((double)n * (double)(n - 1))*100.0 : 0.0;
	data.wccHistogram = graphHistogram(graph, n, false);
	data.wccCount = WCCcount(data.wccHistogram, n);
	data.largestWCCVertices = 0;
	float** WCCgraph = extractMaxComp(graph, data.wccHistogram, n, &data.largestWCCVertices);
	data.largestWCCEdges = countEdges(WCCgraph, data.largestWCCVertices);

	data.largestSCCVertices = 0;
	int* largestSCC = findLargestSCC(graph, n, &data.largestSCCVertices);
	float** SCCgraph = extractSubgraph(graph, largestSCC, data.largestSCCVertices);
	data.largestSCCEdges = countEdges(SCCgraph, data.largestSCCVertices);

	SCCResult SCCs = findSCCs(graph, n);
	data.sccCount = SCCs.sccCount;
	data.sccHistogram = SCChistogram(SCCs, n);
	data.outgoingEdges = countOutgoingEdges(graph, n, SCCs.component, &data.tailCount, &data.headCount, SCCs.sccCount);

	freeGraph(WCCgraph, data.largestWCCVertices);
	freeGraph(SCCgraph, data.largestSCCVertices);
	free(largestSCC);
	free(SCCs.component);
	free(SCCs.nextInComponent);
	free(SCCs.firstInComponent);

	return data;
}

void freeGraphData(GraphData* data) {
	if (!data) return;

	free(data->wccHistogram);
	data->wccHistogram = NULL;
	free(data->sccHistogram);
	data->sccHistogram = NULL;
}
// ---------------------------------------------------------------------------------------------------------------------

// From master thesis LogGraph
void RunlgNTest(AlgorithmFP* algs, char** algNames)
{
    printf("\n\n\n=====lgN TEST=====\n\n");
	FILE *csvFile = fopen("../results/lgN.csv", "w");
    fprintf(csvFile,"time(s), algorithm, edges, vertices \n");
    //fprintf(csvFile, "Size : quantity\n");
    if (csvFile == NULL) {
        perror("Failed to open file");
    }
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }
    int sizes[] = {2048, 4096};

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        double edgeFactors[] = {n, 2*n, 4*n , n*log2(n), 2*n*log2(n)};
        for (int s = 0; s< sizeof(edgeFactors) / sizeof(edgeFactors[0]); s++)
        {
            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {
                // Generate graph.
                float** graph = GeneratelgNGraph(n, edgeFactors[s]);
                /*if (!isConnected(graph, n))
                {
                    printf("Graph not connected");
                }*/


                //FILE *csvFile = fopen("data.csv", "a");
                //fprintf(csvFile, ",%d, %d,",(int)(edgeFactors[s]*n) , n);
                //fclose(csvFile);

                int mStar = 0;
                results[run] = RunTest(graph, n, algs, &mStar);
                for (int x = 0; x < algCount; ++x) {
                    fprintf(csvFile,"%lf, %s, %0.1lf, %d \n", results[run][x], algNames[x], edgeFactors[s], n);
                    //printf("%s, %lf, %0.1lf, %0.1f, %d\n", algNames[x], results[run][x], edgeFactors[s], razmerje, n);
                }
                for (int i=0; i<n; i++)
                {
                    free(graph[i]);
                }
                free(graph);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            //double edges = edgeFactors[s];
            printf("Edges: %0.0lf \n===========\n", edgeFactors[s]);
            printResults(avgResults, algCount, algNames);
            csvExport(avgResults, algCount, algNames, edgeFactors[s], n); // average results export to data.csv

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
        }
    }
}

// From master thesis CompGraph
void RunN_1Test(AlgorithmFP* algs, char** algNames) {
    printf("\n\n\n=====N-1 TEST=====\n\n");
    FILE *csvFile = fopen("N1.csv", "a");
    fprintf(csvFile,"time (s), algorithm, alpha, ratio, vertices \n");
    //fprintf(csvFile, "Size : quantity\n");
    if (csvFile == NULL) {
        perror("Failed to open file");
    }

    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
    }
    int sizes[] = {2048, 4096};
    double edgeFactors[] = {0.5, 0.6, 0.7, 0.8, 0.9, 1.0};
    float ratios[] = {0.5, 0.6, 0.7, 0.8, 0.9};

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        for (int r = 0; r < sizeof(ratios) / sizeof(ratios[0]); ++r) {
            float razmerje = ratios[r];

            printf("Ratio = %0.0f:%0.0f \n", razmerje*100, (1-razmerje)*100);
            for (int s = 0; s< sizeof(edgeFactors) / sizeof(edgeFactors[0]); s++)
            {
                double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
                for (int run=0; run<TESTRUNS; run++)
                {
                    // Generate graph.
                    float** graph = N1Graph(n, razmerje, edgeFactors[s]);
                    int* hist = graphHistogram(graph, n, 0);
                    for (int h = n; h > 0; h--) {
                        if (hist[h] > 0) {
                            //fprintf(csvFile, "%d: %dx, ", h, hist[h]);
                        }
                    }
                    //fprintf(csvFile, "\n");
                    free(hist);


                    /*if (!isConnected(graph, n))
                    {
                        printf("Graph not connected");
                    }*/

                    int mStar = 0;
                    results[run] = RunTest(graph, n, algs, &mStar);


                    for (int x = 0; x < algCount; ++x) {
                        fprintf(csvFile,"%lf, %s, %0.1lf, %0.0f:%0.0f, %d\n", results[run][x], algNames[x], edgeFactors[s], razmerje*100, (1-razmerje)*100, n);
                        //printf("%s, %lf, %0.1lf, %0.1f, %d\n", algNames[x], results[run][x], edgeFactors[s], razmerje, n);
                    }
                    //fprintf(csvFile, "\n");

                    for (int i=0; i<n; i++)
                    {
                        free(graph[i]);
                    }
                    free(graph);
                }

                double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
                //double edges = pow((double)n, edgeFactors[s]);
                printf("Alpha %0.1lf: \n===========\n", edgeFactors[s]);
                printResults(avgResults, algCount, algNames);
                //csvExport(avgResults, algCount, algNames, edgeFactors[s], n); // average results export to data.csv

                for (int f=0; f<TESTRUNS; f++)
                {
                    free(results[f]);
                }
                free(results);
                for (int f=0; f<algCount; f++)
                {
                    free(avgResults[f]);
                }
                free(avgResults);
            }
        }

    }
}

void RunSparseTest(AlgorithmFP* algs, char** algNames)
{
	printf("\n\n\n=====CYCLE+ER TEST=====\n\n");
	int algCount = 0;

	for (int i=0; algs[i] != NULL; i++) {
		printf("%s\n", algNames[i]);
		algCount++;
		printf("%d\n", algCount);
	}
	printf("Here");

	double edgeFactors[] = {1.1, 1.2, 1.3, 1.4, 1.5, 1.6, 1.7, 1.8, 1.9};
	for (int s = 0; s< sizeof(edgeFactors) / sizeof(edgeFactors[0]); s++)
	{
		int n = 2048;

		double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
		for (int run=0; run<TESTRUNS; run++)
		{
			// Generate graph.
			float** graph = CycleX(n, (int)(pow(n, edgeFactors[s]))-n); // substract the number of edges needed to construct a cycle
			int mStar = 0;
			results[run] = RunTest(graph, n, algs, &mStar);
			for (int i=0; i<n; i++)
			{
				free(graph[i]);
			}
			free(graph);
		}
		double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
		printf("|E| = %d^%.1f: \n===========\n",n ,edgeFactors[s]);
		printResults(avgResults, algCount, algNames);

		for (int f=0; f<TESTRUNS; f++)
		{
			free(results[f]);
		}
		free(results);
		for (int f=0; f<algCount; f++)
		{
			free(avgResults[f]);
		}
		free(avgResults);
	}
}

// ---------------------------------------------------------------------------------------------------------------------
// For paper Seminar 2
void csvVizSCCs(int run, double** results, int algCount, char** algNames, float factor, int V, int E, int SCCs, int sccV, int sccE){
	// Gostota za usmerjen graf brez zank: E / (V*(V-1))
	double denom = (double)V * (double)(V - 1);
	double density = (denom != 0.0) ? (double)E / denom : 0.0;
	for (int i = 0; i < algCount; ++i) {
		FILE *csvFile = fopen("../results/logSCCdata.csv", "a");
		fprintf(csvFile, "%s, %0.3lf, %d, %d, %.1lf%%, %.1f, %d, %d, %d\n", algNames[i], results[run][i], V, E, density*100, factor, SCCs, sccV, sccE);
		fclose(csvFile);
	}
}

void RunSCCsTest(AlgorithmFP* algs, char** algNames)
{
	printf("\n--- log SCCs test ---\n\n");
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }

    int sizes[] = {2048, 4096, 8192};
    float edgeFactors[] = {0.1, 1.0, 1.5, 1.9};

	FILE *csvFile = fopen("../results/logSCCdata.csv", "w");
	fprintf(csvFile, "algorithm, time, V, E, %% of possible edges, density, SCCs, sccV, sccE\n");
	fclose(csvFile);

    for (int z = 0; z < sizeof(sizes) / sizeof(sizes[0]); ++z) {
        int n = sizes[z];
        printf("N = %d \n", n);
        for (int s = 0; s< sizeof(edgeFactors) / sizeof(edgeFactors[0]); s++)
        {
            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {
                // Generate graph.
                float** graph = sccGraphAlfa(n, edgeFactors[s]);
            	int graphEdges = countEdges(graph, n);

                int mStar = 0;
                results[run] = RunTest(graph, n, algs, &mStar);

            	int* histogram = graphHistogram(graph, n, false);
            	int WCCs = WCCcount(histogram, n);
            	int max = 0; // size of largest WCC
            	float** WCCgraph = extractMaxComp(graph, histogram, n, &max);
            	int WCCedges = countEdges(WCCgraph, max);

            	int largestSCCSize = 0;
            	int* largestSCC = findLargestSCC(graph, n, &largestSCCSize);
            	float** SCCgraph = extractSubgraph(graph, largestSCC, largestSCCSize);
            	int SCCedges = countEdges(SCCgraph, largestSCCSize);
				SCCResult res = findSCCs(graph, n);
            	int SCCs = res.sccCount;

            	csvVizSCCs(run, results, algCount, algNames, edgeFactors[s], n, graphEdges, SCCs, largestSCCSize, SCCedges);

                // FILE *csvFile = fopen("data.csv", "a");
                // fprintf(csvFile, "Graph (e, N, m)\n");
                // fprintf(csvFile, "%0.2f, %d, %0.0lf\n", edgeFactors[s], n, pow((double)n, edgeFactors[s]));
                // fclose(csvFile);
                // histExport(histogram, n);
                // graphHistSCC(graph, n, 0);
                //
                // csvFile = fopen("data.csv", "a");
                // fprintf(csvFile, "Algorithms (name, time)\n");
                // for (int i = 0; i < algCount; ++i) {  // then fprintf results
                //     fprintf(csvFile, "%s, %0.3lf\n", algNames[i], results[run][i]);
                // }
                // fprintf(csvFile, "\n");
                // fclose(csvFile);
                //analizeGraph(maxComp, max);
                //for (int i = 0; i < algCount; ++i) { // print every recorded time
                    //printf("%s, %lf\n", algNames[i], results[run][i]);
                //}

            	free(histogram);
            	freeGraph(graph, n);
            	freeGraph(WCCgraph, max);
            	freeGraph(SCCgraph, largestSCCSize);

            	free(largestSCC);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            double edges = pow((double)n, edgeFactors[s]);
            printf("Density %0.1lf \n===========\n", edgeFactors[s]);
            printResults(avgResults, algCount, algNames);

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
        }
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// Test for Complete graphs
void RunCompleteTest(AlgorithmFP* algs, char** algNames, int unit)
{
	printf("\n\n\n=====COMPLETE TEST=====\n\n");

	int algCount = 0;
	for (int i=0; algs[i] != NULL; i++)
		algCount++;
	int sizes[] = {512, 1024, 2048, 4096};

	for (int s = 0; s < sizeof(sizes) / sizeof(sizes[0]); s++)
	{
		int n = sizes[s];

		double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
		for (int run=0; run<TESTRUNS; run++)
		{
			// Generate graph.
			float** graph = GenerateCompleteGraph(n, unit);

			int mStar = 0;
			results[run] = RunTest(graph, n, algs, &mStar);
			for (int i=0; i<n; i++)
			{
				free(graph[i]);
			}
			free(graph);
		}
		double** avgResults = ComputeAvg(results, algCount, TESTRUNS);

		printf("Size %d: \n===========\n", n);
		printResults(avgResults, algCount, algNames);

		for (int f=0; f<TESTRUNS; f++)
		{
			free(results[f]);
		}
		free(results);
		for (int f=0; f<algCount; f++)
		{
			free(avgResults[f]);
		}
		free(avgResults);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

void RunGnutellaTest(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }

	const char* folder = "../p2p-Gnutella";
	int fileCount = 0;

	char** files = getFilenamesInFolder(folder, &fileCount);
	if (!files) {
		printf("No files found.\n");
	}

	printf("Gnutella test -> algorithm count: %d\n", algCount);

    double edgeFactors[] = {4, 5, 6, 8, 9, 24, 25, 30, 31};
	char* output = "../results/Gnutella.csv";
	FILE *csvFile = fopen(output, "w");
	fprintf(csvFile, "algorithm, time, V, E, id, wccV, wccE, sccV, sccE\n");
	fclose(csvFile);

    for (int z = 0; z < fileCount; ++z) {

            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {
            	char filename[512];
            	strcpy(filename, folder);
            	strcat(filename, "/");
            	strcat(filename, files[z]);
            	//printf("Running Gnutella test -> %s\n", filename);
            	Graf graf = GenerateGraphGnutella(filename);
                float** graph = graf.matrix;
            	int n = graf.size;
            	printf("N = %d \n", n);
            	//int graphEdges = countEdges(graph, n);

                int mStar = 0;
            	//printf("Running algorithms... \n");
                results[run] = RunTest(graph, n, algs, &mStar);
				//printf("Histogram\n");
            	//int* histogram = graphHistogram(graph, n, false);
            	//int max = 0; // size of largest WCC
            	//printf("Getting WCC graph");
            	//float** WCCgraph = extractMaxComp(graph, histogram, n, &max);
            	//int WCCedges = countEdges(WCCgraph, max);
            	//printf("Getting SCC graph");
            	//int largestSCCSize = 0;
            	//int* largestSCC = findLargestSCC(graph, n, &largestSCCSize);
            	//float** SCCgraph = extractSubgraph(graph, largestSCC, largestSCCSize);
            	//int SCCedges = countEdges(SCCgraph, largestSCCSize);
				//printf("Writing to .csv");
            	//csvViz(run, results, algCount, algNames, edgeFactors[z], n, graphEdges, max, WCCedges, largestSCCSize, SCCedges);
				getGraphDataAndWriteCsv(output, graph, n, run, results, algCount, algNames, edgeFactors, z);
            	//free(histogram);
            	freeGraph(graf.matrix, graf.size);
            	//freeGraph(WCCgraph, max);
            	//freeGraph(SCCgraph, largestSCCSize);
            	//free(largestSCC);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            //double edges = pow((double)n, edgeFactors[z]);
            //printf("Eksponent %0.2lf (%0.0f edges): \n===========\n", edgeFactors[z], edges);
            printResults(avgResults, algCount, algNames);

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
    }

	for (int i = 0; i < fileCount; ++i) {
		printf("Found file: %s/%s\n", folder, files[i]);
		free(files[i]);  // Free each string
	}
	free(files);  // Free the array
}

void RunTxtTest(AlgorithmFP* algs, char** algNames)
{
    int algCount = 0;
    for (int i=0; algs[i] != NULL; i++){
        algCount++;
        //printf("%s\n", algNames[i]);
    }

	const char* folder = "../p2p-Gnutella/SimpleText";
	int fileCount = 0;

	char** files = getFilenamesInFolder(folder, &fileCount);
	if (!files) {
		printf("No files found.\n");
	}

	printf("Simple text test -> algorithm count: %d\n", algCount);

    double edgeFactors[] = {4, 5, 6, 8, 9, 24, 25, 30, 31};
	char* output = "../results/SimpleText.csv";
	FILE *csvFile = fopen(output, "w");
	fprintf(csvFile, "algorithm, time, V, E, id, wccV, wccE, sccV, sccE\n");
	fclose(csvFile);

    for (int z = 0; z < fileCount; ++z) {

            double** results = (double**)malloc(sizeof(double*)*TESTRUNS);
            for (int run=0; run<TESTRUNS; run++)
            {
            	char filename[512];
            	strcpy(filename, folder);
            	strcat(filename, "/");
            	strcat(filename, files[z]);
            	//printf("Running Gnutella test -> %s\n", filename);
            	Graf graf = GenerateGraphTxt(filename);
                float** graph = graf.matrix;
            	int n = graf.size;
            	printf("N = %d \n", n);

                int mStar = 0;
            	//printf("Running algorithms... \n");
                results[run] = RunTest(graph, n, algs, &mStar);

				getGraphDataAndWriteCsv(output, graph, n, run, results, algCount, algNames, edgeFactors, z);
            	freeGraph(graf.matrix, graf.size);
            }
            double** avgResults = ComputeAvg(results, algCount, TESTRUNS);
            printResults(avgResults, algCount, algNames);

            for (int f=0; f<TESTRUNS; f++)
            {
                free(results[f]);
            }
            free(results);
            for (int f=0; f<algCount; f++)
            {
                free(avgResults[f]);
            }
            free(avgResults);
    }

	for (int i = 0; i < fileCount; ++i) {
		printf("Found file: %s/%s\n", folder, files[i]);
		free(files[i]);  // Free each string
	}
	free(files);  // Free the array
}



