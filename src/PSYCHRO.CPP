// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

//==========================================================================
// psychro.cpp -- psychrometric functions
//==========================================================================

// ------------------------ GENERAL COMMENTS -------------------------------

#if 0	/* comments */
c   These functions were adapted from TARP code 5/86 by Chip.
c   All TARP methods were used except for saturation temp/pressure
c   conversion functions where a new faster linear interpolation scheme
c   was added.  Also, barometric pressure is globally defined rather
c   than passed as an argument.
c
c   All functions perform various transformations between the following:
c
c	ta: Air dry bulb temperature (F)
c	td: Air dew point temperature (F)
c	tw:	Air wet bulb temperature (F)
c	ts: Air saturation temperature (F)
c	w:	Humidity ratio (dimless; lb water vapor per lb dry air)
c	h:	Enthalpy (Btu/lb)
c	hs:	Saturation enthalpy (Btu/lb)
c	rh:	Relative humidity (0 - 1)
c	ps:	Saturation pressure (in. Hg)
c	v:	Specific volume (cf/lb)
c	PsyPBar:	Barometric pressure (in. Hg).  This is a public
c			variable (set in psyElevation) and is not passed
c			as an argument.
c
c   According to Walton, these functions are accurate in the range
c   -75 F < ta < 200 F.  *BUT* remember that the dew point of cold, dry
c   air is *much* lower than the dry bulb; the dew point must remain
c   above -100 F due to sat. press. limits in psyPsat().
c
c   The following functions are defined:
c
c	bp  = psyElevation( elev)	barometric pressure as func of elevation; sets PsyPBar
c	w   = psyHumRat1( ta, tw) 	hum rat from dry bulb and wet bulb
c	w   = psyHumRat2( ta, rh)  	hum rat from dry bulb and rel hum
c	ps  = psyPsat( ta) 			saturation pressure from temp
c	td  = psyTDewPoint( w)   	dew point from hum rat (temp at which given hum rat is saturation)
c	h   = psyEnthalpy( ta, w)	enthalpy from dry bulb and hum rat
c	h   = psyEnthalpy2( ta, mu, &w)	enthalpy from dry bulb and degree of saturation
c	hs  = psySatEnthalpy( ta) 	enthalpy of saturated air at temp
c	s   = psySatEnthSlope( ta)	dh/dt for saturated air at ts
c	ts  = psyTWSat( hs, mu, &w)	dry bulb and hum rat from enthalpy and degree of saturation
c	rh  = psyRelHum( ta, td)  	rel hum from dry bulb and dew point (temp at which air wd saturate)
c	rh  = psyRelHum2( ta, tw)	rel hum from dry bulb and wet bulb
c	tw  = psyTWetBulb( ta, w) 	wet bulb from dry bulb and hum rat
c	ta  = psyTDryBulb( w, h) 	dry bulb from hum rat and enthalpy
c	v   = psySpVol( ta, w)    	specific volume from dry bulb and hum rat
c	w   = psyHumRat3( td)		hum rat from dew point (saturated w for t)
c	w   = psyHumRat4( ta, h)  	hum rat from dry bulb and enthalpy
c	ts  = psyTsat( ps)			saturation temp from pressure
c
c	also "detailed functions", if'd out.
c
#endif	// end of comments


// ------------------------------- INCLUDES -------------------------------

#include "CNGLOB.H"
#include "RMKERR.H"
#include "PSYCHRO.H"	// decls for this file

#if defined( _DEBUG)
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//-------------------------------- OPTIONS --------------------------------
// TEST_xxx ... define on command line to include main() for stand-alone
//  testing
//     TEST_TSAT	tests tsat / psat
//     TEST_PSY		tests many fcns for self-consistency
#if defined( TEST_TSAT) || defined( TEST_PSY)
#define TEST
#endif

#undef PSYDETAIL	// if defined, detailed versions of some functions
					// are included.  These were used to develop the
					// faster and less accurate versions that are
					// generally used.

#if defined( TEST)
#define PSYDETAIL	// Need detailed functions for testing
#endif
//===========================================================================

////////////////////////////////////////////////////////////////////////////
// Constants and publics (see also psychro.h)
////////////////////////////////////////////////////////////////////////////
float PsyPBar       = PsyPBarSeaLvl;	// Barometric pressure (in. Hg). Initially std sea level pressure.
// Set by psyElevation().  Many uses.
// const float PsyWmin    = 0.00000001f;	// Minimum humidity ratio handled or returned.
// Used many places to prevent divide by 0.
// NOTE also psyHumRatMinTdp() = w at tdp = -100
//=========================================================================

////////////////////////////////////////////////////////////////////////////
// Saturation pressure table
////////////////////////////////////////////////////////////////////////////
// The following table was generated with psattbl() and then edited into this location.
//    Data is psat (in. Hg) for t = -100, -96, -92, ... 300 (F) BUT values are slightly adjusted
//    (downward) to minimize overall error during linear interpolation (see psattbl()).
//    4 F temp interval is reasonably accurate and handy for deriving interpolation subscripts using >>2 shift.
//    ALSO, 32 F falls on a point in table, where the curve kinks.
const int PSYPSTABSZ = (PSYCHROMAXTX-PSYCHROMINT)/4+1;	// includes both limits

static float psyPsTab[ PSYPSTABSZ] =
{  0.000046322f,   0.000064969f,   0.000090460f,   0.000125064f,   0.000171726f,
0.000234238f,   0.000317456f,   0.000427564f,   0.000572389f,   0.000761785f,
0.001008090f,   0.001326673f,   0.001736584f,   0.002261319f,   0.002929718f,
0.003777022f,   0.004846101f,   0.006188875f,   0.007867966f,   0.009958589f,
0.012550727f,   0.015751619f,   0.019688573f,   0.024512187f,   0.030399958f,
0.037560370f,   0.046237476f,   0.056716014f,   0.069327146f,   0.084454782f,
0.102542691f,   0.124102235f,   0.149721026f,   0.180553958f,   0.211458594f,
0.247444391f,   0.288742691f,   0.336012512f,   0.389977962f,   0.451432765f,
0.521244824f,   0.600361288f,   0.689813197f,   0.790720761f,   0.904298544f,
1.031860709f,   1.174826264f,   1.334724545f,   1.513200998f,   1.712022424f,
1.933082461f,   2.178407907f,   2.450163126f,   2.750656843f,   3.082347631f,
3.447848558f,   3.849933386f,   4.291542530f,   4.775787354f,   5.305956841f,
5.885521889f,   6.518140316f,   7.207663059f,   7.958137512f,   8.773814201f,
9.659149170f,  10.618811607f,  11.657685280f,  12.780872345f,  13.993701935f,
15.301728249f,  16.710739136f,  18.226755142f,  19.856035233f,  21.605083466f,
23.480642319f,  25.489704132f,  27.639513016f,  29.937561035f,  32.391593933f,
35.009620667f,  37.799900055f,  40.770954132f,  43.931564331f,  47.290771484f,
50.857887268f,  54.642482758f,  58.654388428f,  62.903713226f,  67.400817871f,
72.156341553f,  77.181182861f,  82.486511230f,  88.083747864f,  93.984596252f,
100.201019287f, 106.745246887f, 113.629776001f, 120.867355347f, 128.471008301f,
136.454025269f
};
//=========================================================================

////////////////////////////////////////////////////////////////////////////
// Test code
////////////////////////////////////////////////////////////////////////////
#ifdef TEST
// local functions involved in testing
static void psyTest(	// Test psychro functions

	float ta, 	// Dry bulb of test point (F)
	float rh )	// Rel hum of test point (0 - 1)
{
	float td, tw, ts, ps, v, w, w1, w2, w3, h, rh1, ta2, hs, ts2, hss, w4;
	printf( "\nta = %-f   rh = %-f   PsyPBar = %-f\n", ta, rh, PsyPBar);
	w = psyHumRat2( ta, rh);
	h = psyEnthalpy( ta, w);
	td = psyTDewPoint( w);
	tw = psyTWetBulb( ta, w);
	ta2 = psyTDryBulb( w, h);
	printf( "w=%-7f    h=%-7f  td=%10.5f  tw=%10.5f  ta2=%10.5f\n",
	w, h, td, tw, ta2);

	v  = psySpVol( ta, w);
	rh1 = psyRelHum( ta, td);
	w1  = psyHumRat3( td);		// all w's should be equal
	w2  = psyHumRat1( ta, tw);
	w3  = psyHumRat4( ta, h);
	printf( "v=%-7f  rh1=%-7f  w1=%10.6f  w2=%10.6f   w3=%10.6f\n",
		v, rh1, w1, w2, w3);
	printf( "                              %10.8f    %10.8f      %10.8f\n",
		(w1-w)/w, (w2-w)/w, (w3-w)/w);	// error fractions

	ps = psyPsat( td);
	ts = psyTsat( ps);			// should be same as td
	hs = psySatEnthalpy( td);
	hss = psySatEnthSlope( td);
	ts2 = psyTWSat( hs, 1.f, &w4);	// should be same as td
	printf(
		"ps=%10.6f  ts=%10.5f  hs=%10.5f  slope=%10.5f  ts2=%10.5f  w=%7.6f\n",
		ps, ts, hs, hss, ts2, w4);
}						// psyTest
// =====================================================================
static float getval(	// Query user, get float value from keybd

	char *s )	// Prompt string

// returns value obtained from user
{
	char t[50];

	printf( "%s", s);
	gets(t);
	return atof(t);

}		// getval
// =====================================================================
main ()
{
	float ta, rh, ps, ts, tw, w, wsat, tcoil;
	int i;

// Initialize: sets PsyPBar
	psyElevation( getval("Elev: "));

#if defined( TEST_TSAT)
	for (ps = 0.00005f; ps < .03f; ps += .0005f )
		ts = psyTsat(ps);	// step through psat range;
	// psyTsat printfs will indicate bad guesses.
	printf("\n");
	while ((ts = getval("ts: ")) > -999.f)
	{
		ps = psyPsat(ts);
		printf( "ps=%f   exact=%f   inverse=%f\n",
				ps, psyPsatX(ts), psyTsat(ps));
	}
#endif	// TEST_TSAT

#if defined( TEST_PSY)		// use psyTest to perform a group of tests
	while (1)
	{
		ta = getval("Air temp: ");
		if (ta == -999.f)		// -999 says quit
			break;
		if (ta == -998.f)	// -998 says do general checkout
			// -95<=ta<=195 and .05<=rh<=.95
		{
			for ( int iTa=-95; iTa < 200; iTa+=5)
				for (int iRh=5; iRh < 100; iRh+=45)
					psyTest( (float)iTa, iRh/100.f );
		}
		else
		{
			rh = getval("RH: ");
			if (rh > 1.f)
				rh = rh/100.f;
			psyTest( ta, rh);
		}
	}
#endif	// TEST_PSY

	return 0;			// Keep compiler happy
}		// main
// =========================================================================
#endif	// TEST


////////////////////////////////////////////////////////////////////////////
// Public functions
////////////////////////////////////////////////////////////////////////////
float psyElevation(		// compute/set standard barometric pressure
	float elev)	// elevation (feet above sea level)
//   valid range = -16500 - +36000 ft

// See ASHRAE Psychrometrics -- Theory and Practice (1996), p. 9

// Returns standard barometric pressure (in. Hg) for elev
// AND sets global psyPBar, which effects many psychro functions.

// Note: In multi-elevation situations (which have never arisen to date),
//      caller must keep track of elevations and make psyElevation
//		calls as required before other psychro calls.

{
#if 1	// revised per 1996 ASHRAE Psychrometrics, 12-13-06
	return PsyPBar = float( PsyPBarSeaLvl * pow( 1.-6.87535e-6*elev, 5.2559));
#else
	x	return PsyPBar = float( PsyPBarSeaLvl * pow( 1.-6.87535e-6*elev, 5.2561));
#endif
}		// psyElevation
// ==========================================================================
float psyElevForP(			// elevation from barometric pressure
	float bp)		// barometric pressure, in Hg
{
// inverse of psyElevation() (above)
	return float( (1. - pow( double( bp/PsyPBarSeaLvl), 1./5.2559)) / 6.87535e-6);
}		// psuElevForP
// ==========================================================================
float calc_outside_rh(					// rh from db and wb
	float ta,		// dry bulb (F)
	float tw)		// wet bulb (F)
{
	ta = max( 20.f, min( ta, 130.f)); 		// limit db 20 - 130
	tw = max( 20.f, min( tw, ta));	  		// limit tw 20 - ta

	float w = psyHumRat1( ta, tw); 	// hum rat from dry bulb and wet bulb
	if (w < 0.00001)  // too small
		return 5.f;
	float td = psyTDewPoint( w);	// dew point
	float rh = psyRelHum( ta, td);  // rel hum from dry bulb

	return max( 5.f, min( rh*100.f, 100.f));
}		// calc_outside_rh
// ==========================================================================
float calc_outside_wb(				// WB from db and rh
	float ta,		// dry bulb temp (F)
	float rh)		// relative humidity (0 - 100 %)
{
	ta = max( 20.f, min( ta, 130.f));			// limit db 20 - 130
	rh = max( 5.f, min( rh, 100.f)) / 100.f;	// rh .05 - 1.

	float w   = psyHumRat2( ta, rh);
	float tw  = psyTWetBulb( ta, w);  // wet bulb

	return max( 20.f, min( tw, ta));
}		// calc_outside_wb
// ==========================================================================
float psyHumRat1(  // Humidity ratio from dry bulb and wet bulb
	float ta,	// Air dry bulb temp (F)
	float tw)	// Air wet bulb temp (F)

// returns humidity ratio W (lb/lb)
{
	float pw = psyPsat( tw);
	float ws = PsyMwRatio*pw / (PsyPBar-pw);

	float w = ((PsyHCondWtr+32.f-tw*(1.f-PsyShWtrVpr))*ws - PsyShDryAir*(ta-tw))
			  /  (PsyHCondWtr+32.f + PsyShWtrVpr*ta - tw);

	return w < PsyWmin ? PsyWmin : w;
}					// psyHumRat1
//---------------------------------------------------------------------------
float psyHumRatX(		// humidity ratio from tdb and twb (fix twb)
	float ta,		// dry bulb temp (F)
	float& tw)	// wet bulb temp (F) (returned corrected twb(w=0) < twb <= ta)
// returns humidity ratio corresponding to ta and tw (as corrected)
{
	if (tw > ta)
		tw = ta;
	float w = psyHumRat1( ta, tw);
	float wMin = 2.f * psyHumRatMinTdp();	// 2 * (w at tdp = -100F)
	//   2x margin = working room insurance
	if (w < wMin)
	{
		w = wMin;					// large enuf so psyTDewPoint works OK
		tw = psyTWetBulb( ta, w);	// recalc twb at approx 0 moisture
	}
	return w;
}		// psyHumRatX
// ==========================================================================
float psyHumRat2(	// Humidity ratio from dry bulb and rel humidity.
	float ta,		// Air dry bulb temp (F)
	float rh )	// Relative humidity (0 - 1)

// Returns humidity ratio W (lb/lb)
{
	float ptd = rh * psyPsat( ta);
	float w = PsyMwRatio*ptd / (PsyPBar-ptd);
	return w < PsyWmin ? PsyWmin : w;
}					// psyHumRat2
// =========================================================================
float psyPsat(	// Compute saturation pressure given temperature
	float t )	// temp. for which sat. pressure desired (F)

// Returns saturation vapor pressure (in Hg)
{
	float t100 = t + 100.f;
	int itb = (int)t100 & 0xFFFC;	// find 4 deg temp bin by truncation.  Ignore possible overflow if t > 32767
	int ib = itb >> 2;				// table subscript -- one entry per 4 deg
	if (ib < 0 || ib >= PSYPSTABSZ-1)			// if outside table range
	{
#if defined( _DEBUG)
		mbIErr( "psyPsat", "temperature %0.f out of range", t);
#endif
		ib = ib < 0 ? 0 : PSYPSTABSZ-1;
		return psyPsTab[ ib];		// ... and return approp. table extreme
	}
	return psyPsTab[ib] + (psyPsTab[ib+1]-psyPsTab[ib])*(t100-itb)*0.25F;	// Interpolate btw entries ib and ib+1

}		// psyPsat
// ==========================================================================
float psyTDewPoint(	// Compute dew point temperature from humidity ratio
	float w )	// Humidity ratio (lb H2O/lb dry air)

// Returns dew point temp (F)
{
	return psyTsat( PsyPBar*w/(PsyMwRatio+w) );
}		// psyTDewPoint
// ==========================================================================
float psyHumRatMinTdp()		// smallest w consistent with psyTsat
// returns w at tdb = -100 (lowest temp in psyTsTab)
{
	return psyPsTab[ 0] * PsyMwRatio / (PsyPBar - psyPsTab[ 0]);
}		// psyHumRatMinTdp
// ==========================================================================
float psyEnthalpy(	// Enthalpy from dry bulb temp and humidity ratio
	float ta, 		// Air dry bulb temp (F)
	float w)		// Humidity ratio (lb/lb)

// Returns: Enthalpy (Btu/lb)
{
	return PsyShDryAir*ta + w*(PsyHCondWtr+PsyShWtrVpr*ta);

}		// psyEnthalpy
// ==========================================================================
float psyEnthalpy2(	// Enthalpy from dry bulb and degree of saturation

	float t,	// dry bulb, F
	float mu,	// degree of saturation (0 - 1)
	float& w)	// returned: humidity ratio

// returns enthalpy (Btu/lb dry air) and humidity ratio
{
// h = t * cpDryAir + w * (qcond + t * cpWaterVapor)
//                    ^-- w = (degree of saturation) * (wSaturated at t)
	return
		PsyShDryAir * t
		+ (w = mu * psyHumRat3(t)) * (PsyHCondWtr+PsyShWtrVpr*t);

}		// psyEnthalpy2
// ==========================================================================
float psyEnthalpy3(	// Enthalpy from dry bulb and relative humidity

	float t,	// dry bulb, F
	float rh,	// relative humidity (0 - 1)
	float& w)	// returned: humidity ratio

// returns enthalpy (Btu/lb dry air) and humidity ratio
{
// h = t * cpDryAir + w * (qcond + t * cpWaterVapor)
//                    ^-- w = (degree of saturation) * (wSaturated at t)
	return
		PsyShDryAir * t
		+ (w = psyHumRat2(t, rh)) * (PsyHCondWtr+PsyShWtrVpr*t);

}		// psyEnthalpy3
// ==========================================================================
float psySatEnthalpy(	// Enthalpy for saturated air at ts

	float ts )	// Saturated air dry bulb temp (F)
{
// h = t * cpDryAir + w * (qcond + t * cpWaterVapor)
//                    ^-- w = (wSaturated at ts)

	return PsyShDryAir*ts + psyHumRat3(ts) * (PsyHCondWtr+PsyShWtrVpr*ts);

}		// psySatEnthalpy
// ==========================================================================
float psySatEnthSlope(	// dh/dt for saturated air at ts

	float ts )	// Saturated air dry bulb temp (F)
{
// simple/short implementation: approximate derivative with local differential.
// This is FAR FROM fastest approach.  Chip has notes on other methods.

	return 10.f * ( psySatEnthalpy( ts+0.05f) - psySatEnthalpy( ts-0.05f) );

}		// psySatEnthSlope
// ==========================================================================
float psyTWEnthMu(	// dry bulb and humidity ratio from enthalpy and degree of saturation

	const float h,	// enthalpy of air, Btu/lb
	const float mu,	// degree of saturation (0 - 1)
	float& w)	// returned: humidity ratio

// returns temp and humidity ratio of air
{
	int i;
	float w2;
	// improvement: use regression for t1, set t2 = t2+5.
	// This would speed convergence, especially in high h cases
	float t1 = 50.f;			// guess two plausible solutions
	float t2 = 70.f;
	float h1 = psyEnthalpy2( t1, mu, w);
	float h2 = psyEnthalpy2( t2, mu, w2);
	if (fabs(h-h1) > fabs(h-h2))	// make point 1 the closer
	{	tswap( t1, t2);
		tswap( h1, h2);
		w = w2;
	}

	float eps = (float)fabs( 0.0002f*h);	// convergence tolerance
	for (i=0; ++i < 20; )		// iterate to refine solution
	{
		//  generally <5 iterations reqd for room temp h values
		float dt;
		if (fabs(h-h1) < eps)			// if current best is within 1/5000 of target
			break;						//   we're done
		dt = (t2-t1)*(h1-h)/(h1-h2);	// secant method: new guess assuming local linearity.
										// 1/0 not possible due to convergence test just above
		t2 = t1;						// replace older point
		h2 = h1;
		t1 += dt;						// and update current best
		if (t1 > 199.99f)
			t1 = 199.99f;				// keep within valid range of these fcns
		// it is believed that method will not produce t1 < -99.99: no low limit test.
		h1 = psyEnthalpy2( t1, mu, w);	// enthalpy at current best
	}
#if defined( _DEBUG)
	if (i == 20)			// if loop exit due to excess iterations
		mbIErr( "psyTWEnthMu", "convergence failure!");
#endif
	return t1;
}		// psyEnthMu
//=============================================================================
float psyTWEnthRh(	// dry bulb and humidity ratio from enthalpy and relative humidity

	const float h,	// enthalpy of air, Btu/lb
	const float rh,	// relative humidity (0 - 1)
	float& w)		// returned: humidity ratio

// returns temp and humidity ratio of air
{
	int i;
	float w2;
	// improvement: use regression for t1, set t2 = t2+5.
	// This would speed convergence, especially in high h cases. */
	float t1 = 50.f;			// guess two plausible solutions
	float t2 = 70.f;
	float h1 = psyEnthalpy3( t1, rh, w);
	float h2 = psyEnthalpy3( t2, rh, w2);
	if (fabs(h-h1) > fabs(h-h2))	// make point 1 the closer
	{	tswap( t1, t2);
		tswap( h1, h2);
		w = w2;
	}

	float eps = fabs( 0.0001f*h);	// convergence tolerance
	for (i=0; ++i < 20; )			// iterate to refine solution
	{
		//  generally <5 iterations reqd for room temp h values
		float dt;
		if (fabs(h-h1) < eps)			// if current best is within 1/5000 of target
			break;						//   we're done
		dt = (t2-t1)*(h1-h)/(h1-h2);	// secant method: new guess assuming local linearity.
										// 1/0 not possible due to convergence test just above
		t2 = t1;						// replace older point
		h2 = h1;
		t1 += dt;						// and update current best
		if (t1 > 199.99f)
			t1 = 199.99f;				// keep within valid range of these fcns
		// it is believed that method will not produce t1 < -99.99: no low limit test.
		h1 = psyEnthalpy3( t1, rh, w);	// enthalpy at current best
	}
#if defined( _DEBUG)
	if (i == 20)			// if loop exit due to excess iterations
		mbIErr( "psyEnthRh", "convergence failure!");
	else
	{	float hx = psyEnthalpy( t1, w);
		if (frDiff( h, hx) > eps)
			mbIErr( "psyEnthRh", "enthalpy mismatch");
	}
#endif
	return t1;
}		// psyEnthRh
// ==========================================================================
float psyRelHum(	// Relative humidity from dry bulb and dew point
	float ta, 	// dry bulb temperature, F
	float td )	// dew point temperature, F

// Returns: RH as a fraction (0 to 1)
{
	return psyPsat(td)/psyPsat(ta);
}					// psyRelHum
// ==========================================================================
float psyRelHum2(	// Relative humidity from dry bulb and wet bulb
	float ta, 	// dry bulb temperature, F
	float tw )	// wet bulb temperature, F
// Returns: RH as a fraction (0 to 1)
{
	float w = psyHumRatX( ta, tw);	// Note: alters tw within physical limits
	return psyRelHum( ta, psyTDewPoint( w));
}					// psyRelHum2
//---------------------------------------------------------------------------
float psyRelHum3(	// relative humidity from w and wSat
	float w,		// humidty ratio
	float wSat)		// saturation humidity ratio
// w is constrained herein to 0 - wSat (no msg)
// wSat not checked
// returns RH (0 - 1)
{
	w = bracket(0.f, w, wSat);
	double wws = w*wSat/PsyMwRatio;
	return float( wws > 0. ? (w + wws) / (wSat + wws) : 0.);
}		// psyRelHum3
//---------------------------------------------------------------------------
float psyRelHum4(	// Relative humidity from dry bulb and wet bulb
	float ta, 	// dry bulb temperature, F
	float w )	// humidity ratio
// Note psyRelHum3 limits w to valid range
// Returns: RH as a fraction (0 to 1)
{
	float wSat = psyHumRat3( ta);
	return psyRelHum3( w, wSat);
}					// psyRelHum2
// ==========================================================================
float psyTWetBulb(	// Wet bulb temp from dry bulb and humidity ratio
	float ta, 	// Air dry bulb temp (F)
	float w )	// Humidity ratio (lb/lb)

// Returns: Wet bulb temp (F)
{
	int i;

// Use secant method to find thermodynamic wet bulb temp.
// tw1/w1 and tw2/w2 are wetbulb/humidity ratio successive guesses

	if (w < PsyWmin)
		w = PsyWmin;				// prevent 0 w

	float tw1 = ta;					// highest possible wb is db
	float w1 = psyHumRat3( tw1);	// wSat at db
	if (w >= w1)					// if caller's w greater than wSat
		return ta;					//    saturated (no need to look for wb)

	float w2;
	float tw2 = (ta - 5.f);							// guess another wb below db
	while ((w2 = psyHumRat1( ta, tw2)) <= PsyWmin)	// get w
		tw2 = (ta+tw2) / 2.f;						// move guess toward db until w is OK
	float twn = tw2;								// save guess in case of return on 0th iteration

	double wDiff;
	for (i = 0; ++i < 20;)			// iteration limit in case of unanticipated convergence problems
	{
#if 1	// additional convergence check, 5-11-2012
		wDiff = fabs( w2-w1);
		if (wDiff < 2.e-9 || wDiff/w < .001)
#else
x		if (fabs( (w2-w1)/w) < 0.001)		// if guesses very close
#endif
			return twn;						//  call it done
		twn = tw2-(tw2-tw1)*(w2-w)/(w2-w1);	// new guess.  convergence test prevents 0 divide
		tw1 = tw2;							// move guess 2 to guess 1
		w1  = w2;
		tw2 = twn;							// and new guess to guess 2
		w2  = psyHumRat1( ta, tw2);			// humidity ratio for new
	}
#if defined( _DEBUG)
	mbIErr( "psyTWetBulb", "convergence failure!");
#endif
	return twn;
}			// psyTWetBulb
// ==========================================================================
float psyTDryBulb(	// Dry bulb temp from humidity ratio and enthalpy

	float w, 	// Humidity ratio (lb/lb)
	float h )	// Enthalpy (Btu/lb)

// Returns dry bulb temperature (F)
{
	return ( (h-PsyHCondWtr*w)/(PsyShDryAir+PsyShWtrVpr*w) );
}			// psyTDryBulb
// ==========================================================================
int psyLimitRh( float rhMax, float& ta, float& w)
// adjust ta and w to yield specified rh
{
	float h = psyEnthalpy( ta, w);
	if (rhMax < .0001)
	{	w = PsyWmin;
		ta = psyTDryBulb( w, h);
		return 1;
	}

	float rh = psyRelHum4( ta, w);
	if (rh <= rhMax)
		return 0;
	ta = psyTWEnthRh( h, rhMax, w);
	return 1;
}		// psyLimitRh
// ==========================================================================
float psySpVol(	// Specific volume from dry bulb temp and humidity ratio
	float ta, 	// Air dry bulb temp (F)
	float w )	// Humidity ratio (lb/lb)

// Returns specific volume (cf/lb dry air) of air
{
	return PsyRAir*(1.f+w/PsyMwRatio)*(ta+PsyTAbs0)/PsyPBar;
}		// psySpVol
// ==========================================================================
#if 0	// density rework, 10-18-2012
x double psyDensity(		// air density
x	float ta,			// air dry bulb temp (F)
x	float w,			// humidity ratio (lb/lb)
x	double pDelta /*=0.*/)	// differential pressure (in Hg)
x							//   (added to PsyPBar)
x// returns density (lb/cf) (note: not lb dry air)
x{
x	return (1.+w)*(PsyPBar+pDelta)/(PsyRAir*(1.+w/PsyMwRatio)*(ta+PsyTAbs0));
x
x}		// psyDensity
#endif
// ==========================================================================
float psyHumRat3(	// Humidity ratio from dew point temp  (saturated w for t)

	float td )	// Dew point temperature (F)

// Returns humidity ratio (lb/lb)
{
	float ptd = (float)psyPsat(td);
	float w = ptd* PsyMwRatio/(PsyPBar-ptd);
	return w < PsyWmin ? PsyWmin : w;
}					// psyHumRat3
// ==========================================================================
float psyHumRat4(	// Humidity ratio from dry bulb and enthalpy

	float ta, 	// Air dry bulb temperature (F)
	float h )	// Enthalpy (Btu/lb)

// Returns humidity ratio (lb/lb)
{
	float w = (h-ta*PsyShDryAir)/(PsyHCondWtr + PsyShWtrVpr*ta);
	return w < PsyWmin ? PsyWmin : w;
}					// psyHumRat4
// =========================================================================
float psyTsat(	// Saturation temperature for given pressure

	float ps )	// Pressure for which temperature is required (in. Hg)
{
	int i, ips;
#ifdef TEST
	float ts;
	int iguess, xsearch;
#endif

// Form initial guess about what psyPsTab[] bin goes with ps.
//  The guesses are designed to be exact in almost all cases in the 32 - 128 F range.
//  outside that range, they are worse */
	ips = (int)(ps*100.);
	if (ips > 18)		// Above freezing ?
	{
		if (ips <=  60)
			i = (7*ips+1255)/40;		// 32 - 64
		else if (ips <= 171)
			i = (8*ips+4180)/111;		// 64 - 96
		else if (ips <= 429)
			i = (8*ips+11500)/258;	// 96 - 128
		else if (ps < psyPsTab[75])
		{
			i = (ips+3400)/66;		// 128 - 200
			if (i > 74)
				i = 74;
		}
		else
		{
#if defined( _DEBUG)
			mbIErr( "psyTsat", "pressure %e too large", ps);
#endif
			ps = psyPsTab[ 75];
			i = 74;
		}
	}
	else
	{
		if (ips >= 3)
			i = (9*ips + 365)/15;		// -4   - 32
		else if (ps > psyPsTab[0])
			i = (int)(ps*350.) + 15;	// -100 - -4
		else
		{
#if defined( _DEBUG)
			mbIErr( "psyTsat", "pressure %e too small", ps);
#endif
			ps = psyPsTab[ 0];
			i = 0;
		}
	}
#ifdef TEST
	iguess = i;		// remember guess for testing
#endif

// Now check the guess and, if nec., search in the correct direction to find right bin
	if (ps >= psyPsTab[i])
		while (ps >= psyPsTab[i+1])
			i++;
	else
		while (ps < psyPsTab[--i])
			;

// Interpolate to get the saturation temp
#ifdef TEST
	ts = 	// If TESTing, store value for printf below
#else
	return	// otherwise, we're done
#endif
		( (float)((i << 2) - 100)				// table [0] is for -100 F
		  + 4.f*(ps-psyPsTab[i])/(psyPsTab[i+1]-psyPsTab[i]) );

#ifdef TEST
	if (i > iguess)
		xsearch = i-iguess;
	else if (i < iguess-1)
		xsearch = i - iguess + 1;
	else
		xsearch = 0;
	if (xsearch != 0)
		printf( "ps = %f   ts = %f   xs = %d\n", ps, ts, xsearch);

	return ts;
#endif
}			// psyTsat

// --------------------------- DETAILED FUNCTIONS ---------------------------
#ifdef PSYDETAIL
static double dvcmean( double *dp, int n );
// ==========================================================================
void psattbl()

// Build a table of saturation pressure values for linear interpolation version of psyPsat(t)

{
	int it, i, ix, itb, ib, itf;
	double t, psexact;
	double tf, psint, psitot, psetot, pel, per;
	double ps[620];
	float psx[76];
	FILE *f;

	for (it = -102; it <= 202; it++)
	{
		for (itf = 0; itf < 2; itf++)
		{
			t = (double)it + itf*0.5;
			i = 2*(it+102)+itf;
			ps[i] = psyPsatX(t);
		}
	}
	printf("Generating table...\n");
	for (it = -100; it <= 200; it+=4)
	{
		i = 2*(it+102);
		ix = (it+100)/4;
		psx[ix] = (float)(ps[i] - (ps[i-4]+ps[i+4])/2. + dvcmean(ps+i-4, 9));
	}
	printf("Writing table...\n");
	f = fopen( "psyPsat.tab", "wt");
	for (ix = 0; ix < 76; ix++)
	{
		if (ix%5 == 0)
			fprintf(f, "\n");
		fprintf(f, " %12.9f, ", psx[ix]);
	}
	fprintf(f, "\n");
	fclose(f);
}		// psattbl
// ==========================================================================
double psyTsatX(	// Compute saturation temperature from pressure by inverting the detailed fit method.
	// Accurate but *very* slow

	double ps )	// Sat. pressure for which temp is desired (in. Hg)

// Returns temperature for which ps is saturation pressure (F).
// See also psyTsat
{
	double t1, t2, t3, p1, p2;

	t1 = 0.0;
	p1 = 0.037645;
	t2 = 120.00;
	p2 = 3.4474;
	while (fabs((p2-p1)/ps) > 0.0001)
	{
		t3 = t2 - (p2-ps)*(t2-t1)/(p2-p1);
		t1 = t2;
		p1 = p2;
		t2 = t3;
		p2 = psyPsatX(t2);
	}
	return t2;
}			// psyTsatX
// ==========================================================================
double psyPsatX(	// Saturation pressure using detailed fit method

	double tc )	// Temperature for which sat pressure is desired (F)

// Returns sat. pressure (in. Hg.) at temperature tc

/* This code was adapted from TARP.
   The method is found in ASHRAE 1985 (and earlier) HOF, but there are 2 misprints in ASHRAE.
   As usual, George Walton is right ... this version has been fairly carefully tested */
{
	double plog, t, t2, t3, t4;

	t = (tc+PsyTAbs0)*5./9.;		// Abs temp, Deg K
	t4 = t*(t3=t*(t2=t*t));
	if (tc < 32.001)
		plog =  -.56745359e4/t + .63925247e1 - 0.9677843e-2*t
				+ 0.62215701e-6*t2 + 0.20747825e-8*t3
				- 0.9484024e-12*t4 + 0.41635019e1*log(t);
	else
		plog =  -.58002206e4/t + .13914993e1 - .48640239e-1*t
				+ 0.41764768e-4*t2 - 0.14452093e-7*t3 + .65459673e1*log(t);

	return exp(plog)/3386.389;

}			// psyPsatX
// ==========================================================================
static double dvcmean( 		// Compute the mean of a vector of double values

	double *dp,  	// Pointer to initial value in vector
	int n )		// Number of values to use to form mean
{
	double t = 0.;
	for (int i = -1; ++i < n; )
		t += *(dp+i);
	return t/(double)n;
}				// dvcmean
#endif	// PSYDETAIL


// end of psychro.cpp
