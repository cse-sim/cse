// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

/* gmutil.cpp -- geometric utility functions.  Extracted from gmpak to
	         allow linking CALRES without entire gmpak.obj  10-22-88

	         functions here: gmcos2alt, gmazm4, gmazm8 */

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>

#include <GMPAK.H>      // decls for this file

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
LOCAL SI FC NEAR gmazmx( float azm, SI pos);

//===========================================================================
void FC gmcos2alt( 		// get altitude and azimuth for direction cosines

	float *dcos,	// Array of cosines
	float *altp,        // receives altitude (+-Pi/2 for horiz plane, 0 for vertical; COMPLEMENT of TILT used elsewhere).
	float *azmp,	/* receives azimuth, in compass range -Pi to Pi.
			   Pi is returned if altitude is so near Pi/2 that azimuth is undefined. */
	SI tol )		/* 0 for old code 7-16-88; 1 for rob's rewrite with more
			   symettric behavior and much less 'snap' near azm==+-Pi/2 and alt==pi/2. */
/* tol being added 7-16-88 with 0 being added to existing calls not
   related to bug being addressed at the moment.  Sometime review all
   calls and use 1 in as many as possible; eliminate 0 case if poss. */
// 7-22 (-what year? maybe 87?) ********** all calls tentatively updated to tol 1

// MAY BE DUPLICATED INLINE e.g. in subrtpl in subint3.cpp so double precision result may be used.
{
	double val, azm;

	if (tol==0) 		// old code, with which most of program believed to run
	{
		*altp = (float)asin(dcos[2]);
		val =  sqrt(1.0 - dcos[2] * dcos[2]);
		if ( fAboutEqual( val, 0.0) )
		{
			*azmp = kPi;
		}
		else
		{
			val = -dcos[0]/val;
			if ( fAboutEqual(val,1.0) )       /* NO NO NO -- derivative is flat --
					   look at dcos[1] when dcos[0] near 1, or at least use smaller ABOUT0. */
				azm = kPiOver2;
			else if ( fAboutEqual(val,-1.0) )
				azm = -kPiOver2;
			else
				azm = asin(val);
			// asymettry! NO snap near 0 and PI after too much snap near +- pi/2.
			*azmp = (float)( (dcos[1] > 0.F) ? -azm : azm + kPi );
		}
	}
	else                // rob's rewrite 7-16-88
	{
		/* altitude.  > 1 and < -1 execptions insure against domain errors if input
			 is a snerd out of range (no case actually seen -- rob).
			 (do NOT use fAboutEqual: would 'snap' too far!) */

		*altp =  dcos[2] >= 1.F  ? kPiOver2
			   : dcos[2] <= -1.F ? -kPiOver2
			   :                   (float)asin( (double) dcos[2]);

		/* for more accuracy near vertical consider:
		 *  atan2( dcos[2], sqrt(dcos[0]^2+dcos[1]^2) );
		 *  and could then eliminate the +-1. exceptions. */

		/* azimuth: if both x and y direction cosines are too near 0, surface is
		   horizontal and azimuth is undefined (and atan2 will error). */

		/* this is a stricter test (Pi returned less often) than testing
		   dcos[z] for being nearly 1.0; probably about the same as testing
		   dcos[z]^2 for nearly 1.0. */

		if (fAboutEqual( dcos[0], 0.F) && fAboutEqual( dcos[1], 0.F))
		{
			*azmp = kPi;                   /* arbitrarily return Pi */
		}
		else
		{
			/* Need to 'snap' to exactly 90 degree multiples when near same?
			   I don't think so, but if so test dcos[x] and [y] for being near
			   0.0 -- tests of near +- 1.0 would snap from too far away (flat
			   derivative).  Note that calling code in/for tkadj depends on
			   accurate azimuth where z's will be determined from x and y on
			   nearly vertical plane for knee walls; snap would mess this up. */

			/* compute angle for dcos[x] and dcos[y] using 2-arg arctangent(y/x). This
			   fcn handles sign for 4 quadrants, does not require us to adjust for
			   dcos[z], and is believed to be uniformly accurate in all quadrants */

			*azmp = (float) atan2( (double)dcos[0], /*-*/(double)dcos[1] );
			/* arg interchange [and - sign] is to produce
			   compass values (clockwise from y axis). */
		}
	}
}               // gmcos2alt

//==========================================================================
SI FC gmazm4( 		/* Converts true azm to octant value for manual J calcs.
			   Octants are 0-3, mirrored on the north-south axis. */

	float azm )		// True azimuth in radians. 0 is north.

/* Returns the octant value from 0 to 3 : 0 is north 3 is south  */
{
	return gmazmx( azm, TRUE);
}				// gmazm4

//==========================================================================
SI FC gmazm8( 				// Converts true azm to octant value

	float azm )		// True azimuth in radians. 0 is north.

// Returns the octant value from 0 to 7 : 0 is north, 2 is east, 4 is south
{
	return gmazmx( azm, FALSE);
}				// gmazm8

//==========================================================================
LOCAL SI FC NEAR gmazmx( 		// Converts true azm to octant value

	float azm,	// True azimuth in radians. 0 is north
	SI pos )	// if pos, return value from 0 to 4 only

// Returns the octant value from 0 to 7 : 0 is north, 2 is east, 4 is south
{
	double ang = azm/k2Pi;
	ang = ang - (SI)ang;
	ang *= k2Pi;
	if (pos)
	{
		if (ang <= -kPi)       ang += k2Pi;
		else if (ang >= kPi)   ang -= k2Pi;
		ang = fabs(ang);
	}
	else if (ang < 0.)
		ang += k2Pi;

	if (ang < kPi/8.0)		return GMAZM_N;
	if (ang < kPi*(3.0/8.0) )	return GMAZM_NE;
	if (ang < kPi*(5.0/8.0) )	return GMAZM_E;
	if (ang < kPi*(7.0/8.0) )	return GMAZM_SE;
	if (ang < kPi*(9.0/8.0) )	return GMAZM_S;
	if (ang < kPi*(11.0/8.0) )	return GMAZM_SW;
	if (ang < kPi*(13.0/8.0) )	return GMAZM_W;
	if (ang < kPi*(15.0/8.0) )	return GMAZM_NW;
	return GMAZM_N;
}			// gmazmx
//=============================================================================


// end of gmutil.cpp
