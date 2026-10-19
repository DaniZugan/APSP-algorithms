#include "Tree.h"
#include "../utils.h"

#include <climits>
#include <cstdlib>

void SimpleStack_create(SimpleStack* t, int maxSize)
{
	t->size = 0;
	t->maxSize = maxSize;
	t->container = (int*)malloc(sizeof(int)*maxSize);
}
void SimpleStack_push(SimpleStack* t, int a) { t->container[t->size++]=a; }
int SimpleStack_top(SimpleStack* t) { if (t->size>0) return t->container[t->size-1]; else return INT_MAX; }
void SimpleStack_pop(SimpleStack* t) { t->size--; }
void SimpleStack_destroy(SimpleStack* t)
{
	free(t->container);
}
void TreeStack_create(TreeStack* t, int maxSize)
{
	t->size = 0;
	t->container = (SimpleTree**)malloc(sizeof(SimpleTree*)*maxSize);
}
void TreeStack_push(TreeStack* t, SimpleTree* elt) { t->container[t->size++]=elt; }
SimpleTree* TreeStack_top(TreeStack* t) { if (t->size>0) return t->container[t->size-1]; else return NULL; }
void TreeStack_pop(TreeStack* t) { t->size--; }
void TreeStack_destroy(TreeStack* t)
{
	free(t->container);
}

// original Tree
float** FastHourglassHalf(float** graph, int n)
{
	long TreeOp = 0;
	float** W = (float**)malloc(sizeof(float*)*n);
	for (int i=0; i<n; i++)
	{
		W[i] = (float*)malloc(sizeof(float)*n);
		for (int j=0; j<n; j++)
		{
			W[i][j] = graph[i][j];
		}
	}

	int** last = (int**)malloc(sizeof(int*)*n); // Last node on path (without destination)
	int* dfs = (int*)malloc(sizeof(int)*(n-1)); // DFS-order permutation of the vertices
	int* skip = (int*)malloc(sizeof(int)*(n-1)); // Skip array to skip subtrees in the DFS traversal
	int* idfs = (int*)malloc(sizeof(int)*n); // inverse DFS permutation, used when building skip
	float* Wk = (float*)malloc(sizeof(float)*(n-1)); // W[k][] distances rearranged to fit permutation order

	for (int i=0; i<n; i++)
	{
		last[i] = (int*)malloc(sizeof(int)*n);
		for (int j=0; j<n; j++)
		{
			last[i][j] = i;
		}
	}
	SimpleTree* outTrees = (SimpleTree*)malloc(sizeof(SimpleTree)*n);
	for (int i=0; i<n; i++)
		outTrees[i].label = i;

	TreeStack ts;
	TreeStack_create(&ts, n);

	for (int k=0; k<n; k++)
	{
		for (int t=0; t<n; t++) // set up tree
		{
			outTrees[t].nextSibling = NULL;
			outTrees[t].firstChild = NULL;
		}
		for (int t=0; t<n; t++) // set up tree
		{
			if (t != k && isfinite_cust(W[k][t])) // Nothing to do for t=k or non-existing edges
			{
				int parent = last[k][t];
				if (outTrees[parent].firstChild == NULL)
					outTrees[parent].firstChild = &(outTrees[t]);
				else
				{
					outTrees[t].nextSibling = outTrees[parent].firstChild;
					outTrees[parent].firstChild = &(outTrees[t]);
				}
			}
		}

		// Traverse tree to build the DFS order

		SimpleTree* toVisit = outTrees[k].firstChild;
		while (toVisit != NULL)
		{
			//add it to our stack to explore
			TreeStack_push(&ts, toVisit);

			toVisit = toVisit->nextSibling;
		}
		// Now traverse as DFS
		int c = 0;
		while (ts.size != 0)
		{
			toVisit = TreeStack_top(&ts);
			TreeStack_pop(&ts);
			dfs[c] = toVisit->label;
			idfs[toVisit->label] = c;
			if (ts.size > 0)
				skip[c] = TreeStack_top(&ts)->label; // next entry on stack is where we have to skip
			else
				skip[c] = -1; // skip to end
			c++;
			toVisit = toVisit->firstChild;
			while (toVisit != NULL)
			{
				//add it to our stack to explore
				TreeStack_push(&ts, toVisit);
				toVisit = toVisit->nextSibling;
			}
		}

		// Now fix the skip array, since it uses vertex ids, but needs dfs offsets
		for (int v=0; v<c; v++)
		{
			if (skip[v] == -1)
				skip[v] = n-1;
			else
				skip[v] = idfs[skip[v]];
		}

		for (int v=0; v<c; v++)
			Wk[v] = W[k][dfs[v]];

		// Using only bottom half, so loop over all incoming vertices.

		for (int i=0; i<n; i++)
		{
			float Wik = W[i][k];
			if (!isfinite_cust(Wik))
				continue;

			float* Wi = W[i];

			// This loop represents pretty much the entire running time (>>90%)
			// Most likely slowdown is with W[i][j] being essentially random access, since Wk is fixed anyway.

			if (i <= k) // Fastpath
			{
				for (int v=0; v<c;)
				{
					int j = dfs[v];
					TreeOp++;
					if (Wik + Wk[v] < Wi[j])
					{
						Wi[j] = Wik + Wk[v];
						v++;
					}
					else
						v = skip[v]; // we can skip the subtree since we didn't improve our distance
				}
			}
			else
			{
				for (int v=0; v<c;)
				{
					int j = dfs[v];
					TreeOp++;
					if (Wik + Wk[v] < Wi[j])
					{
						Wi[j] = Wik + Wk[v];
						last[i][j] = last[k][j];

						v++;
					}
					else
						v = skip[v]; // we can skip the subtree since we didn't improve our distance
				}
			}
		}
	}


	for (int i=0; i<n; i++)
		free(last[i]);
	free(last);
	free(dfs);
	free(idfs);
	free(skip);
	free(Wk);
	free(outTrees);
	TreeStack_destroy(&ts);

	return W;
}

// overwrites input matrix
void ModTree(float** W, int n)
{
    long TreeOp = 0;

    int** last = (int**)malloc(sizeof(int*)*n); // Last node on path (without destination)
    int* dfs = (int*)malloc(sizeof(int)*(n-1)); // DFS-order permutation of the vertices
    int* skip = (int*)malloc(sizeof(int)*(n-1)); // Skip array to skip subtrees in the DFS traversal
    int* idfs = (int*)malloc(sizeof(int)*n); // inverse DFS permutation, used when building skip
    float* Wk = (float*)malloc(sizeof(float)*(n-1)); // W[k][] distances rearranged to fit permutation order

    for (int i=0; i<n; i++)
    {
        last[i] = (int*)malloc(sizeof(int)*n);
        for (int j=0; j<n; j++)
        {
            last[i][j] = i;
        }
    }
    SimpleTree* outTrees = (SimpleTree*)malloc(sizeof(SimpleTree)*n);
    for (int i=0; i<n; i++)
        outTrees[i].label = i;

    TreeStack ts;
    TreeStack_create(&ts, n);

    for (int k=0; k<n; k++)
    {
        for (int t=0; t<n; t++) // set up tree
        {
            outTrees[t].nextSibling = NULL;
            outTrees[t].firstChild = NULL;
        }
        for (int t=0; t<n; t++) // set up tree
        {
            if (t != k && isfinite_cust(W[k][t])) // Nothing to do for t=k or non-existing edges
            {
                int parent = last[k][t];
                if (outTrees[parent].firstChild == NULL)
                    outTrees[parent].firstChild = &(outTrees[t]);
                else
                {
                    outTrees[t].nextSibling = outTrees[parent].firstChild;
                    outTrees[parent].firstChild = &(outTrees[t]);
                }
            }
        }

        // Traverse tree to build the DFS order

        SimpleTree* toVisit = outTrees[k].firstChild;
        while (toVisit != NULL)
        {
            //add it to our stack to explore
            TreeStack_push(&ts, toVisit);

            toVisit = toVisit->nextSibling;
        }
        // Now traverse as DFS
        int c = 0;
        while (ts.size != 0)
        {
            toVisit = TreeStack_top(&ts);
            TreeStack_pop(&ts);
            dfs[c] = toVisit->label;
            idfs[toVisit->label] = c;
            if (ts.size > 0)
                skip[c] = TreeStack_top(&ts)->label; // next entry on stack is where we have to skip
            else
                skip[c] = -1; // skip to end
            c++;
            toVisit = toVisit->firstChild;
            while (toVisit != NULL)
            {
                //add it to our stack to explore
                TreeStack_push(&ts, toVisit);
                toVisit = toVisit->nextSibling;
            }
        }

        // Now fix the skip array, since it uses vertex ids, but needs dfs offsets
        for (int v=0; v<c; v++)
        {
            if (skip[v] == -1)
                skip[v] = n-1;
            else
                skip[v] = idfs[skip[v]];
        }

        for (int v=0; v<c; v++)
            Wk[v] = W[k][dfs[v]];

        // Using only bottom half, so loop over all incoming vertices.

        for (int i=0; i<n; i++)
        {
            float Wik = W[i][k];
            if (!isfinite_cust(Wik))
                continue;

            float* Wi = W[i];

            // This loop represents pretty much the entire running time (>>90%)
            // Most likely slowdown is with W[i][j] being essentially random access, since Wk is fixed anyway.

            if (i <= k) // Fastpath
            {
                for (int v=0; v<c;)
                {
                    int j = dfs[v];
                    TreeOp++;
                    if (Wik + Wk[v] < Wi[j])
                    {
                        Wi[j] = Wik + Wk[v];
                        v++;
                    }
                    else
                        v = skip[v]; // we can skip the subtree since we didn't improve our distance
                }
            }
            else
            {
                for (int v=0; v<c;)
                {
                    int j = dfs[v];
                    TreeOp++;
                    if (Wik + Wk[v] < Wi[j])
                    {
                        Wi[j] = Wik + Wk[v];
                        last[i][j] = last[k][j];

                        v++;
                    }
                    else
                        v = skip[v]; // we can skip the subtree since we didn't improve our distance
                }
            }
        }
    }
    for (int i=0; i<n; i++)
        free(last[i]);
    free(last);
    free(dfs);
    free(idfs);
    free(skip);
    free(Wk);
    free(outTrees);
    TreeStack_destroy(&ts);
}

// overwrites input W and returns last for VertexRelax (solves APSP inside each component)
int** VertexTree(float** W, int n)
{
    long TreeOp = 0;

    int** last = (int**)malloc(sizeof(int*)*n); // Last node on path (without destination)
    int* dfs = (int*)malloc(sizeof(int)*(n-1)); // DFS-order permutation of the vertices
    int* skip = (int*)malloc(sizeof(int)*(n-1)); // Skip array to skip subtrees in the DFS traversal
    int* idfs = (int*)malloc(sizeof(int)*n); // inverse DFS permutation, used when building skip
    float* Wk = (float*)malloc(sizeof(float)*(n-1)); // W[k][] distances rearranged to fit permutation order

    for (int i=0; i<n; i++)
    {
        last[i] = (int*)malloc(sizeof(int)*n);
        for (int j=0; j<n; j++)
        {
            last[i][j] = i;
        }
    }
    SimpleTree* outTrees = (SimpleTree*)malloc(sizeof(SimpleTree)*n);
    for (int i=0; i<n; i++)
        outTrees[i].label = i;

    TreeStack ts;
    TreeStack_create(&ts, n);

    for (int k=0; k<n; k++)
    {
        for (int t=0; t<n; t++) // set up tree
        {
            outTrees[t].nextSibling = NULL;
            outTrees[t].firstChild = NULL;
        }
        for (int t=0; t<n; t++) // set up tree
        {
            if (t != k && isfinite_cust(W[k][t])) // Nothing to do for t=k or non-existing edges
            {
                int parent = last[k][t];
                if (outTrees[parent].firstChild == NULL)
                    outTrees[parent].firstChild = &(outTrees[t]);
                else
                {
                    outTrees[t].nextSibling = outTrees[parent].firstChild;
                    outTrees[parent].firstChild = &(outTrees[t]);
                }
            }
        }

        // Traverse tree to build the DFS order

        SimpleTree* toVisit = outTrees[k].firstChild;
        while (toVisit != NULL)
        {
            //add it to our stack to explore
            TreeStack_push(&ts, toVisit);

            toVisit = toVisit->nextSibling;
        }
        // Now traverse as DFS
        int c = 0;
        while (ts.size != 0)
        {
            toVisit = TreeStack_top(&ts);
            TreeStack_pop(&ts);
            dfs[c] = toVisit->label;
            idfs[toVisit->label] = c;
            if (ts.size > 0)
                skip[c] = TreeStack_top(&ts)->label; // next entry on stack is where we have to skip
            else
                skip[c] = -1; // skip to end
            c++;
            toVisit = toVisit->firstChild;
            while (toVisit != NULL)
            {
                //add it to our stack to explore
                TreeStack_push(&ts, toVisit);
                toVisit = toVisit->nextSibling;
            }
        }

        // Now fix the skip array, since it uses vertex ids, but needs dfs offsets
        for (int v=0; v<c; v++)
        {
            if (skip[v] == -1)
                skip[v] = n-1;
            else
                skip[v] = idfs[skip[v]];
        }

        for (int v=0; v<c; v++)
            Wk[v] = W[k][dfs[v]];

        // Using only bottom half, so loop over all incoming vertices.

        for (int i=0; i<n; i++)
        {
            float Wik = W[i][k];
            if (!isfinite_cust(Wik))
                continue;

            float* Wi = W[i];

            // This loop represents pretty much the entire running time (>>90%)
            // Most likely slowdown is with W[i][j] being essentially random access, since Wk is fixed anyway.

            for (int v=0; v<c;)
                {
                    int j = dfs[v];
                    TreeOp++;
                    if (Wik + Wk[v] < Wi[j])
                    {
                        Wi[j] = Wik + Wk[v];
                        last[i][j] = last[k][j];

                        v++;
                    }
                    else
                        v = skip[v]; // we can skip the subtree since we didn't improve our distance
                }

        }
    }
    //for (int i=0; i<n; i++)
        //free(last[i]);
    //free(last);
    free(dfs);
    free(idfs);
    free(skip);
    free(Wk);
    free(outTrees);
    TreeStack_destroy(&ts);

	return last;
}

float** NullFunction(float** graph, int n) {
	return graph;
}