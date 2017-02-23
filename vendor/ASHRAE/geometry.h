//==========================================================================
//	geometry.h -- global geometry algoritms function prototypes
//=============================================================================
// Copyright (c) 2014, ASHRAE.
//   This work is a product of ASHRAE 1383-RP and draws on code
//   Copyright (c) 1995-2011, Regents of the University of California
// All rights reserved.
 
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the following disclaimer in the documentation
//   and/or other materials provided with the distribution.
// * Neither the name of ASHRAE, the University of California, Berkeley nor the
//   names of its contributors may be used to endorse or promote products
//   derived from this software without specific prior written permission.
 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//==========================================================================

#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__

const double SMALL_NUM	= 0.00000001;	//represent small number (prevents divide by 0)
// aproximately equal
inline bool equalApr(double a, double b, double eps = SMALL_NUM)
{	return fabs(a-b) <= eps; }

const int xgoREVERSE=0x00008000;		// option for several functions
										//  = reverse point order

#undef USE_MFC		// #define to use MFC collections etc.
					// re transition to stdlib, 1-2017

/////////////////////////////////////////////////////////////////
// class CPV3D: represents 3D point or vector
/////////////////////////////////////////////////////////////////
class CPV3D;
class CAABB;
inline double Distance2( const CPV3D& p1, const CPV3D& p2);
inline double Distance( const CPV3D& p1, const CPV3D& p2);
inline CPV3D CrossProd( const CPV3D& p1, const CPV3D& p2);
//---------------------------------------------------------------
class CPV3D
{
public:
	CPV3D()	{ x = y = z = 0.;}
	CPV3D( const CPV3D& p)	{ x = p.x; y = p.y; z = p.z; }
	CPV3D( double _x, double _y, double _z)
	{	x = _x; y = _y; z = _z; }
	CPV3D( const double v[3]) { x=v[0]; y=v[1]; z=v[2]; }
	CPV3D( const float v[3])  { x=v[0]; y=v[1]; z=v[2]; }
	void Zero() { x = y = z = 0.; }
	CPV3D& FixSmall( double v)
	{	if (fabs(x) < v) x = 0.; if (fabs(y) < v) y = 0.; if (fabs(z) < v) z = 0.; return *this; }
	void Set( const double v[3])
	{	x = v[ 0]; y=v[ 1]; z=v[ 2]; }
	void Set( double _x, double _y, double _z)
	{	x = _x; y = _y; z = _z; }
	void SetX( int iD0, double v0, double v1, double v2)
	{	// set with specified dimension with wrap
		double* pV = (double*)( *this);
		pV[ iD0%3]=v0; pV[ (iD0+1)%3]=v1; pV[ (iD0+2)%3]=v2;
	}
	double* Get( double v[ 3]) const
	{	v[ 0]=x; v[ 1]=y; v[ 2]=z; return v; }
	float* Get( float v[ 3]) const
	{	v[ 0]=float( x); v[ 1]= float( y); v[ 2]= float( z); return v; }
	CPV3D& operator =(const CPV3D& p)
	{	x = p.x; y = p.y; z = p.z; return *this; }
	CPV3D& operator =(double v)
	{	x = y = z = v; }

	operator double*() { return &x; }
	operator const double*() const { return &x; }
	double operator []( int i) const
	{	return *(&x+i); }
	double& operator[]( int i)
	{	return *(&x+i); }
	
	// comparison
	bool IsEqual( const CPV3D& p, double tol=SMALL_NUM) const
	{	return fabs(x-p.x)<=tol && fabs(y-p.y)<=tol && fabs(z-p.z)<=tol; }
	bool operator ==(const CPV3D& p) const
	{	return IsEqual( p); }
	bool operator !=(const CPV3D& p) const
	{	return !IsEqual( p); }
	bool operator <(const CPV3D& p) const
	{	return !(x > p.x || y > p.y || z > p.z || p == *this); }
	bool operator >(const CPV3D& p) const
	{	return !(x < p.x || y < p.y || z < p.z || p == *this); }
	bool IsZero() const	// *exactly* 0
	{	return x==0. && y==0. && z==0.; }

	// operators: unary
	CPV3D operator-() const
	{	return CPV3D( -x, -y, -z); }

	// operators: scalar x CPV3D
	CPV3D operator *( double f) const
	{	return CPV3D( f*x, f*y, f*z); }
	CPV3D operator /( double f) const
	{	return CPV3D( x/f, y/f, z/f); }
	CPV3D& operator *=(double f)
	{	x *= f; y *= f; z *= f; return *this; }
	CPV3D& operator /=(double f)	// no div-by-0 check!
	{	x /= f; y /= f; z /= f; return *this; }

	// dot product
	double DotProd( const CPV3D& p) const
	{	return x*p.x + y*p.y + z*p.z; }
	double operator*( const CPV3D& p) const
	{	return DotProd( p); }

	// operators: add and subtract vectors
	CPV3D operator +( const CPV3D& p) const
	{	return CPV3D( x + p.x, y + p.y, z + p.z); }
	CPV3D operator -( const CPV3D& p) const
	{	return CPV3D( x - p.x, y - p.y, z - p.z); }
	CPV3D& operator +=(const CPV3D& p)
	{	x += p.x; y += p.y; z += p.z; return *this; }
	CPV3D& operator -=(const CPV3D& p)
	{	x -= p.x; y -= p.y; z -= p.z; return *this; }

	void AccumMinBound(const CPV3D& p)
	{	if (p.x < x) x = p.x;
		if (p.y < y) y = p.y;
		if (p.z < z) z = p.z;
	}
	void AccumMaxBound(const CPV3D& p)
	{	if (p.x > x) x = p.x;
		if (p.y > y) y = p.y;
		if (p.z > z) z = p.z;
	}

	double Normalize();
	inline double Length2() const { return x*x + y*y + z*z; }
	inline double Length() const { return sqrt( x*x + y*y + z*z); }
	WStr FmtXYZ( int precOp=3) const;
	CPV3D& Combine( const CPV3D& pt1, const CPV3D& pt2, double f2);
	inline double Distance2( const CPV3D& p) const
	{	return ::Distance2( *this, p); }
	double Distance( const CPV3D& p) const
	{	return ::Distance( *this, p); }
	int DomDim() const
	{	// return dimension of largest component
		const double* p = (double *)this;
		int i01 = fabs( p[ 1]) > fabs( p[ 0]);
		return fabs( p[ 2]) > fabs( p[ i01]) ? 2 : i01;
	}
	WStr Format() const
	{	return WStrPrintf( "(%.4f,%.4f,%.4f)", x, y, z); }

public:
	double x;
	double y;
	double z;
};		// class CPV3D
//-----------------------------------------------------------------------------
// ** Non-member functions **
// distance squared
inline double Distance2( const CPV3D& p1, const CPV3D& p2)
{	return (p2.x - p1.x) * (p2.x - p1.x)
		 + (p2.y - p1.y) * (p2.y - p1.y)
		 + (p2.z - p1.z) * (p2.z - p1.z);
}
// distance
inline double Distance( const CPV3D& p1, const CPV3D& p2)
{	return sqrt( Distance2( p1, p2)); }
// length squared
inline double Length2( const CPV3D p)
{	return p.x*p.x + p.y*p.y + p.z*p.z;	}
// length
inline double Length( const CPV3D p)
{	return sqrt( p.x*p.x + p.y*p.y + p.z*p.z); }
// scalar on left
inline CPV3D operator *( double f, const CPV3D& p)
{	return CPV3D( f*p.x, f*p.y, f*p.z); }
// cross product
inline CPV3D CrossProd( const CPV3D& a, const CPV3D& b)
{	return CPV3D( a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x); }
//-----------------------------------------------------------------------------
// constants
const CPV3D kCPV3D0( 0., 0., 0.);
const CPV3D kCPV3D1( 1., 1., 1.);
const CPV3D kCPV3Dx1( 1., 0., 0.);
const CPV3D kCPV3Dy1( 0., 1., 0.);
const CPV3D kCPV3Dz1( 0., 0., 1.);
//=============================================================================

#if !defined( USE_MFC)
//////////////////////////////////////////////////////////////////////////////
// class CPolygon3D: represents 3D polygon
//                 = array of 3D points with specialized methods
///////////////////////////////////////////////////////////////////////////////
class CPolygon3D
{
public:
	CPolygon3D( int n=4)
	{	if (n>0)
			p3_vrt.reserve( n);
	}
	CPolygon3D( const CPolygon3D& p3)
		: p3_vrt( p3.p3_vrt)
	{	/* Copy( ptsAr); */ }
	virtual ~CPolygon3D() { DeleteAll(); }
	void DeleteAll() { p3_vrt.clear(); }
	int GetSize() const { return p3_vrt.size(); }
	void SetSize( int n) { p3_vrt.resize( n); }
	CPolygon3D& Copy( const CPolygon3D& p3, bool bRev=false)
	{	int sz = p3.GetSize();
		SetSize( sz);
		for (int i=0; i < sz; i++)
			p3_Vrt( i) = p3.p3_Vrt( bRev ? sz-i-1 : i);
		return *this;
	}
	bool IsEmpty() const
	{	return p3_vrt.empty(); }

	// comparison
	int IsEqual( const CPolygon3D& p3, double tol=1e-9) const;
	int operator ==( const CPolygon3D& p3) const { return IsEqual( p3); }
	int operator !=( const CPolygon3D& p3) const { return !IsEqual( p3); }

	// vertex access
#if defined( _DEBUG)
	// debug: access with range check
	const CPV3D& p3_Vrt( int i) const { return p3_vrt.at( i); }
	CPV3D& p3_Vrt( int i) { return p3_vrt.at( i); }
#else
	// release: no range check
	const CPV3D& p3_Vrt( int i) const { return p3_vrt[ i]; }
	CPV3D& p3_Vrt( int i) { return p3_vrt[ i]; }
#endif

	// vertex access w/ wrap
	const CPV3D& p3_VrtW( int i) const
	{	int n = GetSize(); return p3_Vrt( (i+2*n)%n); }
	CPV3D& p3_VrtW( int i)
	{	int n = GetSize(); return p3_Vrt( (i+2*n)%n); }
	CPV3D& operator[](int i) {	return p3_VrtW( i); }
	const CPV3D& operator[](int i) const { return p3_VrtW( i); }

	// add 1 or more vertices at end
	void Add( const CPV3D& vrt)
	{	p3_vrt.push_back( vrt); }
	void Add( double x, double y, double z)
	{	p3_vrt.push_back( CPV3D( x, y, z)); }
	void Add( const double v[3])
	{	Add( CPV3D( v)); }
	void Add( const float v[3])
	{	Add( CPV3D( v)); }
	void Add( const float v[], int nV)
	{	for (int iV=0; iV<nV; iV++)
			Add( v+iV*3);
	}
	void InitAxisAlignedRect( int iD0, double v0, double L1, double L2, double v1=0., double v2=0.);
	void InitVertParallelogram( const CPV3D& p0, const CPV3D& p1, double zHt, int options=0);

	int IsAxisAligned( double* pV=NULL) const;
	bool IsHoriz( double* pZ=NULL) const
	{	return IsAxisAligned( pZ) == 2; }

	int GetAABB( CPV3D& ptMin, CPV3D& ptMax) const;
	int GetAABB( CAABB& box, int options=0) const;
	int GetCenter( CPV3D& ptC) const;
	int GetCentroid( CPV3D& ptC) const;
	int GetBestPlane( class CPlane3D& pln, bool bRev=false) const;
	double Area() const;
	double Area2D( int id0=0, int id1=1) const;
	double UnitNormal( CPV3D& uNormal) const;
	int GetLWRect( double& L, double& W) const;
	void GetLW( double& L, double& W) const;
	void HorizTM( class CT3D& T) const;

	int DoGLContour( class GLUtesselator* pTess, const TCHAR* id, int options=0);

#if defined( _DEBUG)
	static void Test1();
#endif
private:
	WVect< CPV3D> p3_vrt;		// vertices
};	// class CPolygon3D

///////////////////////////////////////////////////////////////////////////////
// class CPolygon3DAr: collection of 3D polygons (optionally shallow)
///////////////////////////////////////////////////////////////////////////////
class CPolygon3DAr
{
private:
	bool ap_bShallow;	// true iff shallow array (don't delete polygons in d'tor)
	WVect< CPolygon3D *> ap_plg;	// polygons
public:
	CPolygon3DAr( bool bShallow=false) : ap_bShallow( bShallow) { }
	~CPolygon3DAr() { if (!ap_bShallow) DeleteAll(); }
	int GetSize() const { return ap_plg.size(); }
	void Add( CPolygon3D* p)
	{	ap_plg.push_back( p); }
	void RemoveAll()
	{	ap_plg.clear(); }
	void DeleteAll()
	{	for (int i=0; i < GetSize(); i++)
			delete ap_Plg( i);
		ap_plg.clear();
	}
	void Copy( const CPolygon3DAr& sh)
	{	DeleteAll();
		for (int n = 0; n < sh.GetSize(); n++)
			Add( new CPolygon3D( *sh[ n]) );
	}
	bool IsEmpty() const
	{	return ap_plg.empty(); }
	// polygon access
#if defined( _DEBUG)
	// debug: access with range check
	CPolygon3D* ap_Plg( int i) const { return ap_plg.at( i); }
	CPolygon3D* ap_Plg( int i) { return ap_plg.at( i); }
#else
	// release: no range check
	CPolygon3D* ap_Plg( int i) const { return ap_plg[ i]; }
	CPolygon3D* ap_Plg( int i) { return ap_plg[ i]; }
#endif
	CPolygon3D* operator[](int i) {	return ap_Plg( i); }
	CPolygon3D* operator[](int i) const { return ap_Plg( i); }

	
	int GetAABB( CPV3D& ptMin, CPV3D& ptMax) const;
	int GetAABB( CAABB& box) const;
	const CPolygon3D* GetLowestHorizPlg() const
	{	CPolygon3DAr ar( true);		// shallow
		if (!GetHorizPlgs(ar))
			return NULL;
		return ar.GetLowOrHighZ_Ar( true);
	}
	const CPolygon3D* GetFirstHorizPlg() const
	{	for (int i=0; i<GetSize(); i++)
			if (ap_Plg( i)->IsHoriz())
				return ap_Plg( i);
		return NULL;
	}
	bool GetHorizPlgs( CPolygon3DAr& ar) const
	{	ar.RemoveAll();
		for (int i=0; i<GetSize(); i++)
			if (ap_Plg( i)->IsHoriz())
				ar.Add( ap_Plg(i));
		return !ar.IsEmpty();
	}
	bool GetElevAndHt( double& elev, double& ht)
	{	CPolygon3DAr ar( true);
		if (!GetHorizPlgs( ar))
			return false;
		return ar.GetHorizElevAndHt( elev, ht);
	}
	CPolygon3D* GetLowOrHighZ_Ar( bool bLow);
	CPolygon3D* GetLowZ_Ar()	{ return GetLowOrHighZ_Ar( true); }
	CPolygon3D* GetHighZ_Ar()  { return GetLowOrHighZ_Ar( false); }
	bool GetHorizElevAndHt( double& elev, double& ht);
};	// class CPolygon3DAr
#else
// MFC versions ...

//////////////////////////////////////////////////////////////////////////////
// class CPolygon3D: represents 3D polygon
//                 = array of 3D points with specialized methods
///////////////////////////////////////////////////////////////////////////////
class CPolygon3D : public CArray< CPV3D, const CPV3D& >
{
public:
	CPolygon3D() : CArray< CPV3D, const CPV3D&>() {}
	CPolygon3D( const CPolygon3D& ptsAr)
		: CArray< CPV3D, const CPV3D&>()
	{	Copy( ptsAr); }
	virtual ~CPolygon3D() { DeleteAll(); }
	void DeleteAll()
	{	RemoveAll();
	}
	CPolygon3D& Copy( const CPolygon3D& ptsAr, bool bRev=false)
	{	int sz = ptsAr.GetSize();
		SetSize( sz);
		for (int i=0; i < sz; i++)
			SetAt( i, ptsAr.GetAt( bRev ? sz-i-1 : i));
		return *this;
	}
	bool IsEmpty() const
	{	return GetSize() == 0; }

	// comparison
	int IsEqual( const CPolygon3D& plg, double tol=1e-9) const;
	int operator ==( const CPolygon3D& plg) const { return IsEqual( plg); }
	int operator !=( const CPolygon3D& plg) const { return !IsEqual( plg); }

	// vertex access
	const CPV3D& p3_Vrt( int i) const { return GetAt( i); }
	CPV3D& p3_Vrt( int i) { return GetAt( i); }

#if 1
	// vertex access w/ wrap
	const CPV3D& p3_VrtW( int i) const
	{	int n = GetSize(); return p3_Vrt( (i+2*n)%n); }
	CPV3D& p3_VrtW( int i)
	{	int n = GetSize(); return p3_Vrt( (i+2*n)%n); }
	CPV3D& operator[](int i) {	return p3_VrtW( i); }
	const CPV3D& operator[](int i) const { return p3_VrtW( i); }
#else
	// point access (wraps)
	const CPV3D& GetPt( int i) const
	{	int n = GetSize(); return *(GetData()+(i+2*n)%n); }
	CPV3D& GetPt( int i)
	{	int n = GetSize(); return *(GetData()+(i+2*n)%n); }
	CPV3D& operator[](int i) {	return GetPt( i); }
	const CPV3D& operator[](int i) const { return GetPt( i); }
#endif

	void AddPoint( double x, double y, double z)
	{	Add( CPV3D( x, y, z)); }

	void InitAxisAlignedRect( int iD0, double v0, double L1, double L2, double v1=0., double v2=0.);
	void InitVertParallelogram( const CPV3D& p0, const CPV3D& p1, double zHt, int options=0);

	int IsAxisAligned( double* pV=NULL) const;
	bool IsHoriz( double* pZ=NULL) const
	{	return IsAxisAligned( pZ) == 2; }

	int GetAABB( CPV3D& ptMin, CPV3D& ptMax) const;
	int GetAABB( CAABB& box, int options=0) const;
	int GetCenter( CPV3D& ptC) const;
	int GetCentroid( CPV3D& ptC) const;
	int GetBestPlane( class CPlane3D& pln, bool bRev=false) const;
	double Area() const;
	double Area2D( int id0=0, int id1=1) const;
	double UnitNormal( CPV3D& uNormal) const;
	int GetLWRect( double& L, double& W) const;
	void GetLW( double& L, double& W) const;
	void HorizTM( class CT3D& T) const;

	int DoGLContour( class GLUtesselator* pTess, const TCHAR* id, int options=0);

#if defined( _DEBUG)
	static void Test1();
#endif
};	// class CPolygon3D
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CPolygon3DAr: array of 3D polygons (optionally shallow)
///////////////////////////////////////////////////////////////////////////////
class CPolygon3DAr : public CTypedPtrArray< CPtrArray, CPolygon3D* >
{
private:
	bool ap_bShallow;	// true iff shallow array (don't delete polygons in d'tor)
public:
	CPolygon3DAr( bool bShallow=false) : ap_bShallow( bShallow) { }
	~CPolygon3DAr() { if (!ap_bShallow) DeleteAll(); }
	void DeleteAll()
	{	for (int i=0; i < GetSize(); i++)
			delete GetAt( i);
		RemoveAll();
	}
	void Copy( const CPolygon3DAr& sh)
	{	DeleteAll();
		for (int n = 0; n < sh.GetSize(); n++)
			Add( new CPolygon3D( *sh[ n]) );
	}
	bool IsEmpty() const
	{	return GetSize() == 0; }
		// polygon access
	CPolygon3D* ap_Plg( int i) const { return GetAt( i); }
	CPolygon3D* ap_Plg( int i) { return GetAt( i); }
#if 0
	CPolygon3D* operator[](int i) {	return ap_Plg( i); }
	CPolygon3D* operator[](int i) const { return ap_Plg( i); }
#endif
	
	int GetAABB( CPV3D& ptMin, CPV3D& ptMax) const;
	int GetAABB( CAABB& box) const;
	CPolygon3D* GetLowestHorizPlg() const
	{	CPolygon3DAr ar( true);		// shallow
		if (!GetHorizPlgs(ar))
			return NULL;
		return ar.GetLowOrHighZ_Ar( true);
	}
	CPolygon3D* GetFirstHorizPlg() const
	{	for (int i=0; i<GetSize(); i++)
			if (GetAt( i)->IsHoriz())
				return GetAt( i);
		return NULL;
	}
	bool GetHorizPlgs( CPolygon3DAr& ar) const
	{	ar.RemoveAll();
		for (int i=0; i<GetSize(); i++)
			if (GetAt( i)->IsHoriz())
				ar.Add( GetAt(i));
		return ar.GetSize() > 0;
	}
	bool GetElevAndHt( double& elev, double& ht)
	{	CPolygon3DAr ar( true);
		if (!GetHorizPlgs( ar))
			return false;
		return ar.GetHorizElevAndHt( elev, ht);
	}
	CPolygon3D* GetLowOrHighZ_Ar( bool bLow);
	CPolygon3D* GetLowZ_Ar()	{ return GetLowOrHighZ_Ar( true); }
	CPolygon3D* GetHighZ_Ar()  { return GetLowOrHighZ_Ar( false); }
	bool GetHorizElevAndHt( double& elev, double& ht);
};	// class CPolygon3DAr
#endif	// USE_MFC
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CPlane3D: plane in 3D
///////////////////////////////////////////////////////////////////////////////
class CPlane3D
{
public:
	CPlane3D() { Zero(); }
	CPlane3D( const CPlane3D& pl) { *this = pl; }
	CPlane3D( const CPV3D& p1, const CPV3D& p2, const CPV3D& p3)
	{	Init( p1, p2, p3); }
	CPlane3D( const class CPolygon3D& plg3);
	CPlane3D( const class CTri3D& tri);
	CPlane3D( const CPV3D& uNorm, double d) { Init( uNorm, d); }
	CPlane3D( const CPV3D& uNorm, const CPV3D& pt) { Init( uNorm, pt); }

	void Zero() { pl_n.Zero(); pl_d=0.; }
	bool Init( const class CTri3D& tri);
	bool Init( const CPV3D& uNorm, double d);
	bool Init( const CPV3D& uNorm, const CPV3D& pt);
	bool Init( const CPV3D& p1, const CPV3D& p2, const CPV3D& p3);
	bool Normalize();
	CPlane3D& FixSmall();
	CPV3D& GetNormal() { return pl_n; }
	const CPV3D& GetNormal() const { return pl_n; }
	operator CPV3D&() { return pl_n; }
	operator const CPV3D&() const { return pl_n; }

	const CPlane3D& operator =(const CPlane3D& pl)
	{	pl_n = pl.pl_n; pl_d=pl.pl_d; return *this; }
	bool operator==( const CPlane3D& pl) const
	{	return IsEqual( pl, 0.); }
	bool operator!=( const CPlane3D& pl) const
	{	return !IsEqual( pl, 0.); }
	bool IsEqual( const CPlane3D& pl, double tol=SMALL_NUM) const
	{	return pl_n.IsEqual( pl.pl_n, tol) && fabs( pl_d-pl.pl_d) < tol; }
	bool IsZero() const
	{	return pl_n.IsZero(); }
	// Distance between point and  plane (perpendicular from point to plane)
	double Distance( const CPV3D& pt) const;
	double Distance( const CPV3D& pt, CPV3D& ptP) const;
	double AngleCos( const CPlane3D& pl) const;	// cosine of the angle between 2 planes
												// -2 - if ERROR
	int IntersectSegment( const CPV3D& a, const CPV3D& b, CPV3D& pt, double& t) const;
	int IntersectRay( const CPV3D& a, const CPV3D& dc, CPV3D& pt) const;
	int IntersectPlane( const CPlane3D& pl, CPV3D& pt, CPV3D& dir) const;

private:
	// plane equation: pl_n.x*X + pl_n.y*Y + pl_n.z*Z + pl_d = 0
	//              or dot( pl_n, p) + pl_d = 0
	// Note: values not necessarily normalized
	CPV3D pl_n;	// normal
	double pl_d;

};	// class CPlane3D
//=============================================================================

/////////////////////////////////////////////////////////
// class CTri3D - 3D triangle
/////////////////////////////////////////////////////////
class CTri3D
{
public:
	CTri3D() {};
	CTri3D( const CTri3D& tri)
	{	*this = tri; }
	CTri3D (const CPV3D& dPt1, const CPV3D& dPt2, const CPV3D& dPt3)
	{	pt[0] = dPt1; pt[1] = dPt2; pt[2] = dPt3;}

	const CTri3D& operator =(const CTri3D& tri)
	{	pt[0] = tri.pt[0]; pt[1] = tri.pt[1]; pt[2] = tri.pt[2]; return *this; }
	void Set9( const double* v);		// set from 9 value vector
	const CPV3D& Vertex( int i) const
	{	return pt[ i]; }
	CPV3D& Vertex( int i) { return pt[ i]; }
	int IsEqual( const CTri3D& tri, double tol=1e-9) const
	{	for (int i=0; i<3; i++)
		{	if (!Vertex( i).IsEqual( tri.Vertex( i), tol))
				return 0;
		}
		return 1;
	}
	CPV3D* Vertex0() { return pt; }
	const CPV3D* Vertex0() const { return pt; }
	double* Vertex( int i, double v[ 3]) const
	{	return pt[ i].Get( v); }
	float* Vertex( int i, float v[ 3]) const
	{	return pt[ i].Get( v); }
	double UnitNormal( CPV3D& uNormal) const;
	double Area() const;	//returns area of triangle
	// side length of triangle
	double EdgeLength( int iE) const
	{	return pt[(iE+1)%3].Distance( pt[(iE+2)%3]); }
	int Split( CTri3D& tri1, CTri3D& tri2, int iS=-1) const;
	void EdgePts( int iE, CPV3D* ptE[ 2])
	{	ptE[ 0] = pt+((iE+1)%3);
	    ptE[ 1] = pt+((iE+2)%3);
	}
	int LongestEdge() const;
	virtual void CopyX( const CTri3D& src, int iVX, const CPV3D& ptX);
	void Centroid( CPV3D& ptCent) const;
	int PointIn( const CPV3D& p) const;
	int RayHit( const CPV3D& p1, const CPV3D& p2,
		const CPV3D& ab, const CPV3D& ac, const CPV3D& n) const;
	int RayHit( const CPV3D& p1, const CPV3D& p2) const;

private:
	CPV3D pt[3];		// vertices
};		// class CTri3D
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CAABB: axis-aligned bounding box
///////////////////////////////////////////////////////////////////////////////
class CAABB
{
friend class CPolygon3D;
public:
	CAABB() { pt[ 0].Zero(); pt[ 1].Zero(); }
	CAABB( const CAABB& box) { Copy( box); }
	CAABB( const CPV3D& ptMin, const CPV3D& ptMax)
	{	Set( ptMin, ptMax); }
	void Set( const CPV3D& ptMin, const CPV3D& ptMax)
	{	pt[ 0] = ptMin; pt[ 1] = ptMax; }
	CAABB& Copy( const CAABB& box)
	{	pt[ 0] = box.pt[ 0];  pt[ 1] = box.pt[ 1];
		return *this;
	}
	CAABB& operator=( const CAABB& box) { return Copy( box); }

	CPV3D GetCenter() const
	{	return (pt[0] + pt[1]) / 2.; }
	void Accum( const CPV3D& pt);
	const CPV3D& GetPt( int i) const
	{	return pt[ i]; }
	CPV3D& GetPt( int i)
	{	return pt[ i]; }
	CPV3D& operator[](int i) {	return GetPt( i); }
	const CPV3D& operator[](int i) const { return GetPt( i); }

private:
	CPV3D pt[ 2];		// [ 0] = min point, [ 1] = max point

};		// class CAABB
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CArrow3D -- wire frame arrow
///////////////////////////////////////////////////////////////////////////////
class CArrow3D
{
public:
	CPV3D a3_tail;			// coords of tail
	CPV3D a3_head;			// coords of head (with pointer)
	CPV3D a3_headEnd[ 2];	// coords of ends of pointer lines

	CArrow3D() {}
	void a3_Zero();
	int a3_Init( CPV3D dir, double length=1., const CPV3D& tail=kCPV3D0);

};	// CArrow3D
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CT3D: 4x4 3D tranformation matrix
///////////////////////////////////////////////////////////////////////////////
class CT3D
{
public:
	CT3D( int options=1) { Init( options); }
	CT3D( const CT3D& M) { Copy( M); }
	virtual ~CT3D() { }
	CT3D& Init( int options=1);
	CT3D& Scale( double sx, double sy, double sz);
	CT3D& Scale( double s);
	CT3D& Translate( double dx, double dy, double dz);
	CT3D& Translate( const CPV3D& p);
	CT3D& Rearrange( int iX1, int iY1, int iZ1);
	CT3D& Rotate( double angle, int xyz);
	CT3D& Concat( const CT3D& M);
	CT3D& Copy( const CT3D& M)
	{	VCopy( v, 16, M.v); return *this; }
	CT3D& operator=( const CT3D& M) { return Copy( M); }
	CPV3D TX( const CPV3D& p) const;
	CPolygon3D TX( const CPolygon3D& plg) const;
	double& Vrc( int r, int c)
	{	return v[ (c<<2) + r]; }
	const double& Vrc( int r, int c) const
	{	return v[ (c<<2) + r]; }
	double& operator()(int r, int c)
	{	return v[ (c<<2) + r]; }
	const double& operator()(int r, int c) const
	{	return v[ (c<<2) + r]; }

private:
	double v[ 16];
};	// class CT3D
//==============================================================================

////////////////////////////////////////////////////////////////////////////////
// public functions
////////////////////////////////////////////////////////////////////////////////
double Det33( double a11, double a12, double a13,
	double a21, double a22, double a23, double a31, double a32, double a33);
//==============================================================================

#endif // __GEOMETRY_H__
