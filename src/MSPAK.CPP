// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// mspak.cpp -- Mass specific routines for CSE
//  used from simulator and setup for it


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// MASSTYPE MSRAT+
#include <IRATS.H>	// SfiB

#include <MSGHANS.H>	// MH_xxxx defns
#include <NUMMETH.H> 	// gaussjb

#include "CNGUTS.H"
#include "mspak.h"

#if defined( _DEBUG)
static const char* trapName = "WallS";		// debug aid -- do DbPrint etc for this surface
#endif

/*-------------------------------- DEFINES --------------------------------*/
/* Define CHECKPRINT to include code which prints various informative
   matrices and other values as we go along ... */
/* #define CHECKPRINT */
#ifdef CHECKPRINT
#include <mxpak.h>	// mxdmul
#include <debugpr.h>
#endif

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/

/*------------------------------- old 1-92 COMMENTS --------------------------------*/

/*

r and h and -ivity and -ance
    r:  resistivity (F-ft2/Btuh): resistance per unit area (of matl etc)
        resistance (F/Btuh): r/area, also 1/ha, for entire component (wall etc)
    h:  conductivity (Btuh/F-ft2): conductance per unit area
    ha: conductance (Btuh/F): h * area: coupling coefficient (aka heat
        transfer coefficient) for entire thing
    ---> many "ance" comments actually meant "ivity", 12-89.

surface h/r
    surface film conductivity
	sfpak:rupSf puts in CN.rinsfilm, .routfilm.
	These get to MASSTYPE.hi, .ho via crsimsrf:siMsty and mspak:
	   msMsty (former discards surface film layers: would duplicate).
	crsimsrf:siaddmass DISCARDS masstype.hi/ho, using standard t24pak
	   values to compute MASSBC.h (also includes rsurf).
    "additional surface resistance"
	Mainly these are values "digested" out of layers, to simplify model:
	  msMsty represents light surface layers in MASSTYPE .rsurfi,
	  .rsurfo and omits them from its output layers;
	  also more may be added via siaddmass rxtra arg.
	MASSTYPE.rsurfi/o ---> MASSBC[].rsurf for MASS in siaddmass.
    MASSBC[].h is COMBINED surface film and rsurf, as a conductivity.

layers and nodes

    layers
	Layers are initially the physicial construction layers and are
	  reworked to form the mass model.
	Physical layers input by user on construction worksheet are in
	  RTFORM3LAYERs indexed under the CN, managed in sch\sfpak.cpp.
	crsimsrf.cpp:siMsty fetches RTFORM3LAYER info with sfpak:sfLayerInfo
	  (that fcn combines framing/cavity pairs), forms array of MASSLAYER,
	  and calls:
	mspak:msMsty which "digests" the layers into the model (deleting
	  some, splitting up others) and outputs another MASSLAYER[] as part
	  of the MASSTYPE entry.

    nodes
	Nodes are where layers meet (each other or the surface), in the
	  r-c mass simulation model.
	For each specific mass crsimsrf.cpp:siaddmass calls mspak.cpp:mssetup
	  to make up the MASSMATRIX for modelling this mass, using the
	  (digested) MASSLAYER array in the MASSTYPE (and also hi, ho
	  from MASSBC, which are based in part on specific mass's tilt).
	MASS.temp[] and MASSMATRIX.val are dimensioned for nodes.
	Relation of # nodes to # layers in MASSTYPE entry? read mssetup() here.

write a fcn summary and call tree?

oldish: ?? complete/integrate/update mspak.h version:
   Construction to Layers to Nodes to Matrix ...
	sfpak.cpp:sfLayerInfo retreives info for one layer from RTFORM3LAYER
	   indexed under CN, merges frame & cavity.
	crsimsrf.cpp:siMsty: uses that to makes array of MASSLAYERS and call
	crsimsrf.cpp:siAddMassType(): allocs MASSTYPE entry and calls
	mspak.cpp:msMsty: edits layers
	   into ANOTHER array of MASSLAYERS in given MASSTYPE entry
	   (uses msTyAddLay)
	 then (siAddMassType) sets up matrix in masstype by calling:
        mspak.cpp:mssetup: sets up the MASSMATRIX for given array of MASSLAYERS
*/

/*===========================================================================================================================

                       			     The Mass Matrix Story
														Rob 7-92.

This is intended to explain the mass matrix setup function, mssetup, and the hourly mass simulate routine, ms_Step.
Not yet documented here are the quantitative rationales for splitting and merging layers (in msMsty).


THE PROBLEM

Simulate the temperature of the surfaces of a "mass" over time.  Account for heat storage and delayed transmission or
return of heat.  The latter is accomplished by simulating, within the algorithm, internal as well as surface temperatures.

NOTATION

[i]       will denote subscription; the []'s will sometimes be omitted when the subscript is a single letter.
T[]       with empty []'s will denote a vector -- all of the T[i] values.

MASS DEFINITION

The "mass" is a 3-dimensional solid assumed to be exposed to given temperatures and possibly heat flow on two opposite
parallel sides and have adiabatic edges.  Its thermal mass and thickness are too large to treat as a single object
at constant temperature or even at constant temperature gradient between the two surface temperatures; the delay in heat
conduction from each surface to the interior and the other surface must be modelled.  The mass is assumed to be of uniform
composition in any plane parallel to the exposed surfaces, but its thickness may be made of multiple layers (each uniform)
of materials with different characteristics.

INPUT DEFINITION

At the point where our interest begins, the mass is described as a set of layers numbered 0 to n-1.  These layers are
based on the physical layers but have been modified for modelling purposes:  Adjacent thin, light layers may have been
combined; thicker, more massive layers may have been devivided into several equal thinner layers of the same material so
that different temperatures at different points through the thickness of the layer can be modelled with this algoritm.

Each layer i has the characteristics (constant input):
   thk[i]         thickness
   vhc[i]         volumetric heat capacity
   cond[i]        conductivity

The boundary conditions at the surfaces of the mass are, per unit area (hourly input):
   Qin, Qout         incident heat (e.g. solar) on inside and outside surfaces.
   Tin, Tout         adjacent temperature to inside (layer 0) and outside (layer n-1)
   hin, hout         (surface film) conductivity between adjacent temp and layer exposed surface (constant)

DESIRED OUTPUT

Temperature at surfaces of mass; also, enthaply (heat stored in mass) is used for program consistency checks.


NODES

The simulation is done using "nodes" at the layer surfaces.  There is one more node than layer.  Each node represents
the material in half of each adjacent layer, and will be numbered here with the lower of the two layer numbers.  Thus
node 3 represents half of layer 3 and half of layer 4.  The surface nodes, 0 and n, each represent half a layer only.
Each node has a temperature, T[i], which is computed hourly on the basis of the prior hour temperatures and the hourly
inputs.

The characteristics of the nodes can be derived as follows (per unit area):

   Thickness: inside node:   node 0:  thk[0]/2
              interior node: node i:  thk[i-1]/2 + thk[i]/2     for 0 < i < n-1
              outside node:  node n:  thk[n-1]/2

   Node heat capacity (per area):                                Note: in msseup code, this 'hc[]' is 'mass', then 'nodehcp[]'.
       hc[i] =                                                   CAUTION: hc is per node but vhc is per layer.
           sum of (thickness times volumetric heat capacity) =
                             hc[0] =        thk[0]/2*vhc[0]
                             hc[i=1..n-1] = thk[i-1]/2*vhc[i-1] + thk[i]/2*vhc[i]
                             hc[n] =        thk[n-1]/2*vhc[n-1]

   Conductance between nodes i and i+1 (per area)                                      CAUTION: u is per node, cond is per layer
       u[i] = sum of (conductivity / thickness)			          (because half-layers are in series)
                u[i=0..n-1] = cond[i]*2/thk[i] + cond[i+1]*2/thk[i+1]


STATE OF SYSTEM

The state of the mass is represented by the node temperature vector:

   T[0..n]     current temperatures of nodes 0 through n.  User is interested in T[0] and T[n], the temperatures at
               the interfaces between the mass and the external world.  T[1..n-1] must be maintained internally for
               computing later surface temperatures, and for computing mass internal energy, used as a check.

The inputs used at each iteration include quantities derived from the layer characteristics, and the
"extended old temperature vector" with two added members representing the current boundary condition heat flows,
initialized as follows:

   Told[0..n] = T[0..n]           prior hour temperatures for nodes 0..n
   Told[n+1] = Tin*hin + Qin      current hour inside boundary condition (well, not quite a heat flow)
   Told[n+2] = Tout*hout + Qout   current hour outside boundary condition

We will show how to derive a constant n+1 by n+3 matrix by which Told can be multiplied each hour to yield the new T.
The matrix derivation is based on the simultaneous solution of heat balance equations for each node, using a "central
difference" representation of the differential equations.


INTERNAL STORAGE

The internal storage saved from hour to hour for a mass consists of the following items computed once before the simulation
begins: hc[], the node heat capacity vector, and M, the mass matrix (described below).  In addition, the temperature
vector T[] is computed each hour, and saved for initializing the next hour's Told[].  Many additional quantities, such
as u's, are computed and used during the initialization of the mass matrix, but are not retained.



HEAT BALANCE EQUATIONS

The basic heat balance differential equation is:        dT/dt = sigma(q)/hc
That is, the temperature change per unit time of
an object is the sum of the heat flows into the object divided by the object's heat capacity.

We can elaborate this by showing the
computation of conductive heat flows, thus:             dT/dt = 1/hc * (sigma((Tj-T)*uj) + sigma(q))
where the Tj's and uj's are temperatures of
and conductances to other objects, and sigma(q) represents any additional heat flows that cannot be
represented in this form.

If we substitute
    t for dt             length of time interval
    T - Told             for dT: temperature change
    C === (T + Told)/2   for each T: central difference; notation C is just for conciseness

the differntial equation becomes the difference equation:

    T = Told + t/hc * (sigma(Cj - C)*uj + sigma(q))

Further, substituting

    s === t/hc,

yielding

    T = Told + s * (sigma(Cj - C)*uj + sigma(q)),							    (1)

makes our expressions more concise later even though little difference shows here.


INTERIOR NODE

Now, let us consider such an equation for an interior node, i, 0 < i < n.
The heat flows are from the two adjacent layers only:

  T[i] =  Told[i]  +  s[i] * ( (C[i-1] - C[i])*u[i-1] + (C[i+1] - C[i])*u[i] )

Multiply through by 2 and substitute the approriate averages for the central differences (C's):

  2*T[i] =  2*Told[i]  +  s[i] * ( (T[i-1]+Told[i-1] - T[i]-Told[i])*u[i-1] + (T[i+i]+Told[i+1] - T[i]-Told[i])*u[i] )

Gather terms, moving the unknowns to the left and the knowns to the right (and shortening s[i] to just s for now):

    -s*u[i-1]*T[i-1] + (2 + s*(u[i-1]+u[1]))*T[i] - s*u[i]*T[i+1]

                                            =  s*u[i-1]*Told[i-1] + (2 - s*(u[i-1]+u[i]))*Told[i] + s*u[i]*Told[i+1]    (2)

The right hand side is constant if Told[] is known, thus, this gives equations 1 thru n-1 of the n+1 simultaneous equations
needed to computed T[] from Told[].


BOUNDARY NODES

Let us consider node 0, the "inside" node.  Its heat flows are from node 1, and the inside adjacent area:

   T[0] = Told[0] + s[0]*( (C[1] - C[0])*u[0] + (Tin - C[0])*hin + Qin)

			(Tin is not shown here as a C because the mass code does not store the prior Tin and compute an average
			as it does with the node temperatures; instead, the caller supplies an average or estimated average Tin.)

Times 2, substitute for C's:

   2*T[0] = 2*Told[0] + s[0]( (T[1]+Told[1] - T[0]-Told[0])*u[0] + (2*Tin - T[0]-Told[0])*hin + 2*Qin_

Move unknowns to left, writing s[0] as simply s:

   (2 + s*(u[0]+hin))*T[0] - s*u[0]*T[1] = (2 - s*(u[0]+hin))*Told[0] + s*u[0]*Told[1] + 2*s*Tin*hin + 2*s*Qin

Recall that we said above that Told[] had two extra members and that  Told[n+1]  was set to  Tin*hin + Qin.
Making this substitution yields:

   (2 + s*(u[0]+hin))*T[0] - s*u[0]*T[1] = (2 - s*(u[0]+hin))*Told[0] + s*u[0]*Told[1] + 2*s*Told[n+1].                 (3)

Similarly, the heat balance equation for the outside node (node n) is (s represents s[n]):

   (substitutions, in order:   u[0]->u[n-1]   [1]->[n-1]   [0]->[n]   hin->hout   n+1->n+2)

   (2 + s*(u[n-1]+hout))*T[n] - s*u[n-1]*T[n-1] = (2 - s*(u[n-1]+hout))*Told[n] + s*u[n-1]*Told[n-1] + 2*s*Told[n+2].   (4)


PRE-COMPUTATION and MATRIX NOTATION

The system of n+1 equations consisting of (3), n-1 (2)'s, and (4) can be solved for each T[] given Told[], where Told
consists of the prior hour's T[] plus the boundary condition information as specified above.  This computation can be
expedited by computing a matrix M such that the multiplying the Told[] vector by B yields the new T[] vector:

    T = M * Told

Since M does not change during the simulation, it can be computed once in advance.

Let us write the equations for a 4-node (3-layers, n=3) mass in matrix notation, leaving the Told's unknown for now.
I'm dropping the []'s around single-character subscripts for brevity, and using []'s to enclose vectors and matrices.

(Reader: skip this: Intermediate steps saved to permit checking: 1: equations copied, subscript values substituted.
(
( (2 + s0*(u0+hin))*T0 - s0*u0*T1                                        = (2 - s0*(u0+hin))*Told0 + s0*u0*Told1 + 2*s0*Told4
(  -s1*u0*T0  +  (2 + s1*(u0+u1))*T1  -  s1*u1*T2                        =    s1*u0*Told0 +  (2 - s1*(u0+u1))*Told1 + s1*u1*Told2
(                   -s2*u1*T1  +  (3 + s2*(u1+u2))*T2  -  s2*u2*T3       =    s2*u1*Told1 +  (3 - s2*(u1+u2))*Told2 + s2*u2*Told3
(                                     - s3*u2*T2 + (2 + s3*(u2+hout))*T3 = s3*u2*Told2 + (2 - s3*(u2+hout))*Told3 + 2*s0*Told5
(
( 2: seperate T's from coefficients, align to form columns fitting page width:
(        T0          T1           T2           T3              Told0        Told1       Told2         Told3    Told4  Told5
(   2+s0*(u0+hin)  -s0*u0          0            0        = 2-s0*(u0+hin)    s0*u0         0             0       2*s0   0
(     -s1*u0     2+s1*(u0+u1)   -s1*u1          0        =  s1*u0*Told0  2-s1*(u0+u1)   s1*u1           0        0     0
(        0         -s2*u1*T1  2+s2*(u1+u2)   -s2*u2      =       0          s2*u1    2-s2*(u1+u2)     s2*u2      0     0
(        0            0         -s3*u2    2+s3*(u2+hout) =       0            0         s3*u2     2-s3*(u2+hout) 0   2*s3
(
( 3: won't fit accross page. )

    [  2+s0*(u0+hin)  -s0*u0          0            0        ]   [T0]
    [    -s1*u0     2+s1*(u0+u1)   -s1*u1          0        ]   [T1]
    [       0         -s2*u1*T1  2+s2*(u1+u2)   -s2*u2      ] * [T2]  =
    [       0            0         -s3*u2    2+s3*(u2+hout) ]   [T3]

                            [ 2-s0*(u0+hin)    s0*u0         0             0       2*s0  0  ]   [Told0]
                            [  s1*u0*Told0  2-s1*(u0+u1)   s1*u1           0         0   0  ]   [Told1]
                            [       0          s2*u1    2-s2*(u1+u2)     s2*u2       0   0  ] * [Told2]
                            [       0            0         s3*u2     2-s3*(u2+hout)  0 2*s3 ]   [Told3]
                                                                                                [Told4]
                                                                                                [Told5]

    where:  u[i=0..2] === cond[i]*2/thk[i] + cond[i+1]*2/thk[i+1]
            s[i] ===      t/hc[i]
              s[0] ===      2*t/(thk[0]*vhc[0])
              s[i=1..2] === 2*t/(thk[i-1]*vhc[i-1] + thk[i]*vhc[i])
              s[3] ===      2*t/(thk[2]*vhc[2])
            Told4 ===     Tin*hin + Qin                         (old temp vector extended to include boundary conditions)
            Told5 ===     Tout*hout + Qout


Let A be the 4 x 4 matrix containing coeffiecients of T's (the unknowns) and B be the 4 x 6 matrix of coefficients of
Told's (to compute constant part of equations each hour).  Then the above can be represented as:

     A * T = B * Told
or
     T = Ainv * B * Told

Where Ainv is the inverse of A.  "Ainv * B" is our 4 x 6 "mass matrix" M, which can be computed once before the hourly
simulation begins.

In the program, the mass matrix M is computed by passing A and B to the Gauss-Jordan subroutine, which solves A = V for
V equal to each column of B, and replaces that column of B with the solution.  Stated another way, the Gauss-Jordan
algorithm performs row operations on A and B until A is reduced to an identity matrix; B has then been been multiplied
by the inverse of A.  Right?  The resulting contents of the B matrix storage is the mass matrix M.

Here is an alternate, informal explanation of that step, in terms of solution of simultaneous equations rather than
matrix inversion:  Consider  A * T = V, where vector V is the ith column of B.  Solve for vector T.  The result is
the T value if Told[i] is 1 and the rest of Told is 0; it is thus a vector representing the Told[i] contribution to T,
or the vector of coefficients by which to multiply Told[i] before adding to T; or the appropriate contents of column i
of the mass matrix (because of the definition of matrix multiplication).


RELATION TO OTHER DOCUMENTATION

In the undated documentation in Phil Niles handwriting that Chip gave me, an additional vector "C" is used in place
of the two extra elements of Told and the two extra columns in B.

In the program, B does have the two extra columns, and is referred to as BC in some comments.  The boundary condition
hourly inputs are stored in two elements appended to the old temperature vector.

CAUTION: n in the program is the number of nodes; here it is the number of layers, one less.

Mssetup uses  ut = u * t  and  1/hc  instead of  u  and  t/hc.


RECONCILIATION TO NILES' DERIVATION

Nile's derivation was for uniform layers (representing a uniform mass, divided into layers to model the delayed heat flow).
Thus we can drop most subscripts and the computations involving two half-layers, yielding

    u =           cond/thk
    s[0] = s[n] = 2*t/(vhc*thk)                      surface nodes have 1/2 heat capacity of interior nodes
    s[1..n-1] =   t/(thk*vhc)

Niles uses rho (density) times c (specific heat) where we use vhc.

1. r: Niles' r corresponds to our s*u for interior layers:

   r = s * u
     = t/(thk * vhc) * cond/thk
     = t * cond / (thk*thk * vhc)
     = t * cond / (thk*thk * rho * c)                   Checks.  Verifies all interior coefficents in A.

2. Beta: for A[0,0] we have  2 + s0*(u0 + hin)  and Niles has  2 + 2*r*Beta1,
where
       Beta1 ===  1 + hin*thk/cond.

Substituting for r and Beta1 in Niles expression:

      2 + 2*r*Beta1 = 2 + 2 * (t * cond / (thk*thk * vhc) ) * (1 + hin*thk/cond)
                    = 2 + 2 * t / (thk * vhc) * (cond/thk + hin)

Substituting for s0 and u0 in our expression:

      2 + s0*(u0 + hin) = 2 + 2*t/(vhc*thk)*(cont/thk + hin)            Checks.  Verifies A[0,0] and presumably [n,n].

3. A[0,1]:  Niles has -2*r; we have -s0*u0.

     -2*r = -2 * t * cond / (thk*thk *vhc)

     -s0*u0 = 2*t/(vhc*thk) * cond/thk                         Checks.  Verifies A[0,1] and A[n,n-1].

4. B[0..n,0..n]: Ok if A[] ok, by inspection (only differences are in sign).

5. B[0,n+1] and B[n,n+2]: Niles has 4*r*thk/cond; we have 2*s0.

      4*r*thk/cond = 4 * (t * cond / (thk*thk)*vhc) ) * thk/cond
                   = 4 * t / (thk * vhc)

      2*s0 = 2 * 2*t/(vhc*thk)                                 Checks.
*/

//========================================================================
static inline double seriesU( double u1, double u2)
{	double u = (u1 <= 0. || u2 <= 0.)
			? 0.
			: 1./(1/u1 + 1./u2);
	return u;
}		// ::seriesU
//========================================================================
float FC msrsurf( float h, float rsurf)		// return series conductivity of a conductivity h and resistivity rsurf

/* Used to compute mass surface conductivity by adding surface rval
   (derived from mass construction by msMsty) rsurf (F-ft2/Btuh)
   to standard value film h (Btuh/F-ft2) ret'd eg by t24massh(). */
{
	if (h > 0.F)			// don't div by 0
		h = 1.F/(rsurf + 1.F/h);		// add resistivities and invert
	return (float)h;
}
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class MASSLAYER: represents single, homogeneous material layer
//                  most members inline
////////////////////////////////////////////////////////////////////////////
MASSLAYER& MASSLAYER::ml_Set(			// extract layer from input LR object
	int iLSrc,				// idx of source layer
	const LR* pLR,			// layer info as input (from e.g. CON)
	int options /*=0*/)		// options
							// 1: extract info for framing (else cavity)
							//    (no effect if LR not framed)
{
	options;	// TODO: implement framed scheme, 8-10
	const MAT* pMat = MatiB.GetAt( pLR->lr_mati);
	return ml_Set( iLSrc, pLR->lr_thk, pLR->lr_vhc, pLR->lr_uvy,
		pMat->mt_condTRat, pMat->mt_condCT);

}		// MASSLAYER::ml_Set
//-----------------------------------------------------------------------------
MASSLAYER& MASSLAYER::ml_Set(		// initialize mass layer
	int iLSrc,		// idx of source layer
	double thick,	// layer thickness, ft
	double vhc,		// VHC, Btu/F-ft3
	double cond,	// nominal conductivity, Btuh-ft/ft2-F (at condTRat)
	float condTRat/*=70.f*/,	// rating temp for cond, F
	double condCT/*=0.*/)		// conductivity temp coefficient, 1/F
{	ml_iLSrc = iLSrc;
	ml_thick = thick;
	ml_vhc = vhc;
	ml_condRtd = cond;
	ml_condTRat = condTRat;
	ml_condCT = condCT;
	ml_SetNom();	// sets ml_cond, ml_condNom, and ml_RNom to 70 F values
	return* this;
}		// MASSLAYER::ml_Set
//---------------------------------------------------------------------------
void MASSLAYER::ml_SetNom()		// set nominal values
// sets ml_cond, ml_condNom and ml_RNom to 70 F values
{	ml_condNom = float( ml_SetCond( 70.f));
	ml_RNom = ml_R();
}		// MASSLAYER::ml_SetNom
//---------------------------------------------------------------------------
void MASSLAYER::ml_Slice(
	int nSL)		// thickness divisor (>0)
{	ml_thick /= nSL;
	ml_SetNom();
}		// MASSLAYER::ml_Slice
//---------------------------------------------------------------------------
void MASSLAYER::ml_ChangeThickness(
	double thkF)	// thickness multiplier (> 0)
{	ml_thick *= thkF;
	ml_SetNom();
}		// MASSLAYER::ml_ChangeThickness
//---------------------------------------------------------------------------
int MASSLAYER::ml_IsMergeable(			// test if layers can be merged
	const MASSLAYER& ml) const		// candidate for merging
// returns 1 iff ml can be combined with *this
{
	// NOTE: ml_iLSrc is not compared
	int ret =
		  ( (ml_condCT == 0. && ml.ml_condCT == 0.)				// no temp dependence
		    || (    fabs( ml_condCT - ml.ml_condCT) < .00001	// or same temp dependence
		         && fabs( ml_condTRat - ml.ml_condTRat) < .00001))
	   && ( (ml_vhc < MINMASSVHC && ml.ml_vhc < MINMASSVHC)		// both massless
	        || (   fabs( ml_condRtd - ml.ml_condRtd) < .00001	// or same properties
		        && fabs( ml_vhc - ml.ml_vhc) < .00001));
	return ret;
}		// MASSLAYER::ml_IsMergeable
//---------------------------------------------------------------------------
MASSLAYER& MASSLAYER::ml_Merge(		// merge layers
	const MASSLAYER& ml)		// layer to be merged into *this
// this->ml_iLSrc is retained (
// returns *this (merged layer)
{
	ASSERT( ml_IsMergeable( ml));
	double mrgThick = ml_thick + ml.ml_thick;
	if (mrgThick > 0.)
	{
		double mrgCondRtd = mrgThick / (ml_RRtd() + ml.ml_RRtd());
		double mrgVhc = (ml_HC() + ml.ml_HC()) / mrgThick;

		ml_Set( ml_iLSrc, mrgThick, mrgVhc, mrgCondRtd, ml_condTRat, ml_condCT);
		// ml_Set() calls ml_SetNom()
	}
	else
		err( PERR, "MASSLAYER::ml_Merge: ml_thick = 0");
	return *this;
}		// MASSLAYER::Merge
//----------------------------------------------------------------------------
int MASSLAYER::ml_SubLayerCount(		// determine reqd # of sublayers
	float lThkF) const		// thickness factor (default = 0.5)
							//   min sublayer thickess = 8 hr penetration depth * thk
							//   lThkF == 0 -> always returns 1 (suppress layer split)
// returns # of sublayer required
{
	int nSL = 1;
	if (lThkF > .001f)
	{	double dp = ml_PenetrationDepth( 8.);	// penetration depth for 8 hour period, ft
		double lMax = dp * lThkF;				// max layer thickness
		nSL = max( 1, int( ceil( ml_thick / lMax)));
	}
	return nSL;
}		// ml_SubLayerCount
//---------------------------------------------------------------------------
double MASSLAYER::ml_PenetrationDepth(
	float period) const	// hours
{
	return ml_IsMassless()
		 ? 9999.		// return huge
		 : sqrt( period*ml_condNom/(kPi*ml_vhc));

}	// MASSLAYER::ml_PenetrationDepth
//---------------------------------------------------------------------------
double AR_MASSLAYER::ml_GetProperties(		// sum properties (check function)
	double& cfactor,	// returned: cfactor at 70 F, Btuh/ft2-F
	double& hc) const	// returned: heat capacity, Btu/ft2-F
// returns surface-to-surface resistance at 70 F, ft2-F/Btuh
{
	double RNom = 0.;
	hc = 0.;
	const_iterator itL, endL;
	for (itL=begin(),endL=end(); itL != endL; itL++)
	{	const MASSLAYER& ml = *itL;
		RNom += ml.ml_RNom;
		hc += ml.ml_HC();
	}
	cfactor = RNom > 0. ? 1./RNom : 0.;

	return RNom;
}	// AR_MASSLAYER::ml_GetProperties
//-----------------------------------------------------------------------------
float AR_MASSLAYER::ml_GetPropertiesBG(		// determine characteristics for BG model
	int& interiorInsul)	const	// returned: 1 iff construction has interior insulation
// returns RNom = total surface-to-surface resistance of all layers
{
	double RNom = 0.;
	interiorInsul = -1;
	const_iterator itL, endL;
	for (itL=begin(),endL=end(); itL != endL; itL++)
	{	const MASSLAYER& ml = *itL;
		if (interiorInsul < 0)
		{	int isStructural = ml.ml_condNom >= 0.8;
			interiorInsul = !isStructural;
		}
		RNom += ml.ml_RNom;
	}
	return RNom;
}	// AR_MASSLAYER::ml_GetPropertiesBG
//---------------------------------------------------------------------------
double AR_MASSLAYER::ml_AdjustResistance(		// alter insulation layers
	double RMax)	// maximum allowed surface-to-surface resistance
					//  ft2-F/Btuh
// reduces or eliminates insulation layer(s) (outermost first)
//   to bring R down to RMax; does nothing if R <= RMax;
//   re matching overall effective below grade R

// returns surface-to-surface resistance after
//   adjustment (if any), ft2-F/Btuh
{
	double cfactor;
	double hc;
	double RNom = ml_GetProperties( cfactor, hc);
	if (RNom <= RMax)
		return RNom;	// resistance sufficient as-is
						//   no adjustment required

	[[maybe_unused]] double RReduce = 0.;
	double RAdj = RNom;
	iterator itL;
	int nL = size();
	for (int iL=nL-1; iL >= 0 && RAdj > RMax; iL--)
	{	itL = begin() + iL;
		MASSLAYER& ml = *itL;
		if (ml.ml_IsInsulation())
		{	double Rml = ml.ml_RNom;
			// worry about RmL == 0.?
			double thkF = (Rml - RAdj + RMax) / Rml;
			if (thkF < .1 || ml.ml_thick < 0.02)
			{	// all (or almost all) of layer needs to be eliminated
				//  or layer already very thin (< .25 in approx)
				//  --> delete entire layer
				erase( itL);
				thkF = 0.;
			}
			else
				// a fraction of layer will suffice
				ml.ml_ChangeThickness( thkF);
			RAdj -= Rml * (1. - thkF);		// update overall resistance
		}
	}
#if defined( _DEBUG)
	double RAdjX = ml_GetProperties( cfactor, hc);
	if (fabs( RAdjX - RAdj) > .0001)
		printf( "\nml_AdjustResistance inconsistency");
#endif
	return RAdj;
}		// AR_MASSLAYER::ml_AdjustResistance
//---------------------------------------------------------------------------
void AR_MASSLAYER::ml_AddSoilLayer(		// add soil layer to outside of construction
	int iLSrc,			// idx of source layer, -1 if not known
	float RSoil)		// resistance of layer
// adds "typical" soil layer as outermost layer
// used in below grade surfaces to capture hour-to-hour behavior
{
	push_back(
		MASSLAYER(
			iLSrc,
			BGSoilCond*RSoil,		// required thickness, ft
			BGSoilDens*BGSoilSpHt,	// vhc, Btu/ft3-F
			BGSoilCond));
}		// AR_MASSLAYER::ml_AddSoilLayer
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// struct HCI: helper re variable inside convective coefficients
/////////////////////////////////////////////////////////////////////////////
#if defined( CONV_ASHRAECOMPARE)
void HCI::hc_InitAsh(
	float tiltD,		// surface tilt, deg (0=ceiling, 90=wall, 180=floor)
	int si)				// side: 0=inside, 1=outside
{
// ASHRAE still air values used by e.g. 1199-RP
static float hStill[ ][ 2] =
// tAir<=tSrf  tAir>tSrf
{    .162f,     .712f,		// ceiling (tilt < 45)
     .542f,     .542f,		// wall (45 <= tilt < 135)
     .712f,     .162f		// floor (tilt >= 135)
};
	// TODO: generalize for arbitrary tilt?
	// TODO: handle non-canonical tilts (e.g. -5, 190, ...)?
	int iT =   tiltD < 45.f  ? 0
		     : tiltD < 135.f ? 1
			 :                 2;

	hc_t[ si]   = hStill[ iT][ 0];
	hc_t[ 1-si] = hStill[ iT][ 1];
	hc_t[ 2] = .88f;		// elevated value when system on (from 1199-RP)

	VMul1( hc_t, 3, Top.tp_hConvF);		// adjust re elevation

	hc_iXHist = 0;
	VSet( hc_history, hcHISTL, hc_t[ 0]);

}		// HCI::hc_Init
//---------------------------------------------------------------------------
double HCI::hc_HCAsh(
	int mode,		// mode: 0=still air, 1=system on
	float tAir,		// air temp adjacent to surface, F
	float tSrf)		// surface temp, F
{
	int iHC = mode == 1 ? 2 : tAir > tSrf;
	return hc_HC( hc_t[ iHC]);
}		// HCI::hc_HCAsh
#endif	// CONV_ASHRAECOMPARE
//--------------------------------------------------------------------------
void HCI::hc_Init( float hc)
{
	hc_iXHist = 0;
	VSet( hc_history, hcHISTL, hc);
}	// HCI::hc_Init
//---------------------------------------------------------------------------
double HCI::hc_HC(		// maintain history, return multi-step average
	double hc)	// current step HC value
{
	hc_iXHist = (++hc_iXHist)%hcHISTL;		// 0 .. hcHISTL-1 with wrapping
	hc_history[ hc_iXHist] = hc;			// add current value to history
											// note: hc_t[] values include elevation
											//   adjustment (see hc_Init())

	double hcAvg = VSum( hc_history, hcHISTL) / hcHISTL;		// compute average
	return hcAvg;

}		// HCI::hc_HC


//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class MASSBC: mass surface boundary condition
/////////////////////////////////////////////////////////////////////////////
RC MASSBC::bc_Setup(
	int msi,		// parent mass subsr
	int ty,			// adjacency type: MSBCZONE,
	int si)			// 0 = inside; 1 = outside
{
	bc_msi = msi;		// parent
	bc_ty = ty;			// adjacency type
	MSRAT* pMS = MsR.GetAt( msi);		// parent MSRAT
	const SFI* sf = pMS->ms_GetSFI();
	bc_zi = sf->sf_GetZI( si);
	if (!si)
	{	bc_h = sf->x.uI;
		bc_rsurf = pMS->ms_pMM->mm_rsurfi;		// inside addl (opaque) surface r
	}
	else
	{	bc_h = sf->x.sfExCnd==C_EXCNDCH_ADIABATIC
				?  0.f  :  sf->x.uX;	// surf film u: surf's outside's; 0 if adiabatic.
										// trial showed 0 nec for simulator
		bc_rsurf = pMS->ms_pMM->mm_rsurfo;			// outside addl surface r

		VD bc_exTa = VD sf->x.sfExT;			// outside air temp if MSBCSPECT. Hourly expr allowed; move as NANDAT.
	}

	bc_h = msrsurf( bc_h, bc_rsurf);		// combine rsurf with surface conductance
	bc_ha = bc_h * sf->x.xs_area;

	return RCOK;

}	// MASSBC::bc_Setup
//===========================================================================

/////////////////////////////////////////////////////////////////////////////
// class MSRAT: runtime model mass surface
//   Note: some members elsewhere
/////////////////////////////////////////////////////////////////////////////
MSRAT::~MSRAT()
{	DelSubObjects();
}			// MSRAT::~MSRAT
//---------------------------------------------------------------------------
/*virtual*/ void MSRAT::DelSubObjects(
	[[maybe_unused]] int options /*=0*/)
{
	delete ms_pMM;
	ms_pMM = NULL;
}		// MSRAT::DelSubObjects
//-----------------------------------------------------------------------------
/*virtual*/ void MSRAT::Copy(const record* pSrc, int options/*=0*/)
{
	DelSubObjects();

	// use base class Copy().  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED
	record::Copy( pSrc, options);				// src and this must be same record type.
	ms_pMM = ((MSRAT *)pSrc)->ms_pMM->Clone();
}			// MSRAT::Copy
//-----------------------------------------------------------------------------
int MSRAT::ms_IsMassless() const
{	return ms_pMM ? ms_pMM->mm_IsMassless() : 0;
}		// MSRAT::ms_IsMassless
//-----------------------------------------------------------------------------
RC msBegIvl(			// beg-of-interval all mass init
	int ivl)	// C_IVLCH_Y, _M, _D, _H, (_S)
// Note: not called for C_IVLCH_S as of 2-1-2012
//       change caller logic if substep init required
{
	MSRAT* mse;
#if 0
x	if (ivl != C_IVLCH_S)		// skip subhour (nothing currently required, 9-10)
#endif
		RLUP( MsR, mse)					// for mse = mass record 1 to n
			mse->ms_BegIvl( ivl);
	return RCOK;
}	// msBegIvl
//-----------------------------------------------------------------------------
void MSRAT::ms_BegIvl(		// beg-of-interval this mass init
	int ivl)	// C_IVLCH_Y, _M, _D, _H
				//   _S call not needed
{
	double qIE = ms_area * ms_pMM->mm_qIE;

	switch (ivl)
	{
	case C_IVLCH_Y:
		ms_ebErrCount = 0;
	case C_IVLCH_M:    			// at first call of each month. CAUTION: tp_DoDateDowStuff not yet called.
		inside.qxmnet =  inside.qxmtot =  0.;	// init monthly net and total heat xfer at each boundary
		outside.qxmnet = outside.qxmtot = 0.;	// ... to 0
#if 0
		ms_qm = ms_qd;	// internal energy at start of month = internal energy at start of day (set at end of previous day)
						// above inits ms_qm for 1st non-isWarmup month, also for start of warmup.
						//  ms_qm is set in doEndIvl, at end prior month, for end-of-interval probe-ability
						//  but that fails during startup cuz isEndMonth is not on on last day of Warmup
#endif

	case C_IVLCH_D:
		inside.qxdnet =  inside.qxdtot =  0.;	// init daily net and total heat xfer at each boundary
		outside.qxdnet = outside.qxdtot = 0.;	// ... to 0

	case C_IVLCH_H:
		inside.qxhnet =  inside.qxhtot =  0.;	// init daily net and total heat xfer at each boundary
		outside.qxhnet = outside.qxhtot = 0.;	// ... to 0
		for (int ivlx=ivl; ivlx<=C_IVLCH_H; ivlx++)
		{	ms_QIE( ivlx) = qIE;
			ms_QBal( ivlx) = 0.;
			ms_Flags( ivlx) = 0;
		}

		if (ms_isFD)
		{	MASSFD* pMFD = (MASSFD *)ms_pMM;
			if (pMFD->mm_flags & msfTEMPDEPENDENT)
				pMFD->mf_InitNodes( 1);
		}
	}

#if 0 && defined( _DEBUG)
	if (ivl <= C_IVLCH_M && trapName && strMatch( name, trapName))
		DbPrintf( "BegIvl %d %c %s:  ms_qm=%.f  ms_dqm=%.f  ms_QIE=%.f  ms_QIEDelta=%.f\n",
		   ivl, Top.isWarmup ? 'W' : '-', name,
		   ms_qm, ms_dqm, ms_QIE( ivl), ms_QIEDelta( ivl));
#endif

}		// MSRAT::ms_BegIvl
//-----------------------------------------------------------------------------
RC msEndIvl(			// end-of-interval mass calcs
	int ivl)	// C_IVLCH_Y, _M, _D, _H, _S
{
	if (ivl != C_IVLCH_S)		// skip subhour (nothing currently required, 9-10)
	{	MSRAT* mse;
		RLUP( MsR, mse)					// for mse = mass record 1 to n
			mse->ms_EndIvl( ivl);
	}
	return RCOK;
}	// msBegIvl
//-----------------------------------------------------------------------------
void MSRAT::ms_EndIvl(			// end-of-interval this mass calcs
	int ivl)	// C_IVLCH_Y, _M, _D, _H
				//   (_S call not needed)
{
	double qIE = ms_area * ms_pMM->mm_qIE;

	switch (ivl)
	{
	case C_IVLCH_Y:    		// last call of run
	case C_IVLCH_M:    		// last call of month
	case C_IVLCH_D:			// last call of day
		{
			for (int ivlx=ivl; ivlx<=C_IVLCH_D; ivlx++)
			{	ms_Flags( ivlx) |= ms_Flags( ivlx+1);
				ms_QBal( ivlx) += ms_QBal( ivlx+1);
			}
#if 0
			ms_dqd = qIE - ms_qd;		// delta for today
			ms_qd = qIE; 				// set start-of-day internal for next day
			if (Top.isEndMonth) 		// do here for q.  CAUTION: not done at end warmup; probably not at end autoSizing 6-95.
			{	ms_dqm = qIE - ms_qm;		// delta for month
				ms_qm = qIE;				// set start-of-month internal energy for next month.
			}
#endif
			inside.qxmnet += inside.qxdnet;  	// sum daily to monthly: inside net heat flow
			inside.qxmtot += inside.qxdtot;  	// .. inside total flow
			outside.qxmnet += outside.qxdnet;	// .. outside net
			outside.qxmtot += outside.qxdtot;	// .. outside total
		}

	case C_IVLCH_H:
		for (int ivlx=ivl; ivlx<=C_IVLCH_H; ivlx++)
			ms_QIEDelta( ivlx) = qIE - ms_QIE( ivlx);
	}

#if 0 && defined( _DEBUG)
	if (fabs( ms_dqm - ms_QIEDelta( C_IVLCH_M)) > .01)
		printf( "M error\n");
	if (fabs( ms_dqd - ms_QIEDelta( C_IVLCH_D)) > .01)
		printf( "D error\n");
	if (trapName && strMatch( name, trapName))
	{	if (ivl <= C_IVLCH_M)
		{	DbPrintf( "EndIvl %d %c %s:  ms_qm=%.f  ms_dqm=%.f  ms_QIE=%.f  ms_QIEDelta=%.f\n",
				ivl, Top.isWarmup ? 'W' : '-', name,
				ms_qm, ms_dqm, ms_QIE( ivl), ms_QIEDelta( ivl));
			if (ivl == C_IVLCH_Y)
				printf( "Y\n");
		}
	}
#endif
}		// MSRAT::ms_EndIvl
//-----------------------------------------------------------------------------
void MSRAT::ms_QXSurf(			// mass surface heat transfer for interval
	int ivl,	// C_IVCH_M, C_IVLCH_D, C_IVLCH_H
	double& qxNet,			// returned: net heat flow, Btu (= sum( qx)
	double& qxTot) const	// returned: total heat flow, Btu (=sum( abs(qx))
// + = into mass
{
	switch (ivl)
	{
	case C_IVLCH_M:
		qxNet = inside.qxmnet + outside.qxmnet;
		qxTot = inside.qxmtot + outside.qxmtot;
		break;

	case C_IVLCH_D:
		qxNet = inside.qxdnet + outside.qxdnet;
		qxTot = inside.qxdtot + outside.qxdtot;
		break;

	case C_IVLCH_H:
		qxNet = inside.qxhnet + outside.qxhnet;
		qxTot = inside.qxhtot + outside.qxhtot;
		break;

	default:
		qxNet = 0.;
		qxTot = 0.;
	}
}		// MSRAT::ms_QXSurf
//-----------------------------------------------------------------------------
void MSRAT::ms_RddInit(			// mass runtime init
	double t)		// initialization temp, F
{
	XSRAT* pXR = ms_GetXSRAT();

	ms_pMM->mm_RddInit( t);

	// init (both copies) of probe-able copies of surface temps
	pXR->x.xs_SBC( 0).sb_tSrf = pXR->x.xs_SBC( 1).sb_tSrf = t;
	inside.bc_surfTemp = outside.bc_surfTemp = t;
#if 0
	ms_qd = ms_pMM->mm_qIE * ms_area;
#endif
	ZNR* zp;
	if (IsMSBCZone( inside.bc_ty))		// boundary condition type: adiabatic, ambient, or zone. If zone...
	{
		zp = ZrB.p + inside.bc_zi;		// point inside adjacent zone
		zp->haMass += inside.bc_ha;		// sum h*area between zone and masses
		zp->zn_bcon += inside.bc_ha;		// add mass h*area to zone b const part
	}
	if (IsMSBCZone( outside.bc_ty))		// boundary condition type: adiabatic, ambient, or zone. If zone...
	{
		zp = ZrB.p + outside.bc_zi;		// point outside adjacent zone
		zp->haMass += outside.bc_ha;	// sum h*area between zone and masses
		zp->zn_bcon += outside.bc_ha;	// add mass h*area to zone b const part
	}
}		// MSRAT::ms_RddInit
//-----------------------------------------------------------------------------
ZNR* MSRAT::ms_GetZone(		// get zone adjacent to mass
	int si) const	// side 0=inside, 1=outside
{
	const MASSBC& bc = si ? outside : inside;
	if (!IsMSBCZone( bc.bc_ty) || bc.bc_zi <= 0)
		return NULL;

	return ZrB.GetAt( bc.bc_zi);
}		// MSRAT::ms_GetZone
//-----------------------------------------------------------------------------
RC MSRAT::ms_SetMSBCTNODE(		// alter inside boundary condition re external UZM surface coupling
	int erOp /*=WRN*/)
// Obsolete? 7-2012
{
	const char* msg=NULL;
	RC rc = RCOK;
	if (!ms_isFD)
		msg = "sfModel not 'forward_difference'";
	else if (ms_NLayer() != 1)
		msg = "must have exactly one layer";
	else if (outside.bc_ty != MSBCADIABATIC)
		msg = "sfExCnd not 'adiabatic'";
	if (!IsBlank( msg))
		err( erOp, "Surface '%s': UZM MSBCTNODE linkage fail (%s)", name, msg);
	else
	{	inside.bc_ty = MSBCTNODE;
		inside.bc_exTa = 70.f;
	}
	return rc;
}		// MSRAT::ms_SetMSBCTNODE
//-----------------------------------------------------------------------------
void MSRAT::ms_StepMX( 		// simulate mass: update mass node temperatures for cur hr / subhr
	float dur,		// timestep duration, hr
	float tDbO)		// outdoor temp, F
{
	MASSMATRIX* pMMX = (MASSMATRIX*)ms_pMM;
	int ibc;
	MASSBC* bcp;
	double tempWas[2];
	XSRAT* pXR = ms_GetXSRAT();
#ifdef MASSPRINT
o	static float qold;
o	float dq, qnew;
#endif
	double q[2];
		// boundary in & out heat flows PER UNIT AREA for mass, for msstep call, in form  Q + h * Tz.
		// Well, not exactly heat flows ... NET heat flow would be Q + h * (Tz - Tmass).
		// Not adjusted for subhrDur -- believe that's allowed for in mass matrix setup, 1-95.

	for (ibc = 0; ibc < 2; ++ibc)					// loop over inside (0) and outside (1) of mass
	{
		bcp = ibc ? &outside : &inside;   			// point to current boundary
		SBC& sbc = pXR->x.xs_SBC( ibc);
		tempWas[ ibc] = bcp->bc_surfTemp;
#if defined( _DEBUG)
		if (sbc.sb_tSrf != tempWas[ ibc])
			printf( "Yak");
#endif
		switch (bcp->bc_ty)
		{
		case MSBCADIABATIC:
			q[ibc] = 0.F;
			break;				// no heat flow to mass

		case MSBCSPECT:
		case MSBCGROUND:		// TODO: OK?
			q[ibc] = bcp->bc_h * bcp->bc_exTa;
			break;		// h*t; no q.  h is per area.

		case MSBCAMBIENT:
			{	float split = bcp->bc_h * bcp->bc_rsurf;
				q[ibc] = bcp->bc_h * tDbO + (1.f-split)*sbc.sb_sg/ms_area;	// h*t + qSolar/area
			}
			break;

		case MSBCZONE:
			q[ibc] = bcp->bc_h * ZrB.p[bcp->bc_zi].tz 	// surface h (per area) * zone est avg temp
					 + (sbc.sb_sg + bcp->rIg)/ms_area;	// solar and red internal gain (11-95) per area
			break;
		default:
			;
		}
	}

	pMMX->mx_Step( q[ 0], q[ 1]);
			// boundary conditions in form q + h*ta, per unit area.

	for (ibc = 0; ibc < 2; ++ibc)			// loop over inside (0) and outside (1) of mass
	{
		bcp = ibc ? &outside : &inside;   	// point to current boundary
		SBC& sbc = pXR->x.xs_SBC( ibc);
		bcp->bc_surfTemp = pMMX->mm_tSrf[ ibc];
								// copy surface layer temp to MASSBC for probing convenience
								// (can't probe a pointed-to array such as MASS.temp[])
		sbc.sb_tSrf = bcp->bc_surfTemp;		// keep SBC dup consistent

		switch (bcp->bc_ty)
		{
		case MSBCZONE:       // masses' UAT contribution to zone heat balance 'a':
			{	ZNR& z = ZrB[ bcp->bc_zi];
				(isSubhrly ? z.aMassSh : z.aMassHr)	// member for hour or subhour
					+= bcp->bc_ha*bcp->bc_surfTemp;		// h * area * mass surface temp
				if (!z.zn_IsConvRad())
				{	z.qMsSg += sbc.sb_sg;  	// add surf's inside solar gain to adjacent zone's .qMsSg
					z.qrIgMs += bcp->rIg;	// add surf's inside radiant int gain to adj zn's .qrIgMs 11-95
				}
				else
				{	z.zn_hcAMsSh  += bcp->bc_ha;
					z.zn_hcATMsSh += bcp->bc_ha*bcp->bc_surfTemp;
				}
			}
			// fall thru
		case MSBCSPECT:
		case MSBCAMBIENT:
		case MSBCGROUND:
			{	// heat flow into mass, for ebal check, = area * h * (adjacent temp - mass temp).
				// for mass use average of temps b4 and after ms_Step cuz ms_Step uses central difference approach
				//  TESTED: using avg temp not b4 or after temp reduces energy "imbalance" by about 1000x, 7-92.
				double qtem = ( ms_area*q[ibc] 				// area * h * adjacent temp (see above)
					 - bcp->bc_ha*(tempWas[ibc] + bcp->bc_surfTemp)/2.)	// - area*h * mass average temp
				     * dur;												// times hr/subhr duration

										// accum mass hourly and daily net and abs heat flows for energy bal checks
				bcp->qxhnet += qtem;
				bcp->qxhtot += fabs(qtem);
				bcp->qxdnet += qtem;
				bcp->qxdtot += fabs(qtem);
			}
			break;

			//case MSBCADIABATIC:
		default:
			break;
		}
	}
#ifdef MASSPRINT
o		qnew = msenth(mse);
o		dq = qold - qnew;
o		qold = qnew;
o		if (!Top.isWarmup)
o			printf( "\nhr=%d  xfer=%f dq=%f err=%f", Top.iHr, qtemp, dq, qtemp-dq);
#endif
}               // MSRAT::ms_StepMX
//--------------------------------------------------------------------------------------------------
void MSRAT::ms_StepFD()		// update mass temps with forward difference model
{
	int ibc;
	MASSBC* bcp;
	MASSFD* pMFD = (MASSFD *)ms_pMM;
	double tempWas[2];
	BCX bcx[ 2];
	XSRAT* pXR = ms_GetXSRAT();

	for (ibc = 0; ibc < 2; ++ibc)					// loop over inside (0) and outside (1) of mass
	{
		bcx[ ibc].Clear();
		bcp = ibc ? &outside : &inside;	// point to current boundary
		SBC& sbc = pXR->x.xs_SBC( ibc);
		tempWas[ ibc] = bcp->bc_surfTemp;	// save temp before msstep for use afterwards re qxdnet

		switch (bcp->bc_ty)
		{
		case MSBCADIABATIC:
			// all 0.
			break;				// no heat flow to mass

		case MSBCTNODE:
			bcx[ ibc].bcxTy = bcxTNODE;
			bcx[ ibc].tFixed = bcp->bc_exTa;
			// fall thru

		case MSBCSPECT:
		case MSBCAMBIENT:
		case MSBCZONE:
		case MSBCGROUND:
			bcx[ ibc].FromSBC( sbc);
			break;

		default:
			;
		}
	}

	pMFD->mf_Step( bcx);
	ms_Flags( C_IVLCH_H) |= pMFD->mm_flags;
	ms_QBal( C_IVLCH_H) += ms_area * pMFD->mm_qBal;

	for (ibc = 0; ibc < 2; ++ibc)			// loop over inside (0) and outside (1) of mass
	{
		bcp = ibc ? &outside : &inside;   	// point to current boundary
		SBC& sbc = pXR->x.xs_SBC( ibc);
		bcp->bc_surfTemp = pMFD->mm_tSrf[ ibc];	// copy surface layer temp to MASSBC for probing convenience 1-19-94
												// (can't probe a pointed-to array such as MSRAT.temp[])
		sbc.sb_tSrf = bcp->bc_surfTemp;			// keep SBC dup consistent

		switch (bcp->bc_ty)
		{
		case MSBCTNODE:
		case MSBCZONE:       // masses' contribution to zone heat balances
			{	ZNR& z = ZrB[ bcp->bc_zi];
				if (!z.zn_IsConvRad())
					z.aMassSh += bcp->bc_ha*bcp->bc_surfTemp;		// subhr combined model h * area * mass surface temp
				bcx[ ibc].ZoneAccum( ms_area, z);
#if defined( DEBUGDUMP)
				if (DbDo( dbdRCM))
					bcx[ ibc].DbDumpZA( strtprintf( "%s %s", z.name, name), ms_area);
#endif
#if 0
x				z.qMsSg += sbc.sb_sgTarg.st_tot;   	// add surf's inside solar gain to adjacent zone's .qMsSg
x				z.qrIgMs += bcp->rIg;	// add surf's inside radiant int gain to adj zn's .qrIgMs 11-95
#elif 0
x				z.qMsSg += bcp->sgTarg.st_tot;   	// add surf's inside solar gain to adjacent zone's .qMsSg
x				z.qrIgMs += bcp->rIg;	// add surf's inside radiant int gain to adj zn's .qrIgMs 11-95
#endif
			}
			// fall thru
		case MSBCSPECT:
		case MSBCAMBIENT:
		case MSBCGROUND:
		case MSBCADIABATIC:		// UZM mass coupling scheme adds fake mm_qSrf to "adiabatic" outside
								//   surfaces; this must be accounted for in heat balance 9-16-2010
			{	// accum mass hourly and daily net and abs heat flows for energy bal checks
				double qtem = ms_area * pMFD->mm_qSrf[ ibc] * Top.tp_subhrDur;
				bcp->qxhnet += qtem;
				bcp->qxhtot += fabs(qtem);
				bcp->qxdnet += qtem;
				bcp->qxdtot += fabs(qtem);

			}
			break;

		default:
			break;
		}
	}
}		// MSRAT::ms_StepFD
//-----------------------------------------------------------------------------
double MSRAT::ms_InternalEnergy() const	// current internal energy wrt 0 F
// Internal energy = heat capacity times temp above a reference)
// Returns total internal energy, Btu (not Btu/ft2)
{
	return ms_area * ms_pMM->mm_InternalEnergy();
}	                   // MSRAT::ms_Energy
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class MASSMODEL: base class for mass models
///////////////////////////////////////////////////////////////////////////////
/*virtual*/ MASSMODEL& MASSMODEL::Copy( const MASSMODEL& mmSrc, [[maybe_unused]] int options/*=0*/)
{
	mm_pMSRAT = mmSrc.mm_pMSRAT;		// caller may have to fix
	mm_model = mmSrc.mm_model;
	VCopy( mm_tSrf, 2, mmSrc.mm_tSrf);
	VCopy( mm_qSrf, 2, mmSrc.mm_qSrf);
	mm_layers = mmSrc.mm_layers;
	mm_rsurfi = mmSrc.mm_rsurfi;
	mm_rsurfo = mmSrc.mm_rsurfo;
	mm_hcTot = mmSrc.mm_hcTot;
	mm_flags = mmSrc.mm_flags;
	mm_qIE = mmSrc.mm_qIE;
	mm_qIEDelta = mmSrc.mm_qIEDelta;
	mm_qBal = mmSrc.mm_qBal;
	return *this;
}		// MASSMODEL::Copy
//----------------------------------------------------------------------------
const SFI* MASSMODEL::mm_GetSFI() const
{	return mm_GetMSRAT()->ms_GetSFI();
}		// MASSMODEL::mm_GetSFI
//----------------------------------------------------------------------------
RC MASSMODEL::mm_FromLayersFD(			// base init from physical layers
	const AR_MASSLAYER& arML,	// Array of physical layers (made by caller from e.g. CON LR)
   								// Layer 0 is inside. Notes:
								//   1.  layers w/ thick <= 0 illegal
								//   2.  layers w/ conductivity  <= 0 illegal
								//   3.  layers w/ vhc <= low value treated
								//       as non-massive (see code)
	[[maybe_unused]] int options /*=0*/)			// option bits (unused)

// Returns RCOK if OK; with following changes from arML --
//   1.  Outside light surfaces changed to rsurfs
//   2.  Adjacent interior light surfaces combined
//   3.  Massive surface broken up into modellable sublayers
// ELSE explanatory error code (not msg'd here). If ret value is != RCOK, NOT USABLE
{
	RC rc = RCOK;
	mm_hcTot = 0.;		// total heat capacity

	int nML = arML.size();

	if (nML == 0)			// don't even try if there are no layers
		return MH_R0122;	// "R0122: no layers in mass"

	mm_layers.clear();  	// no layers yet (insurance)

	rc = RCOK;         		// No error yet
	mm_rsurfi = 0.f;
	MASSLAYER wML;		// working copy of physical layer
	MASSLAYER tML;		// working copy of mass layer
	for (int iML = 0; iML < nML; iML++)
	{
		wML = arML[ iML];			// working copy
		if (wML.ml_thick <= 0.)		// Caller shd deal with 0 thickness layers
		{	if (rc == RCOK)
				rc = MH_R0123;		// "R0123: mass layer with 0 thickness encountered"
			continue;				//  skip layer
		}
		if (wML.ml_cond <= 0.)		// Can't deal with 0 conductivity
		{	if (rc == RCOK)
				rc = MH_R0124;		// "R0124: mass layer with 0 conductivity encountered"
			wML.ml_cond = .0001;	// set to small value, keep going
		}

		if (iML == 0)
			tML = wML;
		else if (tML.ml_IsMergeable( wML))
			tML.ml_Merge( wML);
		else
		{	mm_AddLayerFD( tML, &rc);
			tML = wML;
		}
		if (iML == nML-1)
			mm_AddLayerFD( tML, &rc);

	}  // physical layer loop

	// Did we end up with any layers?
	if (mm_layers.size() == 0)
		if (rc == RCOK)		// No, error; report it if first
			rc = MH_R0122;	// "R0122: no layers in mass"

	// mm_rsurfo = (float)rcomb;  ???

	return rc;             // Another return above
}		// MASSMODEL::mm_FromLayersFD
//-----------------------------------------------------------------------------
RC MASSMODEL::mm_AddLayerFD(		// add a MASSLAYER for forward difference model
	const MASSLAYER& ml,	// layer to be added
	RC* prc)				// prior error (RCOK if no errors yet)
// subdivides layer per numerical stability
// Returns RCOK iff layer successfully added to mm_layers
{
	RC rc = RCOK;

	const SFI* sf = mm_GetSFI();
	int nSL = ml.ml_SubLayerCount( sf->x.xs_lThkF);
#if 0 && defined( _DEBUG)
x	if (nSL > 1)
x		ml.ml_SubLayerCount( sf->x.xs_lThkF);	// call again re debugging
#endif
	[[maybe_unused]] int nML = mm_layers.size();
	{	mm_hcTot += ml.ml_HC();
		MASSLAYER tML = ml;
		tML.ml_Slice( nSL);
		for (int iSL=0; iSL<nSL; iSL++)
			mm_layers.push_back( tML);
	}
	if (*prc == RCOK)
		*prc = rc;
	return rc;
}                // MASSMODEL::mm_AddLayerFD
//-----------------------------------------------------------------------------
RC MASSMODEL::mm_FromLayers(			// base init from physical layers
	const AR_MASSLAYER& arML,	// Array of physical layers (made by caller from e.g. CON LR)
   								// Layer 0 is inside. Notes:
								//   1.  layers w/ thick <= 0 illegal
								//   2.  layers w/ conductivity  <= 0 illegal
								//   3.  layers w/ vhc <= low value treated
								//       as non-massive (see code)
								//   4.  At least 1 layer must be massive
	int options /*=0*/)			// option bits
								//   1: do not split or merge layers

/* Returns RCOK if OK; with following changes from arML --
     1.  Outside light surfaces changed to rsurfs
     2.  Adjacent interior light surfaces combined
     3.  Massive surface broken up into modellable sublayers
     4.  .mushlayer (homogenized mass layer) set up
     5.  .heaviest member set up (currently #ifd out)
   ELSE explanatory error code (not msg'd here). If ret value is != RCOK, NOT USABLE */
{
	RC rc = RCOK;
	BOOL bNoChange = (options & 1) != 0;
	mm_hcTot = 0.;		// total heat capacity

	/* Table of minimum vhcs for massive layers.  Any layer with a lower
	   vhc treated as light.  Make successive passes over all layers with
	   decreasing criterion until at least 1 massive layer is found. */
	static FLOAT vhcMassMin[] = { 1.F, .2F, 0.F, -1.F };
	int nML = arML.size();

	if (nML == 0)		// don't even try if there are no layers
		return MH_R0122;		// "R0122: no layers in mass"

	mm_layers.clear();  	// no layers yet (insurance)
	float thkcomb, rcomb = 0;
	double mushthick, mushr, mushhc;

	// Loop over layers with several vhc massiveness thresholds to try to
	// limit number of layers but also make sure we get some.  End loop
	//   1. when we find some layers OR
	//   2. when we can't get any lighter
	for (int ivhc = 0; vhcMassMin[ ivhc] >= 0.f && mm_layers.size() == 0; ivhc++)
	{
		rc = RCOK;         		// No error yet
		int seenmass = FALSE;
		thkcomb = rcomb = 0.f;
		mushthick = mushr = mushhc = 0.;		// re out-of-service mushlayer, may be useful
		mm_rsurfi = 0.f;
		for (int i = 0; i < nML; i++)
		{
			const MASSLAYER* pML = &arML[ i];
			if (pML->ml_thick <= 0.f)			// Caller shd deal with 0 thickness layers
			{	if (rc == RCOK)
					rc = MH_R0123;	// "R0123: mass layer with 0 thickness encountered"
				continue;			// skip layer
			}
			double cond = pML->ml_cond;
			if (cond <= 0.f)		// Can't deal with 0 conductivity
			{	if (rc == RCOK)
					rc = MH_R0124;	// "R0124: mass layer with 0 conductivity encountered"
				cond = .0001f;		// set to small value, keep going
			}
			double rval = pML->ml_thick / cond;	// rval of current layer (cond cannot be 0, checked above)
			if (bNoChange || pML->ml_vhc > vhcMassMin[ ivhc])	// massive (or treating all as massive)?
			{
				seenmass = TRUE;  					// A mass layer has been seen
				double hc = pML->ml_HC();	// hc of current layer
				mushhc += hc;
				mushthick += pML->ml_thick;   			// Accum mush values
				mushr += rval;
				if (rcomb > 0.f)
				{
#if 1
					mm_AddLayer(		// Add accumulated resistive layer (below)
						1.f,  			//   Thickness = 1 ft
						0.f,  			//   vhc = 0.
						1.f/rcomb,		//   Conductivity = 1/r
						&rc );			//   Cumulative RC, NOP if not RCOK

#else
					untested idea: preserve thickness of resistive layer(s)
					if (thkcomb < .0001f)
						thkcomb = 1.f;
					mm_AddLayer(		// Add accumulated resistive layer (below)
						thkcomb,  		//   Thickness
						0.f,  			//   vhc = 0.
						thkcomb/rcomb,	//   Conductivity = 1/r
						&rc );			//   Cumulative RC, NOP if not RCOK
#endif
					rcomb = 0.f;
					thkcomb = 0.f;
				}
				int sublayers = bNoChange
									? 1
									: min( 4, int( pML->ml_thick * 12.f) + 1);
											// approx. number of 1 inch slices (4 max)
				double thick = pML->ml_thick/(float)sublayers;  	// Actual sublayer thckns
				for (int j = 0; j < sublayers; j++)		// Add sublayers
					mm_AddLayer(			// Add layer (fcn below). nop if error.
						(float)thick,		//   Sublayer thickness
						pML->ml_vhc,   		//   vhc
						(float)cond,		//   Conductivity
						&rc );      		// Cumulative RC, NOP if not RCOK
			}
			else
			{	// layer is not massive
				if (!seenmass)
					mm_rsurfi += (float)rval;	// if mass layer has not been seen, accum inside rsurf
				else
				{	thkcomb += pML->ml_thick;	// otherwise, accum for interior light layer or rsurfo
					rcomb += rval;
				}
			}
		}  // Physical layer loop
	}  // vhcMassMin loop

// Did we end up with any layers?
	if (mm_layers.size() == 0)
		if (rc == RCOK)		// No, error; report it if first
			rc = MH_R0122;	// "R0122: no layers in mass"

// Store outside extra surface resistance
	mm_rsurfo = (float)rcomb; 	// Any leftover rvalue is rsrufo

#if 0	// unused 8-17-2010
x // Mush layer = single homogenized layer
x	if (mushthick > 0.)
x	{	mushlayer.ml_Set(
x			float( mushthick),			// thkns, ft
x			float( mushhc/mushthick),	// vhc
x			float( mushthick/mushr));	// cond (mushr not 0 if mushthick not 0)
x	}
#endif

	return rc;             // Another return above
}		// MASSTYPE::mm_FromLayers
//-----------------------------------------------------------------------------
RC MASSMODEL::mm_AddLayer(		// add a MASSLAYER
	float thick,	// layer thickness, ft, > 0
	float vhc,		// layer volumetric heat cap, Btu/ft3-F
	float cond,		// layer conductivity, Btuh/F-ft2
	RC* prc )		// pt to cummulative error rc, NOPs if *prc != RCOK

// Returns RCOK if layer successfully added (this updated)
{
	// Don't do anything if there's been an error
	if (*prc != RCOK)
		return *prc;

	RC rc = RCOK;

#if 0	// now re-sizes as needed, 11-20-14
x // Check absolute size limit -- need space for 1 more layer + 1 (since nodes = layers + 1)
x	int nML = mm_layers.size();
x	if (nML+2 > MXMAXINV)	// MXMAXINV (nummeth.h) = 50, 1-95.
x	{
x		rc = MH_R0125;		// "R0125: number of mass layers exceeds program limit.
x		// Reduce thickness and/or number of layers in construction"
x		goto done;
x	}
#endif

// Get MASSLAYER pointer, store data
	mm_hcTot += thick*vhc;
	mm_layers.push_back( MASSLAYER( -1, thick, vhc, cond));

	*prc = rc;	// *prc is RCOK or we would not be here; simply set it to new value
	return rc;
}                // MASSMODEL::mm_AddLayer
//=============================================================================


///////////////////////////////////////////////////////////////////////////////
// class MASSMATRIX: central-difference (aka "matrix") mass model
///////////////////////////////////////////////////////////////////////////////
MASSMATRIX::MASSMATRIX(
	MSRAT* pMSRAT /*=NULL*/,
	int model /*=0*/)
	: MASSMODEL( pMSRAT, model), mx_hc(), mx_M(), mx_temp()
{
	mx_SetSize( 3);
}		// MASSMATRIX::MASSMATRIX
//-----------------------------------------------------------------------------
MASSMATRIX::MASSMATRIX( const MASSMATRIX& mx)
	: MASSMODEL( mx), mx_hc(), mx_M(), mx_temp()
{
	Copy( mx);
}		// MASSMATRIX::MASSMATRIX
//-----------------------------------------------------------------------------
void MASSMATRIX::mx_SetSize(
	int n)		// # of nodes (# layers + 1)
				//  (0 is OK)
{
	mx_hc.resize( n);
	mx_M.resize( n*(n+2));
	mx_temp.resize( n);
	mx_tOld.resize( n+2);
}		// MASSMATRIX::mx_SetSize
//-----------------------------------------------------------------------------
/*virtual*/ MASSMATRIX& MASSMATRIX::Copy( const MASSMODEL& mmSrc, int options /*=0*/)
{
	MASSMODEL::Copy( mmSrc, options);		// base class
	const MASSMATRIX& mx = (MASSMATRIX&)mmSrc;
	mx_hc = mx.mx_hc;
	mx_M = mx.mx_M;
	mx_temp = mx.mx_temp;
	return *this;
}		// MASSMATRIX::Copy
//-----------------------------------------------------------------------------
/*virtual*/ MASSMATRIX* MASSMATRIX::Clone() const
{
	MASSMATRIX* p = new MASSMATRIX( *this);
	return p;
}		// MASSMATRIX::Clone
//-----------------------------------------------------------------------------
RC MASSMATRIX::mx_Setup(			// set up matrix for central difference model

	float t, 			// time step duration (hours). 1 or 1/nSubsteps is used.
	float hi, float ho)	// inside & outside surface heat transfer coeffs (conductivity) (Btuh/F-ft2).  MASSBC[].h's are passed.

// Checked here:  0 < nlayers;  matrix not singular.

// Layers Restrictions: The thickness and conductivity of each layer must be strictly positive (not checked here).
//  		        The volumetric heat capacity of an interior layer can be 0 to simulate a resistive layr.
//	  		However, 2 resistive layers CANNOT be juxtaposed and there CANNOT be resistive layers on the boundaries.
//			In either case one can add the resistivities (done in msMsty for cr, by user for ht)

// returns RCOK or (message issued)
{
	RC rc = RCOK;
	int nML = mm_NLayer();		// # of layers
	int n = nML+1;				// # of nodes = # layers + 1
	if (n <= 1)
	{	rc = MH_R0120;		// "R0120: Mass '%s': no layers"
		err( PWRN,			// display program error msg, wait for key
			 (char *)rc, Name());				//  arg for msg
		mx_SetSize( 0);
		return rc;
	}

	mx_SetSize( n);

	// note all values are per unit area (makes no difference since only result used is temperatures)
	double bi, bo;		// adjustments of ut at boundaries: Niles' Beta.
	double ut0,ut1;  	// conductance on each side of node, times t (time step duration).
	double hc0,hc1,hc;	// hc0, hc1: heat capacities of half-layers on each side of (interior) node
	double utemp;		// ut/hc: subexpression precomputed at boundaries, where used 4 times

	double* nodehcp = &mx_hc.front();

// allocate temp A and B matrices for computing mass matrix M.  combine?
	WVect< double> A0( n*n, 0.);
	double* A = &A0.front();
	WVect< double> B0( n*(n+2), 0.);
	double* B = &B0.front();
	WVect< int> scratch( 3*(n+2), 0);

	MASSLAYER* layer = mm_layers.GetLayers();	// working pointer

// Node 0
	ut1  = t*layer->ml_cond/layer->ml_thick;		// time step times conductance to node 1
	bi   = 1. + hi*layer->ml_thick/layer->ml_cond;	// adjusts conductance to boundary  hi*T + Q
	hc1  = layer->ml_HC()/2.;		// heat capacity per unit area of node (boundary node includes 1/2 layer only)
	utemp = ut1/hc1;				// subexpression: u*t/hc
	*A++ = 2. + utemp*bi;			// A[0,0]: coeff of T0 in node 0 equation (T===temp)
	*A++ = -utemp;					// A[0,1]: coeff of T1
	*B++ = 2. - utemp*bi;			// B[0,0]: coeff of old T0 in node 0 equation right side (ie constant then)
	*B++ = utemp;					// B[0,1]: coeff of old T1 ditto
	B0[ n]  = 2.*t/hc1;				// B[0,n]: coeff of boundary cond  hi*Ti + Qi  in node 0 equation right side.
	//   rest of B[,n] column is left 0.
	*nodehcp++ = hc1;				// node heat capacity, for msenth.

// Middle nodes
	for (int i = nML; --i > 0; )
	{
		A += nML-1;				// += n-2: point 1 col left of diagonal in next row
		B += n;					// ditto (B has 2 more columns than A)
		layer++;					// point next layer
		hc0 = hc1;				// prior heat cap in next layer is now hc in previous layer
		hc1 = layer->ml_HC()/2.;	// compute heat cap in next layer.  Only 1/2 layer goes with this node.
		hc = hc0 + hc1;				// total heat capacity associated with node
		ut0 = ut1;				// conductance (u) times time step (t): old next is now previous
		ut1  = t*layer->ml_cond/layer->ml_thick;	// conductance thru layer to next node
		*A++ = -ut0/hc;				// column left of diagonal: coeff of T[j-1] in eqn j
		*A++ = 2. + (ut0+ut1)/hc;		// on diagonal: coeff of T[j]
		*A++ = -ut1/hc;				// right of diagonal: T[j+1]
		*B++ = ut0/hc;				// col left of diagonal: coeff of old T[j-1] in eqn j right side
		*B++ = 2. - (ut0+ut1)/hc;		// diagonal: old T[j]
		*B++ = ut1/hc;				// right of diagonal
		*nodehcp++ = hc;				// save node heat capacity, for msenth
	}

// Node n-1.  hc0 = hc1  and  ut0 = ut1  are not moved.
	A += nML-1;				// point last row, next to last col
	B += n;					// point last row, 3rd from last col (B has 2 additional columns)
	bo = 1. + ho*layer->ml_thick/layer->ml_cond;	// multiply ut by this at boundary
	utemp = ut1/hc1;				// u*t/hc for this node
	*A++ = -utemp;				// coeff of T[n-2] in eqn n-1
	*A   = 2. + utemp*bo;			// lower right corner of A: coeff T[n-1]
	*B++ = utemp;				// coeff of old T[n-2] in eqn n-1 right side
	*B++ = 2. - utemp*bo;			// coeff old T[n-1]
	*(++B) =  2.*t/hc1;				// pre ++ skips element: coeff of  ho+To + Qo in  eqn n-1 right side.
	//   only non-0 element in B[,n+1] column.
	*nodehcp = hc1;				// save node heat capacity for msenth

#if 0 && defined( DEBUGDUMP)
	A = A0.GetData();
	for (i=0; i<n; i++)
		VDbPrintf( dbdALWAYS, i==0 ? "\nA matrix --\n" : "", A+i*n, n, " %8.4f");
	B = B0.GetData();
	for (i=0; i<n; i++)
		VDbPrintf( dbdALWAYS, i==0 ? "\nBC matrix --\n" : "", B+i*(n+2), n+2, " %8.4f");
#endif
#if defined( CHECKPRINT)
	// Print matrices if debug (unused since 80s)
	mxprint( "A matrix ---",  (char *)A0, DTDBL, n, n);
	mxprint( "BC matrix ---", (char *)B0, DTDBL, n+2, n);
	USI nbytes = n*n*sizeof(double);
	double *asav;
	dmal( DMPP( asav), nbytes, ABT);
	memcpy( asav, A0, nbytes);
#endif

// Calculate mass matrix M = A-inv * B
	// multiplies each column of B times A-inverse
	// or (equivalently) solves A=each column of B, and replaces the B column with the solution.
	// Either way, B is replaced with M: the matrix by which to multiply extended old temp vector to get new temps.
	//  ("extended old temp vector" means boundary h*T + Q's have been stored in last 2 elements: why B, M not square.)
#ifndef CHECKPRINT
	int invflag = FALSE;
#else
	int invflag = TRUE;		// non-0 for return of A-inverse for debugging printout, else not needed
#endif
	// solve/invert, return nz on error.
	if (gaussjb( &A0.front(), n, &B0.front(), n+2, invflag))		// solve/invert, return nz on error.  gaussjb.cpp
	{	rc = MH_R0121; 			// "R0121: %s(): mass matrix too large or singular, cannot invert"
		err( PWRN,				// display program error msg, wait for key, rmkerr.cpp
			 (char *)rc, "mx_Setup");		//  arg for msg
		// not necessarily an internal error; rework to issue input error if actually tends to occur.
		// delete massmxp;			// and set massmxp NULL for return
		// massmxp = NULL;
		goto done;
	}
	mx_M = B0;

#ifdef CHECKPRINT
	double *aia;
	dmal( DMPP( aia), nbytes, ABT);
	mxdmul( asav, n, n, A0, n, aia);
	mxprint( "A-INV --", (char *)A0, DTDBL, n, n);
	mxprint( "A-INV * A --", (char *)aia, DTDBL, n, n);
	mxprint( "A-INV * B matrix --", (char *)B0, DTDBL, n+2, n);
	mxprint( "Node HCs --", (char *)massmxp->val, DTFLOAT, n, 1);
	may now be double
	dmfree( DMPP( asav));
	dmfree( DMPP( aia));
#endif
done:
	return rc;					// another error return above
}			// MASSMATRIX::mx_Setup
//=============================================================================
void MASSMATRIX::mx_Step( 		// simulate mass: update mass node temperatures for current time step (hour or subhour)
	double qsurf1, double qsurf2) 	// Current in and outside boundary conditions,  q + h*T (Btuh/ft2)  (T = temp, not time)
{
	int n = mm_NNode();		// working copy
	int n2 = n + 2;  		// n + 2: # columns in M / # old temps after boundaries added.

// set up old temp vector
	double* T0  = mx_Temp();	// where temps are kept / working pointer to store new temps
	double* T = T0;
	double* TOld = mx_TOld();	// prior temps
	VCopy( TOld, n, T);			// TOld[0..n-1] is T[]
	TOld[n] = qsurf1;
	TOld[n+1] = qsurf2;			// TOld[n..n+1] is boundary condition q's

// find new temperatures: T = M * TOld.  Hmm... seems to be T += M * TOld.
	double* M = &mx_M.front();		// pointer to matrix M = Ainv * B, as computed in mx_Setup
	for (int k = n; --k >= 0; )		// loop over n new temps = n rows of M
	{
		double t = 0.;				// for accumulating Temp[k] change
		for (int l = -1; ++l < n2; )	// loop over n+2 cols of M = n+2 elements of TOld[]
			t += TOld[ l] * *M++;      		// sum TOld[l] times M[ ,l]
		*T++ = t;				// Add temp to old value, point next new temp
	}

	// latest surface temps
	mm_tSrf[ 0] = T0[ 0];
	mm_tSrf[ 1] = T0[ n-1];
	mm_qSrf[ 0] = 0.;	// TODO
	mm_qSrf[ 1] = 0.;

	double qIE = mm_InternalEnergy();
	mm_qIEDelta = qIE - mm_qIE;
	mm_qIE = qIE;

#if defined( DEBUGDUMP)
	if (DbDo( dbdMASS))
	{	DbPrintf( "mx_step '%s': qsurf1=%.2f  qsurf2=%.2f\n", Name(), qsurf1, qsurf2);
		VDbPrintf( "", mx_Temp(), n, " %.2f");
	}
#endif
}               // MASSMATRIX::mx_Step
//-----------------------------------------------------------------------------
void MASSMATRIX::mm_InitTemps( double t)
{	VSet( mx_Temp(), mm_NNode(), t);
	mm_qIE = mm_InternalEnergy();
	mm_qIEDelta = 0.;
}		// MASSMATRIX::mm_InitTemps
//-----------------------------------------------------------------------------
double MASSMATRIX::mm_InternalEnergy() const	 // current mass internal energy above 0 F
// internal energy = heat capacity times temp abovereference
// returns current mass internal energy per unit area, Btu/ft2
{
// return area times inner product of mass's nodes' temps & hc's
	return VIProd(  		// inner (dot) product of double vectors
			   mx_Temp(), 		// current temps of nodes
			   mm_NNode(),		// # nodes (vector length)
			   &mx_hc.front());	// heat capac (Btu/F-ft2)
}	                   // MASSMATRIX::mm_InternalEnergy
//-----------------------------------------------------------------------------
void MASSMATRIX::mm_FillBoundaryTemps(
	float tLrB[],
	int tLrBDim) const
{
	VSet( tLrB, tLrBDim, -99.f);
}		// MASSMATRIX::mm_FillBoundaryTemps
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class MASSFD: forward-difference mass model
// class MASSNODE: single node in forward difference model
///////////////////////////////////////////////////////////////////////////////
int MASSNODE::mn_Init( MASSFD* pMFD, const MASSLAYER& ml)
// returns nz iff this node has temperature-dependent properties
{
	mn_pMFD = pMFD;
	mn_hc = mn_hcEff = ml.ml_HC();
	mn_thick = ml.ml_thick;
	mn_uh = 2.*ml.ml_C();			// init to nominal conductance
	if (mn_thick > 0.)
	{	// coefficients for temp-dependent cond
		//   cond = A + B*T
		double A, B;
		ml.ml_CondAB( A, B);
		mn_uhA = 2.*A/mn_thick;			// adjust for half-thickness
		mn_uhB = 2.*B/mn_thick;
	}
	else
		mn_uhA = mn_uhB = 0.;
	mn_ubn = 0.;
	VZero( mn_f, 3);
	mn_tauErrCount = 0;

	return mn_IsCondTempDependent();

}	// MASSNODE::mn_Init
//----------------------------------------------------------------------------
void MASSNODE::mn_TStepSrf(			// update temp of surface node (1st or last)
	const BCX& bcx,		// boundary conditions
	double told,		// prior step node temp, F
						// adjacent node: [iL+1] for inside, [iL-1] for outside
	double ubnAdj,		//    coupling to adjacent node, Btuh/F
	double toldAdj)		//    prior step temp of adjacent node, F
// returns updated node temp, F
{
	// Note: bcx.bcxTy == bcxTNODE not allowed with >1 layer
	//       see single-node special case in MASSFD::mf_Step()
	double C = bcx.C() + ubnAdj;
	double tss = (bcx.CTQ() + ubnAdj*toldAdj) / C;
	double dtOvrTau = mn_DtOvrTau( C);
	mn_t = (1. - dtOvrTau)*told + dtOvrTau*tss;
}		// MASSNODE::mn_TStepSrf
//-----------------------------------------------------------------------------
void MASSNODE::DbDump( int iL) const
{	DbPrintf( "    L %d  t=%0.2f  cond=%0.4f  hc=%0.4f  hcEff=%0.4f  ubn=%0.4f  f=%0.4f/%0.4f/%0.4f\n",
			iL, mn_t, mn_Cond(), mn_hc, mn_hcEff, mn_ubn, mn_f[ 0], mn_f[ 1], mn_f[ 2]);
}	// MASSNODE::DbDump
//-----------------------------------------------------------------------------
MASSFD::MASSFD(
	MSRAT* pMSRAT /*=NULL*/,
	int model /*=0*/)
	: MASSMODEL( pMSRAT, model), mf_nd(), mf_tOld()
{
	mf_SetSize( 3);
}		// MASSFD::MASSFD
//-----------------------------------------------------------------------------
MASSFD::MASSFD( const MASSFD& mf)
	: MASSMODEL( mf), mf_nd(), mf_tOld()
{
	Copy( mf);
}		// MASSFD::MASSFD
//-----------------------------------------------------------------------------
void MASSFD::mf_SetSize( int n)
{
	mf_nd.resize( n);
	mf_tOld.resize( n);
}		// MASSFD::mf_SetSize
//----------------------------------------------------------------------------
/*virtual*/ void MASSFD::mm_InitTemps( double t)
{
	int nL = mm_NNode();
	for (int iL=0; iL < nL; iL++)
		mf_nd[ iL].mn_t = t;

	// reset surface node effective HCs (*crucial* for tracking internal energy change)
	mf_nd[ 0].mn_hcEff = mf_nd[ 0].mn_hc;
	mf_nd[ nL-1].mn_hcEff = mf_nd[ nL-1].mn_hc;

	mm_qIE = mm_InternalEnergy();
	mm_qIEDelta = 0.;
}		// MASSFD::mm_InitTemps
//-----------------------------------------------------------------------------
/*virtual*/ MASSFD& MASSFD::Copy( const MASSMODEL& mmSrc, int options /*=0*/)
{
	MASSMODEL::Copy( mmSrc, options);	// base class
	[[maybe_unused]] const MASSFD& mf = (MASSFD&)mmSrc;
	mf_nd = mf_nd;
	mf_tOld = mf_tOld;
	return *this;
}		// MASSFD::Copy
//-----------------------------------------------------------------------------
/*virtual*/ MASSFD* MASSFD::Clone() const
{
	MASSFD* p = new MASSFD( *this);
	return p;
}		// MASSFD::Clone
//-----------------------------------------------------------------------------
RC MASSFD::mf_Setup()			// set up forward difference model from mm_layers
// returns RCOK or error RC (msg'd here)
{
	RC rc = RCOK;
	int nL = mm_layers.size();
	mf_SetSize( nL);
	// clear?
	if (nL <= 0)
		rc = MH_R0122;	// "R0122: no layers in mass"
	if (!rc)
	{	[[maybe_unused]] int nLx = nL - 1;
		MASSNODE* nd = &mf_nd.front();
		int iL;
		for (iL=0; iL<nL; iL++)
		{	if (nd[ iL].mn_Init( this, mm_layers[ iL]))
				mm_flags |= msfTEMPDEPENDENT;
		}
		mf_InitNodes( 0);
	}

	if (rc)
		err( PWRN,			// display program error msg, wait for key
			 (char *)rc, "mf_Setup");				//  arg for msg

	return RCOK;
}		// MASSFD::mf_Setup
//-----------------------------------------------------------------------------
void MASSFD::mf_InitNodes(		// derive node constants
	int doCondUpdate /*=0*/)	// 1: update any time-dependent conductivities
{
	MASSNODE* nd = &mf_nd.front();
	int nL = mf_nd.size();

	int nLx = nL - 1;
	int iL;
	if (doCondUpdate)
	{	int change = 0;
		for (iL=0; iL<nL; iL++)
		{	if (nd[ iL].mn_IsCondTempDependent())
			{
#if 1
				nd[ iL].mn_UpdateUh();
				change++;
#else
x			Experiment: skip calc if mn_uh changes all small
x           Did not save any measurable time, 3-7-2012
x			If pursued:
x			  1) make test factional
x			  2) don't store new mn_uh unless change not small
x				double uhOld = nd[ iL].mn_uh;
x				nd[ iL].mn_UpdateUh();
x				double d = nd[ iL].mn_uh - uhOld;
x				change += fabs( d) > .0001;
#endif
			}
		}
		if (!change)
			return;		// no changes
		// else something changed
		// re-derive constants for *all* layers
		// (not worth trouble to track which layer(s) changed)
	}

	// conductance to next layer
	for (iL=0; iL<nLx; iL++)
		nd[ iL].mn_ubn = seriesU( nd[ iL].mn_uh, nd[ iL+1].mn_uh);
	nd[ nLx].mn_ubn = 0.;		// last node: no coupling

	// interior nodes pre-computed factors
	[[maybe_unused]] int hcChange = 0;		// nz if node HC altered (not used here)
	for (iL=1; iL<nLx; iL++)
	{	double dtOvrTau = nd[ iL].mn_DtOvrTau( nd[ iL-1].mn_ubn + nd[ iL].mn_ubn);
		nd[ iL].mn_f[ 0] = dtOvrTau*nd[ iL-1].mn_ubn/(nd[ iL-1].mn_ubn + nd[ iL].mn_ubn);
		nd[ iL].mn_f[ 1] = 1. - dtOvrTau;
		nd[ iL].mn_f[ 2] = dtOvrTau*nd[ iL].mn_ubn/(nd[ iL-1].mn_ubn + nd[ iL].mn_ubn);
	}
}	// MASSFD::mf_InitNodes
//-----------------------------------------------------------------------------
void BCX::SetDerived(
	const MASSNODE& nd)		// mass node adjacent to boundary
{
	bd = nd.mn_uh + hc + hr;
	if (bd > 0.)
	{	f = nd.mn_uh/bd;
		cx = hc * hr / bd;
		tsx = (ta*hc + tr*hr + qrAbs)/bd;
	}
	else
	{	f = 0.;
		cx = 0.;
		tsx = 0.;
	}
	cc = hc * f;
	cr = hr * f;
	qrNd = qrAbs * f;

}		// BCX::SetDerived
//-----------------------------------------------------------------------------
#if defined( _DEBUG)
// RELEASE versions are inline, see mspak.h
double BCX::TSrf() const		// surf temp at this boundary
// returns surface temp, F
{
	return tsx + tNd*f;
}	// BCX::TSrf
//-----------------------------------------------------------------------------
double BCX::QSrf(				// heat flux at surface
	double tNdL)		// prior step node temp adjacent to surface, F
						//   (forward diff heat gain is derived from prior step)
// returns heat flow at mass surface, Btuh/ft2 + = into surface
{
	qSrf = cc * (ta - tNdL) + cr * (tr - tNdL) + qrNd;
	return qSrf;
}	// BCX::QSrf
#endif	// _DEBUG
//-----------------------------------------------------------------------------
void BCX::ZoneAccum(			// accumulate to zone heat balance values
	double A,		// exposed area of this boundary, ft2
	ZNR& z)	const	// adjacent zone
{
	z.zn_nAirSh += A*(cc*tNd + qrAbs*hc/bd);
	z.zn_dAirSh += A*cc;
	z.zn_nRadSh += A*(cr*tNd + qrAbs*hr/bd);
	z.zn_dRadSh += A*cr;
	z.zn_cxSh   += A*cx;

	z.zn_hcAMsSh  += A*cc;
	z.zn_hcATMsSh += A*(cc*tNd - qrNd);
	z.zn_hrAMsSh  += A*cr;
	z.zn_hrATMsSh += A*cr*tNd;

}	// BCX::ZoneAccum
//----------------------------------------------------------------------------
void BCX::DbDumpZA(				// dump zone accum components
	const char* tag,	// line tag for report
	double A)			// exposed area of this boundary, ft2
{
	DbPrintf(
		"%s  A=%0.1f  cc=%0.4f  cr=%0.4f,  cx=%0.4f  tNd=%0.2f  qrAbs=%0.2f  hc=%0.3f  hr=%0.3f  bd=%0.3f\n"
		"    nAir=%0.2f  dAir=%0.2f  nRad=%0.2f  dRad=%0.2f  CX=%0.2f\n",
		tag,
		A, cc, cr, cx, tNd, qrAbs, hc, hr, bd,
		A*(cc*tNd + qrAbs*hc/bd),
		A*cc,
		A*(cr*tNd + qrAbs*hr/bd),
	    A*cr,
		A*cx);
}		// BCX::DbDumpZA
//-----------------------------------------------------------------------------
void MASSFD::DbDump( BCX* bcx) const
{
	int nL = mm_NNode();
	int nLx = nL-1;
	const MASSNODE* nd = &mf_nd.front();
	const double* tOld = &mf_tOld.front();
	double qBal = fabs( mm_qBal) < 1.e-6 ? 0. : mm_qBal;	// prevent -0 / 0 diffs
	DbPrintf( "Surf '%s'  qIE=%.4f  qIEdelta=%.5f  qSrfNet*dt=%.5f  qBal=%.5f\n"
		     "       tOldi=%.4f  qSrfi*dt=%.5f  HCi=%.4f  HCEffi=%.4f  tNdi=%.4f  qIEi=%.4f\n"
			 "       tOldo=%.4f  qSrfo*dt=%.5f  HCo=%.4f  HCEffo=%.4f  tNdo=%.4f  qIEo=%.4f\n",
		Name(), mm_qIE, mm_qIEDelta, Top.tp_subhrDur*(mm_qSrf[ 0]+mm_qSrf[ 1]), qBal,
		tOld[ 0],   Top.tp_subhrDur*mm_qSrf[ 0], nd[   0].mn_hc, nd[   0].mn_hcEff, nd[   0].mn_t,
			nd[  0].mn_InternalEnergy(),
		tOld[ nLx], Top.tp_subhrDur*mm_qSrf[ 1], nd[ nLx].mn_hc, nd[ nLx].mn_hcEff, nd[ nLx].mn_t,
			nd[ nLx].mn_InternalEnergy());
	if (bcx)
	{	bcx[ 0].DbDump( "   Inside");
		for (int iL=0; iL<nL; iL++)
			nd[ iL].DbDump( iL);
		bcx[ 1].DbDump( "   Outside");
	}
}		// MASSFD::DbDump
//-----------------------------------------------------------------------------
void BCX::DbDump( const char* tag)
{
	DbPrintf( "%s  ty=%d  hc=%.4f  hr=%.4f  cc=%.4f  cr=%.4f  cx=%.4f  bd=%.4f  f=%.4f\n"
		"       tFixed=%.3f  ta=%.3f  tr=%.3f  qrAbs=%.2f  tsx=%0.2f  qrNd=%.2f  tNd=%.4f  tSrf=%.4f  qSrf=%.4f\n",
		tag, bcxTy, hc, hr, cc, cr, cx, bd, f,
		tFixed, ta, tr, qrAbs, tsx, qrNd, tNd, TSrf(), qSrf);

}		// BCX::DbDump
//----------------------------------------------------------------------------
void BCX::FromSBC(
	const SBC& sbc)
{
	ta = sbc.sb_txa;
	tr = sbc.sb_txr;

	hc = sbc.sb_hxa;
	hr = sbc.sb_hxr;

	qrAbs = sbc.sb_qrAbs;
}		// BCX::FromSBC
//=============================================================================
void MASSFD::mf_Step( 		// forward difference mass simulation step (subhourly)
	BCX bcx[ 2])		// boundary conditions
						//    [0] = inside, [1] = outside
{
#if 0 && defined( DEBUGDUMP)
x	if (DbDo( dbdMASS))
x		printf( "Hit\n");
#endif

#if 0
	if (mm_flags && msfTEMPDEPENDENT)
		mf_InitNodes( 1);		// something is temp dependent, re-init node constants
#endif

	int nL = mm_NNode();		// # of nodes
	int nLx = nL-1;				// idx of last node
	MASSNODE* nd = &mf_nd.front();
	double* tOld = &mf_tOld.front();

	VCopy( tOld, nL, &nd[ 0].mn_t, int( sizeof( MASSNODE)));		// prior step temps
	bcx[ 0].SetDerived( nd[   0]);
	bcx[ 1].SetDerived( nd[ nLx]);

	if (nL == 1)
	{	// single layer
		double C = bcx[ 0].C() + bcx[ 1].C();	// bcx[ 1].C() = 0 for adiabatic outside of bcxTNODE
		double dtOvrTau = nd[ 0].mn_DtOvrTau( C);
		if (bcx[ 0].bcxTy == bcxTNODE)
		{	// specified temp for node 0
			// set temp and derive fake exterior heat flow to preserve energy balance
			nd[ 0].mn_t = bcx[ 0].tFixed;
			double tss = (nd[ 0].mn_t - (1.-dtOvrTau)*tOld[ 0]) / dtOvrTau;
			bcx[ 1].qrNd = tss*C - bcx[ 0].CTQ();		// outside surface heat flow
		}
		else
		{	double tss = (bcx[ 0].CTQ() + bcx[ 1].CTQ())/C;
			nd[ 0].mn_t = (1. - dtOvrTau)*tOld[ 0] + dtOvrTau*tss;
		}
	}
	else
	{	// multiple layers
		// inside
		nd[ 0].mn_TStepSrf( bcx[ 0], tOld[ 0], nd[ 0].mn_ubn, tOld[ 1]);
		// interior (may be none)
		for (int iL=1; iL<nLx; iL++)
			nd[ iL].mn_TStep( tOld+iL);
		// outside
		nd[ nLx].mn_TStepSrf( bcx[ 1], tOld[ nLx], nd[ nLx-1].mn_ubn, tOld[ nLx-1]);
	}


	for (int i=0; i<2; i++)
	{	bcx[ i].tNd = nd[ i*nLx].mn_t;				// handy copy of temp of node adjacent to boundary
		mm_tSrf[ i] = bcx[ i].TSrf();
		mm_qSrf[ i] = bcx[ i].QSrf( tOld[ i*nLx]);
							// note: heat flow based on prior step temp
							//   (forward difference)
	}

	// energy balance
	// compare internal energy change to surface flux
	mm_qIEDelta = mm_InternalEnergyDelta( tOld);
	mm_qIE += mm_qIEDelta;		// must use delta scheme due to possibly changed node HCs
	double qDelta = Top.tp_subhrDur * (mm_qSrf[ 0] + mm_qSrf[ 1]);
	mm_qBal = mm_qIEDelta - qDelta;	// energy balance, Btu/ft2
									//  >0 = more energy appeared in mass than was added

#if defined( DEBUGDUMP)
	if (DbDo( dbdMASS))
		DbDump( bcx);
#endif

#if defined( _DEBUG)
	if (!Top.isWarmup)
	{	// check that internal energy change = surface flux
		double aDiff = fabs( mm_qBal);
		if (aDiff > .001 && aDiff > .001*(fabs( mm_qIEDelta)+fabs( qDelta)))
			warn( "Surf '%s', %s: forward difference energy balance error (qD=%.3f vs ieD=%.3f)",
				Name(), Top.When( C_IVLCH_S), qDelta, mm_qIEDelta);

		// check balance at surfaces
		//   conduction flux should equal all xfers to surface
		for (int i=0; i<2; i++)
		{	double qSrfNew = bcx[ i].QSrf( nd[ i*nLx].mn_t);
			double qX = bcx[ i].QX();
			aDiff = fabs( qX - qSrfNew);
			if (aDiff > .01 && aDiff > .001*(fabs( qX)+fabs( qSrfNew)))
				warn( "Mass '%s' (%d), %s: surface energy balance error (qX=%.3f vs qSrf=%.3f)",
					Name(), i, Top.When( C_IVLCH_S), qX, qSrfNew);
			if (bcx[ i].bcxTy != bcxTARQ)
				break;		// skip outsize of bcxTNODE (test does not work)
		}
	}
#endif

}               // MASSFD::mf_Step
//======================================================================-------
/*virtual*/ double MASSFD::mm_InternalEnergy() const	// current mass internal energy above 0 F
// returns current mass internal energy per unit area, Btu/ft2
{
	double q = 0.;
	int nL = mm_NNode();
	const MASSNODE* nd = &mf_nd.front();
	for (int iL=0; iL<nL; iL++)
		q += nd[ iL].mn_InternalEnergy();
	return q;
}	                   // MASSFD::mm_InternalEnergy
//-----------------------------------------------------------------------------
/*virtual*/ double MASSFD::mm_InternalEnergyDelta(	// current mass internal energy change
	double* tOld)	const	// prior node temps, F
// returns current mass internal energy per unit area, Btu/ft2
{
	double q = 0.;
	int nL = mm_NNode();
	const MASSNODE* nd = &mf_nd.front();
	for (int iL=0; iL<nL; iL++)
		q += (nd[ iL].mn_t - tOld[ iL]) * nd[ iL].mn_hcEff;
	return q;
}		// MASSFD::mm_InternalEnergyDelta
//-----------------------------------------------------------------------------
#if 0
/*virtual*/ double MASSFD::mm_BoundaryTemp(		// temperature at layer boundary
	int iB) const		// boundary idx
						//  idx iB indicates btw layer iB-1 and iB
						//  iB=0: inside surface temp
						//  iB=nL: outside surface temp
// returns temp at boundary, F
//         -99. if error (invalid iB)
{
	double tB = -99.;
	int nL = mm_NNode();
	if (iB == 0)
		tB = mm_tSrf[ 0];
	else if (iB == nL)
		tB = mm_tSrf[ 1];
	else if (iB < nL)
	{	const MASSNODE& ndL = mf_nd[ iB-1];
		const MASSNODE& ndR = mf_nd[ iB];
		tB = (ndL.mn_t*ndL.mn_uh + ndR.mn_t*ndR.mn_uh)
			/ (ndL.mn_uh + ndR.mn_uh);
	}
	return tB;
}		// MASSFD::mm_BoundaryTemp
#endif
//-----------------------------------------------------------------------------
void MASSFD::mm_FillBoundaryTemps(		// temperatures at actual layer boundaries
	float tLrB[],			// where to store
	int tLrBDim) const		// dim of tLrB[]
// fills tLrB[] with temps (F) at construction layer boundaries
//       tLrB[ 0] = inside surface temp
//       tLrB[ nConstructionLayers] = outside surface temp
{
	int nL = mm_NLayer();		// # of modeled layers
	tLrB[ 0] = mm_tSrf[ 0];		// boundary 0 = inside surface
	int iB = 0;
	for (int iL=0; iL<nL-1; iL++)
	{	const MASSLAYER& ml = mm_layers[ iL+1];
		if (ml.ml_iLSrc != iB)		// if next modelled layer belongs
									//   to different construction layer
		{
#if defined( _DEBUG)
			if (ml.ml_iLSrc < iB)		// source layers s/b in ascending order
										//   some may be skipped due to layer merging
				printf( "\nLayer order?");
#endif

			iB = ml.ml_iLSrc;
			if (iB >= tLrBDim)
				return;		// no more space
			const MASSNODE& ndL = mf_nd[ iL];
			const MASSNODE& ndR = mf_nd[ iL+1];
			tLrB[ iB] = (ndL.mn_t*ndL.mn_uh + ndR.mn_t*ndR.mn_uh)
						/ (ndL.mn_uh + ndR.mn_uh);
		}
	}
	if (iB < tLrBDim-1)
		tLrB[ iB+1] = mm_tSrf[ 1];

}		// MASSFD::mm_FillBoundaryTemps
//=============================================================================

// mspak.cpp end
