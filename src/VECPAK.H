// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// vecpak.h -- Include file for vector operation routines

#ifndef _VECPAK_H
#define _VECPAK_H
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
template< typename T, typename TR = T> TR VSumWrap(		// vector sum with wrap
	const T* v,  		// vector to sum
	int nV,				// # of values to sum
	int n)				// dim of v
{
	TR vTot = 0;
	for (int iV = 0; iV < nV; iV++)
		vTot += TR(v[iV % n]);
	return vTot;
}		// VSumWrap< T>
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
//-----------------------------------------------------------------------------
template< typename T> T VSumMaxMean(
	const T* v,  		// vector
	int n,				// dim of v
	T& vMin,			// returned
	T& vMax)
{
	double vSum = 0.;
	if (n <= 0)
	{		vMin = vMax = T(0);
		vSum = 0.;
	}
	else
	{	for (int i = 0; i < n; i++)
		{	if (i == 0)
			{
				vMin = vMax = *v;
				vSum = double(*v);
			}
			else
			{	if (*v < vMin)
					vMin = *v;
				else if (*v > vMax)
					vMax = *v;
				vSum += *v;
			}
			v++;
		}
	}
	return T(vSum);
}		// VSumMaxMin
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
template< typename T1, typename T2> void VAccumMin(	// vector-vector min
	T1* v,				// where to accum
	int n,				// dim of v
	const T2* vSrc)		// source
{
	for (int i = 0; i < n; i++)
		v[i] = T1(min(v[i], T1(*vSrc++)));
}		// VAccumMin< T1, T2>
//--------------------------------------------------------------------------
template< typename T1, typename T2> void VAccumMax(	// vector-vector max
	T1* v,				// where to accum
	int n,				// dim of v
	const T2* vSrc)		// source
{
	for (int i = 0; i < n; i++)
		v[i] = T1(max(v[i], T1(*vSrc++)));
}		// VAccumMax< T1, T2>

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
	size_t nSiz;	// active size of vals[]
	double vSum;	// current sum of vals[0..nCur-1]
	size_t iOld;	// idx of oldest value in vals[]
	size_t nCur;	// # of vals[] currently set
public:
	VMovingSum(size_t _nSiz = 0) : vals(nullptr), nSiz(0), vSum(0), iOld(0), nCur(0)
	{	if (_nSiz > 0)
			vm_Init(_nSiz);
	}
	~VMovingSum() { delete[] vals; vals = nullptr; nSiz = 0;  }
	bool vm_Init(int _nSiz)
	{	delete[] vals;
		nSiz = max(1, _nSiz);
		vals = new double[nSiz];
		return vm_Clear();
	}
	bool vm_Clear()
	{	iOld = nCur = 0;
		vSum = 0.;
		if (!vals)
		{	nSiz = 0;
			return false;
		}
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
			*pSumMax = T(vSum);
		return vSum;
	}
};		// class VMovingSum

///////////////////////////////////////////////////////////////////////////////
// class VX: vector with min/max/mean, commonly used for 24 hour values
///////////////////////////////////////////////////////////////////////////////
template< typename T, int n> class VX
{
	T v[n];
	T vMin;
	T vMean;
	T vMax;
	T vSum;
public:
	VX() { Clear(); }
	VX(const VX& s) { Copy(s); }
	VX& Copy(const VX& s)
	{	VCopy(v, n, s.v);
		vMin = s.vMin; vMax = s.vMax; vMean = s.vMean; vSum = s.vSum;
		return *this;
	}
	void Clear()
	{	Set(T(0));
	}
	void Set(T vAll)
	{	VSet(v, n, vAll);
		vMin = vMax = vMean = vAll;
		vSum = n * vAll;
	}
	void Set(const float* _v)
	{	VCopy(v, n, _v);
		SetStats();
	}
	void Set(const double* _v)
	{	VCopy(v, n, _v);
		SetStats();
	}
	T Min() const
	{  return vMin; }
	T Max() const
	{	return vMax; }
	T Sum() const
	{	return vSum; }
	T Mean() const
	{	return vMean; }

	void SetStats()
	{	vSum = VSumMaxMean(v, n, vMin, vMax);
		vMean = vSum / n;
	}
	operator T* () { return v; }
	T* VPtr() { return v; }
	VX& operator =(T vAll) { Set(vAll); return *this; }
	VX& operator =(const VX vx2) { return Copy(vx2); }
	T& operator[](int i) { return v[i]; }
	T operator()(int i) const
	{	return i < n ? v[i] : i == n ? vMin : i == n + 1 ? vMean : i == n + 2 ? vMax : vSum;
	}
	const T& operator[](int i) const { return v[i]; }
#if 0
	T FLH()	// full-load hours
	{	SetStats();
		return vMax > 0 ? vSum / vMax : 0;
	}
	int MaxLoc() const
	{	return VMaxLoc(v, n); }
	int MinLoc() const
	{	return VMinLoc(v, n); }

	// operations (caller must do SetStats())
	VX& CopyW(const VX vx2, double weight)
	{	VCopyW(v, n, vx2.v, weight);
		return *this;
	}
	VX& Add(const VX vx2) { VAdd(v, n, vx2.v); return *this; }
	VX& Sub(const VX vx2) { VSub(v, n, vx2.v); return *this; }
	VX& Mul(const VX vx2) { VMul(v, n, vx2.v); return *this; }
	VX& Mul(double f) { VMul(v, n, f); return *this; }
	VX& Accum(const VX& vx2, double w = 1.)
	{	VAdd(v, n, vx2.v, w);
		SetStats();
		return *this;
	}
	T AbsDiff(const VX vx2) const
	{	return VAbsDiff(v, n, vx2.v);
	}
	T AbsSum() const
	{	return VAbsSum(v, n);
	}
	static CWString Tag(int i, int offset = 0)
	{	static const char* _tags[] = { _T("Min"), _T("Avg"), _T("Max"), _T("Tot") };
		return i < n ? tFormat(_T("%3d"), i + offset) : _tags[i - n];
	}
#endif
};		// class VX
//-----------------------------------------------------------------------------
// VX24 vectors
const int vxMIN = 24;	// pseudo idxs (use with operator())
const int vxMEAN = 25;
const int vxMAX = 26;
const int vxSUM = 27;
const int X24 = 28;		// loop limit

#if 0
template< typename T> class VX24 : public VX< T, 24>
{
	VX24& operator *(const VX24& v2) { return (VX< T, 24>::operator *)(v2); }
};
#endif
class VF24 : public VX< float, 24>
{
public:
	VF24& operator =(float vAll) { Set(vAll); return *this; }
};
class VD24 : public VX< double, 24>
{
public:
	VD24& operator =(double vAll) { Set(vAll); return *this; }
#if 0
	VD24& operator *(const VD24& v2) const
	{
		return static_cast<VD24&>((VX< double, 24>::operator *)(v2));
	}
#endif
};
//----------------------------------------------------------------------------
#if 0
// if needed
template< typename T, int n> void VXSet(VX< T, n>* pVX, int nVX, T vAll)
{
	for (int iVX = 0; iVX < nVX, iVX++)
		pVX[iVX].Set(vAll);
}
template< typename T, int n> void VXAccum(VX< T, n>* pVX, int nVX, VX< T, n>* pVXSrc)
{
	for (int iVX = 0; iVX < nVX; iVX++)
		pVX[iVX].Accum(pVXSrc[iVX]);
}
inline void VXSet(VF24* pVX24, int nVX, float vAll)
{
	for (int iVX = 0; iVX < nVX; iVX++)
		pVX24[iVX].Set(vAll);
}
inline void VXSet(VD24* pVX24, int nVX, double vAll)
{
	for (int iVX = 0; iVX < nVX; iVX++)
		pVX24[iVX].Set(vAll);
}
inline void VXAccum(VF24* pVX24, int nVX, const VF24* pVX24Src, double w = 1.)
{
	for (int iVX = 0; iVX < nVX; iVX++)
		pVX24[iVX].Accum(pVX24Src[iVX], w);
}
inline void VXAccum(VD24* pVX24, int nVX, const VD24* pVX24Src, double w = 1.)
{
	for (int iVX = 0; iVX < nVX; iVX++)
		pVX24[iVX].Accum(pVX24Src[iVX], w);
}
#endif

#endif
// end of vecpak.h
