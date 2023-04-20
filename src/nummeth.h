// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// nummeth.h -- declarations for numerical methods functions

#if !defined( _NUMMETH_H)
#define _NUMMETH_H

#include <list>
#include <stack>

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
int gaussjb( double* a, int n, double* b, int m, int invflg=0);

int secant(	double (*pFunc)( void *pO, double &x), void *pO, double f,
	double eps, double &x1, double &f1, double x2, double f2=DBL_MIN,
	double epsLo=-1.);

int regula(double (*pFunc)(void* pO, double& x), void* pO, double f,
	double eps, double& x1, double xMin, double xMax);


///////////////////////////////////////////////////////////////////////////////
// class DGRAPH: directed graph
///////////////////////////////////////////////////////////////////////////////
template<typename T> class DGRAPH
{
public:
	DGRAPH(int nV, T base = 0) : dg_nV{ nV }, dg_base{ base }
	{	dg_edges.resize(nV);
		dg_visited.resize(nV);
	}
	void dg_Clear() { dg_edges = {}; }
	void dg_AddEdge(T ivFrom, T ivTo)
	{	dg_edges[ivFrom].push_back(ivTo);
	}
	void dg_AddEdges(T ivFrom, T ivTo[])
	{	for (int i = 0; ivTo[i] - dg_base >= 0; i++)
			dg_AddEdge(ivFrom - dg_base, ivTo[i] - dg_base);
	}
	void dg_TopologicalSort( std::vector<T>& vSorted);

private:
	T dg_base;	// 0 or 1 based
	int dg_nV;	// # of verticies
	std::vector< std::list<T>> dg_edges;	// edges
	std::stack< T> dg_stack;				// working stack
	std::vector< bool> dg_visited;			// visited flags
	void dg_TopologicalSortUtil(T v);
};		// class DGRAPH
//=============================================================================
template<typename T> void DGRAPH<T>::dg_TopologicalSortUtil(
	T v)
{
	// Mark the current node as visited.
	dg_visited[v] = true;

	// Recur for all the vertices adjacent to this vertex
	std::list<T>::iterator vItr;
	for (vItr = dg_edges[v].begin(); vItr != dg_edges[v].end(); ++vItr)
		if (!dg_visited[*vItr])
			dg_TopologicalSortUtil(*vItr);

	// Push current vertex to stack which stores result
	dg_stack.push(v);
}	// DGRAPH::dg_topologicalSortUtil
//-----------------------------------------------------------------------------
template<typename T> void DGRAPH<T>::dg_TopologicalSort(
	std::vector< T>& vSorted)
{
	for (auto f : dg_visited)
		f = false;

	// Call the recursive helper function to store topological
	// sort starting from all vertices one by one
	dg_stack = {};
	for (int i = 0; i < dg_nV; i++)
		if (!dg_visited[i])
			dg_TopologicalSortUtil( i);

	vSorted.clear();
	while (!dg_stack.empty())
	{
		vSorted.push_back(dg_stack.top()+dg_base);
		dg_stack.pop();
	}

}	// DGRAPH::dg_TopologicalSort

#if 0
// Driver program to test above functions
int main()
{

	// Create a graph given in the above diagram
	Graph g(6);
	g.addEdge(5, 2);
	g.addEdge(5, 0);
	g.addEdge(4, 0);
	g.addEdge(4, 1);
	g.addEdge(2, 3);
	g.addEdge(3, 1);

	cout << "Following is a Topological Sort of the given graph: ";
	g.topologicalSort();

	return 0;
}
#endif


#if 0
x // prior functions (source lost as of 1-9-2013)
x // svdfitb.c
x float FC svdfitb( float **data, SI nRow, SI colMap[], float *a);
x // lsfit.c
x // (all functions currently #ifd out -- need work. 9-3-90).
x // svdecomp.c
x SI   FC svdcmp(	float **a, SI m, SI n, float *w, float **v);
x void FC svbksb(	float **u, float *w, float **v,	SI m, SI n, float *b, float *x);
x // spline.c
x void FC spline( float x[], float y[], SI n, float yp1, float ypn, float u[], float y2[]);
x SI   FC splint( float x[], float y[], float y2[], SI n, float X, float *pY);
x // nmroot.c
x SI FC nmRootBrent( double (*)(double), double, double, double, double * );
x // nrutilb.c
x void      FC nrerror( char *error_text);
x float *   FC vector( SI nl, SI nh);
x void      FC free_vector( float *v, SI nl, SI nh);
x SI *      FC ivector( SI nl, SI nh);
x void      FC free_ivector( SI *v, SI nl, SI nh);
x double *  FC dvector( SI nl, SI nh);
x void      FC free_dvector( double *v, SI nl, SI nh);
x float **  FC matrix( SI nrl, SI nrh, SI ncl, SI nch);
x void      FC free_matrix( float **m, SI nrl, SI nrh, SI ncl, SI nch);
x SI **     FC imatrix( SI nrl, SI nrh, SI ncl, SI nch);
x void      FC free_imatrix( SI **m, SI nrl, SI nrh, SI ncl, SI nch);
x double ** FC dmatrix( SI nrl, SI nrh, SI ncl, SI nch);
x void      FC free_dmatrix( double **m, SI nrl, SI nrh, SI ncl, SI nch);
x float **  FC submatrix( float **a, SI oldrl, SI oldrh, SI oldcl, SI oldch, SI newrl, SI newcl);
x void      FC free_submatrix( float **b, SI nrl, SI nrh, SI ncl, SI nch);
x float **  FC convert_matrix( float *a, SI nrl, SI nrh, SI ncl, SI nch);
x void      FC free_convert_matrix( float **b, SI nrl, SI nrh, SI ncl, SI nch);
#endif

#endif // _NUMMETH_H

// nummeth.h end
