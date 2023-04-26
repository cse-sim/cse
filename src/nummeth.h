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
	DGRAPH(int nV, T base = 0) : dg_nV{ nV }, dg_base{ base },
		dg_edges(nV), dg_edgesRev(nV) { }
	void dg_AddEdge(T ivFrom, T ivTo)
	{	dg_edges[ivFrom].push_back(ivTo);
		dg_edgesRev[ivTo].push_back(ivFrom);
	}
	void dg_AddEdges(T ivFrom, const T ivTo[])
	{	for (int i = 0; ivTo[i] - dg_base >= 0; i++)
			dg_AddEdge(ivFrom - dg_base, ivTo[i] - dg_base);
	}
	bool dg_TopologicalSort( std::vector<T>& vSorted);
	bool dg_CountRefs(T ivRoot, std::vector< int>& vRefCounts);
	int dg_ChildCount(T iV) const
	{	return dg_edges[iV-dg_base].size(); }
	int dg_ParentCount(T iV) const
	{	return dg_edgesRev[iV-dg_base].size(); }

private:
	T dg_base;	// 0 or 1 based
	int dg_nV;	// # of verticies
	std::vector< std::vector<T>> dg_edges;		// edges
	std::vector< std::vector<T>> dg_edgesRev;	// reverse edges
	std::vector< byte> dg_status;	// 0: not seen; 1: processing; 2: seen
	bool dg_TopologicalSortDFS(T iV, std::vector<T>& vSorted);
	bool dg_CountRefsDFS(T iV, std::vector<int>& vRefCounts);
};		// class DGRAPH
//=============================================================================
template<typename T> bool DGRAPH<T>::dg_TopologicalSortDFS(
	T iV,	// starting vertex
	std::vector< T>& vSorted)	// returned: updated sorted list
// recursive helper for dg_TopologicalSort()
// returns true iff success
//        false if cyclic
{
	if (dg_status[ iV] == 2)
		return true;		// already seen
	if (dg_status[iV] == 1)
	{	// cyclic: put offending vertex into vSorted[ 0]
		vSorted.clear();
		vSorted.push_back(iV+dg_base);
		return false;
	}

	dg_status[iV] = 1;	// active vertex

	// Recurs for all the vertices adjacent to this vertex
	//  (depth first)
	for (auto tV : dg_edges[iV])
	{	if (!dg_TopologicalSortDFS(tV, vSorted))
			return false;	// cyclic somewhere below here
	}

	vSorted.push_back(iV + dg_base);
	dg_status[iV] = 2;	// seen
	return true;

}	// DGRAPH::dg_topologicalSortDFS
//-----------------------------------------------------------------------------
template<typename T> bool DGRAPH<T>::dg_TopologicalSort(
	std::vector< T>& vSorted)
// returns true on success (vSorted filled)
//    else false iff cyclic (vSorted[ 0] set to 1st offending vertex)
{
	vSorted.clear();
	dg_status.assign(dg_nV, 0);

	// Depth-first search starting from each vertex
	int ret = -1;
	for (int iV = 0; iV < dg_nV; iV++)
	{
		if (!dg_TopologicalSortDFS(iV, vSorted))
			return false;
	}
	return true;
}	// DGRAPH::dg_TopologicalSort
//------------------------------------------------------------------------------
template<typename T> bool DGRAPH<T>::dg_CountRefs(	// # of refs to each vertex in tree
	T ivRoot,		// root vertex
	std::vector< int>& vRefCounts)
// WHY: allows identifying duplicate references in a specific subtree
// returns true iff success (vRefCounts[ iV] = refs/visits to each vertex)
//    else false (cyclic)
{
	vRefCounts.assign(dg_nV, 0);
	dg_status.assign(dg_nV, 0);

	return dg_CountRefsDFS(ivRoot-dg_base, vRefCounts);
}		// DGRAPH::dg_CountRefs
//-----------------------------------------------------------------------------
template<typename T> bool DGRAPH<T>::dg_CountRefsDFS(
	T iV,		// current vertex
	std::vector< int>& vRefCounts)
{
	if (dg_status[iV] == 1)
	{	// cyclic: put offending vertex into vRefCounts[ 0]
		vRefCounts.clear();
		vRefCounts.push_back(iV + dg_base);
		return false;
	}

	++vRefCounts[iV+dg_base];	// count current

	for (auto tV : dg_edges[iV])
	{	if (!dg_CountRefsDFS(tV, vRefCounts))
			return false;	// cyclic somewhere below here
	}
	return true;
}		// DGRAPH::dg_CountRefsDFS
//=============================================================================

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
