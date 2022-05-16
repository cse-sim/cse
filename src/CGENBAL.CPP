// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cgenbal.cpp -- energy balance check for CSE


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// includes dtypes.h: MASSBC

#include <ANCREC.H>		// record: base class for rccn.h classes
#include <rccn.h>
#include "mspak.h"

#include <MSGHANS.H>	// MH_xxxx defns

#include <CVPAK.H> 		// FMTSQ FMTUNITS cvin2s

#include <CNGUTS.H>		// declarations for this file, MsR ZrB ResR

#define DO_LATENT		// define to include latent balance check, 9-12

/*----------------------- LOCAL FUNCTION DECLARATIONS ---------------------*/
static RC cgecheck( double tot, double net, float tol, double absTol,
	const char* what, const char* which, IVLCH ivl, int& balErrCount);

//====================================================================
void cgenbal(		// Check energy balances; issue warning message if out of tolerance
	IVLCH ivl,		// Interval to check: C_IVLCH_Y, _M, _D, _H, _S.
	FLOAT tol )		// check tolerance: generally 0.001 or so

{
#if defined( SUPPRESS_ENBAL_CHECKS)
	return;
#else
	double ovNet = 0., ovTot = 0.;			// accumulated overall net and total (sum abs value) energy transfer

// Loop over zones, checking each and accumulating overall

	ZNR *zp;
	RLUP( ZrB, zp)				// for zp = zone 1 to n (cnglob.h)
	{
		ZNRES_IVL_SUB* zrp =					// zone results for interval to check
			&ZnresB.p[zp->ss].curr.Y - 1 + ivl    	// point ...curr.Y, .M, .D, .H
				- (ivl==C_IVLCH_S);   				// adjustment needed for _S cuz no .HS
		double zTot = zrp->zr_TotAbsSen();
		double zNet = zrp->qsBal;		// get (float) net total from record, computed/accum in cnguts.cpp.
		ovNet += zNet;
		ovTot += zTot;				// add zone to overall, checked at end
		cgecheck( zNet, zTot, tol, .1, "zone '%s'", zp->name, ivl, zp->zn_ebErrCount);	// issue message if out of tolerance


#if defined( DO_LATENT)
		if (zp->zn_IsConvRad())
		{
			zNet = zrp->qlBal;						// get (float) net latent total from record.
			zTot = zrp->zr_TotAbsLat();
			ovNet += zNet;
			ovTot += zTot;				// add zone to overall, checked at end
			int iSink = 0;
			cgecheck( zNet, zTot, tol, .5, "zone '%s' latent", zp->name, ivl,
				ivl >= C_IVLCH_D ? zp->zn_ebErrCount : iSink);		// kludge to avoid double print of short-interval count
		}
#endif
	}

	MSRAT* mse;
	RLUP( MsR, mse)
		mse->ms_Enbal( ivl, tol, ovNet, ovTot);

// check overall balance
	cgecheck( ovNet, ovTot, tol, .1, "overall", NULL, ivl, Top.tp_ebErrCount);

#endif
}		// cgenbal
//-----------------------------------------------------------------------------
RC MSRAT::ms_Enbal(
	int ivl,		// Interval to check: C_IVLCH_Y, _M, _D, _H, _S.
	float tol,		// check tolerance: generally 0.001 or so
	double& ovNet,	// overall (whole project) net energy
	double& ovTot)	// ditto total energy
// returns RCOK if balance within tol or not checked
//    else RCBAD
{
	RC rc = RCOK;
	if (ivl > C_IVLCH_H)
		return rc;		// don't check subhour

	int mFlags = ms_Flags( ivl);
	double qIEDelta = ms_QIEDelta( ivl);		// interval change in mass internal energy

#if 0
x debug code, possibly useful 2-2012
x	if (ivl <= C_IVLCH_D)
x	{	double qIEDeltaX;
x		switch (ivl)
x		{
x		case C_IVLCH_M:
x			qIEDeltaX = ms_dqm;
x			break;
x
x		case C_IVLCH_D:
x			qIEDeltaX = ms_dqd;
x			break;
x
x		default:
x			qIEDeltaX = 0.;
x		}
x
x		if (fabs( qIEDeltaX - qIEDelta) > .01)
x			printf( "IE mismatch!\n");
x	}
#endif

	double mNet, mTot;
	ms_QXSurf( ivl, mNet, mTot);		// transfer at mass surface for interval
	mNet -= qIEDelta;
	mTot += fabs( qIEDelta);

	ovNet += mNet;
	ovTot += mTot;			// add this mass's net and total to overall met and total

	if (!(mFlags & msfNOBAL)
	 && fabs( mNet) > .0001f * tol * ms_QIE( C_IVLCH_D))
										// no message for error that is small fraction of mass's enthalpy:
										// speculative insurance re accuracy limits when qxdtot small, 2-95.
		rc = cgecheck( mNet, mTot, 		// issue msg if out of tolerance
				2*tol, 					// increase tolerance for masses: larger errors than zones.
										//   (2*tol needed with tol=.0001 for bug0050.zip:slab.inp, 2-95)
				.1,
				"surface '%s'", name, ivl, ms_ebErrCount);

	return rc;

}		// MSRAT::ms_Enbal
//====================================================================
static RC cgecheck(		// Check energy balance and display messages if reqd
	double net, 		// net heat flow (should be 0 if all were perfect)
	double tot, 		// total heat flow (sum of abs values of flows)
	float tol,  		// fractional error tolerance
	double absTol,		// absolute tol (no error if net <= absTol)
	const char* what, 	// type of object being checked for msg
						//   include %s to include which (e.g. zone '%s')
	const char* which,	// NULL or name of object being checked for msg
	IVLCH ivl,			// interval being checked
	int& balErrCount)	// cummulative count of errors for this what/which
						//   returned updated
// returns RCOK if balance within tol
//    else RCBAD
{
const int balErrCountWarnMax = 20;		// max # of short-interval errors to report (else just count)
// calc balance fraction
	DBL errf = tot > 0.  ?  fabs(net)/tot		// if tot is nz, use ratio.
						 :  net==0. ? 0. : 1.;	// if tot is 0, force error unless net is 0 too.

// check balance
	int bBalOK = errf <= tol || fabs(net) <= absTol; 	// never complain about
	if (bBalOK && !Top.tp_IsLastStep())
		return RCOK;			// balance OK and not end of run

	int bShortIvl = ivl >= C_IVLCH_D;

	const char* whatwhich = strtprintf( what, which);
	const char* xWhich = "";
	if (which)
		xWhich = strtprintf( " '%s'", which);

	if (!bBalOK)
	{
		balErrCount += bShortIvl;	// count short-interval errors
		if (!bShortIvl || balErrCount < balErrCountWarnMax)
			warn( "R0140: %s energy balance error for %s: %s (%1.6f)",
				whatwhich,
				Top.When( ivl),			// interval, eg "January"
				cvin2s( 				// magnitude of error with units, cvpak.cpp
					&net, DTDBL, UNENERGY1, 6, FMTSQ+FMTUNITS+4),
				errf );   				// error fraction, calc'd just above
		else if (bShortIvl && balErrCount == balErrCountWarnMax)
			warn( "Skipping further%s short-interval energy balance warning messages.",
				xWhich);
	}
	if (ivl == C_IVLCH_M && balErrCount > 0 && Top.tp_IsLastStep())
		warn( "Total%s short-interval energy balance error count = %d",
				xWhich, balErrCount);

	return bBalOK ? RCOK : RCBAD;

}		       // cgecheck

// end of cgenbal.cpp
