// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// nummeth.h -- declarations for numerical methods functions

/*-------------------------------- DEFINES --------------------------------*/

/*------------------------- FUNCTION DECLARATIONS -------------------------*/
int gaussjb( double* a, int n, double* b, int m, int invflg=0);

int secant(	double (*pFunc)( void *pO, double &x), void *pO, double f,
	double eps, double &x1, double &f1, double x2, double f2=DBL_MIN,
	double epsLo=-1.);

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

// nummeth.h end
