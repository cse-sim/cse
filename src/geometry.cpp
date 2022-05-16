//==========================================================================
//	Geometry.cpp -- contains different computational geomertic algoritms
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


#include "CNGLOB.H"
#include "geometry.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// debugging output
//   use MFC TRACE if available
//   else printf
#if !defined( TRACE)
#define TRACE printf
#endif

///////////////////////////////////////////////////////////////////////////////
// class CT3D: 4x4 3D transformation
///////////////////////////////////////////////////////////////////////////////
CT3D& CT3D::Init(
	int options/*=1*/)		//  0: set all to 0
							//  1: set to identity matrix
							// -1: don't init
// initialize matrix
{
	if (options == 1)
	{	// identity matrix
		for (int r=0; r<4; r++)
		{	for (int c=0; c<4; c++)
				Vrc( r, c) = double( r==c);
		}
	}
	else if (options >= 0)
		VZero( v, 16);
	// else don't init
	return *this;
}		// CT3D::Init
//----------------------------------------------------------------------------
CT3D& CT3D::Scale( double sx, double sy, double sz)
{	Vrc( 0, 0) *= sx;
	Vrc( 1, 1) *= sy;
	Vrc( 2, 2) *= sz;
	return *this;
}	// CT3D::Scale
//----------------------------------------------------------------------------
CT3D& CT3D::Scale( double s)
{	Vrc( 0, 0) *= s;
	Vrc( 1, 1) *= s;
	Vrc( 2, 2) *= s;
	return *this;
}	// CT3D::Scale
//----------------------------------------------------------------------------
CT3D& CT3D::Translate( double dx, double dy, double dz)
{	Vrc( 0, 3) += dx;
	Vrc( 1, 3) += dy;
	Vrc( 2, 3) += dz;
	return *this;
}		// CT3D::Translate
//----------------------------------------------------------------------------
CT3D& CT3D::Translate( const CPV3D& pt)
{	Vrc( 0, 3) += pt.x;
	Vrc( 1, 3) += pt.y;
	Vrc( 2, 3) += pt.z;
	return *this;
}		// CT3D::Translate
//-----------------------------------------------------------------------------
CT3D& CT3D::Rearrange(		// apply axis rearrangement (aka swizzling)
	int iX1,		// source for X
	int iY1,		// source for Y
	int iZ1)		// source for Z
// source vars are dimension+1 and sign of data source
// examples:
//     Rearrange( +1, -2, +3)  means x' = x, y' = -y, z' = z (reflection)
//     Rearrange( +1, +3, +2)  means x' = x, y' = z, z' = y (axis swap)
{
	CT3D R( 0);		// 0 matrix

	static const double pm1[] = { -1., +1. };
	R( 0, abs( iX1)-1) = pm1[ iX1 > 0];
	R( 1, abs( iY1)-1) = pm1[ iY1 > 0];
	R( 2, abs( iZ1)-1) = pm1[ iZ1 > 0];
	R( 3, 3) = 1.;
	return Concat( R);
}		// CT3D::Rearrange
//-----------------------------------------------------------------------------
CT3D& CT3D::Rotate(		// apply rotation (left hand coord system)
	double angle,	// angle to rotate (radians)
					//   + per left hand rule (clockwise)
	int xyz)		// 0, 1, or 2 = axis about which to rotate
{
	double sinA = sin( angle);
	double cosA = cos( angle);
	if (xyz < 0)
	{	sinA = -sinA;
		xyz = abs( xyz);
	}
	CT3D R( 1);		// identity matrix
	if (xyz == 0)
	{	R( 1, 1) = cosA;
		R( 1, 2) = -sinA;
		R( 2, 1) = sinA;
		R( 2, 2) = cosA;
	}
	else if (xyz == 1)
	{	R( 0, 0) = cosA;
		R( 0, 2) = sinA;
		R( 2, 0) = -sinA;
		R( 2, 2) = cosA;
	}
	else
	{	R( 0, 0) = cosA;
		R( 0, 1) = -sinA;
		R( 1, 0) = sinA;
		R( 1, 1) = cosA;
	}
	return Concat( R);
}		// CT3D::Rotate
//----------------------------------------------------------------------------
CT3D& CT3D::Concat(			// concatenate matrices
	const CT3D& M)
// return this = M * this (modified in place)
{
	CT3D MX( -1);	// don't init
	for (int r=0; r<4; r++)
	{	for (int c=0; c<4; c++)
		{	MX( r, c) = M( r, 0) * Vrc( 0, c);
			for (int i=1; i<4; i++)
				MX( r, c) += M( r, i) * Vrc( i, c);
		}
	}
	return Copy( MX);
}		// CT3D::Concat
//----------------------------------------------------------------------------
CPV3D CT3D::TX(		// apply transformation to a point
	const CPV3D& p) const	// input point (column vector)
// return p' = [this]*p
{
	// implicit w = p[ 3] = 1
	// init point to last col
	CPV3D tP( Vrc(0, 3), Vrc( 1, 3), Vrc( 2, 3));
	for (int r=0; r<3; r++)
	{	for (int c=0; c<3; c++)
			tP[ r] += Vrc( r, c) * p[ c];
	}
	return tP;
}	// CT3D::TX
//----------------------------------------------------------------------------
CPolygon3D CT3D::TX(		// apply transformation to polygon
	const CPolygon3D& plg) const	// source polygon
{
	CPolygon3D tPlg;
	int nV = plg.GetSize();
	tPlg.SetSize( nV);
	for (int iV=0; iV<nV; iV++)
		tPlg[ iV] = TX( plg[ iV]);
	return tPlg;
}	// CT3D::TX
//----------------------------------------------------------------------------
double Det33(		// determinant of 3x3 matrix
	double a11, double a12, double a13,
	double a21, double a22, double a23,
	double a31, double a32, double a33)
{
	return a11*(a22*a33-a23*a32) + a12*(a23*a31-a21*a33)+a13*(a21*a32-a22*a31);
}	// ::Det33
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// 3D point / vector
///////////////////////////////////////////////////////////////////////////////
WStr CPV3D::FmtXYZ(			// format coordinates
	int precOp/*=3*/) const		// lo bits = precision
								// options: 0x100 = surround with parens (x,y,z)
// returns coords as comma-separated string (with optional parens
{	
	int prec = precOp & 0xff;
	bool bParens = (precOp & 0x100) != 0;
	WStr t;
	if (bParens)
		t = "(";
	t += WStrFmtFloatDTZ( x, prec) + ',' + WStrFmtFloatDTZ( y, prec) + ',' + WStrFmtFloatDTZ( z, prec);
	if (bParens)
		t += ")";
	return t;
}		// CPV3D::FmtXYZ
//-----------------------------------------------------------------------------
CPV3D& CPV3D::Combine(		// weighted combination of points
	const CPV3D& pt1, const CPV3D& pt2, double f2)
// Example: call with f2 = .5 to return midpoint
{
	double f1 = 1. - f2;
	x = f1*pt1.x + f2*pt2.x;
	y = f1*pt1.y + f2*pt2.y;
	z = f1*pt1.z + f2*pt2.z;
	return *this;
}		// CPV3D::Combine
//-----------------------------------------------------------------------------
double CPV3D::Normalize()
{	double d = Length();
	if (d != 1.)
	{	if (d > 1e-9)
			(*this) /= d;
		else
		{	d = 0.;
			Zero();
		}
	}
	return d;
}		// CPV3D::Normalize
//-----------------------------------------------------------------------------
int CPV3D::AzmTilt(		// normal -> azm/tilt (radians)
	double& azmR,			// returned: azm (0=N, +clockwise from above)
	double& tiltR) const	// returned: tilt (0=facing up, pi/2=vertical, pi=facing down)	
// returns 0 iff *this has 0 length
//    else 1 = success
{
	double d = Length();
	if (d < 1e-9)
	{	azmR = 0.;
		tiltR = 0.;
		return 0;
	}

	if (fabs( z/d) > 1.-1e-9)
	{	tiltR = z>0. ? 0. : -kPi;
		azmR = 0.;
	}
	else
	{	tiltR = acos( z/d);
		azmR = atan2( x/d, y/d);
	}

	return 1;
}	// CPV3D::AzmTilt
//----------------------------------------------------------------------------
int CPV3D::AzmTiltD(		// normal -> azm/tilt (degrees)
	float& azmD,
	float& tiltD) const
{	
	double azmR, tiltR;	
	int ret = AzmTilt( azmR, tiltR);	// returns radians
	azmD = float( DEG( azmR));
	tiltD = float( DEG( tiltR));
	return ret;
}		// CPV3D:AzmTiltD
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CPolygon3D
///////////////////////////////////////////////////////////////////////////////
int CPolygon3D::IsEqual( const CPolygon3D& p3, double tol /*=1e-9*/) const
{	int n = GetSize();
	if (n != p3.GetSize())
		return 0;		// different # of verticies
	for (int i=0; i<n; i++)
	{	if (!p3.p3_Vrt( i).IsEqual( p3_Vrt( i), tol))
			return 0;	// point mismatch
	}
	return 1;
}		// CPolygon3D::IsEqual
//-----------------------------------------------------------------------------
void CPolygon3D::InitAxisAlignedRect(		// initialize a axis-aligned rectangle
	int iD0,	// constant dimension (0=x, 1=y, 2=z)
	double v0,	// value for constant dimension
	double L1,	// length in dimension (iD0+1)%3
	double L2,	// length in dimension (iD0+2)%3
	double v1 /*=0.*/,		// origin in dimension (iD0+1)%3
	double v2 /*=0.*/)		// origin in dimension (iD0+2)%3
{
	SetSize( 4);
	
	p3_Vrt( 0).SetX( iD0, v0, v1, v2);
	p3_Vrt( 1).SetX( iD0, v0, v1+L1, v2);
	p3_Vrt( 2).SetX( iD0, v0, v1+L1, v2+L2);
	p3_Vrt( 3).SetX( iD0, v0, v1, v2+L2);

#if defined( _DEBUG)
	double L, W;
	GetLW( L, W);
	if (  (fabs( L - L1) > .001 && fabs( L-L2) > .001)
	   || (fabs( W - L1) > .001 && fabs( W-L2) > .001))
	   TRACE( "CPolygon3D::InitAxisAlignedRect: L/W mismatch!");
#endif

}		// CPolygon3D::InitAxisAlignedRect
//----------------------------------------------------------------------------
void CPolygon3D::InitVertParallelogram(
	const CPV3D& p0,	// base line
	const CPV3D& p1,	
	double zHt,			// height
	int options/*=0*/)	// options
						//   xgoREVERSE: reverse point order
{
	SetSize( 4);

	int rev = (options&xgoREVERSE) != 0;
	p3_Vrt( rev) = p0;
	p3_Vrt( !rev) = p1;

	p3_Vrt( 2) = p3_Vrt( 1);
	p3_Vrt( 2).z += zHt;

	p3_Vrt( 3) = p3_Vrt( 0);
	p3_Vrt( 3).z += zHt;
#if defined( _DEBUG)
	double lenBase = p0.Distance( p1);
	double L, W;
	GetLW( L, W);
	if (  (fabs( L - zHt) > .001 && fabs( L-lenBase) > .001)
	   || (fabs( W - zHt) > .001 && fabs( W-lenBase) > .001))
	{	TRACE( "CPolygon3D::InitVertParallelogram: L/W mismatch!");
		GetLW( L, W);		// call again re debugging
	}
#endif
}		// CWPolygon3D::InitVertParallelogram
//------------------------------------------------------------------------
int CPolygon3D::GetAABB( CPV3D& ptMin, CPV3D& ptMax) const
// get bounding rectangle (bounding box if not planar)
// returns # of points
{
	int ret = GetSize();
	if (ret <= 0)
	{	ptMin.Zero();
		ptMax.Zero();
	}
	else
	{	ptMin = ptMax = p3_Vrt( 0);
		for (int i=1; i<GetSize(); i++)
		{	const CPV3D& P = p3_Vrt( i);
			ptMin.AccumMinBound( P);
			ptMax.AccumMaxBound( P);
		}
	}
	return ret;
}	// CPolygon3D::GetAABB
//-----------------------------------------------------------------------------
int CPolygon3D::GetAABB(		// axis-aligned bounding box
	CAABB& box,			// returned: AABB for this polygon
						//   (or updated if no init)
	int options /*=0*/) const	// option bits
								//   1: don't init (re AABB of shell)
// returns # of points
{
	int bInit = !(options & 1);
	int nV = GetSize();
	if (nV <= 0 && bInit)
	{	box.pt[ 0].Zero();
		box.pt[ 1].Zero();
	}
	else
	{	int iV0 = 0;
		if (bInit)
		{	box.pt[ 0] = box.pt[ 1] = p3_Vrt( 0);	// init min and max to 1st vertex
			iV0 = 1;								// start compare at 2nd vertex
		}
		for (int iV=iV0; iV<nV; iV++)
		{	CPV3D P = p3_Vrt( iV);
			box.pt[ 0].AccumMinBound( P);
			box.pt[ 1].AccumMaxBound( P);
		}
	}
	return nV;
}	// CPolygon3D::GetAABB
//-----------------------------------------------------------------------------
int CPolygon3D::GetCenter( CPV3D& ptC) const		// center
// find center of bounding rectangle (not true centroid)
// returns # of points
{
	CAABB box;
	int ret = GetAABB( box);
	ptC = box.GetCenter();
	return ret;
}		// CPolygon3D::GetCenter
//-----------------------------------------------------------------------------
int CPolygon3D::GetCentroid( CPV3D& ptC) const		// centroid
// returns # of points
{
	ptC.Zero();
	int nP = GetSize();
	if (nP > 1)
	{	ptC = p3_Vrt( 0);
		for (int iP=1; iP<nP; iP++)
			ptC += p3_Vrt( iP);
		ptC /= nP;
	}
	return nP;
}		// CPolygon3D::GetCentroid
//-----------------------------------------------------------------------------
int CPolygon3D::GetBestPlane(			// best-fit plane for this polygon
	CPlane3D& pln,				// returned: plane
	bool bRev/*=false*/) const	//   TRUE: reverse side
								//   = inside plane as opposed to outside
{
	CPV3D ptC;
	int nP = GetCentroid( ptC);
	if (nP > 2)
	{	CPV3D uNorm;
		UnitNormal(	uNorm);
		if (bRev)
			uNorm *= -1.;
		double d = ptC.DotProd( uNorm);
		pln.Init( uNorm, -d);
		pln.Normalize();
	}
	else
		pln.Zero();
	return nP;
}		// CPolygon3D::GetBestPlane
//----------------------------------------------------------------------------
double CPolygon3D::CheckFix(
	int options /*=0*/)	// option bits
						//   1: alter verticies to be exactly on best plane
// returns largest distance from any vertex to best plane
//             (*this modified iff options&1)
//         -1 if failure, *this unchanged (can't make plane, )
{
	CPlane3D pln;
	int nP = GetBestPlane( pln);
	if (nP < 3)
		return -1.;

	return pln.CheckFixPolygon( *this, options);

}		// CPolygon3D
//-----------------------------------------------------------------------------
double CPolygon3D::Area2D(
	int id0/*=0*/, int id1/*=1*/) const
{
    double sum = 0.;
	int n = GetSize();
	for (int i=0; i<n; i++)
		sum += p3_VrtW( i+1)[ id0]*(p3_VrtW( i+2)[ id1]-p3_VrtW( i)[ id1]);
    return sum / 2.;
}		// CPolygon3D::Area2D
//-----------------------------------------------------------------------------
double CPolygon3D::UnitNormal(			// find unit normal (aka direction cosines)
	CPV3D& uNormal) const	// returned: unit normal vector
// returns: unnormalized length = area of polygon
//          -1 if normal cannot be determined
{
    // get the Newell normal
	for (int i=0; i<3; i++)
		uNormal[ i] = Area2D( (i+1)%3, (i+2)%3);

    // get length of the Newell normal
    double nLen = uNormal.Length(); // sqrt( nwx*nwx + nwy*nwy + nwz*nwz );
	if (nLen < SMALL_NUM)
		return -1.;		// uNormal is (0,0,0) or close to it

    // compute the unit normal
	uNormal /= nLen;

    return nLen;    // area of polygon = length of Newell normal
}	// CPolygon3D::UnitNormal
//-----------------------------------------------------------------------------
double CPolygon3D::Area() const
{
	CPV3D uNormal;
	double area = UnitNormal( uNormal);
	return area;
}		// CPolygon3D::Area
//------------------------------------------------------------------------------
int CPolygon3D::GetLWRect(		// length / width assuming rectangular, orthogonal surf
	double& L,				// returned: length (always > 0)
	double& W) const		// returned: width (always > 0)
{
	int ret;
	CPV3D ptMin, ptMax;
	GetAABB( ptMin, ptMax);
	CPV3D ptX( ptMax - ptMin);
	if (fabs( ptX.x) < .01)
	{	// y-z plane
		L = ptX.y;
		W = ptX.z;
		ret = 0;
	}
	else if (fabs( ptX.y) < .01)
	{	// x-z plane
		L = ptX.x;
		W = ptX.z;
		ret = 1;
	}
	else if (fabs( ptX.z) < .01)
	{	// x-y plane
		L = ptX.x;
		W = ptX.y;
		ret = 2;
	}
	else
	{	L = 0.;
		W = 0.;
		ret = -1;
	}
	return ret;
}		// CPolygon3D::GetLWRect
//------------------------------------------------------------------------------
void CPolygon3D::GetLW(	// length and width of surrounding rectangle
	double& L,				// returned: length (always > 0)
	double& W) const		// returned: width (always > 0)
{
	CT3D THoriz;
	HorizTM( THoriz);
	CPolygon3D plgX = THoriz.TX( *this);
	CAABB box;
	plgX.GetAABB( box);
#if defined( _DEBUG)
	if (fabs( box[ 0].z) > .001 || fabs( box[ 1].z) > .001)
	{	// if horiz transform is OK, z is 0
		TRACE( "CPolygon3D::GetLW: bad z");
		HorizTM( THoriz);		// recall for debugging
	}
#endif
	L = box[ 1].x - box[ 0].x;
	W = box[ 1].y - box[ 0].y;
}	// CPolygon3D::GetLW
//------------------------------------------------------------------------------
void CPolygon3D::HorizTM(	// generate transformation matrix to horiz
	CT3D& T) const
{
	CPV3D cent;
	GetCentroid( cent);
	CPV3D DC;
	UnitNormal( DC);
	double tiltR = acos( DC[ 2]);
	double azmR = atan2( DC[ 1], DC[ 0]);
	azmR += kPiOver2;
	T.Init();					// identity matrix
	T.Translate( -cent);
	T.Rotate( -azmR,  2);		// rotate by azm about Z-axis
	T.Rotate( -tiltR, 0);		// rotate by tilt about X-axis
}	// CPolygon3D::HorizTM
//-----------------------------------------------------------------------------
int CPolygon3D::IsAxisAligned( double* pV /*=NULL*/) const
// returns -2: degenerate (point or line)
//         -1: no
//      0 - 2: dimension that is constant (0=x, 1=y, 2=z)
{	
	if (GetSize() <= 2)
	{	if (pV)
			*pV = 0.;
		return -2;
	}
	const CPV3D& pt0( p3_Vrt( 0));
	int iDMisMatch[ 3] = { 0 }; // 0 matches so far, 1=does not
	int nMisMatch = 0;
	for (int iV=1; iV<GetSize(); iV++)
	{	nMisMatch = 0;
		for (int iD=0; iD<3; iD++)
		{	iDMisMatch[ iD] |= !equalApr( p3_Vrt( iV)[ iD], pt0[ iD]);
			nMisMatch += iDMisMatch[ iD];
		}
		if (nMisMatch == 3)
			break;		// mismatch in all 3 dimensions, can't be aligned
	}
	int iDX = nMisMatch <= 1 ? -2						// 0 or 1: degenerate
	        : nMisMatch == 2 ? VFind( iDMisMatch, 3, 0)	// 2: aligned
		    :                  -1;						// 3: not aligned
	if (pV)
		*pV = iDX >= 0 ? pt0[ iDX] : 0.;
	return iDX;
}		// CPolygon3D::IsAxisAligned
//------------------------------------------------------------------------------
#if defined( _DEBUG)
/*static*/ void CPolygon3D::Test1()		// test code
{
	CPolygon3D plg3D;

	CPV3D p0( 0., 0., 0.);
	CPV3D p1( 1., 1., 0.);
	plg3D.InitVertParallelogram( p0, p1, 1.);
	plg3D.InitVertParallelogram( p1, p0, 1.);
	p1.Set( 1., -1., 0.);
	plg3D.InitVertParallelogram( p0, p1, 1.);
	plg3D.InitVertParallelogram( p1, p0, 1.);
	p1.Set( -1., 1., 0.);
	plg3D.InitVertParallelogram( p0, p1, 1.);
	plg3D.InitVertParallelogram( p1, p0, 1.);
	p1.Set( -1., -1., 0.);
	plg3D.InitVertParallelogram( p0, p1, 1.);
	plg3D.InitVertParallelogram( p1, p0, 1.);


}		// CPolygon3D::Test1
#endif	// _DEBUG
//==============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CPolygon3DAr - array of CPolygon3D
///////////////////////////////////////////////////////////////////////////////
bool CPolygon3DAr::GetHorizElevAndHt(
	double& elev,		// zLo = z of lowest polygon
	double& ht)			// zHi - zLo
// returns true iff success
{	
	elev = 0; ht = 0;
	CPolygon3D* pL = GetLowZ_Ar();
	if (!pL || pL->GetSize() == 0)
		return false;
	CPolygon3D* pH = GetHighZ_Ar();
	if (!pH || pH->GetSize()==0)
		return false;
	elev = pL->p3_Vrt(0).z;
	ht = pH->p3_Vrt(0).z - elev;
	return true;
}
//----------------------------------------------------------------
CPolygon3D* CPolygon3DAr::GetLowOrHighZ_Ar( bool bLow)
{
	CPolygon3D* pA = NULL;
	if (GetSize() > 0)
	{	pA = ap_Plg( 0);
		for (int i=1; i<GetSize(); i++)
		{	CPolygon3D* pAX = ap_Plg( i);
			const CPV3D& p = pA->p3_Vrt( 0);
			const CPV3D& px = pAX->p3_Vrt( 0);
			if ((bLow && px.z < p.z) || (!bLow && px.z > p.z) )
				pA = pAX;
		}
	}
	return pA;
}		// CPolygon3DAr::GetLowOrHighZ_Ar
//----------------------------------------------------------------
int CPolygon3DAr::GetAABB( CPV3D& ptMin, CPV3D& ptMax) const
// get axially aligned bounding box
// returns # of polygons
{
	ptMin.Zero();
	ptMax.Zero();
	int nPlg = GetSize();
	if (nPlg > 0)
	{	ap_Plg( 0)->GetAABB( ptMin, ptMax);
		for (int i=1; i < nPlg; i++)
		{	const CPolygon3D* pA = ap_Plg( i);
			CPV3D p[2];
			pA->GetAABB( p[0], p[1]);
			ptMin.AccumMinBound( p[ 0]);
			ptMax.AccumMaxBound( p[ 1]);
		}
	}
	return nPlg;
}	// CPolygon3DAr::GetAABB
//-----------------------------------------------------------------------------
int CPolygon3DAr::GetAABB( CAABB& box) const
// get axially aligned bounding box
// returns # of polygons
{
	CPV3D ptMin, ptMax;
	int nPlg = GetAABB( ptMin, ptMax);
	box.Set( ptMin, ptMax);
	return nPlg;
}	// CPolygon3DAr::GetAABB
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// class CTri3D - 3D triangle
///////////////////////////////////////////////////////////////////////////////
void CTri3D::Set9( const double* v)		// set from 9 value vector
// source v = x1, y1, z1, x2, y2, z2, x3, y3, z3
{
	for (int iV=0; iV<3; iV++)
		pt[ iV].Set( v + iV*3);
}		// CTri3D::Set9
//-----------------------------------------------------------------------------
double CTri3D::UnitNormal(			// find unit normal (aka direction cosines)
	CPV3D& uNormal) const	// returned: unit normal vector
// returns: unnormalized length = area of triangle
{
	uNormal = CrossProd( pt[ 0]-pt[ 2], pt[ 0]-pt[ 1]);

    double nLen = uNormal.Length();

    // compute the unit normal
	if (nLen > 0.)
		uNormal /= nLen;

    return nLen / 2.;    // area of polygon = length normal
}	// CPolygon3D::UnitNormal
//----------------------------------------------------------------------------
double CTri3D::Area() const
{
	return Length( CrossProd( pt[ 0]-pt[ 2], pt[ 0]-pt[ 1])) / 2.;
}
//-----------------------------------------------------------------------------
int CTri3D::Split(	// subdivide triangle
	CTri3D& tri1,			// returned: subtringle 1
	CTri3D& tri2,			// returned: subtriange 2
	int iS /*=-1*/)	const	// side to divide (-1 = use longest)
// returns idx of side that has been divided
{
	if (iS < 0)
		iS = LongestEdge();

	int iV1 = (iS+1)%3;		// edge 0 is betw pt1 and pt2
	int iV2 = (iS+2)%3;
	CPV3D ptM;
	ptM.Combine( pt[ iV1], pt[ iV2], .5);
	tri1.CopyX( *this, iV1, ptM);
	tri2.CopyX( *this, iV2, ptM);

	return iS;

}	// CTri3D::Split
//-----------------------------------------------------------------------------
int CTri3D::LongestEdge() const
// returns idx of longest edge (0 - 2)
{
	double eL = -1.;
	int iEL = 0;
	for (int iE=0; iE<3; iE++)
	{	// use length^2 (just need max)
		double s = Distance2( pt[ (iE+1)%3], pt[ (iE+2)%3]);
		if (s > eL)
		{	eL = s;
			iEL = iE;
		}
	}
	return iEL;
}		// CTri3D::LongestEdge
//------------------------------------------------------------------------------
/*virtual*/ void CTri3D::CopyX(				// copy with vertex substitution
	const CTri3D& src,		// source triangle
	int iVX,				// exception vertex (0 - 2)
	const CPV3D& ptX)	// exception pt
{
	for (int iV=0; iV<3; iV++)
		pt[ iV] = iV==iVX ? ptX : src.pt[ iV];
}	// CTri3D::CopyX
//-------------------------------------------------------------------------------
void CTri3D::Centroid(
	CPV3D& ptCent) const
{
	for (int iD=0; iD<3; iD++)
		ptCent[ iD] = (pt[ 0][ iD] + pt[ 1][ iD] + pt[ 2][ iD])/3.;

}	// CTri3D::Centroid
//-----------------------------------------------------------------------------
int CTri3D::PointIn(
	const CPV3D& p) const
{
	CPV3D a = pt[ 0] - p;
	CPV3D b = pt[ 1] - p;
	CPV3D c = pt[ 2] - p;

	double ab = a * b;
	double ac = a * c;
	double bc = b * c;
	double cc = c * c;

	if (bc*ac - cc*ab < 0.)
		return 0;
	double bb = b * b;

	if (ab*bc - ac*bb < 0.)
		return 0;
	return 1;
}		// CTri3D::PointIn
//-----------------------------------------------------------------------------
int CTri3D::RayHit(			// determine if ray intersects this triangle
	const CPV3D& p1,		// origin of ray
	const CPV3D& p2,		// another point on ray
	const CPV3D& ab,		// side 2 of trangle
	const CPV3D& ac,		// side 1 of triangle
	const CPV3D& n) const	// normal to triangle

// returns 1 iff ray intersects triangle
{
    CPV3D qp( p1 - p2);

    // Compute denominator d. If d <= 0, segment is parallel to or points
    // away from triangle, so exit early
    double d = qp.DotProd( n);
    if (d <= 0.)
		return 0;

    // Compute intersection t value of pq with plane of triangle. A ray
    // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
    // dividing by d until intersection has been found to pierce triangle
    CPV3D ap( p1 - pt[ 0]);
    double t = ap.DotProd( n);
    if (t < 0.) 
		return 0;
    // if (t > d) // for segment; exclude this code line for a ray test
	//	return 0;

    // Compute barycentric coordinate components and test if within bounds
    CPV3D e( CrossProd( qp, ap));
    double v = ac.DotProd( e);
    if (v < 0. || v > d)
		return 0;
    double w = -ab.DotProd( e);
    if (w < 0. || v + w > d)
		return 0;

    return 1;
}	// CTri3D::RayHit
//-----------------------------------------------------------------------------
int CTri3D::RayHit(			// determine if ray intersects this triangle
	const CPV3D& p1,		// origin of ray
	const CPV3D& p2) const	// another point on ray
// returns 1 if ray intersects triangle
{
    CPV3D ab( pt[ 1] - pt[ 0]);			// side 2 of triangle
    CPV3D ac( pt[ 2] - pt[ 0]);			// side 1 of triangle

    CPV3D n( CrossProd( ab, ac));		// normal to triangle

	return RayHit( p1, p2, ab, ac, n);
}		// CTri3D::RayHit
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CPlane3D: represents a plane in 3D space
///////////////////////////////////////////////////////////////////////////////
// plane equation: Ax + By + Cz + D = 0
// The sign of s = Ax + By + Cz + D determines which side of the 
//   point (x,y,z) lies with respect to the plane. If s>0, then point lies
//   on the same side as the normal (A,B,C), if s<0 - then it lies on the opposite side,
//   if s==0 - point(x,y,z) lies on the plane
//--------------------------------------------------------------------------
CPlane3D::CPlane3D( const CPolygon3D& plg3)
{
	if (plg3.GetSize() < 3)
		Zero();
	else
		// TODO: verify points not co-linear
		Init( plg3[ 0], plg3[ 1], plg3[ 2]);
}		// CPlane3D::CPlane3D
//--------------------------------------------------------------------------
CPlane3D::CPlane3D( const CTri3D& tri)
{
	Init( tri);
}		// CPlane3D::CPlane3D
//--------------------------------------------------------------------------
bool CPlane3D::Init(		// plane from normal and distand to origin
	const CPV3D& uNorm,
	double d)
// returns true on success
//        false if uNorm=0, all 0
{
	pl_n = uNorm;
	pl_d = d;
	return Normalize();		// sets all 0 if degenerate
}		// CPlane3D::Init
//--------------------------------------------------------------------------
bool CPlane3D::Init(		// plane from normal and point on plane
	const CPV3D& uNorm,
	const CPV3D& pt)
// returns true on success
//        false if uNorm=0, all 0
{
	pl_n = uNorm;
	pl_d = uNorm.DotProd( pt);
	return Normalize();		// sets all 0 if degenerate
}		// CPlane3D::Init
//--------------------------------------------------------------------------
bool CPlane3D::Init(const CPV3D& p1, const CPV3D& p2, const CPV3D& p3)
{	pl_n.x = Det33(	1., p1.y, p1.z,	1., p2.y, p2.z, 1., p3.y, p3.z );
	
	pl_n.y = Det33(	p1.x, 1., p1.z,	p2.x, 1., p2.z, p3.x, 1., p3.z );
	
	pl_n.z = Det33(	p1.x, p1.y, 1., p2.x, p2.y, 1., p3.x, p3.y, 1. );
	
	pl_d = - Det33(	p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z );

	return true;		// TODO: detect degenerate cases
}	// CPlane3D::Init
//--------------------------------------------------------------------------
bool CPlane3D::Init( const CTri3D& tri)
{	return Init( tri.Vertex( 0), tri.Vertex( 1), tri.Vertex( 2));
}		// CPlane3D::Init
//--------------------------------------------------------------------------
bool CPlane3D::Normalize()
{
	double div2 = pl_n.Length2();
	if (div2 == 0.)
	{	Zero();
		return false;
	}
	double div = sqrt( div2);
	pl_n /= div;
	pl_d /= div;
	return true;
}		// CPlane3D::Normalize
//--------------------------------------------------------------------------
CPlane3D& CPlane3D::FixSmall()
{
	double div2 = pl_n.Length2();
	if (div2 == 0.)
		Zero();
	else
	{	double tiny = sqrt( div2)*SMALL_NUM;
		pl_n.FixSmall( tiny);
		if (fabs( pl_d) < tiny)
			pl_d = 0.;
	}
	return *this;
}		// CPlane3D::FixSmall		
//--------------------------------------------------------------------------
double CPlane3D::Distance(		// distance: plane to point
	const CPV3D& pt) const
{
	double div = pl_n.Length();
	return (div > SMALL_NUM) ? (pl_n*pt + pl_d) / div : 0.;
}		// CPlane3D::Distance
//--------------------------------------------------------------------------
double CPlane3D::Distance(		// distance: plane to point + closest point
	const CPV3D& pt,		// point
	CPV3D& ptP) const		// returned: closest point
{
	double div = pl_n.Length();
	double dist;
	if (div < SMALL_NUM)
	{	ptP.Zero();
		dist = 0.;
	}
	else
	{	dist = (pl_n * pt + pl_d) / div;
		ptP = pt - pl_n*(dist/div);
	}
	return dist;
}	// CPlane3D::Distance
//--------------------------------------------------------------------------
double CPlane3D::CheckFixPolygon(		// check polygon against plane
	CPolygon3D& p3,
	int options /*=0*/) const	// option bits
								//  1: change polygon vertices to nearest point on plane
// determines how close polygon is to this plane
//   optionally alters each polygon vertex to closest point on plane
// returns largest distance between any polygon vertex and plane
//         -1 if failure (incomplete plane, )
{
	double div = pl_n.Length();
	if (div < SMALL_NUM)
		return -1.;

	bool bFix = options & 1;

	double distMax = 0.;
	int nV = p3.GetSize();
	for (int iV=0; iV<nV; iV++)
	{	CPV3D& pt = p3[ iV];
		double dist = (pl_n * pt + pl_d) / div;
		if (dist > distMax)
			distMax = dist;
		if (bFix)
			p3[ iV] = pt - pl_n*(dist/div);
	}
	return distMax;
}	// CPlane3D::CheckFixPolygon
//-----------------------------------------------------------------------------
double CPlane3D::AngleCos(		// cos of angle between two planes
	const CPlane3D& pl) const
// returns -2 if planes do not intersect
{	
	double top = pl_n.DotProd( pl.pl_n);
	double bot = sqrt( pl_n.Length2() * pl.pl_n.Length2());
	return ( fabs(bot) > SMALL_NUM) ? top / bot : -2.0;
}		// CPlane3D::AngleCos
//-----------------------------------------------------------------------------
int CPlane3D::IntersectSegment(		// intersect line segment
	const CPV3D& a,		// seg beg
	const CPV3D& b,		// seg end
	CPV3D& pt,			// returned: intersection point
	double& t) const	// returned: dimless position on seg
						//   of intersection point (0 at a, 1 at b)
// returns 1 iff intersection (t and pt set)
//    else 0 (t < 0 || t > 1), pt not set
{
	CPV3D ab( b - a);
	
	int ret = 0;
	double nDotSeg = pl_n * ab;
	if (nDotSeg != 0.)
	{	t = (-pl_d - pl_n * a) / nDotSeg;
		ret = t >= 0. && t <= 1.;
		if (ret)
			pt = a + t * ab;
	}
	return ret;
}		// CPlane3D::IntersectSegment
//-----------------------------------------------------------------------------
int CPlane3D::IntersectRay(
	const CPV3D& a,		// origin of ray
	const CPV3D& dc,	// direction cosines of ray
	CPV3D& pt) const	// returned: intersection point
// returns 1 iff intersection (pt set)
//    else 0 (pt not set)
{
	int ret = 0;
	double nDotRay = pl_n * dc;
	if (nDotRay != 0.)
	{	double t = (-pl_d - pl_n * a) / nDotRay;
		ret = t >= 0.;
		if (ret)
			pt = a + t * dc;
	}
	return ret;
}		// CPlane3D::IntersectRay
//-----------------------------------------------------------------------------
int CPlane3D::IntersectPlane(	// intersect planes
	const CPlane3D& pl,		// other plane
	CPV3D& pt,				// returned: a point on intersection line
	CPV3D& dir) const		// returned: direction of intersection line
// method source = Real Time Collision Detection, 2005, p. 210
//   
// returns 0 if planes are parallel (do not intersect)
//         1 otherwise
{
	dir = CrossProd( pl_n, pl.pl_n);

	double dNorm2 = dir.Length2();

	if (dNorm2 < .0001)
	{	pt.Zero();
		dir.Zero();
		return 0;
	}

	pt = CrossProd( pl.pl_d*pl_n - pl_d*pl.pl_n, dir) / dNorm2;

	dir /= sqrt( dNorm2);

	return 1;

}		// CPlane3D::IntersectPlane
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CAABB -- axis aligned bounding box
///////////////////////////////////////////////////////////////////////////////
void CAABB::Accum( const CPV3D& px)
{
	for (int iD=0; iD<3; iD++)
	{	if (px[ iD] < pt[ 0][ iD])
			pt[ 0][ iD] = px[ iD];
		else if (px[ iD] > pt[ 1][ iD])
			pt[ 1][ iD] = px[ iD];
	}
}		// CAABB::Accum
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class CArrow3D: represents 3D wireframe arrow
//                 re display of e.g. surface normals 
///////////////////////////////////////////////////////////////////////////////
void CArrow3D::a3_Zero()
{	a3_head.Zero();
	a3_tail.Zero();
	a3_headEnd[ 0].Zero();
	a3_headEnd[ 1].Zero();
}		// CArrow3D::a3_Zero
//-----------------------------------------------------------------------------
int CArrow3D::a3_Init(			// generate an arrow
	CPV3D dir,							// direction
	double length /*=1.*/,				// length
	const CPV3D& tail /*=kCPV3D0*/)		// position of non-head end
										//  default = origin
{
	double lenDir = dir.Normalize();
	if (lenDir == 0.)
	{	a3_Zero();
		return 0;
	}

	// beg and end points
	length *= lenDir;
	a3_tail = tail;
	a3_head = tail + dir*length;

	const float headSz = 0.9f;

	// cross point
	// = where line connecting arrow head ends intersects main line
	CPV3D ptCross( tail + headSz*length*dir);

	// plane normal to arrow through cross point
	CPlane3D plnNorm( dir, ptCross);

	// axis-aligned plane through cross point
	//   arrow near vertical: use XZ plane
	//   else use XY plane (horizontal)
	const CPV3D& normAxis = fabs( dir.z) > .9 ? kCPV3Dy1 : kCPV3Dz1;
	CPlane3D plnAxis( normAxis, ptCross);

	// intersect planes to find arrow head base line
	CPV3D lnPt;
	CPV3D lnDir;
	plnNorm.IntersectPlane( plnAxis, lnPt, lnDir);

	// arrowhead end points
	a3_headEnd[ 0] = ptCross + (1. - headSz)*length*lnDir;
	a3_headEnd[ 1] = ptCross - (1. - headSz)*length*lnDir;

	return 1;

}		// CArrow3D::a3_Init

// geometry.cpp end

