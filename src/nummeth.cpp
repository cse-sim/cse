// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// nummeth.cpp -- numerical method functions

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <NUMMETH.H> 	// decls for this file

//==============================================================================
int FC gaussjb(		// Solve a system of equations using Gauss-Jordan elimination

	double* a,			// square matrix of coefficients.  NOTE: this is a
						//   a C 2D array, NOT a Numerical Recipes "matrix"
	int n,				// dimension of a -- e.g. a is a[n,n]
						//  n *MUST BE* <= MAXN (see code)
	double* b,			// Set of m right hand side vectors for which
    					//	solutions are sought.  m can be 0.
	int m,				// b is b[n][m]
	int invflg /*=0*/)	// invert flag - if nz, a-inverse is returned in a
						//  if 0,  a is returned garbage (not generating inverse faster)

// returns:
// -1: failure: n too large
//	0: success (a-inverse in place of a (if invflg nz), b-solutions in b )
//  1: matrix is singular
{
	static const int MAXN = 1000;	// max n supported
	int ipiv[ MAXN];
	int indxr[ MAXN];
	int indxc[ MAXN];

	int i, j, ll, irow = 0, icol = 0, l, k;
	double f, big, pivinv;
	double *arow, *arcol, *arow2;
	double *brow, *brcol, *brow2;

	if (n > MAXN)
		return -1;

	memset( ipiv, 0, n * sizeof( int) );

	for (i = 0; i < n; i++)		/* Main loop */
	{
		big = 0.;
		arow = a;
		for (j = 0; j < n; j++)
		{
			if (*(ipiv+j) != 1)
			{
				arcol = arow;
				for (k = 0; k < n; k++)
				{
					if (*(ipiv+k) == 0)
					{
						if ((f=fabs(*arcol)) >= big)
						{
							big = f;
							irow = j;
							icol = k;
						}
					}
					else if (*(ipiv+k) > 1)
						return 1;		/* singular */
					arcol++;
				}
			}
			arow += n;
		}

		/* Now that the pivot point is found we rotate rows */
		*(ipiv+icol) += 1;
		arow = a + irow*n;
		brow = b + irow*m;
		arow2 = a + icol*n;
		brow2 = b + icol*m;
		if (irow != icol)
		{
			VSwap( arow, n, arow2, n);
			VSwap( brow, n, brow2, m);
		}
		*(indxr+i) = irow;
		*(indxc+i) = icol;

		double* arcol2 = arow2 + icol;
		if (*arcol2 == 0.)
			return 1;		/* singular */
		pivinv = 1./(*arcol2);
		*arcol2 = 1.;
		arcol2 = arow2;
		double* brcol2 = brow2;
		/*lint -e564  suppress "variable arcol2 depends on order of eval" */
		for (l = 0; l < n; l++)
			*(arcol2++) *= pivinv;
		for (l = 0; l < m; l++)
			*(brcol2++) *= pivinv;
		/*lint +e564 */

		arow = a;
		brow = b;
		for (ll = 0; ll < n; ll++)
		{
			if (ll != icol)
			{
				double dum = *(arow+icol);
				*(arow+icol) = 0.;
				arcol = arow;
				arcol2 = arow2;
				brcol  = brow;
				brcol2 = brow2;
				/*lint -e564  suppress "variable arcol depends on order of eval"*/
				for (l = 0; l < n; l++)
					*arcol++ -= *(arcol2++)*dum;
				for (l = 0; l < m; l++)
					*brcol++ -= *(brcol2++)*dum;
				/*lint +e654 */
			}
			arow += n;
			brow += m;
		}
	}          /* End of main loop */

	if ( invflg )
		for (l = n-1; l >= 0; l--)
			if ( (irow = *(indxr+l)) != (icol = *(indxc+l)) )
				VSwap( a+irow, n, a+icol, n);

	return 0;
}
//-----------------------------------------------------------------------------
int secant(							// find x given f(x) (secant method)
	double (*pFunc)( void *pO, double &x),
						// function under investigation; note that it
						//   may CHANGE x re domain limits etc.
	void *pO,			// pointer passed to *pFunc, typ object pointer
	double f,			// f( x) value sought
	double eps,			// convergence tolerance, hi- or both sides
						//   see also epsLo
	double &x1,			// x 1st guess,
						//   returned with result
	double &f1,			// f( x1), if known, else pass DBL_MIN
						//   returned: f( x1), may be != f, if no converge
	double x2,			// x 2nd guess
	double f2/*=DBL_MIN*/,	// f( x2), if known
	double epsLo/*=-1.*/)	// lo-side convergence tolerance

// convergence = f - eps(Lo) <= f1 <= f + eps

// Note: *pFunc MAY have side effects ... on return, f1 = *pFunc( x1)
//      is GUARANTEED to be last call

// returns result code, x1 and f1 always best known solution
//		0: success
//		>0= failed to converge, returned value = # of iterations
//	    <0= 0 slope encountered, returned value = -# of iterations
{
	double fHi = f + eps;
	double fLo = f - (epsLo >= 0. ? epsLo : eps);

	if (f1 == DBL_MIN)
    	f1 = (*pFunc)( pO, x1);

    if (f1 <= fHi && f1 >= fLo)		// if 1st guess good
		return 0;					//   success: don't do *pFunc( x2)
									//   (side effects)

	if (f2 == DBL_MIN)
		f2 = (*pFunc)( pO, x2);

    if (fabs(f-f1) > fabs(f-f2))	// make point 1 the closer
    {	double swap;
		swap = x1; x1=x2; x2=swap;
		swap = f1; f1=f2; f2=swap;
    }

	int i;
    for (i=0; ++i < 20; )		// iterate to refine solution
    {	if (f1 <= fHi && f1 >= fLo)
		{
			i = 0;		// success
     	  	break;		//   done; last *pFunc call ...
						//     1st iteration: *pFunc( x2) + swap
						//    >1st iteration: *pFunc( x1) below
		}

		if (fabs(f1-f2) < 1.e-20)		// if slope is 0
		{	i = -i;			// tell caller
			break;
		}

    	double xN = x1+(x2-x1)*(f-f1)/(f2-f1);
						// secant method: new guess assuming local linearity.
    	x2 = x1;   		// replace older point
    	f2 = f1;
		x1 = xN;
    	f1 = (*pFunc)( pO, x1);		// new value
    }
	return i;
}			// ::secant
//-----------------------------------------------------------------------------
int regula(							// find x given f(x) (regula-falsi method)
	double (*pFunc)(void* pO, double& x),
	// function under investigation; note that it
	//   may CHANGE x re domain limits etc.
	void* pO,			// pointer passed to *pFunc, typ object pointer
	double f,			// f( x) value sought
	double eps,			// convergence tolerance, hi- or both sides
	double& x1,			// x 1st guess,
						//   returned with result
	double xMin,		// minimum value of x
	double xMax)		// maximum value of x

// returns result code, x1 and f1 always best known solution
//		0: success
//		>0= failed to converge, returned value = # of iterations
//	    <0= bad min / max
{
	double fHi = f + eps;
	double fLo = f - eps;

	double f1 = (*pFunc)(pO, x1);

	if (f1 <= fHi && f1 >= fLo)		// if 1st guess good
		return 0;					//   success: don't do *pFunc( x2)
									//   (side effects)

	double fMin = (*pFunc)(pO, xMin);

	if (fMin <= fHi && fMin >= fLo)		// if 1st guess good
	{
		x1 = xMin;
		return 0;					//   success: don't do *pFunc( x2)
									//   (side effects)
	}

	double fMax = (*pFunc)(pO, xMax);

	if (fMax <= fHi && fMax >= fLo)		// if 1st guess good
	{
		x1 = xMax;
		return 0;					//   success: don't do *pFunc( x2)
									//   (side effects)
	}

	// Error if fMin and fMax are the same sign
	if ((fMin - f) * (fMax - f) > 0.)
	{
		return -1;
	}

	if ((f1 - f) * (fMin - f) < 0.)
	{
		xMax = x1;
		fMax = f1;
	}
	else
	{
		xMin = x1;
		fMin = f1;
	}

	int i;
	for (i = 0; ++i < 20; )		// iterate to refine solution
	{
		x1 = xMin - (xMax - xMin) * (fMin - f) / (fMax - fMin);
		f1 = (*pFunc)(pO, x1);

		if (f1 <= fHi && f1 >= fLo)
		{
			i = 0;		// success
			break;		//   done; last *pFunc call ...
						//     1st iteration: *pFunc( x2) + swap
						//    >1st iteration: *pFunc( x1) below
		}

		if ((f1 - f) * (fMin - f) < 0.)
		{
			xMax = x1;
			fMax = f1;
		}
		else
		{
			xMin = x1;
			fMin = f1;
		}

	}
	return i;
}			// ::regula
//=============================================================================
template< typename T, size_t NI> class PWLFUNC
{
public:
	PWLFUNC(T (*pFunc)(T x), T xMin, T xMax);
	T operator()( T x)
	{
		int i = int(x / NI);
		if (i < 0 || i >= NI)
			return (*pwl_pFunc)(x);
		else
			return pwl_a[i] + pwl_b[i] * x;
	}

private:
	T (*pwl_pFunc)(T x);
	T pwl_xMin;
	T pwl_xMax;
	T pwl_a[NI];
	T pwl_b[NI];

	void pwl_Setup();

};
//-------------------------------------------------------------------------------
template< typename T, size_t NI> PWLFUNC< T, NI>::PWLFUNC(
	T(*pFunc)(T x),	T xMin, T xMax)
		: pwl_pFunc( pFunc), pwl_xMin( xMin), pwl_xMax( xMax)
{
	pwl_Setup();
}	// PWLFUNC::PWLFUNC
//-------------------------------------------------------------------------------
template< typename T, size_t NI> void PWLFUNC< T, NI>::pwl_Setup()
{
	T xStep = (pwl_xMax - pwl_xMin) / (NI-1);
	for (int i = 0; i < NI; i++)
	{
		T x = pwl_xMin + i*xStep;
		pwl_a[i] = (*pwl_pFunc)(x);
	}


}

// ===============================================================================

static float pow33(float x) { return powf(x, 1.f/3.f); }
static PWLFUNC< float, 11> powk(pow33,0.f, 10.f);

#if 0
void test()
{
	float x = powk(3.f);

}
#endif

// end of nummeth.cpp