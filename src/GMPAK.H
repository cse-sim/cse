// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// abbreviated gmpak declarations for CSE -- gmutil.cpp.

#ifdef GMPAKH
    error duplicate gmpak.h include -- add ifdef GMPAKH
#endif
#define GMPAKH


/*-------------------------------- DEFINES --------------------------------*/
#if 0	// unused in CSE, if 0'd 9-92.
x/* Definitions for gmlnsgpl etc. indicating whether calcs are for
x   a LINE (infinite length), RAY (semi-infinite), or SEG (point to point). */
x#define SEG 0
x#define LINE 1
x#define RAY 2
x#define NEWENDCROSS 0x1000	 /* Enables new defintion of GMENDCROSS
x				    condition in gmlnsgpl() */
x
x/* line intersection cases -- positive case are not usually fatal */
x#define LNTCROSS 2	/* "T" intersection */
x#define LNCONSECT 1	/* consective line segments */
x#define LNNOCROSS 0	/* no intersection */
x#define LNCROSS -2	/* plain old fashion "X" */
x#define LNOVERLAP -3	/* co-linear and overlapping */
x#define LN0LEN -4	/* line has 0 length */
x#define LNSETOK   0	/* gmlnset() is ok */
x
x/* line - plane intersections.	Note that gmlnsgpl() code depends on
x   these values.  Do not change without looking at that routine */
x#define GMCOPLANAR -1	/* line and plane are coplanar */
x#define GMNOCROSS 0	/* line and plan do not cross */
x#define GMCROSS 1	/* plain old cross */
x#define GMENDCROSS 2	/* end point of segment lies on plane */
x
x
x/* plane-plane state */
x#define GMPARALLEL -2	/* planes are parallel */
x
x#define GM2D 2
x#define GM3D 3
x#define LENGTH2D GM2D
x#define LENGTH3D GM3D
x
x/* angle calculation cases */
x#define ANGUNDEF 0		/* One of the vectors is 0 */
x#define ANGOK 1 		/* Angle is perfectly determined */
x#define ANGPI 2 		/* Angle is Pi or -Pi */
#endif

/* Azimuth Definitions */
#define GMAZM_N  0
#define GMAZM_NE 1
#define GMAZM_E  2
#define GMAZM_SE 3
#define GMAZM_S  4
#define GMAZM_SW 5
#define GMAZM_W  6
#define GMAZM_NW 7

#if 0	// unused in CSE, if 0'd 9-92.
x/* Inside polygon test */
x#define OUTPOL 0
x#define INPOL 1
x#define ONLIN 2
x#define ONPT 3
x
x#define DOT3D(a1,a2) (a1[0]*a2[0]+a1[1]*a2[1]+a1[2]*a2[2])
x
x/* note POINT is defined in dtypes.h (included via cnglob.h).
x	POFF is also defined there.
x	a POFF is an offset into project's points array (points.c) */
#endif

/*------------------------- FUNCTION DECLARATIONS -------------------------*/

// gmutil.cpp: gmpak routines shared with CALRES (including BSGS CALRES)
void FC gmcos2alt( float *dcos, float *altp, float *azmp, SI tol);
SI FC gmazm4( float azm);
SI FC gmazm8( float azm);

// end of gmpak.h
