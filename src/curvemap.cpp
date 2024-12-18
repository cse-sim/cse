// Copyright (c) 1997-2024 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

///////////////////////////////////////////////////////////////////////////////
// curve and performance map classes and fucntions
///////////////////////////////////////////////////////////////////////////////

#include "cnglob.h"
#include "ancrec.h"
#include "rccn.h"
#include "msghans.h"	// MH_S0600 ...
#include "irats.h"
#include "vecpak.h"
#include "hvac.h"
#include <btwxt/btwxt.h>
#include "curvemap.h"

///////////////////////////////////////////////////////////////////////////////
// PYxxx: polynominal curves
///////////////////////////////////////////////////////////////////////////////

#define NORMALIZE	// define to normalize coeffs & continue when poly is not 1.0 as should be. 7-14-92.

//=============================================================================================================================
 RC PYLINEAR::normalize( 			// if poly coefficients are inconsistent, normalize and/or issue message.

	 record *r, 			// non-sub record containing the poly subrecord -- needed re error message
	 SI fn,			// field number of polynomial subrecord in r
	 const char* descrip,		// description of argument, eg "relative flow", for use in "at %s = %g"
	 DBL x /*=1.0*/ )   		// argument value for which poly should be 1.0: usually 1.0 for a cubic

// returns non-RCOK to stop run (error message already issued)
	{
		float v = val(x);				// get polynomial value for input x
		fn += PYLINEAR_K;				/* adjust to field number of k[], what user input & what is in CULT tables,
    						   for 4 uses in error calls below. */
#ifdef NORMALIZE
		if (v != 0)  				// if v is 0, normalizing would divide by 0.  Fall thru to error message
		{
			for (SI i = 0; i < 2; i++)
				k[i] /= v;					// normalize coefficients
#if 1							// omit if normalize warning not desired
			if (fabs(v - 1.) > .001)				// if much wrong, tell user, at least for now 7-14-92
				return r->oWarn( 				// returns RCOK.  MH_S0687 text used 4 places!
							  MH_S0687,		/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
							   "    for %s = %g.\n"
							   "    Coefficients will be normalized and execution will continue." */
							  r->mbrIdTx( fn),	 	// get id text for record/field. slow: do not do til error detected.
							  v, descrip, x );
#endif
		}
		else					// zero value is error: cannot normalize (would /0)
#else
		if (v==0. ||  fabs(v - 1.) > .001)			// zero value is error: cannot normalize (would /0)
#endif
			return r->ooer( fn,			// returns RCBAD.  MH_S0688 text used 4 places!
						 MH_S0688,		// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"
						 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
						 v, descrip, x );
		return RCOK;
}			// PYLINEAR::normalize
//=============================================================================================================================
RC PYCUBIC::normalize( 			// if cubic poly coefficients are inconsistent, normalize and/or issue message.

		record *r, 			// non-sub record containing the poly subrecord -- needed re error message
		SI fn,			// field number of polynomial subrecord in r
		const char* descrip,		// description of argument, eg "relative flow", for use in "at %s = %g"
		DBL x /*=1.0*/ )   		// argument value for which poly should be 1.0: usually 1.0 for a cubic

// returns non-RCOK to stop run (error message already issued)
	{
		float v = val(x);				// get polynomial value for input x
		fn += PYCUBIC_K;				/* adjust to field number of k[], what user input & what is in CULT tables,
    						   for 4 uses in error calls below. */
#ifdef NORMALIZE
		if (v != 0)  				// if v is 0, normalizing would divide by 0.  Fall thru to error message
		{
			for (SI i = 0; i < 4; i++)  		// 4 coefficients, k0 thru k3, for x^0 thru x^3
				k[i] /= v;				// normalize coefficients
#if 1						// omit if normalize warning not desired
			if (fabs(v - 1.) > .001)			// if much wrong, tell user, at least for now 7-14-92
				return r->oWarn( 							// returns RCOK 7-92
							  MH_S0687,	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
						   "    for %s = %g.\n"
						   "    Coefficients will be normalized and execution will continue." */
							  r->mbrIdTx( fn),	 // get id text for record/field. slow: do not do til error detected.
							  v, descrip, x );
#endif
		}
		else					// zero value is error: cannot normalize (would /0)
#else
		if (v==0. ||  fabs(v - 1.) > .001)		// zero value is error: cannot normalize (would /0)
#endif
			return r->ooer( fn,			// returns RCBAD
						 MH_S0688,		// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"
						 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
						 v, descrip, x );
		return RCOK;
}			// PYCUBIC::normalize
//=============================================================================================================================
RC PYCUBIC2::normalize( 		// if cubic-with-x0 poly coefficients are inconsistent, normalize and/or issue message.

	record *r, 			// non-sub record containing the poly subrecord -- needed re error message
	SI fn,			// field number of polynomial subrecord in r
	const char* descrip,	// description of argument, eg "relative flow", for use in "at %s = %g"
	DBL x /*=1.0*/ )   		// argument value for which poly should be 1.0: usually 1.0 for a cubic

// returns non-RCOK to stop run (error message already issued)
{
	float v = val(x);				// get polynomial value for input x
	fn += PYCUBIC2_K;				/* adjust to field number of k[], what user input & what is in CULT tables,
    						   for 4 uses in error calls below. */
#ifdef NORMALIZE
	if (v != 0)  				// if v is 0, normalizing would divide by 0.  Fall thru to error message
	{
		for (SI i = 0; i < 5; i++)    		// 5 *FIVE* values: k0 thru k3, coeff for x^0 thru x^3, and x0: x floor.
			k[i] /= v;				// normalize coefficients
#if 1						// omit if normalize warning not desired
		if (fabs(v - 1.) > .001)			// if much wrong, tell user, at least for now 7-14-92
			return r->oWarn( 			// returns RCOK 7-92
						  MH_S0687,	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
                    			   "    for %s = %g.\n"
                    			   "    Coefficients will be normalized and execution will continue." */
						  r->mbrIdTx( fn),	 // get id text for record/field. slow: do not do til error detected.
						  v, descrip, x );
#endif
	}
	else					// zero value is error: cannot normalize (would /0)
#else
	if (v==0. ||  fabs(v - 1.) > .001)		// zero value is error: cannot normalize (would /0)
#endif
		return r->ooer( fn,			// returns RCBAD
				 MH_S0688,		// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"
				 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
				 v, descrip, x );
	return RCOK;
}			// PYCUBIC2::normalize
//=============================================================================================================================
RC PYBIQUAD::normalizeCoil( 			// if poly coefficients are inconsistent, normalize and/or issue message:
	// interface using arguments appropriate for cooling coil

	AH *ah, 		// air handler containing coil: dsTWbEn and dsTDbCnd are used.
	SI fn )		// field number of polynomial subrecord in ah
{
	return normalize( ah, fn, "rated conditions of ahccDsTWbEn", "ahccDsTDbCnd", ah->ahcc.dsTWbEn, ah->ahcc.dsTDbCnd);

}		// PYBIQUAD::normalizeCoil
//=============================================================================================================================
RC PYBIQUAD::normalize( 		// if biquadratic poly coefficients are inconsistent, normalize and/or issue message.

	record *r, 			// non-sub record containing the poly subrecord -- needed re error message
	SI fn,			// field number of polynomial subrecord in r
	const char* descX, const char* descY,	// descriptive texts, for use in "for %s=%g and %s=%g"
	DBL x, DBL y,   		// argument value for which poly should be 1.0
	BOO noWarn /*=FALSE*/ )	// true to suppress normalization warning (eg if defaulted & coeff known non-normal)
// returns non-RCOK to stop run (error message already issued)
{
	float v = val(x,y);				// get polynomial value for inputs x,y
	fn += PYBIQUAD_K;				/* adjust to field number of k[], what user input & what is in CULT tables,
						   for 4 uses in error calls below. */
#ifdef NORMALIZE
	if (v != 0)  				// if v is 0, normalizing would divide by 0.  Fall thru to error message
	{
		for (SI i = 0; i < 6; i++)  		// six coefficients
			k[i] /= v;				// normalize coefficients
#if 1						// omit if normalize warning not desired
		if ( !noWarn				// if warning not suppressed
				&&  fabs(v - 1.) > .02 )   		// if much wrong, tell user, at least for now 7-14-92
			return r->oInfo( 			// returns RCOK 7-92
						  MH_S0689,	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
					   "    for %s=%g and %s=%g.\n"
					   "    Coefficients will be normalized and execution will continue." */
						  r->mbrIdTx( fn),	// get id text for record/field. slow: do not do til error detected.
						  v, descX, x, descY, y );
#endif
	}
	else					// zero value is error: cannot normalize (would /0)
#else
	if (v==0. ||  fabs(v - 1.) > .001)
#endif
		return r->ooer( fn,			// returns RCBAD
					 MH_S0690,		/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
					   "    for %s=%g and %s=%g\n" */
					 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
					 v, descX, x, descY, y );
	return RCOK;
}			// PYBIQUAD::normalize
//=============================================================================

///////////////////////////////////////////////////////////////////////////////
// class PMACCESS: supports runtime access to perf map
//                 retains pointer to Btwxt interpolator
// class PERFORMANCEMAP: input representation of performance map
//      class PMGRIDAXIS:  perfmap grid values (independent vars, e.g. DBT, speed, )
//      class PMLOOKUPDATA:  perfmap performance data (e.g. capacity, power, )
///////////////////////////////////////////////////////////////////////////////
PMACCESS::PMACCESS()
	: pa_pParent( nullptr), pa_pPERFORMANCEMAP( nullptr), pa_pRGI( nullptr),
	pa_capRef( 0.), pa_speedFMin( 0.), pa_speedFRated( 0.)
{
}	// PMACCESS::PMACCESS
//------------------------------------------------------------------------------
/*virtual*/ PMACCESS::~PMACCESS()
{	delete pa_pRGI;
	pa_pRGI = nullptr;
}	// PMACCESS::~PMACCESS()
//------------------------------------------------------------------------------
RC PMACCESS::pa_Init(		// input -> Btwxt conversion
	const PERFORMANCEMAP* pPM,	// source PERFORMANCEMAP
	record* pParent,			// parent (e.g. RSYS)
	const char* tag,		// identifying text for this interpolator (for messages)
	double capRef)			// reference capacity (e.g. cap47 or cap95)
{
	RC rc = RCOK;

	delete pa_pRGI;		// insurance

	pa_pParent = pParent;
	pa_capRef = capRef;

	pa_pPERFORMANCEMAP = pPM;	// source performance map

	rc = pPM->pm_SetupBtwxt(pParent, tag, pa_pRGI, pa_tdbRated, pa_speedFRated);

	if (!rc)
	{
		int targetDim = static_cast<int>(pa_pRGI->get_number_of_dimensions());
		pa_vTarget.resize( targetDim);

		int resultDim = int(pa_pRGI->get_number_of_grid_point_data_sets());
		pa_vResult.resize( resultDim);

		pa_speedFMin = pa_pRGI->get_grid_axis(1).get_values()[0];
	}

	return rc;

}	// PMACCESS::pa_Init
//-----------------------------------------------------------------------------
RC PMACCESS::pa_GetCapInp(float dbtOut, float speedF, float& cap, float& inp)
{
	double capRat, inpRat;
	RC rc = pa_GetCapInpRatios(dbtOut, speedF, capRat, inpRat);
	cap = capRat * pa_capRef;
	inp = inpRat * abs(pa_capRef);
	return rc;
}	// RSYS::pa_GetCapInp
//-----------------------------------------------------------------------------
RC PMACCESS::pa_GetCapInpRatios(
	float dbtOut,		// outdoor temp, F
	float speedF,		// speed fraction
	double& capRat,		// returned: cap / capRef
	double& inpRat)		// returned: inp / inpRef
{
	RC rc = RCOK;

	// copy data to pre-allocated vector
	pa_vTarget.data()[0] = dbtOut;
	pa_vTarget.data()[1] = speedF;

	pa_vResult = (*pa_pRGI)(pa_vTarget);

	capRat = pa_vResult.data()[0];
	inpRat    = pa_vResult.data()[1];

	return rc;
}	// PMACCESS::pa_GetCapInpRatios
//-----------------------------------------------------------------------------
RC PMACCESS::pa_GetRatedCapCOP(		// get rated values from performance map
	float dbtOut,	// outdoor temp of rating, F (47, 95, )
	float& cap,		// return: capacity, Btuh
	float& COP,		// return: COP
	PMSPEED whichSpeed /*=pmSPEEDRATED*/)	// speed selector
										// (pmSPEEDMIN, pmSPEEDRATED, pmSPEEDMAX)
// return values are net based on rating fan power
// returns RCOK iff success
{
	RC rc = RCOK;

	float speedF = pa_GetSpeedF(whichSpeed);

	double capRat, inpRat;
	rc = pa_GetCapInpRatios(dbtOut, speedF, capRat, inpRat);

	cap = pa_capRef * capRat;
	COP = capRat / inpRat;

	return rc;
}		// PMACCESS::pa_GetRatedCapCOP
//-----------------------------------------------------------------------------
double PMACCESS::pa_GetRatedFanFlowFactor(
	float speedF)		// speed fraction

// returns (air flow at speedF) / (air flow at rated speed)
{
	double flowFactor, inpRat;
	RC rc = pa_GetCapInpRatios(pa_tdbRated, speedF, flowFactor, inpRat);
	if (rc)
	{	pa_pParent->oer("pa_GetRatedFanFlow fail (speedF = %0.3f)", speedF);
		flowFactor = 1.;
	}

	return flowFactor;
}		// PMACCESS::pa_GetRatedFanFlowFactor
//=============================================================================
/*static*/ RC PERFORMANCEMAP::pm_Top()
// Check/init *all* PERFORMANCEMAPs for validity at run start.
// Other checks (e.g. pm_SetupBtwxt()) are only for referenced PERFORMANCEMAPS.
{
	RC rc = RCOK;

	PERFORMANCEMAP* pPM;
	RLUP(PerfMapB, pPM)
		rc |= pPM->pm_RunInit();

	return rc;
}		// PERFORMANCEMAP::pm_TopAllCkF
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_RunInit()	// Start-of-run checks and inits
{
	// check self
	RC rc = pm_CkF();

	// check children
	pm_gxCount = 0;
	pm_luNValuesExp = 1;
	if (!rc)
	{
		PMGRIDAXIS* pGX;
		RLUPC(PMGXB, pGX, pGX->ownTi == ss)
		{
			rc |= pGX->pmx_CkF();	// sets pmx_nValues
			++pm_gxCount;
			pm_luNValuesExp *= pGX->pmx_nValues;
		}

		PMLOOKUPDATA* pLU;
		RLUPC(PMLUB, pLU, pLU->ownTi == ss)
		{
			rc |= pLU->pmv_CkF();
			if (!rc)
				// check consistency lookup <-> grid
				rc |= pLU->pmv_CheckNValues(pm_luNValuesExp);
		}
	}

	if (rc)
	{	// don't leave possibly bogus info
		pm_gxCount = 0;
		pm_luNValuesExp = 0;
	}

	return rc;

}		// PERFORMANCEMAP::pm_RunInit
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_CkF()
// called twice:
//   at object input
//   at run start (when all expression values known)
{
	RC rc = RCOK;

	// no checks required

	return rc;
}		// PERFORMANCEMAP::pm_CkF
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_GetGridAxis(		// retrieve child grid axis
	int iGX,		// axis idx (1 based)
	PMGRIDAXIS*& pGXRet) const
// returns RCOK iff success (pGXRet set)
//    else nz RCxxx (msg issued)
{
	RC rc = PMGXB.GetIthChild(this, iGX, pGXRet);

	return rc;
}	// PERFORMANCEMAP::pm_GetGridAxis
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_GetLookupData(
	int iLU,		// lookup data sought (1 based)
	PMLOOKUPDATA*& pLURet) const
{
	RC rc = PMLUB.GetIthChild(this, iLU, pLURet);

	return rc;
}	// PERFORMANCEMAP::pm_GetLookupData
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_GXCheckAndMakeVector(
	int iGX,					// axis idx (1 based)
	PMGRIDAXIS* &pGX,		// returned: pointer to PMGRIDAXIS if found
	std::vector< double>& vGX,	// returned: grid values
	std::pair<int, int> sizeLimits) const	// supported size range
// returns RCOK iff return values are usable
//    else non-RCOK, msg(s) issued
{
	RC rc = pm_GetGridAxis(iGX, pGX);
	if (rc)
		vGX.clear();
	else
		rc = pGX->pmx_CheckAndMakeVector(vGX, sizeLimits);

	return rc;

}	// PERFORMANCEMAP::pmx_pm_GXCheckAndMakeVector
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_LUCheckAndMakeVector(
	int iLU,			// lookup data idx (1 based)
	PMLOOKUPDATA* &pLU,	// returned: PMLOOKUPDATA found
	std::vector< double>& vLU,	// returned: values
	int expectedSize,
	double vMin /*=-DBL_MAX*/,
	double vMax /*=DBL_MAX*/) const
{
	RC rc = pm_GetLookupData(iLU, pLU);
	if (rc)
		vLU.clear();
	else
		rc = pLU->pmv_CheckAndMakeVector(vLU, expectedSize, vMin, vMax);

	return rc;

}	// PERFORMANCEMAP::pm_CheckAndMakeVector
//-----------------------------------------------------------------------------
RC PERFORMANCEMAP::pm_SetupBtwxt(		// input -> Btwxt conversion
	record* pParent,		// parent (e.g. RSYS)
	const char* tag,		// identifying text for this interpolator (for messages)
	Btwxt::RegularGridInterpolator*& pRgi,		// returned: initialized Btwxt interpolator
	double& tdbOutRef,				// returned: outdoor dbt for reference values, F
	double& speedFRef) const		// returned: speed fraction for rated values

{
	RC rc = RCOK;

	delete pRgi;		// delete prior if any
	pRgi = nullptr;

	// check input data and convert to vector
	// WHY vector conversion
	//   * convenient
	//   * allows modification of values w/o changing input records
	//     (which may have multiple uses)

	// grid variables
	const char* msg{ nullptr };	// 
	if (PMGXB.CheckChildCount(this, { 2, 2 }, msg) != RCOK)
		return oer("Incorrect number of PMGRIDAXISs. %s.", msg);

	std::vector< PMGRIDAXIS*> pGX(2);
	std::vector< std::vector<double>> vGX(2);	// axis values
	rc |= pm_GXCheckAndMakeVector(1, pGX[0], vGX[0], { 1, 10 });
	rc |= pm_GXCheckAndMakeVector(2, pGX[1], vGX[1], { 1, 10 });

	if (rc)
		return rc;

	int LUSize = int( vGX[0].size() * vGX[1].size());		// # of LU values

	tdbOutRef = pGX[0]->pmx_refValue;

	// normalize spd to minspd .. 1.
	double scale = 1./vGX[1][vGX[1].size()-1];
	VMul1(vGX[1].data(), int( vGX[1].size()), scale);
	speedFRef = pGX[1]->pmx_refValue * scale;

	// lookup values
	PMLOOKUPDATA* pLUCap;
	std::vector< double> vCapRat;
	rc |= pm_LUCheckAndMakeVector(1, pLUCap, vCapRat, LUSize);
	PMLOOKUPDATA* pLUCOP;
	std::vector< double> vCOP;
	rc |= pm_LUCheckAndMakeVector(2, pLUCOP, vCOP, LUSize, 0.1, 20.);

	if (rc)
		return rc;

	// message handling linkage
	auto MX = std::make_shared< CourierMsgHandlerRec>( pParent);

	// transfer grid value to Btwxt axes
	std::vector<Btwxt::GridAxis> gridAxes;
	for (int iGX=0; iGX<2; iGX++)
	{
		gridAxes.emplace_back(
			Btwxt::GridAxis(
				vGX[ iGX],		// grid values
				Btwxt::InterpolationMethod::linear, Btwxt::ExtrapolationMethod::constant,
				{ -DBL_MAX, DBL_MAX },		// extrapolation limits
				pGX[ iGX]->pmx_type.CStr(),	// axis name (from PMGRIDAXIS user input)
				MX));						// message handler

	}

	// finalize lookup data
	
	std::vector< std::vector< double>> values;
	values.resize(2);
	for (int iLU = 0; iLU<LUSize; iLU++)
	{	
		values[0].push_back(vCapRat[ iLU]);
		values[1].push_back(vCapRat[ iLU]/max(.01, vCOP[iLU]));
	}

	std::vector< Btwxt::GridPointDataSet> gridPointDataSets;
	gridPointDataSets.emplace_back(values[0], "Capacity ratio");
	gridPointDataSets.emplace_back(values[1], "Input ratio");

	// construct Btwxt object
	pRgi = new Btwxt::RegularGridInterpolator(gridAxes, gridPointDataSets, tag, MX);

	return rc;
}		// PERFORMANCEMAP::pm_SetupBtwxt
//=============================================================================
PERFORMANCEMAP* PMGRIDAXIS::pmx_GetPERFMAP() const
{
	return PerfMapB.GetAtSafe( ownTi);
}	// PMGRIDAXIS::pmx_GetPERFMAP
//-----------------------------------------------------------------------------
/*virtual*/ void PMGRIDAXIS::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	pmx_type.Release();
	record::Copy( pSrc, options);
	pmx_type.FixAfterCopy();
}		// PMGRIDAXIS::Copy
//-----------------------------------------------------------------------------
RC PMGRIDAXIS::pmx_CkF()		// check PMGRIDAXIS
// note: called from cul at input and from pm_TopAllChk() at run
{
	RC rc = RCOK;

	int nSet{ 0 };			// # of grid values that user provided
	int nEvaluated{ 0 };	// # of grid values that have been evaluated
							//   (i.e. not yet-to-be evaluated expressions)
	rc = ArrayStatusCounts(PMGRIDAXIS_VALUES, nSet, nEvaluated);
	// nSet may be 0 if pmGXValues is omitted
	// detected/msg'd in cul, no msg needed here

	if (!rc)
	{
		pmx_nValues = nSet;
		// check values iff all evaluated
		//   input with expressions checked on 2nd call (from pm_TopAllCkF())
		if (pmx_nValues == nEvaluated
			&& !VStrictlyAscending(pmx_values, pmx_nValues))
			rc |= oer("%s values must be in strictly ascending order.",
				pmx_type.CStr());
	}

	return rc;
}		// PMGRIDAXIS::pmx_CkF
//-----------------------------------------------------------------------------
RC PMGRIDAXIS::pmx_CheckNValues(	// runtime check of # of values
	std::pair< int, int> sizeLimits) const	// size range supported by caller
{
	RC rc = RCOK;

#if defined( _DEBUG)
	// check for legal size limits
	//    should often be checkable at compile time but array dimension not accessible
	if (sizeLimits.second > mbrArrayDim(PMGRIDAXIS_VALUES)-1)
		rc |= err(PABT, "sizeLimits program error");
#endif

	const char* msg{ nullptr };
	if (limitCheckCount(pmx_nValues, sizeLimits, msg) != RCOK)
	{	rc |= oer("Incorrect number of values for '%s'. %s",
				pmx_type.CStr(), msg);
	}

	return rc;
}		// PMGRIDAXIS::pmx_CheckNValues
//-----------------------------------------------------------------------------
RC PMGRIDAXIS::pmx_CheckAndMakeVector(
	std::vector< double>& vGX,	// returned: input data as vector
	std::pair< int, int> sizeLimits) const	// size range supported by caller
{
	RC rc = RCOK;

	vGX.clear();

	rc |= pmx_CheckNValues(sizeLimits);

	// check for strictly ascending is in pmx_CkF()

	if (!rc)
	{
		vGX.assign(pmx_values, pmx_values+pmx_nValues);
	}

	return rc;
}		// PMGRIDAXIS::pmx_CheckAndMakeVector
//=============================================================================
PERFORMANCEMAP* PMLOOKUPDATA::pmv_GetPERFMAP() const
{
	return PerfMapB.GetAtSafe( ownTi);
}	// PMLOOKUPDATA::pm_LUGetPERFMAP
//-----------------------------------------------------------------------------
/*virtual*/ void PMLOOKUPDATA::Copy( const record* pSrc, int options/*=0*/)
{
	options;
	pmv_type.Release();
	record::Copy( pSrc, options);
	pmv_type.FixAfterCopy();
}		// PMLOOKUPDATA::Copy
//-----------------------------------------------------------------------------
RC PMLOOKUPDATA::pmv_CkF()
// returns RCOK iff success
//    else other RC (msg'd within)
{
	RC rc = RCOK;

	int nSet{ 0 };			// # of lookup array elements provide by user
	int nEvaluated{ 0 };	// # of lookup array elements that are evaluated
							//   i.e. not unevaluated expressions
	rc = ArrayStatusCounts( PMLOOKUPDATA_VALUES, nSet, nEvaluated);

	if (!rc)
		pmv_nValues = nSet;

	return rc;
}		// PMLOOKUPDATA::pmv_CkF
//-----------------------------------------------------------------------------
RC PMLOOKUPDATA::pmv_CheckNValues(
	int nValuesExp) const	// expected number of values
{
	RC rc = RCOK;
	if (pmv_nValues != 1 && pmv_nValues != nValuesExp)
	{
		const char* expectedMsg = strtprintf(nValuesExp == 1 ? "%d" : "1 or %d",
						nValuesExp);
		rc |= oer("Incorrect number of values for '%s'. Expected %s, found %d.",
			pmv_type.CStr(), expectedMsg, pmv_nValues);
	}
	return rc;
}	// PMLOOKUPDATA::pmv_CheckNValues
//-----------------------------------------------------------------------------
RC PMLOOKUPDATA::pmv_CheckAndMakeVector(
	std::vector< double>& vLU,	// returned: vector containing input values
	int nValuesExp,				// number of values expected
	double vMin /*=0.*/,		// min value allowed
	double vMax /*=DBL_MAX*/) const	// max value allowed
{
	RC rc = RCOK;

	vLU.clear();

	// (probably) redundant size check
	//   note pmv_nValues can be 1
	rc |= pmv_CheckNValues(nValuesExp);

	if (!rc)
	{
		if (pmv_nValues == 1)
			vLU.assign(nValuesExp, pmv_values[0]);	// fill with single value
		else
			vLU.assign(pmv_values, pmv_values+nValuesExp);

		// limit check all values
		// note cnrecs.def FLOAT_GEZ prevents < 0
		for (int iV=0; iV<nValuesExp; iV++)
		{	if (vLU[iV] < vMin || vLU[iV] > vMax)
			{
				rc |= oer("%s %s[%d] (%g) must be in range %g - %g",
					pmv_type.CStr(), mbrIdTx(PMLOOKUPDATA_VALUES), iV+1, vLU[iV], vMin, vMax);
				// Run will not proceed
				// Change to safe value to avoid trouble during further setup
				vLU[iV] = bracket(vMin, vLU[iV], vMax);
			}
		}
	}

	return rc;
}		// PMLOOKUPDATA::pm_CheckAndMakeVector
//=============================================================================
// 
// curvemap.cpp end
