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
class DGRAPH
{
public:
	DGRAPH(int nV, int base = 0) : dg_nV{ nV },
		dg_edges(nV), dg_edgesRev(nV) { }
	void dg_AddEdge(int ivFrom, int ivTo)
	{	dg_edges[ivFrom].push_back(ivTo);
		dg_edgesRev[ivTo].push_back(ivFrom);
	}
	template< typename T>
	void dg_AddEdges(T ivFrom, const T* ivTo, int count)
	{
		for (int i = 0; i<count; i++)
			dg_AddEdge(int(ivFrom), int(ivTo[i]));
	}
	bool dg_TopologicalSort( std::vector<int>& vSorted);
	bool dg_CountRefs(int ivRoot, std::vector< int>& vRefCounts);
	int dg_ChildCount(int iV) const
	{	return int(dg_edges[iV].size()); }
	int dg_ParentCount(int iV) const
	{	return int(dg_edgesRev[iV].size()); }

private:
	int dg_nV;	// # of verticies
	std::vector< std::vector<int>> dg_edges;		// edges
	std::vector< std::vector<int>> dg_edgesRev;	// reverse edges
	std::vector< byte> dg_status;	// 0: not seen; 1: processing; 2: seen
	bool dg_TopologicalSortDFS(int iV, std::vector<int>& vSorted);
	bool dg_CountRefsDFS(int iV, std::vector<int>& vRefCounts);
};		// class DGRAPH

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
