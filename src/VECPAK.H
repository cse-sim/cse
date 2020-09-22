// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// vecpak.h -- Include file for vector operation routines

////////////////////////////////////////////////////////////////////////////
// vector template functions
////////////////////////////////////////////////////////////////////////////
template< typename T> void VZero(		// vector clear to 0
	T* v,				// vector to 0
	int n,				// dim of v
	int stride)			// v stride ... allows 0'ing
						//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	*v = (T)0;
		v = (T *)(((char *)v)+stride);
	}
}		// VZero< T>
//-----------------------------------------------------------------------------
template< typename T> void VZero(		// vector clear to 0
	T* v,				// vector to 0
	int n)				// dim of v
{	for (int i=0; i < n; i++)
		*v++ = (T)0;
}		// VZero< T>
//--------------------------------------------------------------------------
template< typename T> void VSet(		// vector set to v
	T* v,				// vector to set
	int n,				// dim of v
	T vSet)				// value
{	for (int i=0; i < n; i++)
		*v++ = vSet;
}		// VSet< T>
//--------------------------------------------------------------------------
template< typename T> void VSet(		// vector set to v
	T* v,				// vector to set
	int n,				// dim of v
	T vSet,				// value
	int stride)			// v stride ... allows setting
								//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	*v = vSet;
		v = (T *)(((char *)v)+stride);
	}
}		// VSet< T>
//--------------------------------------------------------------------------
template< typename T> void VSeries(		// vector set to arithmetic series
	T* v,				// vector to set
	int n,				// dim of v
	T v0,				// 0th value
	T vInc=1,			// increment value
	int stride=sizeof( T))		// v stride ... allows setting
								//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	*v = v0 + i*vInc;
		v = (T *)(((char *)v)+stride);
	}
}		// VSeries< T>
//--------------------------------------------------------------------------
template< typename T,typename TR=T> TR VSum(		// vector sum
	const T* v,  		// vector to sum
	int n)				// dim of v
{	TR vTot = 0;
	for (int i=0; i < n; i++)
		vTot += TR(*v++);
	return vTot;
}		// VSum< T>
//--------------------------------------------------------------------------
template< typename T,typename TR=T> TR VSum(		// vector sum
	const T* v,  		// vector to sum
	int n,				// dim of v
	int stride)			// v stride ... allows summing
						//   e.g. array of structs
{	TR vTot = 0;
	for (int i=0; i < n; i++)
	{	vTot += TR( *v);
		v = (const T *)(((char *)v)+stride);
	}
	return vTot;
}		// VSum< T>
//--------------------------------------------------------------------------
template< typename T,typename TR=T> TR VAbsSum(		// vector absolute value sum
	const T* v,  		// vector to sum
	int n)				// dim of v
{	TR vTot = 0;
	for (int i=0; i < n; i++)
		vTot += abs( TR(*v++));
	return vTot;
}		// VSum< T>
//--------------------------------------------------------------------------
template< typename T> T VMin(		// vector min value
	const T* v,  		// vector
	int n)				// dim of v
{	T vMin = n <= 0 ? 0 : *v;
	for (int i=0; i < n; i++)
	{	if (*v < vMin)
			vMin = *v;
		v++;
	}
	return vMin;
}		// VMin< T>
//--------------------------------------------------------------------------
template< typename T> T VMin(		// vector min value
	const T* v,  		// vector
	int n,				// dim of v
	int stride)			// v stride ... allows min-ing
						//   e.g. array of structs
{	T vMin = n <= 0 ? 0 : *v;
	for (int i=0; i < n; i++)
	{	if (*v < vMin)
			vMin = *v;
		v = (const T *)(((char *)v)+stride);
	}
	return vMin;
}		// VMin< T>
//--------------------------------------------------------------------------
template< typename T> T VMax(		// vector max value
	const T* v,  		// vector to sum
	int n)				// dim of v
{	T vMax = n <= 0 ? 0 : *v;
	for (int i=0; i < n; i++)
	{	if (*v > vMax)
			vMax = *v;
		v++;
	}
	return vMax;
}		// VMax< T>
//--------------------------------------------------------------------------
template< typename T> T VMax(		// vector max value
	const T* v,  		// vector to sum
	int n,				// dim of v
	int stride)			// v stride ... allows max-ing
						//   e.g. array of structs
{	T vMax = n <= 0 ? 0 : *v;
	for (int i=0; i < n; i++)
	{	if (*v > vMax)
			vMax = *v;
		v = (const T *)(((char *)v)+stride);
	}
	return vMax;
}		// VMax< T>
//--------------------------------------------------------------------------
template< typename T1, typename T2> T1 VSum2(		// vector sum
	const T2* v,  		// vector to sum
	int n)				// dim of v
{	T1 vTot = 0;
	for (int i=0; i < n; i++)
		vTot += T1( *v++);
	return vTot;
}		// VSum2< T1, T2>
//--------------------------------------------------------------------------
template< typename T1, typename T2> T1 VSum2(		// vector sum
	const T2* v,  		// vector to sum
	int n,				// dim of v
	int stride)			// v stride ... allows 0'ing
						//   e.g. array of structs
{	T1 vTot = 0;
	for (int i=0; i < n; i++)
	{	vTot += T1( *v);
		v = (const T2 *)(((char *)v)+stride);
	}
	return vTot;
}		// VSum2< T1, T2>
//--------------------------------------------------------------------------
template< typename T1, typename T2> void VAccum(	// vector accumulation
	T1* v,				// where to accum
	int n,				// dim of v
	const T2* vSrc)		// source
{	for (int i=0; i < n; i++)
		v[ i] = T1( v[ i] + T1( *vSrc++));	// not +=, gives warnings for e.g. short+short)
}		// VAccum< T1, T2>
//--------------------------------------------------------------------------
template< typename T1, typename T2> void VAccum(	// vector accumulation
	T1* v,				// where to accum
	int n,				// dim of v
	const T2* vSrc,		// source
	int stride)			// vSrc stride ... allows accumulating from
						//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	v[ i] = T1( v[ i] + T1( *vSrc));	// not +=, gives warnings for e.g. short+short)
		vSrc = (T2 *)(((char *)vSrc)+stride);
	}
}		// VAccum< T1, T2>
//--------------------------------------------------------------------------
template< typename T1, typename T2> void VAccum(	// weighted vector accum
	T1* v,				// where to accum
	int n,				// dim of v
	const T2* vSrc,		// source
	double weight)		// weighting
								//   e.g. array of structs
{	for (int i=0; i < n; i++)
		v[ i] = T1( v[ i] + T1( *vSrc++)*weight);	// not += (give warnings for e.g. short+short)
}		// VAccum< T1, T2>
//--------------------------------------------------------------------------
template< typename T1, typename T2> void VAccum(	// weighted vector accum
	T1* v,				// where to accum
	int n,				// dim of v
	const T2* vSrc,		// source
	double weight, 		// weighting
	int stride)			// vSrc stride ... allows accumulating from
						//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	v[ i] = T1( v[ i] + (T1)(*vSrc * weight));	// not += (give warnings for e.g. short+short)
		vSrc = (T2 *)(((char *)vSrc)+stride);
	}
}		// VAccum< T1, T2>
//-------------------------------------------------------------------------
template< typename T1, typename T2> void VCopy(		// vector copy
	T1* v,				// copy dest
	int n,				// dim of v
	const T2* vSrc)			// source
{	for (int i=0; i < n; i++)
		*v++ = T1( *vSrc++);
}		// VCopy< T1, T2>
//-------------------------------------------------------------------------
template< typename T1, typename T2> void VCopy(		// vector copy
	T1* v,				// copy dest
	int n,				// dim of v
	const T2* vSrc,		// source
	double weight)		// weighting
{	for (int i=0; i < n; i++)
		*v++ = T1( *vSrc++)*weight;
}		// VCopy< T1, T2>
//-------------------------------------------------------------------------
template< typename T1, typename T2> void VCopy(		// vector copy
	T1* v,				// copy dest
	int n,				// dim of v
	const T2* vSrc,		// source
	int stride)			// vSrc stride ... allows copying from
						//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	v[ i] = T1( *vSrc);
		vSrc = (T2 *)(((char *)vSrc)+stride);
	}
}		// VCopy< T1, T2>
//-------------------------------------------------------------------------
template< typename T1, typename T2> void VCopy(	// weighted vector copy
	T1* v,				// copy dest
	int n,				// dim of v
	const T2* vSrc,		// source
	double weight,		// weighting
	int stride)			// vSrc stride ... allows copying from
						//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	v[ i] = T1( *vSrc * weight);
		vSrc = (T2 *)(((char *)vSrc)+stride);
	}
}		// VCopy< T1, T2>
//-------------------------------------------------------------------------
template< typename T> void VSwap(		// vector swap
	T* v1,				// copy dest
	int n,				// dim of v
	T* v2)				// source
{
	for (int i=0; i<n; i++)
	{	T dum = *v1;
		*v1++ = *v2;
		*v2++ = dum;
	}
}          // VSwap< T1>
//-------------------------------------------------------------------------
template< typename T> void VSwap(		// vector swap
	T* v1,				// copy dest
	int n,				// dim of v
	T* v2,				// source
	int step)			// step (not stride!), allows e.g. swapping matrix rows
    					// (e.g. for an n x n matrix, call with step = n).
    					// Neg. values are OK if you know what you're doing. */
{
	for (int i=0; i<n; i++)
	{	T dum = *v1;
		*v1 = *v2;
		*v2 = dum;
		v1 += step;
		v2 += step;
	}
}          // VSwap< T1>
//-------------------------------------------------------------------------
template< typename T> BOOL VEqual(		// vector *EXACT* match
	const T* v1,		// vector 1
	int n,				// dim of v
	const T* v2,			// vector 2
	int stride=sizeof( T))		// v2 stride ... allows comparing
								//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	if (v1[ i] != *v2)
			return FALSE;
		v2 = (T *)(((char *)v2)+stride);
	}
	return TRUE;
}		// VEqual< T>
//-------------------------------------------------------------------------
template< typename T> BOOL VEqual(		// vector *EXACT* match to single v
	const T* v,			// vector
	int n,				// dim of v
	T vC,				// single value
	int stride=sizeof( T))		// v stride ... allows comparing
								//   e.g. array of structs
{	for (int i=0; i < n; i++)
	{	if (*v != vC)
			return FALSE;
		v = (T *)(((char *)v)+stride);
	}
	return TRUE;
}		// VEqual< T>
//-------------------------------------------------------------------------
template< typename T> int VFind(		// vector find entry
	T* v,				// vector
	int n,				// dim of v
	T vX)				// value sought
// returns i where v[ i] == vX, -1 if not found
{	for (int i=0; i < n; i++)
	{	if (*v++ == vX)
			return i;
	}
	return -1;
}		// VFind< T>
//-------------------------------------------------------------------------
template< typename T> int VFind(		// vector find entry
	T* v,				// vector
	int n,				// dim of v
	T vX,				// value sought
	int stride)			// v2 stride ... allows comparing
						//   e.g. array of structs
// returns i where v[ i] == vX, -1 if not found
{	for (int i=0; i < n; i++)
	{	if (*v == vX)
			return i;
		v = (T *)(((char *)v)+stride);
	}
	return -1;
}		// VFind< T>
//-------------------------------------------------------------------------
template< typename T1, typename T2> void VMul1(	// vector * constant
	T1* v,				// vector
	int n,				// dim of v
	T2 f)				// factor
{	if (f != T2( 1))
		for (int i=0; i < n; i++)
			v[ i] = T1( v[ i] * f);
}		// VMul1< T1, T2>
//-------------------------------------------------------------------------
template< typename T> T DotProd3(			// 3-vector dot product
	const T* v1, const T* v2)
{
	return v1[ 0]*v2[ 0] + v1[ 1]*v2[ 1] + v1[ 2]*v2[ 2];
}		// DotProd< T>
//-------------------------------------------------------------------------
template< typename T> T DotProd(			// n-vector dot product
	const T* v1,			// vector 1
	const T* v2,			// vector 2
	int n)					// dim of v1, v2
// see also VIProd (next)
{
	T t = 0;
	for (int i=0; i < n; i++)
		t += *v1++ * *v2++;
	return t;
}		// DotProd< T>
//-------------------------------------------------------------------------
template< typename T,typename TR=T> TR VIProd(	// inner product
	const T* v1,			// vector 1
	int n,					// dim of v1, v2
	const T* v2)			// vector 2
{
	TR t = 0;
	for (int i=0; i < n; i++)
	 	t += TR(*v1++) * TR(*v2++);
	return t;
}		// VIProd< T>
//-------------------------------------------------------------------------
template< typename T,typename TR=T> TR VIProd(	// inner product w/ stride
	const T* v1,			// vector 1
	int n,					// dim of v1, v2
	const T* v2,			// vector 2
	int stride)				// v1 / v2 stride
{
	TR t = 0;
	for (int i=0; i < n; i++)
	{ 	t += TR( *v1) * TR( *v2);
		v1 = (T *)(((char *)v1)+stride);
		v2 = (T *)(((char *)v2)+stride);
	}
	return t;
}		// VIProd< T>
//=============================================================================
template< typename T> class VMovingSum
{
	double* vals;	// value history (ring buffer)
	int nSiz;		// active size of vals[]
	double vSum;	// current sum of vals[0..nCur-1]
	int iOld;		// idx of oldest value in vals[]
	int nCur;		// # of vals[] currently set
public:
	VMovingSum() : vals( nullptr), nSiz( 0),vSum( 0),iOld(0),nCur(0) {}
	~VMovingSum() { delete[] vals; vals = nullptr; }
	bool vm_Init(int _nSiz)
	{	delete[] vals;
		vals = nullptr;
		iOld = nCur = 0;
		vSum = 0.;
		if (_nSiz < 0)
			_nSiz = 1;
		vals = new double[_nSiz];
		if (!vals)
			return false;
		nSiz = _nSiz;
		VZero(vals, nSiz);
		return true;
	}
	T vm_Sum(T val, T* pSumMax=nullptr)
	{
		if (nCur < nSiz) 		// if don't yet have nSiz
		{	vals[nCur++] = val;	// add new value to end of array, increment # values in sub
		}
		else 					// already have n values
		{	vSum -= vals[ iOld];
			vals[iOld++] = val; // store new value over old. Next value is now oldest,
			if (iOld == nSiz) 	// with wrap at end array
				iOld = 0;		// .. .
		}
		vSum += val;			// add new value to sum
		// vMean = sum / nCur;	// if needed
		if (pSumMax && vSum > *pSumMax)
			*pSumMax = vSum;
		return vSum;
	}
};		// class VMovingSum

/*------------------------- FUNCTION DECLARATIONS -------------------------*/

#if 0
// vecpak.cpp -- Integer functions
extern void FC iVecAdd( SI *i2, const SI *i1, int n);
extern void FC lVecAdd( LI *i2, const LI* i1, int n);

// vecpak.cpp -- Long integer function
extern double FC fVecSum( const float* f, int n);
extern void FC iVecAdd( SI *i2, const SI *i1, int n);

// vecpak.cpp -- Float functions
extern void FC fVecSet( float *f, int n, float v);
extern void FC fVecAdd( float *f2, const float *f1, int n);
extern void FC fVecSub( float *f2, float *f1, int n);

extern double FC fVecIprod( float *f1, float *f2, int n);
extern void FC vecCopyD2F( double *vdp, float *vfp, int n);

/* vecpak.cpp -- Double functions */
extern void FC dVecSet( double *d, int n, double v);
extern double FC dVecIprod( double *d1, double *d2, int n);
extern double FC dVecMean( double *, SI);
extern void FC dVecSwap( double *, double *, int, int);

extern double FC fVecAbsum( const float* f, int n);
#endif


// end of vecpak.h
