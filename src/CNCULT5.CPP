// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// search NUMS for text to extract 6-96

// ah ecoty = TWO_STAGE not implemented; should get error message. 8-92.

// cncult5.cpp: cul user interface tables and functions for CSE:
//   part 5: fcns for checking terminals and air handlers, called from cncult2.cpp:topCkf.


/* Notes re AH ZN and ZN2 supply temp control methods, Rob 5-95:

   1. These are needed to simulate single zone constant volume systems.
      VAV is still allowed but could be removed (says Bruce 5-95) if
      hard-to-fix problems continue to be encountered. (As of 5-95, VAV
      ZN/ZN2 believed to work, but accurate simulation has been hard to
      achieve, especially when min vol is neither 0 nor same as max.

   2. Additional terminals are probably not disallowed on AH but are
      untested and not needed -- disallow when reason found. */

/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// USI SI LI; includes cndefns.h

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// TOPRATstr PRIBASEstr SFIstr
#include <MSGHANS.H>	// MH_S0600 ...

#include <HVAC.H>		// CoolingSHR
#include <PSYCHRO.H>	// psyAltitude psyPBar

#include <CNGUTS.H>	// ZrB Top

#include <CUPARSE.H>	// perl
#include <EXMAN.H>	// exInfo
#include <CUEVF.H>	// EVFRUN
#include <CUL.H>	// FsSET oer oWarn quifnn classObjTx

//declaration for this file:
//#include <cncult.h>	// use classes, globally used functions
#include <IRATS.H>	// declarations of input record arrays (rats)
#include <CNCULTI.H>	// cncult internal functions shared only amoung cncult,2,3,4.cpp


/*-------------------------------- OPTIONS --------------------------------*/

#if 0 // removing TUQDSLHWARN 1-4-95 (with temp backtrack, later edit out if no complaints)
x #define TUQDSLHWARN	// defined: issue warning for giving tuQDsLh -- not to be implemented, 9-92.
x 			// undefined: old code (input was complete here, never used elsewhere)
x 			// later code tuQDsLh totally out of program if no objections to this change
#endif

#define NORMALIZE	// define to normalize coeffs & continue when poly is not 1.0 as should be. 7-14-92.

/*-------------------------------- DEFINES --------------------------------*/

/*---------------------------- LOCAL VARIABLES ----------------------------*/


//================================= FUNCTIONS FOR CHECKING TERMINALS AND AIR HANDLERS =========================================


//============================================================================================================================
RC FC topAh1( void)		// check / set up air handlers, part 1, done before terminals

// adds air handler run records so terminal check code can access/store into them, eg to chain zhx's.
// checking and most setup is done in topAh2.
{
	AH *ahi, *ah;
	RC rc /*=RCOK*/;		// (redundant rc inits removed 12-94 when BCC 32 4.5 warned)

// copy air handler input records to run records

	CSE_E( AhB.al( AhiB.n, WRN) )			// delete (unexpected) old ah's, alloc needed # ah's at once for min fragmentation
	// ah's are not owned (names must be unique), leave .ownB 0.
	RLUP( AhiB, ahi)				// loop air handler input records (cnglob.h macro)
	{
		AhB.add( &ah, ABT, ahi->ss);   		// add air handler runtime record.  Error unexpected cuz ratCre'd.
		// same subscript must be used due to ref's stored eg in terminals.
		*ah = *ahi;				// copy entire input record into run record including name
	}
	return rc;
}			// topAh1
//============================================================================================================================
RC FC topTu()			// check / set up terminals

// called between topAh1 and topAh2 -- assumes ah run records already added.
{
	RC rc = TuB.RunDup(TuiB, &ZrB);
	return rc;
}			// topTu

//============================================================================================================================
RC FC topAh2()		// check / set up air handlers, part 2, done after terminal check/setup

// topTu must be done first (terminals used to find zones for air handler)
// ah input records have already been copied to run records by topAh1.
{
	RC rc=RCOK;
	AH* ah;
	RLUP( AhB, ah)				// loop air handler run records (cnglob.h macro)
		rc |= ah->setup();		// check/set up one air handler run record (below)
	return rc;
}		// topAh2
//=============================================================================================================================
RC AH::setup()			// check / set up one air handler run record

// topTu  should be done b4 this fcn (terminals chain is used to find ah's zones)
// topHp1 should be done b4 this fcn (HW coil is chained to its HEATPLANT)
{

	RC rc=RCOK;

	//--------------------------- air handler checks and defaults (with some setup) ------------------------------

	// loop zones served, determine area

	float area = 0.f;
	for (ZNR *zp = NULL;  nxZn(zp);  )		// cnah.cpp:AH::nxZn: 1st (zp NULL)/next zone for ah
		area += zp->i.znArea;				// accumulate area (re min outside air default)

	// determine # terminals served

	SI nTus = 0;
	for (TU *tu = NULL;  nxTu(tu);  )
		nTus++;						// count terminals (re ahCtu default)
	if (!nTus)  rc |= oer( (char *)MH_S0614);		// "Air handler has no terminals!"

	// outside air (do before return fan)

	rc |= require( AH_SFAN + FAN_VFDS);			// be sure supply fan cfm given b4 next checks
	if (sstat[AH_OAVFDSMN] & FsSET)				// if outside air design min flow given, check it
	{
		if (sstat[AH_SFAN + FAN_VFDS] & FsVAL)		// if sfanVfDs has value (not autoSize -- NAN!) 7-95
			if (oaVfDsMn > sfan.vfDs)
				rc |= ooer( AH_OAVFDSMN, AH_SFAN + FAN_VFDS,
							(char *)MH_S0615,			/* "oaVfDsMn > sfanVfDs: min outside air greater (%g) "
							   "than supply fan capacity (%g)" */
							oaVfDsMn, sfan.vfDs );
	}
	else							// outside air design min flow not given, default it
	{
		float def = 0.15f * area;				// .15 cfm per square foot served area
		if ( sstat[AH_SFAN + FAN_VFDS] & FsVAL		// if sfanVfDs has value (not autoSize -- NAN!) 7-95
				&&  def > sfan.vfDs )
			rc |= ooer( AH_OAVFDSMN, AH_SFAN + FAN_VFDS,
						(char *)MH_S0616,    			/* "oaVfDsMn required: default (%g) of .15 cfm per sf"
                 					   " of served area (%g) \n"
                 					   "    would be greater than supply fan capacity (%g)" */
						def, area,  sfan.vfDs );
		else
			oaVfDsMn = def;					// ok, store the default
	}

	if (!IsSet(AH_OAHX + HEATEXCHANGER_VFDS))
	{
		ah_oaHx.hx_VfDs = oaVfDsMn;
	}

	// economizer

	if (oaEcoTy==C_ECOTYCH_NONE)				// if no economizer (default)
	{
		rc |= disallowN( (char *)MH_S0617,			// "when no economizer (oaEcoTy NONE or omitted)"
						 AH_OALIMT, AH_OALIME, 0 );		// not allowed when no eco
	}
	//else ...  eco present

	float lkSum = oaOaLeak + oaRaLeak;
	if (lkSum > 1.f)
		rc = ooer( AH_OAOALEAK, AH_OARALEAK,
				   (char *)MH_S0618, 				// "oaOaLeak + oaRaLeak = %g: leaking %d% of mixed air!"
				   lkSum, (INT)(lkSum * 100 + .5f) );

	// check fan and coil subrecords

	if (rc)   return rc;						// don't attempt if errors: defaults may be bad
	rc |= sfan.fn_setup( C_FANAPPCH_SFAN, this, AH_SFAN, -1.f, NULL);	// supply fan.  No default cfm.
	rc |= rfan.fn_setup( C_FANAPPCH_RFAN, this, AH_RFAN, 		// return fan. do after supply fan. Incl autoSize checks.
					  (sstat[AH_SFAN+FAN_VFDS] & FsVAL)	//  if sfanVfDs has a value (not autoSize -- NAN!)
#if 0//1991..95
x                            ?  sfan.vfDs - oaVfDsMn		//    default rfan cfm: sfan less min outside air
#else//7-11-95
						 ?  sfan.vfDs			/*    dflt rfan cfm: sfan cfm. min outside air formerly subtracted
							      (presuming it leaked out of bldg), but leak not modeled
							      in any way elsewhere so deleted 7-11-95. */
#endif
						 :  0,				/*    autoSizing sfan: require rfanVfDs if not autoSized.
							      FAN::setup issues specific msg. */
					  &sfan );				//  default part load curve: same as supply fan


	if (rfan.fanTy == C_FANTYCH_NONE)
	{
#define ZFAN(m) (AH_RFAN + FAN_##m)
#define HX(m) (AH_OAHX + HEATEXCHANGER_##m)
		const char* when = "when there is no return fan";
		rc |= ignoreN( when, ZFAN(VFDS), ZFAN(PRESS), ZFAN(EFF), ZFAN(SHAFTPWR),
				 ZFAN(ELECPWR), ZFAN(MTRI), ZFAN(ENDUSE), ZFAN(CURVEPY+PYCUBIC_K),
				 HX(SENEFFH), HX(VFDS), HX(F2), HX(SENEFFH+1), HX(LATEFFH),
				 HX(LATEFFH+1), HX(SENEFFC), HX(SENEFFC+1), HX(LATEFFC),
				 HX(LATEFFC+1), HX(BYPASS), HX(AUXPWR), 0);
#undef ZFAN
#undef HX
	}


	rc |= cch.setup(  this, AH_CCH);   				// crankcase heater, below. must call b4 heat coil.
	rc |= ahhc.setup( C_COILAPPCH_AHHC, this, AH_AHHC, 0, 0);	// heating coil. AHHEATCOIL::setup, below.
	rc |= ahcc.setup( C_COILAPPCH_AHCC, this, AH_AHCC, 0, 0);	// cooling coil. COOLCOIL::setup, below. Do after sfan.

	// control terminal for ZONE ts sp control

	// default ahCtu to the terminal if exactly 1 zone terminal.  (0 is not an inputtable ahCtu value.)
	BOO dflCtu = FALSE;
	if (!ahCtu)  if (nTus==1)
		{
			ahCtu = tu1;     // tu1: ah TU chain head, set above in topTu.
			dflCtu=TRUE;
		}

	// check for valid terminal reference
	if (ahCtu)  rc |= ckRefPt( &TuB, ahCtu, "ahCtu");

	if ( ahCtu
			&&  !rc )    						// not if ahCtu might be a bad TU reference
	{
		tu = TuB.p + ahCtu;					// point controlling terminal
		if (!dflCtu)						// no errmsg for dfl'd ctu: don't know ctu use intended, 4-1-92.
			// --> runtime must also check ahCtu for having a setpoint.
			if (!(tu->cmAr & cmStBOTH))
				rc |= tu->ooer( TU_TUTH, TU_TUTC,
							(char *)MH_S0619,		/* "Control Terminal for air handler %s:\n"
				             	   "    Must give heating setpoint tuTH and/or cooling setpoint tuTC" */
							name );
		tu->ctrlsAi = ss;					// set .ctrlsAi of terminal that controls this ah
	}

	// supply temperature control

	// supply setpoint required if have economizer or coil(s), else disallowed
	// (changed from RQD 5-8-92 cuz users would give ahTsSp w/o coil then wonder why no heating/cooling occurred)

	if (oaEcoTy==C_ECOTYCH_NONE  &&  ahhc.coilTy==C_COILTYCH_NONE  &&  ahcc.coilTy==C_COILTYCH_NONE)
		rc |= disallow( (char *)MH_S0620, AH_AHTSSP);	// "when air handler has no econimizer nor coil\n"
							               				//   (did you forget to specify your coil?)"
	else
		rc |= require( (char *)MH_S0621, AH_AHTSSP);		// "unless air handler has no economizer nor coil"

	// if supply temp setpoint / ctrl method value stored (thus constant), can check now for user convenience.
	// must REPEAT CHECKS AT RUNTIME in case variable expr given!
	if (sstat[AH_AHTSSP] & FsVAL  &&  ISNCHOICE(ahTsSp) )  	// if now set, to a choice (not a number)
		switch (CHN(ahTsSp))
		{
			//case C_TSCMNC_WZ: case C_TSCMNC_CZ:  no requirements: ahW/CzCzns default to ALL.

		case C_TSCMNC_ZN2:
		case C_TSCMNC_ZN:
			if (!ahCtu)  					// not if defaulted above
				rc |= require( (char *)MH_S0622, AH_AHCTU);	// "when ahTsSp is ZN or ZN2 and "
															// "more than one terminal served"
			break;

		case C_TSCMNC_RA:
			rc |= requireN( (char *)MH_S0623, 				// "when ahTsSp is RA"
							AH_AHTSMN, AH_AHTSMX, AH_AHTSRAMN, AH_AHTSRAMX, 0 );
			break;
		}

	// ahFanCycles default: YES when ahTsSp==ZN (but not ZN2): hourly variable, done dynamically in cnah.cpp:AH::begHour.

	// fanCycles setup time checks if inputs constant; must also check at runtime as run-variable

	if ( sstat[AH_AHTSSP] & FsVAL 					// if ahTsSp given and not runtime-variable
			&&  ISNCHOICE(ahTsSp) && CHN(ahTsSp)==C_TSCMNC_ZN		// and if ahTsSp is ZN (not ZN2)
			&&  !(sstat[AH_AHFANCYCLES] & FsSET)				/* and if ahFanCycles NOT given --> defaults yes at runtime
    								   (yes given... fall thru) */
			||  sstat[AH_AHFANCYCLES] & FsVAL && CHN(ahFanCycles)==C_NOYESVC_YES )	// OR if ahFanCycles is constant YES
	{
		// fanCyles and ZN2 (fan stays on even when ctrl zone is floating) are contradictory
		if (ISNCHOICE(ahTsSp) && CHN(ahTsSp)==C_TSCMNC_ZN2)
			rc |= ooer( AH_AHFANCYCLES, (char *)MH_S0624, name); 	// "ahFanCycles=YES not allowed when ahTsSp is ZN2"

		// only 1 terminal can be used (or would have to force other-tu fraction of full flow to match control tu)
		// (with ahTsSp=ZN/2 and ahFanCycles=NO we are not yet enforcing single tu as would work even if unreal, 6-92)
		if (nTus > 1)
			rc |= ooer( AH_AHTSSP, (char *)MH_S0625); // "More than one terminal not allowed on air handler when fan cycles"

		// control terminal required (redundant insurance, as defaulted above to only tu)
		if (!ahCtu)
			rc |= ooer( AH_AHTSSP, (char *)MH_S0626);		// "No control terminal: required when fan cycles."

		// extra warnings if fanCycles known now (can be runtime variable)

		// warn if constant volume outside air specified, cuz min const volume won't occur
		if ( oaMnCm==C_OAMNCH_VOL			// if outside air spec'd by volume (default) not fraction
				&&  (!ISNUM(oaMnFrac) || oaMnFrac != 0.)	// if minimum multplier is expression or non-0 (default 1.0)
				&&  oaVfDsMn > 0. )				// if outside flow air has not been set to 0 (default .15 area, above)
			oWarn( (char *)MH_S0627);	/* "When fan cycles, \"constant volume\" outside air works differently than you\n"
					   "    may expect: outside air will be supplied only when fan is on per zone\n"
					   "    thermostat.  To disable outside air use the command \"oaVfDsMn = 0;\"." */

		// warn if tu max flows not same as supply fan
		//   Runtime handles differences, but with constant values, difference is probably an error.
		//   Don't know whether system will heat, cool, or both, so just check given non-0 values.
		if ( sstat[AH_SFAN+FAN_VFDS] & FsVAL			// skip if sfan des flow not set (due to other error or autoSize)
				&&  ahCtu)						// skip if no control terminal (messaged just above)
		{
			TU *ctu = TuB.p + ahCtu;				// point control terminal's TU record.
			DBL sfanMx = sfan.vfDs * sfan.vfMxF;		// max supply fan flow, cfm, to display in error message
			DBL sfanHi = sfanMx * 1.001; 			// max supply fan flow plus roundoff allowance
			DBL sfanLo = sfanMx * .999				// min supply fan flow avail to terminal, less roundoff allowance,
						 * (1.0 - max( ahROLeak, ahSOLeak));	/* .. less generous allowance for duct leakage
         							   (currently AVERAGE of ROLeak, SOLeak used in cnah.cpp, 6-92) */
			if (ctu->sstat[TU_TUVFMXC] & FsVAL)   		// if constant (not expr) value given for tu max cooling flow
			{
				float tuMx = ctu->tuVfMxC;			// fetch the value.
				if (tuMx != 0.)					// if 0, probably no cooling intended.
					if (tuMx < sfanLo || tuMx > sfanHi)		/* if tu flow clearly different from sfan flow
							   even if user adjusted tu flow for ah duct leakage */
						oWarn( (char *)MH_S0628,		/* "Supply fan maximum flow (vfDs x vfMxF) is %g,\n"
							   "    but terminal '%s' max cooling air flow (tuVfMxC) is %g.\n"
							   "    Usually, these should be equal when fan cycles.\n"
							   "    The more limiting value will rule." */
							   sfanMx, ctu->name, tuMx );
			}
			if (ctu->sstat[TU_TUVFMXH] & FsVAL)   		// if constant (not expr) value given for tu max heating flow
			{
				float tuMx = ctu->tuVfMxH;				// fetch the value.
				if (tuMx != 0.)						// if 0, probably no heating intended.
					if (tuMx < sfanLo || tuMx > sfanHi)			/* if tu flow clearly different from sfan flow
								   even if user adjusted tu flow for ah duct leakage */
						oWarn( (char *)MH_S0629,		/* "Supply fan maximum flow (vfDs x vfMxF) is %g,\n"
							   "    but terminal '%s' max heating air flow (tuVfMxH) is %g.\n"
							   "    Usually, these should be equal when fan cycles.\n"
							   "    The more limiting value will rule." */
							   sfanMx, ctu->name, tuMx );
			}
		}

		// error if tuVfMn now known not 0: runtime would not know whether fraction of tuVfMxC or tuVfMxH.

		if (ahCtu)						// skip if no control terminal (messaged above)
		{
			TU *ctu = TuB.p + ahCtu;
			if ((ctu->sstat[TU_TUVFMN] & FsVAL) && ctu->tuVfMn > 0.)		// if > 0 constant given for terminal min flow
				oer( (char *)MH_S0630,			/* "Control terminal '%s':\n"
							   "    tuVfMn=%g: must be zero or omitted when fan cycles." */
					 ctu->name, ctu->tuVfMn );

#ifdef AUTOSIZE
			// error if tuVfMn autoSized when known that fan cycles. 7-95.
			/* related checks: AUTOSIZE tuVfMn & runtime-variable ahFanCyles: See TU::setup.
			                   AUTOSIZE tuVfMn, no ahFanCycles, variable ahTsSp sometimes ZN: believe no input check 8-28-95.
			   runtime: when fanCycles, autoSized tuVfMn left 0 (cnztu) but error if already non-0 (cnah1). */

			if (ctu->sstat[TU_TUVFMN] & FsAS)
				ctu->oer( "Can't give AUTOSIZE tuVfMn when air handler fan cycles:\n"
					 "    When fan cycles, terminal minimum flow must be zero, but \n"
					 "    AUTOSIZE tuVfMn makes minumum flow equal to maximum flow.");	// NUMS
#endif
		}
	}	// end if (now known that fan cycles)

	// setback/setup/warmup/optimum start
	// future

	// autoSizing coils 6-95 (fan stuff is in FAN::setup) (much coil stuff might well evolve to be in COILXXXX::setup... 7-1-95)

	if (rc)   return rc;					// so don't give confusing err msgs if something already bad

#if 0 // content gone 7-2-95
*   /* autosizing supply temp control method check  if will use autoSize part A const-temp model for this AH
*      -- tentatively if autoSizing fan or connected terminal, but not coils only, 6-95 */
*   if (asFlow)						// set in TU::setup and in FAN::setup
*      if (sstat[AH_AHTSSP] & FsVAL  &&  ISNCHOICE(ahTsSp) )	// if now set, to a choice (not a number)
*         switch (CHN(ahTsSp))				// must REPEAT CHECKS AT RUNTIME in case variable expr given!
*		  {
*	#if 0		// No, I think we should just let RA operate normally during sizing - no ah ts changes. Rob 7-2-95.
*    *       case C_TSCMNC_RA:  rc |= oer( "Cannot use \"ahTsSp = RA\" when autoSizing supply fan\n"
*    *                                           "    or any connected terminal.");
*    *                          break;
*	#endif
*           //case C_TSCMNC_WZ:    case C_TSCMNC_CZ:
*           //case C_TSCMNC_ZN2:   case C_TSCMNC_ZN:
*		  }
#endif

	if (sstat[AH_AHHC+AHHEATCOIL_CAPTRAT] & FsAS)		// if AUTOSIZE ahhcCaptRat given
		switch (ahhc.coilTy)
		{
		case C_COILTYCH_HW:
		case C_COILTYCH_GAS:
		case C_COILTYCH_OIL:
		case C_COILTYCH_ELEC:
		case C_COILTYCH_AHP:
			hcAs.az_active = TRUE;
			break;	// set runtime flag for implemented coil type(s)

		case C_COILTYCH_NONE:
			rc |= oer( "Cannot give \"AUTOSIZE ahhcCaptRat\""
					   " when air handler has no heat coil.");
			break;	// NUMS
		}

	if (sstat[AH_AHCC+COOLCOIL_CAPTRAT] & FsAS)		// if AUTOSIZE ahccCaptRat given
	{
		switch (ahcc.coilTy)
		{
		case C_COILTYCH_DX:      //rc |= oer( "Cannot (yet) autoSize DX coils.");  break;	// not impl 7-95. NUMS
		case C_COILTYCH_ELEC:
			ccAs.az_active = TRUE;
			break;	// set runtime flag for implemented coil type(s)

		case C_COILTYCH_CHW:
			rc |= oer( "Cannot autoSize CHW coils.");
			break;		// not impl 7-95. NUMS

		case C_COILTYCH_NONE:
			rc |= oer( "Cannot give \"AUTOSIZE ahccCaptRat\""
					   " when air handler has no cool coil.");
			break;	// NUMS
		}
		if (sstat[AH_AHCC+COOLCOIL_CAPSRAT] & FsSET)
			rc |= oer( "Cannot give 'capsRat' when autoSizing captRat.");			// NUMS
	}

	if (!rc)					// skip if error above cuz ccAs.az_active etc might not be correct
	{
		if (IsSet( AH_FXCAPC))			// if  coilOversize = fraction  given
			if (!ccAs.az_active)
				oer( "Cannot give 'ahFxCapC' when not autoSizing cooling coil.");	// NUMS
		if (IsSet( AH_FXCAPH))			// if  coilOversize = fraction  given
			if (!hcAs.az_active)
				oer("Cannot give 'ahFxCapH' when not autoSizing heating coil.");	// NUMS
		if (IsSet( AH_FXVFFAN))			// if  ahFxVfFan = fraction  given
			if (!fanAs.az_active)
				oer( "Cannot give 'fanOversize' when not autoSizing fan.");	// NUMS
	}

	// may need addl check for autosizing control terminal and ah together, 6-95.


	//-------------------------------- air handler setup ---------------------------------

	// set up above: TU[ahCtu].ctrlsAi, .

	// frFanOn = 1.0;   is done hourly in cnah.cpp:AH::begHour, if !fcc.

	if (rc)   return rc;					// don't attempt setup after errors

	// expect there to be subexprs to compute eg about leaks and losses...

	ahTsSpPr = -999.;			// for correct self-init even if ahTsSp=0 specified (mimicing ahTsMn), 6-23-92.

	// set up change flag setting by expression manager -- makes table entries if given fields contain non-constant expressions

	// many more, get specific flags to set as coded... outside air, ...

	// set air handler "must-compute" flag on changes in these fields
	chafSelf( AH_AHPTF,  AH_AHTSSP, AH_AHHC + COIL_SCHED, AH_AHCC + COOLCOIL_SCHED, 0);

	return rc;					// additional returns above including E macro
}			// AH::setup
//============================================================================================================================
RC CCH::setup( 			// check/initialize a CRANKCASE HEATER subrecord
	AH* ah, 		// airHandler containing the cch. generalize if necessary; ahhc.ty must then be passed as an arg.
	SI cchFn )		// field number of crankcase heater subrecord in air handler record

// call in AH::setup before coils setups
{
//uncomment as need found
//#define REQUIRESCCH(subFn) r->require( cchFn+CCH_##subFn)		// issue error message if field of subrecord not given
//#define ERSCCH(subFn)      ah->ooer( cchFn+CCH_##subFn,   		// subrecord field err msg: addl arg(s) and ) must follow
#define ERS2CCH(f1,f2)     ah->ooer( cchFn+CCH_##f1, cchFn+CCH_##f2,  	// subrecord 2-field error message ditto
#define R ((record *)ah)

	RC rc=RCOK;
	UCH* fss = (UCH *)R + R->b->sOff + cchFn;		// start of field status bytes in record for CCH members

//--- default control method according to ah coil type(s)

	if (!(fss[CCH_CCHCM] & FsSET))					// if cch control method not specified by caller
		cchCM = ah->ahhc.coilTy==C_COILTYCH_AHP  ?  C_CCHCM_PTC_CLO 	// default to PTC_CLO if ah has AHP (air heat pump) heat coil
				:  C_CCHCM_NONE;  	// else default cm to NONE

//--- if no crankcase heater, disallow all members, return

	CCHCM cm = cchCM;
	if (cm==C_CCHCM_NONE)
	{
		for (SI subFn = 0;  subFn < CCH_NFIELDS;  subFn++)  		// for all members of cch subrecord
			if (fss[subFn] & FsSET)   					// if member given in input
			{
				if (subFn==CCH_CCHCM && cm==C_CCHCM_NONE)  		// explicit "none"
					continue;						// always ok
				rc = R->cantGiveEr( cchFn+subFn, (char *)MH_S0631);	// "when cchCM is NONE"
			}
		return rc;
	}

//--- appropriateness warning vs heat and cool types

	BOO cchOk = FALSE;
	switch (ah->ahcc.coilTy)
	{
		//case C_COILTYCH_xxx:			// AHP cooling function -- if coil type added for it
	case C_COILTYCH_DX:			// cch might be appropriate
		cchOk++;
		break;
	}
	switch(ah->ahhc.coilTy)
	{
		//case C_COILTYCH_xxx ...			add appropriate types
	case C_COILTYCH_AHP:
		cchOk++;
		break;	// cch appropriate with air source heat pump!
	}
	if (!cchOk)
		ah->oWarn( (char *)MH_S0632);		/* "Crankcase heater specifed, but neither heat coil nor cool coil \n"
					   "    is a type for which crankcase heater is appropriate" */

//--- checks for PTC

	if (cm==C_CCHCM_PTC || cm==C_CCHCM_PTC_CLO)
	{
		if (pMx <= pMn)  ERS2CCH(PMX,PMN) (char *)MH_S0633, pMx, pMn);		// "cchPMx (%g) must be > cchPMn (%g)"
			if (tMx > tMn)   ERS2CCH(TMX,TMN) (char *)MH_S0634, tMx, tMn);		// "cchTMx (%g) must be <= cchTMn (%g)"
		}
else
	ah->disallowN( (char *)MH_S0635,  					// "when cchCM is not PTC nor PTC_CLO"
				   cchFn+CCH_PMN,  cchFn+CCH_TMX,  cchFn+CCH_TMN,  cchFn+CCH_DT,  0 );

//--- checks/defaults for TSTAT

	if (cm==C_CCHCM_TSTAT)
	{
		if (!(fss[CCH_TOFF]))  tOff = tOn;					// if tOff not given by user, default it to tOn.
		if (tOff < tOn)  ERS2CCH(TOFF,TON) (char *)MH_S0636, tOff, tOn);   	// "cchTOff (%g) must be > cchTOn (%g)"
		}
	else
		ah->disallowN( (char *)MH_S0637,  cchFn+CCH_TOFF,  cchFn+CCH_TON,  0 );  	// "when cchCM is not TSTAT"


//--- setup heater energy use during ARI tests (Niles V-K-5-a)

	//p47Off, p47On: 		cch power in during off [and on] parts of ARI cycling test. p47On not used in rob's ahp code.
	//p47, p17:			cch power inputs during continuous 47 and 17 degree operation.
	switch(cm)			// switch sets non-0 values (record is initially all 0)
	{
	case C_CCHCM_CONSTANT:
		p47Off = /*p47On=*/ p47 = p17 = pMx;
		break;	// input is always pMx
	case C_CCHCM_CONSTANT_CLO:
		p47Off = pMx;
		break;	// pMx when compr off, 0 when on
	case C_CCHCM_PTC:
		p47Off = /*p47On=*/ p47 = p17 = pMn;
		break;	// input pMn when running or cycling
	case C_CCHCM_PTC_CLO:
		p47Off = pMn;
		break;	// pMn when off part subhr (cycling), 0 when on
	case C_CCHCM_TSTAT:
		break;					// 0 (preset) when running or cycling.
#if 0//looks wrong
x      default:	return ooer( cchCM, (char *)MH_S0638, (INT)cm);		// "Internal error: Bad cchCM %d"
#else//7-95. untested.
	default:
		return ah->ooer( cchFn+CCH_CCHCM, (char *)MH_S0638, (INT)cm); 	// "Internal error: Bad cchCM %d"
#endif
	}
	//convert power to energy for energy members: restore if needed.
	//e47Off = p47Off * tmCycOff;    	// tmCycOff is a #define in AHHEATCOIL in rccn.h
	//e47On  = p47On * tmCycOn;	   	// tmCycOn  is a #define in AHHEATCOIL in rccn.h

	return rc;

#undef REQUIRESCCH
#undef ERSCCH
#undef ERS2CCH
#undef IDSCCH
}			// CCH::setup
//============================================================================================================================
RC COIL::setup(		// check/set up a base class coil subrecord

	COILAPPCH app,		// application; drives checks & defaults, former member: C_COILAPPCH_TUHC, _AHHC, _AHCC.
	record *r,			// record containing COIL.  Assumed to be AH if a DX coil.
	SI coilFn, 			// field number of COIL subrecord in r
	BOO rqd, BOO prohib )	// non-0 if required or disallowed, respectively, else optional, terminal coil only

// see also COOLCOIL::setup, HEATCOIL::setup, and AHHEATCOIL::setup, below. They call this.
// "COIL" remains in application only as base class for COOLCOIL, HEATCOIL, AHHEATCOIL.
{
#define REQUIRESC(subFn) r->require( coilFn+COIL_##subFn)		// issue error message if field of subrecord not given
#define ERSC(subFn)      r->ooer( coilFn+COIL_##subFn,   		// subrecord field err msg: addl arg(s) and ) must follow
#define ERS2C(f1,f2)     r->ooer( coilFn+COIL_##f1, coilFn+COIL_##f2,  	// subrecord 2-field error message ditto
#define IDSC(subFn)      r->mbrIdTx( coilFn+COIL_##subFn)		// subrecord field name, from CULT search for polymorphism

	RC rc = RCOK;
	UCH* fss = (UCH *)r + r->b->sOff + coilFn;		// start of field status bytes in r for COIL members
	COILTYCH ty = coilTy;

// check app (now arg, was member preset by CULT), determine if have this coil, do defaults dependent on type and presence

	switch (app)
	{
	case C_COILAPPCH_TUHC:
		if ( !(fss[COIL_COILTY] & FsSET)		// if no coil type input
				&&  rqd )				// but terminal needs one (has local heat)
			ty = coilTy = C_COILTYCH_ELEC;	// default to electric coil
		break;					/* terminal coil may disallow many inputs (eg auxOn)
		       						   by having no CULT entries for them */
	case C_COILAPPCH_AHCC:
	case C_COILAPPCH_AHHC:
		break;		// (ahcc, ahhc: omitted coilTy means absent coil)

	default:
		return r->oer( (char *)MH_S0639, (UI)app); 		// "Internal error: cncult5.cpp:ckCoil: bad coil app 0x%x"
	}

// return if coil not present or not allowed

	if ( ty==C_COILTYCH_NONE					// if no coil specified
			||  prohib )						// or context disallows coil (terminal with no lh)
		return rc;

//--- aux energy drain checks: power without meter accumulates to AHRES pAuxC/H only; meter without power warns here.

#define MTRWRN( mtri, pow)    if (fss[COIL_##mtri] & FsSET)  if (!(fss[COIL_##pow] & FsSET)) \
			             r->oWarn( (char *)MH_S0640, IDSC(mtri), IDSC(pow));
		// "'%s' given, but no '%s' to charge to it given"
	MTRWRN( AUXMTRI, AUX);
#undef MTRWRN

	return rc;					// more returns above!

#undef REQUIRESC
#undef ERSC
#undef ERS2C
#undef IDSC
}		// COIL::setup
//============================================================================================================================
RC COOLCOIL::setup(				// check/set up a cooling coil subrecord: has additional inputs

	COILAPPCH app,		// application; drives checks & defaults, former member: C_COILAPPCH_TUHC, _AHHC, _AHCC.
	record *r,			// record containing COOLCOIL, assumed by some code to be AH
	SI coolCoilFn,		// field number of COOLCOIL subrecord in r
	BOO rqd, BOO prohib )	// non-0 if required or disallowed, respectively, else optional, terminal coil only

	// coil application is determined by testing 'app' member.
	// error message texts here assume app is _AHCC and type mbr name is 'ahccType' (search "ahccType" for places to generalize).
	// should be done before topCp2, which uses COOLPLANT.ah1 and .mwDsCoils, which are set here.
{
#define AHR ((AH*)r)
#define REQUIRESCC(subFn) r->require( coolCoilFn+COOLCOIL_##subFn)		// issue error message if field of subrecord not given
#define ERSCC(subFn)      r->ooer( coolCoilFn+COOLCOIL_##subFn,   		// subrecord field err msg: addl arg(s) and ) must follow
#define ERS2CC(f1,f2)     r->ooer( coolCoilFn+COOLCOIL_##f1, coolCoilFn+COOLCOIL_##f2,  	// subrecord 2-field error message ditto
#define IDSCC(subFn)      r->mbrIdTx( coolCoilFn+COOLCOIL_##subFn)		// subrecord field name, from CULT search for polymorphism

// default cubic polynomial coefficients: array of 4 floats. uses r, fss, coolCoilFn.
#define DFLCUBIC( subFn, k0,k1,k2,k3 )			\
    if (!(fss[COOLCOIL_##subFn+PYCUBIC_K] & FsSET))  	\
    {  static float dat[]={k0,k1,k2,k3};  		\
       memcpy( r->field(coolCoilFn+COOLCOIL_##subFn+PYCUBIC_K), dat, sizeof(dat) );	\
    }
// default biquadratic polynomial coefficients: array of 6 floats. uses r, fss, coolCoilFn.
#define DFLBIQUAD( subFn, k0,k1,k2,k3,k4,k5 )		\
    if (!(fss[COOLCOIL_##subFn+PYBIQUAD_K] & FsSET))	\
    {  static float dat[]={k0,k1,k2,k3,k4,k5};  	\
       memcpy( r->field(coolCoilFn+COOLCOIL_##subFn+PYBIQUAD_K), dat, sizeof(dat) ); 	\
    }

	RC rc /*=RCOK*/;

//--- first setup the general coil which is base class of COOLCOIL

	CSE_E( COIL::setup( app, r, coolCoilFn, rqd, prohib))

//--- if coil not specified, issue error for each member given, return

	UCH* fss = (UCH *)r + r->b->sOff + coolCoilFn;		// start of field status bytes in r for COOLCOIL members
	COILTYCH ty = coilTy;
	if ( ty==C_COILTYCH_NONE					// if no coil specified
			/*||  prohib*/ )						// [or context disallows coil]
	{
		for (SI subFn = 0;  subFn < COOLCOIL_NFIELDS;  subFn++) 	// for all members of coolcoil subrecord
			if (fss[subFn] & FsSET)   				// if member given in input
#if 1 // rob 10-95 for .471 per Bruce email 10-17-95
				if (subFn != COOLCOIL_COILTY)			// don't say "can't give ahccType when ahccType is NONE or omitted"
#endif
					rc = r->cantGiveEr( coolCoilFn + subFn,
										(char *)MH_S0641 );		// "when ahccType is NONE or omitted"
		/* <-- generalize above subtext if used for other coils
		   (test app -- for speed, avoid mbrIdTx's
		   & sprintf's executed b4 error detected) */
		CHN(sched) = C_OFFAVAILVC_OFF;		// set absent coil OFF (this is run record) for appl code. ASSUMED in cnah.cpp.
		return rc;
	}

//--- additional checks for present cooling coil

// check coil type. If here, type known given or already defaulted.

	//switch(app) if multiple apps get here
	switch(ty)
	{
	case C_COILTYCH_DX:
	case C_COILTYCH_CHW:
	case C_COILTYCH_ELEC:
		break;
		//badCoilTy:
	default:
		rc = ERSCC(COILTY) (char *)MH_S0642, 					// "Bad coil type '%s' for %s"
		r->getChoiTx( coolCoilFn+COOLCOIL_COILTY, 1, ty),
			IDSCC(COILTY) );
		return rc;			// don't continue with bad type
	}

// non-CHW coil: check/init capacities. CHW does not use capacity inputs.

	if (ty != C_COILTYCH_CHW)
	{
		// all but CHW require total capacity, captRat.
		rc |= REQUIRESCC(CAPTRAT);			// <--- generalize type member name if this used for other coils!
		if (fss[COOLCOIL_CAPTRAT] & FsVAL)		// not if omitted or FsAS: NAN's!
		{
			if (ty != C_COILTYCH_ELEC && !r->IsSet(coolCoilFn + COOLCOIL_CAPSRAT))			// all but CHW, ELEC req sens cap 'capsRat' unless captRat autoSized.
			{
				capsRat = SHRRat*captRat;
			}
			captRat = -fabs(captRat);  			// be sure rated capacities are stored negative
			capsRat = -fabs(capsRat);			// ..
			// nan not expected for capsRat: constant, not autoSizable, 7-95.
			if (capsRat < captRat)   					// negatives!
				rc = ERS2CC( CAPSRAT, CAPTRAT) (char *)MH_S0643, 		// "Sensible capacity '%s' larger than Total capacity '%s'"
				IDSCC(CAPSRAT), IDSCC(CAPTRAT) );
		}
	}

		if (rc)  return rc;						// do not continue with missing members

#if 0 // uses gone to cncoil.cpp 7-95
x     // some common stuff
x     BOO vfRgiven = fss[COOLCOIL_VFR] & FsSET;
x #ifdef AUTOSIZE
x     float fanVfR = (AHR->sstat[AH_SFAN + FAN_VFDS] & FsVAL) 	// if supply fan cfm set to a value
x 		         ?  AHR->sfan.vfDs			// get supply fan cfm, for vfR default or check
x 		    	 :  0.;					// else 0 (eg if setup calls rearranged or autoSize)
x #else
x x    float fanVfR = (AHR->sstat[AH_SFAN + FAN_VFDS] & FsSET) ?  AHR->sfan.vfDs	// get supply fan cfm, for vfR default or check
x x    							    :  0.;		// if unset use 0 (eg if setup calls rearranged)
x #endif
#endif

// CHW coil checks/setup

	if (ty==C_COILTYCH_CHW)
	{
		// inputs required for CHW coil
		rc |= r->requireN( (char *)MH_S0644,			// "when ahccType is CHW"
						   coolCoilFn + COOLCOIL_CPI,		// ahccCoolplant
						   coolCoilFn + COOLCOIL_GPMDS,		// ahccGpmDs
						   0 );
		if (rc)  return rc;					// do not proceed e.g. with missing ahccCoolplant.

			// units conversions
#define LBperGAL 8.337 			// @ 62 F, CRC handbook. Note cons.cpp shows 8.3454 -- for 39F?. also in cncult6.cpp.
		mwDs = gpmDs * LBperGAL * 60.;		// convert gallons-per-minute into pounds per hour for design water flow

		// coolplant & chaining
		COOLPLANT *cp;
		if (ckRefPt( &CpB, r, cpi, "ahccCoolplant", NULL, (record **)&cp)==RCOK)	// <---- generalize mbr nm if others get here
		{
			nxAh4cp = cp->ah1;
			cp->ah1 = AHR->ss;		// chain ah's w chw coils to their coolplant
			cp->mwDsCoils += mwDs;				/* accumulate coil design water flow for COOLPLANT
									(re check for too much coil flow for pumps) */
		}

		// inputs NOT allowed with CHW coil
		r->disallowN( (char *)MH_S0645,				// "when ahccType is CHW"
						coolCoilFn + COOLCOIL_CAPTRAT,		// inputs allowed for most coil types, not CHW
						coolCoilFn + COOLCOIL_CAPSRAT,
						coolCoilFn + COOLCOIL_SHRRAT,
						coolCoilFn + COOLCOIL_MTRI,		// cuz COOLPLANT posts the input energy to meter
						0 );
	}
	else					/* coilTy != C_COILTYCH_CHW.  issue error messages for CHW-specific inputs,
							last (low priority) as these are relatively uninformative messages. */
	{
		r->disallowN( (char *)MH_S0646,			// "when ahccType is not CHW"
						// <--- generalize above type name if this used for other coils!
						coolCoilFn + COOLCOIL_CPI,
						coolCoilFn + COOLCOIL_GPMDS,
						coolCoilFn + COOLCOIL_NTUIDS,
						coolCoilFn + COOLCOIL_NTUODS,
						//coolCoilFn + COOLCOIL_K1,		disallowed below if not CHW or DX
						//coolCoilFn + COOLCOIL_VFR,		disallowed below if not CHW or DX
						//coolCoilFn + COOLCOIL_DSTDBEN,		disallowed below if not CHW or DX
						0 );
	}

// DX coil checks/setup

	if (ty==C_COILTYCH_DX)
	{
		if (minFsldPlr > minUnldPlr)
			rc = ERS2CC( MINFSLDPLR, MINUNLDPLR) "%s (%g) > %s (%g)", IDSCC(MINFSLDPLR), IDSCC(MINUNLDPLR) );	// error msg

		if (!(fss[COOLCOIL_MINFSLDPLR] & FsSET))
			minFsldPlr = minUnldPlr;		// default to no false-loading range
		//eirMinUnldPlr = pydxEirUl->val(minUnldPlr);				is below, after pydxEirUl dflt'd & ck'd

		// note CULT table defaults k1 to -.4 (value from verbal communication from Niles 6-5-92, also in spec for DX,CHW).

		// disallow both ahccVfR and ahccVfRperTon, 8-28-95
		if (fss[COOLCOIL_VFR] & FsSET && fss[COOLCOIL_VFRPERTON] & FsSET)
			rc = r->oer( (char *)MH_S0652, 					// "Cannot give both %s and %s" NUMS no, new use.
										IDSCC(VFR), IDSCC(VFRPERTON) );

		// Use same assumptions as RSYS to default if vfRperTon is set. Otherwise, use CULT default (0.77)
		if (!r->IsSet(coolCoilFn + COOLCOIL_SHRRAT) && r->IsSet(coolCoilFn + COOLCOIL_VFRPERTON))
		{
			SHRRat = CoolingSHR(dsTDbCnd ,dsTDbEn, dsTWbEn, vfRperTon);
		}

			// disallow ahccVfR when autoSizing, 8-28-95.
			// Note Bruce says ok to completely remove ahccVfR for DX, 8-95.
			if ( fss[COOLCOIL_VFR] & FsSET				// if ahccVfR given
			&&  fss[COOLCOIL_CAPTRAT] & FsAS 			// if autoSizing ahccCaptRat
			&&  AHR->sstat[AH_SFAN+FAN_VFDS] & FsAS ) 		// and autoSizing supply fan (tentative 8-95)
				rc = r->oer( "Cannot give %s when autoSizing. Use %s.",		// NUMS
								IDSCC(VFR), IDSCC(VFRPERTON) );

			// pyDxCaptFLim must be >= 1, 8-28-95
			if (pydxCaptFLim < 1.0)			// capacity(flow) poly upper limit, default 1.05
				rc = r->oer( "%s must be >= 1.", 			// NUMS (?)
								IDSCC(PYDXCAPTFLIM) );

		// default eirRat for DX coil
		if (!(fss[COOLCOIL_EIRRAT] & FsSET))  eirRat = .438;	// .438: DOE2 PTAC Energy Input Ratio value, per Niles

						// default the dx polynomials.  Values are DOE2 "PTAC" values as shown in Niles' Program Spec Document.

						DFLBIQUAD( PYDXCAPTT, 1.1839345f,  -.0081087f,   .00021104f, -.0061425f,   .00000161f, -.0000030f  );
						//DFLBIQUAD( PYDXCAPST, 6.3112709,  -.1129951,   .00043336,  .00377381, -.0000499,   .00006375 ); 	deleted 5-29-92
						DFLCUBIC(  PYDXCAPTF,  .8f,         .2f,        0.f,         0.f          );
						//DFLCUBIC(  PYDXCAPSF,  .6,         .4,        0.,         0.          );           		deleted 5-29-92
						DFLBIQUAD( PYDXEIRT,  -.6550461f,   .03889096f, -.0001925f,   .00130464f,  .00013517f, -.0002247f  );
						DFLCUBIC(  PYDXEIRUL,  .125f,       .875f,      0.f,         0.f          );

						// maybe later optimize DX polynomials -- to code

						// normalize dx coil polynomial coefficients and/or issue error messages if values not 1.0 for inputs that should produce 1.0.

						rc |=   pydxCaptT.normalizeCoil( (AH *)r, coolCoilFn + COOLCOIL_PYDXCAPTT);
								//rc |= pydxCapsT.normalizeCoil( (AH *)r, coolCoilFn + COOLCOIL_PYDXCAPST);
								rc |=   pydxCaptF.normalize( r, coolCoilFn + COOLCOIL_PYDXCAPTF, "relative flow", 1.0);
										//rc |= pydxCapsF.normalize( r, coolCoilFn + COOLCOIL_PYDXCAPSF, "relative flow", 1.0);		might want to restore
										rc |=   pydxEirT.normalizeCoil(  (AH *)r, coolCoilFn + COOLCOIL_PYDXEIRT);
												rc |=   pydxEirUl.normalize( r, coolCoilFn + COOLCOIL_PYDXEIRUL, "relative load", 1.0);   // functions below in cncult5.cpp.
																																}
	else	/* coilTy != C_COILTYCH_DX.  issue error messages for DX-specific inputs,
    						   last (low priority) as these are relatively uninformative messages. */
	{
	   r->disallowN( (char *)MH_S0648,			// "when ahccType is not DX"
			   // <--- generalize above type member name if this used for other coils!
			   coolCoilFn + COOLCOIL_MINTEVAP,
			   //coolCoilFn + COOLCOIL_K1,		disallowed below if not CHW or DX
			   //coolCoilFn + COOLCOIL_VFR,		disallowed below if not CHW or DX
			   coolCoilFn + COOLCOIL_VFRPERTON,		// 8-28-95
			   //coolCoilFn + COOLCOIL_DSTDBEN,		disallowed below if not CHW or DX
			   coolCoilFn + COOLCOIL_DSTDBCND,
			   coolCoilFn + COOLCOIL_DSTWBEN,
			   coolCoilFn + COOLCOIL_EIRRAT,		// (base class member, also allowed for some heat coil types)
			   coolCoilFn + COOLCOIL_MINUNLDPLR,
			   coolCoilFn + COOLCOIL_MINFSLDPLR,
			   coolCoilFn + COOLCOIL_PYDXCAPTT + PYBIQUAD_K,
			   coolCoilFn + COOLCOIL_PYDXCAPTF + PYCUBIC_K,
			   coolCoilFn + COOLCOIL_PYDXCAPTFLIM,
			   coolCoilFn + COOLCOIL_PYDXEIRT  + PYBIQUAD_K,
			   coolCoilFn + COOLCOIL_PYDXEIRUL + PYCUBIC_K,
			   0 );
	}

// members disallowed if neither DX nor CHW (ie if ELEC)

	if (ty != C_COILTYCH_DX  &&  ty != C_COILTYCH_CHW)
	   r->disallowN( (char *)MH_S0649,			// "when ahccType is not DX nor CHW"
			   // <--- generalize above type mbr name if other coils get here!
			   coolCoilFn + COOLCOIL_K1,
			   coolCoilFn + COOLCOIL_VFR,
			   coolCoilFn + COOLCOIL_DSTDBEN,
			   0 );

	return rc;

#undef AHR
#undef REQUIRESCC
#undef ERSCC
#undef ERS2CC
#undef IDSCC
#undef DFLCUBIC
#undef DFLBIQUAD
}		// COOLCOIL::setup
//============================================================================================================================
RC HEATCOIL::setup(				// check/set up a heating coil subrecord: has additional inputs

	COILAPPCH app,		// application; drives checks & defaults, former member: C_COILAPPCH_TUHC, _AHHC, _AHCC.
	record *r,			// record containing HEATCOIL.  May be assumed by some code to be AH?
	SI heatCoilFn,		// field number of HEATCOIL subrecord in r
	BOO rqd, BOO prohib,	// non-0 if required or disallowed, respectively, else optional, terminal coil only
	BOO isAhh ) 		// non-0 if called AHHEATCOIL::setup: allow AHP.

// for air handler heat coil, terminal local heat coil. coil application is determined by testing 'app' member.
// (tuhc here 9-24-92; watch for not-yet-generalized code and error messages)
// topHp1 should be done b4 this fcn (cuz here we chain HW coils to their HEATPLANTs)
{
//uncomment as needed
//#define AHR ((AH*)r)
#define REQUIRESHC(subFn) r->require( heatCoilFn+HEATCOIL_##subFn)		// issue error message if field of subrecord not given
#define ERSHC(subFn)      r->ooer( heatCoilFn+HEATCOIL_##subFn,   		// subrecord field err msg: addl arg(s) and ) must follow
#define ERS2HC(f1,f2)     r->ooer( heatCoilFn+HEATCOIL_##f1, heatCoilFn+HEATCOIL_##f2,  	// subrecord 2-field error message ditto
#define IDSHC(subFn)      r->mbrIdTx( heatCoilFn+HEATCOIL_##subFn)		// subrecord field name, from CULT search for polymorphism

// default cubic polynomial coefficients: array of 4 floats. uses r, fss, heatCoilFn.
#define DFLCUBIC( subFn, k0,k1,k2,k3 )			\
    if (!(fss[HEATCOIL_##subFn+PYCUBIC_K] & FsSET))  	\
    {  static float dat[]={k0,k1,k2,k3};  		\
       memcpy( r->field(heatCoilFn+HEATCOIL_##subFn+PYCUBIC_K), dat, sizeof(dat) );	\
    }
//// default biquadratic polynomial coefficients: array of 6 floats. uses r, fss, heatCoilFn.
//#define DFLBIQUAD( subFn, k0,k1,k2,k3,k4,k5 )		\
//    if (!(fss[HEATCOIL_##subFn+PYBIQUAD_K] & FsSET))	\
//    {  static float dat[]={k0,k1,k2,k3,k4,k5};  	\
//       memcpy( r->field(heatCoilFn+HEATCOIL_##subFn+PYBIQUAD_K), dat, sizeof(dat) ); 	\
//    }

	RC rc /*=RCOK*/;
	char *when = "bug";			// for various error message subtexts per coil application. cannot be MH.

//--- first setup the general coil which is base class of HEATCOIL

	CSE_E( COIL::setup( app, r, heatCoilFn, rqd, prohib))

//--- if coil not specified, issue error for each member given, return

	UCH* fss = (UCH *)r + r->b->sOff + heatCoilFn;		// start of field status bytes in r for HEATCOIL members
	COILTYCH ty = coilTy;
	if ( ty==C_COILTYCH_NONE					// if no coil specified
			||  prohib )						// or context disallows coil
	{
		for (SI subFn = 0;  subFn < HEATCOIL_NFIELDS;  subFn++)  		// for all members of heatcoil subrecord
			if (fss[subFn] & FsSET)   						// if member given in input
			{
				if (subFn==HEATCOIL_COILTY && ty==C_COILTYCH_NONE)  		// explicit "none"
					continue;							// always ok even when prohibited (tu w/o lh)
				switch (app)							// get subtext for following error message
				{
				case C_COILAPPCH_AHHC:
					when = "when ahhcType is NONE or omitted";
					break;
				case C_COILAPPCH_TUHC:
					when = "when tuhcType is NONE or omitted";
					break;
				}
				rc = r->cantGiveEr( heatCoilFn + subFn, when );
			}
		CHN(sched) = C_OFFAVAILVC_OFF;	// set absent/disallowed coil OFF (this is run record) for appl code. ASSUMED in cnah.cpp.
		return rc;
	}

//--- additional checks for present heating coil

// check coil type: application-dependent. If here, type known given or already defaulted.

	switch (app)
	{
	case C_COILAPPCH_AHHC:
		if (isAhh) if (ty==C_COILTYCH_AHP)   break;	// allow AHP in AHHEATCOIL only
		if (ty==C_COILTYCH_GAS)   break;
		if (ty==C_COILTYCH_OIL)   break;
		if (ty==C_COILTYCH_HW)    break;
		if (ty==C_COILTYCH_ELEC)  break;
		goto badCoilTy;

	case C_COILAPPCH_TUHC:
		if (ty==C_COILTYCH_HW)    break;
		if (ty==C_COILTYCH_ELEC)  break;

	badCoilTy:
		return ERSHC(COILTY) (char *)MH_S0650, 					// "Bad coil type '%s' for %s"
			   r->getChoiTx( heatCoilFn+HEATCOIL_COILTY, 1, ty),
				IDSHC(COILTY) );
	}

// checks common to all coil types but AHP (AHP checks are done by caller AHHEATCOIL::setup)

	if (ty != C_COILTYCH_AHP)
	{

		// check for required members given

		rc |= REQUIRESHC(CAPTRAT);				// all heat coils but AHP require total capacity input
		if (rc)  return rc;					// do not continue with missing members or bad type

		// heating coil rated capacity should be > 0

		if (fss[COIL_CAPTRAT] & FsVAL)     			// expression input may be accepted for heating coils
			if (captRat <= 0.)
				rc |= ERSHC(CAPTRAT) (char *)MH_S0651, captRat);	// "Heating coil rated capacity 'captRat' %g is not > 0"

				// disallow giving both efficiency and eir (could accept if consistent, but eir can be expr, too much bother).

				if (fss[HEATCOIL_EIRRAT] & FsSET)   				// if eirRat (ahhcEirR) given (expression allowed)
					if (fss[HEATCOIL_EFFRAT] & FsSET)				// if effRat (ahhcEffR) also given
						ERS2HC( EIRRAT, EFFRAT) (char *)MH_S0652, 			// "Cannot give both %s and %s"
						IDSHC(EIRRAT), IDSHC(EFFRAT) );
					}

	// electric heat coil additional input checks

	if (ty==C_COILTYCH_ELEC)
	{
		switch (app)								// get subtext for following error message
		{
		case C_COILAPPCH_AHHC:
			when = "when ahhcType is ELECTRIC";
			break;	// when is sprintf arg: MH not supported!
		case C_COILAPPCH_TUHC:
			when = "when tuhcType is ELECTRIC";
			break;
		}
		if (fss[HEATCOIL_EIRRAT] & FsSET)					// error if eirRat given
			if ( !(fss[HEATCOIL_EIRRAT] & FsVAL)					// and value is not constant
					||  eirRat != 1.0 )							//     or not 1.0
				ERSHC(EIRRAT) (char *)MH_S0653, when, IDSHC(EIRRAT) );		// "%s, %s must be constant 1.0 or omitted"
				if (fss[HEATCOIL_EFFRAT] & FsSET)					// if effRat given (CULT requires constant)
					if (effRat != 1.0)							// error if value not 1.0
					ERSHC(EFFRAT) (char *)MH_S0654, when, IDSHC(EFFRAT) );		// "%s, %s must be 1.0 or omitted"
					// CULT defaults eirR to 1.0, and code doesn't use it anyway 7-92, so no need to default here.
	}

	// gas furnace/oil furnace additional defaults and checks

	if (ty==C_COILTYCH_GAS || ty==C_COILTYCH_OIL)
	{
		switch (app)								// get subtext for following error message
		{
		case C_COILAPPCH_AHHC:
			when = "when ahhcType is GAS or OIL";
			break;
		case C_COILAPPCH_TUHC:
			when = "when tuhcType is GAS or OIL";
			break;
		}

		// rated load energy input ratio, or its alternate specification, efficiency
		if (fss[HEATCOIL_EIRRAT] & FsSET)				// if eirRat (ahhcEirR) given (expression allowed)
		{
			if (fss[HEATCOIL_EIRRAT] & FsVAL)				// if constant given, can check now
				if (eirRat < 1.0)						// (runtime check also needed in case expr given)
					ERSHC(EIRRAT) (char *)MH_S0655, IDSHC(EIRRAT) );  	// "%s must be >= 1.0"
					//if eff also given, error issued above.
		}
		else						// eir not given
		{
			if (!(fss[HEATCOIL_EFFRAT] & FsSET))		// if eff also not given
			{
				// use eirR 1.0 defaulted by CULT?
				// or default eirR to ??
				// error message: either eir or eff required:
				ERS2HC( EIRRAT, EFFRAT) (char *)MH_S0656,   			// "%s,\n    Either %s or %s must be given"
					  when, IDSHC(EIRRAT), IDSHC(EFFRAT) );
			}
			else if (effRat > 1.0)				// redundant: field limit should check for <= 1.
				ERSHC(EFFRAT) (char *)MH_S0657, IDSHC(EFFRAT) ); 	// "%s must be <= 1.0"
				else if (effRat <= 0.)     				// /0 insurance: field limits should check for > 0.
				ERSHC(EFFRAT) (char *)MH_S0658, IDSHC(EFFRAT) ); 	// "%s must be > 0."
				else
					eirRat = 1./effRat;
						 }

			 // default and check polynomial for adjusting energy input for less than full load
			 DFLCUBIC( PYEI, .01861f, 1.094209f, -.112819f, 0.f );	// store default coefficients if user gave none. DOE2 values.
				 rc |= pyEi.normalize( r, heatCoilFn + HEATCOIL_PYEI, "relative load", 1.0);	// fix and/or msg if py(1.0) != 1.0

	}
	else					// not gas furnace nor oil furnace. issue msgs for inputs allowed for them only.
	{
		switch (app)								// get subtext for following error message
		{
		case C_COILAPPCH_AHHC:
			when = "when ahhcType is not GAS or OIL";
			break;
		case C_COILAPPCH_TUHC:
			when = "when tuhcType is not GAS or OIL";
			break;
		}
		r->disallowN( when, //heatCoilFn + HEATCOIL_EIRRAT,		no, permit 1.0 with electric
					  //heatCoilFn + HEATCOIL_EFFRAT,		no, permit 1.0 with electric
					  heatCoilFn + HEATCOIL_PYEI + PYCUBIC_K,
					  heatCoilFn + HEATCOIL_STACKEFFECT,
					  0 );
	}

	// HW coil additional checks and defaults

	if (ty==C_COILTYCH_HW)
	{
		char *hpiMbrName = nullptr;
		switch (app)								// get subtexts for following error messages
		{
		case C_COILAPPCH_AHHC:
			when = "when ahhcType is HW";
			hpiMbrName = "ahhcHeatplant";
			break;
		case C_COILAPPCH_TUHC:
			when = "when tuhcType is HW";
			hpiMbrName = "tuhcHeatplant";
			break;
		}

		// CAUTION table (cncult.cpp) may accept expressions for captRat.
		// note code above has required captRat and checked for > 0.

		r->disallowN( when, heatCoilFn + HEATCOIL_MTRI,		// disallow mtr cuz energy use recorded thru plant
					  heatCoilFn + HEATCOIL_EIRRAT,
					  heatCoilFn + HEATCOIL_EFFRAT,
					  0 );

		rc |= r->requireN( when, heatCoilFn + HEATCOIL_HPI, 0);

		if (rc==RCOK)
		{
			HEATPLANT *hp;
			rc |= ckRefPt( &HpB, r, hpi, hpiMbrName, NULL, (record **)&hp);	// check heat plant ref / access hp record
			switch (app)								// chain coil to its HEATPLANT, in AH or TU chain
			{
			case C_COILAPPCH_AHHC:
				nxAh4hp = hp->ah1;
				hp->ah1 = r->ss;
				break;
			case C_COILAPPCH_TUHC:
				nxTu4hp = hp->tu1;
				hp->tu1 = r->ss;
				break;
			}
		}
	}
	else						// not hot water coil. issue msgs for inputs allowed for HW only.
	{
		switch (app)								// get subtext for following error message
		{
		case C_COILAPPCH_AHHC:
			when = "when ahhcType is not HW";
			break;
		case C_COILAPPCH_TUHC:
			when = "when tuhcType is not HW";
			break;
		}
		r->disallowN( when, heatCoilFn + HEATCOIL_HPI, 0 );
	}

	// AHP (air source heat pump) additional checks, and not-AHP checks: see AHHEATCOIL::setup: AHP members not in HEATCOIL record.

	return rc;			// more returns above, incl E macros

#undef AHR
#undef REQUIRESHC
#undef ERSHC
#undef ERS2HC
#undef IDSHC
#undef DFLCUBIC
//#undef DFLBIQUAD
}		// HEATCOIL::setup
//============================================================================================================================
RC AHHEATCOIL::setup(				// check/set up an ah heating coil subrecord, incl inputs for AHP

	COILAPPCH app,		// application; drives checks & defaults, former member: C_COILAPPCH_TUHC, _AHHC, _AHCC.
	record *r,			// record containing AHHEATCOIL.  assumed by some code to be AH?
	SI ahhCoilFn,		// field number of AHHEATCOIL subrecord in r
	BOO rqd, BOO prohib )	// non-0 if required or disallowed, respectively, else optional, terminal coil only

// for air handler heat coil; uses HEATCOIL::setup and COIL::setup. coil application is determined by testing 'app' member.
// topHp1 should be done first, (cuz HEATCOIL::setup chains HW coils to their HEATPLANTs)
// CCH::setup must be called first -- crankcase heater setup values used here.
{
//uncomment as needed
#define AHR ((AH*)r)
#define REQUIRESAHC(subFn) r->require( ahhCoilFn+AHHEATCOIL_##subFn)		// issue error message if field of subrecord not given
#define ERSAHC(subFn)      r->ooer( ahhCoilFn+AHHEATCOIL_##subFn,   		// subrecord field err msg: addl arg(s) and ) must follow
#define ERS2AHC(f1,f2)     r->ooer( ahhCoilFn+AHHEATCOIL_##f1, ahhCoilFn+AHHEATCOIL_##f2,  	// subrecord 2-field error message ditto
//#define IDSAHC(subFn)      r->mbrIdTx( ahhCoilFn+AHHEATCOIL_##subFn)		// subrecord field name, from CULT search for polymorphism
	RC rc /*=RCOK*/;

//--- first setup the base HEATCOIL coil which is base class of AHHEATCOIL.  Covers all types but AHP (Air source heat pump).

	CSE_E( HEATCOIL::setup( app, r, ahhCoilFn, rqd, prohib, TRUE)) 	// and this calls COIL::setup.
		// last arg says called from AHHEATCOIL: allow AHP.

//--- if coil not specified, issue error for each derived class member given, return.

	UCH* fss = (UCH *)r + r->b->sOff + ahhCoilFn;		// start of field status bytes in r for HEATCOIL members
	COILTYCH ty = coilTy;
	SI subFn;
	if ( ty == C_COILTYCH_NONE					// if no coil specified
			||  prohib )						// or context disallows coil
	{
		//HEATCOIL::setup has done:
		// CHN(sched) = C_OFFAVAILVC_OFF;	// set absent/disallowed coil OFF for appl code. ASSUMED in cnah.cpp.
		// for all members of derived class not in base class: start at HEATCOIL_NFIELDs for normal-case speed.
		for (subFn = HEATCOIL_NFIELDS;  subFn < AHHEATCOIL_NFIELDS;  subFn++ )  		// loop field numbers
			if (fss[subFn] & FsSET)   							// if member given in input
				rc = r->cantGiveEr( ahhCoilFn + subFn, (char *)MH_S0659);  		// "when ahhcType is NONE or omitted"
		return rc;
	}

//--- if not AHP, HEATCOIL::setup has completed setup; disallow ahp inputs and return.

	if (ty != C_COILTYCH_AHP)
	{
#if 1		// currently 10-92 all AHHEATCOIL derived class members are for AHP's, so check by field numbers
		// for all members of derived class not in base class: start at HEATCOIL_NFIELDs.
		for (subFn = HEATCOIL_NFIELDS;  subFn < AHHEATCOIL_NFIELDS;  subFn++ )  		// loop field numbers
			if (fss[subFn] & FsSET)   							// if member given in input
				rc = r->cantGiveEr( ahhCoilFn + subFn, (char *)MH_S0660);  		// "when ahhcType is not AHP"

#else		// use this form when AHP inputs are no longer an identifiable range of field numbers
*       	#define f ahhCoilFn+AHHEATCOIL_##
*       	rc |= r->disallowN( "when ahhcType is not AHP",
*				   f CAP17,  f CAP47,  f CAP35,  f FD35DF,  f CAPIA,  f CAPSUPHEAT,
*				   f TFRMN,  f TFRMX,  f TFRPK,  f DFRFMN,  f DFRFMX, f DFRCAP, f DFRRH,
*				   f TOFF,   f TON,    f IN17,   f IN47,    f INIA,   f CD,     0 );
*       	#undef f
#endif
		return rc;
	}

//------ rest of function sets up AHP (air source heat pump) whose members are in the AHHEATPUMP derived class record only

// AHP required members
	rc |= r->requireN( (char *)MH_S0661,  					// "when ahhcType is AHP"
					   ahhCoilFn+AHHEATCOIL_CAPTRAT,
					   ahhCoilFn+AHHEATCOIL_COP17,  ahhCoilFn+AHHEATCOIL_COP47,  0 );

// some AHP input checks
	if (r->IsAusz(ahhCoilFn+AHHEATCOIL_CAPTRAT))
		rc |= r->disallowN("when ahhcCaptRat is AUTOSIZE",
			ahhCoilFn+AHHEATCOIL_CAP17, ahhCoilFn+AHHEATCOIL_CAP35, 0);
	if (r->IsSet(ahhCoilFn+AHHEATCOIL_CAP17))
		rc |= r->disallowN("when ahpCap17 is given", ahhCoilFn+AHHEATCOIL_CAPRAT1747, 0);
	if (dfrFMx <= dfrFMn)
		rc |= ERS2AHC(DFRFMX,DFRFMN) (char *)MH_S0663,dfrFMx,dfrFMn);	//"ahpDfrFMx (%g) must be > ahpDfrFMn (%g)"
	if (tOff > tOn)
		rc |= ERS2AHC(TOFF,TON)      (char *)MH_S0664,  tOff,  tOn);	//"ahpTOff (%g) must be <= ahpTOn (%g)"
	if (cd >= 1.0)
		rc |= ERSAHC(CD)             (char *)MH_S0665,   cd);		// "ahpCd (%g) must be < 1"  rob addition
	// 3 checks for tFrMn < tFrPk < tFrMx
	if (tFrMn >= tFrMx)
		rc |= ERS2AHC(TFRMN,TFRMX)   (char *)MH_S0666, tFrMn,tFrMx);	// "ahpTFrMn (%g) must be < ahpTFrMx (%g)"
	if (tFrMn >= tFrPk)
		rc |= ERS2AHC(TFRMN,TFRPK)   (char *)MH_S0667, tFrMn,tFrPk);	// "ahpTFrMn (%g) must be < ahpTFrPk (%g)"
	if (tFrPk >= tFrMx)
		rc |= ERS2AHC(TFRPK,TFRMX)   (char *)MH_S0668, tFrPk,tFrMx);	// "ahpTFrPk (%g) must be < ahpTFrMx (%g)"
	// check tFrMn < 35 < tFrMx cuz of how used with cap35 & to prevent /0. Rob's addition.
	if (tFrMn >= 35.)
		rc |= ERS2AHC(TFRMN,TFRPK)   (char *)MH_S0669,  tFrMn);	// "ahpTFrMn (%g) must be < 35."
	if (35. >= tFrMx)
		rc |= ERS2AHC(TFRPK,TFRMX)   (char *)MH_S0670,  tFrMx);	// "ahpTFrMx (%g) must be > 35."
	//additional members specific to other coil types disallowN'd below.

	if (rc)  return rc;				// stop here if any error has been detected



//--- finally, disallow inputs specific to other coil types

	rc |= r->disallowN( (char *)MH_S0675,				// "when ahhcType is AHP"
			//ahhCoilFn + AHHEATCOIL_CAPTRAT,		more specific message above
			ahhCoilFn + AHHEATCOIL_EIRRAT,
			ahhCoilFn + AHHEATCOIL_EFFRAT,
			//ahhCoilFn + AHHEATCOIL_PYEI + PYCUBIC_K,	disallowed by HEATCOIL::setup when not GAS or OIL
			//ahhCoilFn + AHHEATCOIL_STACKEFFECT,   	..
			//ahhCoilFn + AHHEATCOIL_HPI,   		disallowed by HEATCOIL::setup when not HW
			0 );

	return rc;					// additional (error and/or non-AHP) returns above

#undef AHR
#undef REQUIRESAHC
#undef ERSAHC
#undef ERS2AHC
#undef IDSAHC
}		// AHHEATCOIL::setup
//============================================================================================================================
RC FAN::fn_setup(					// check/set up a fan subrecord, including autoSizing stuff.

	FANAPPCH app,	// fan use, required for correct checking and defaults: C_FANAPPCH_SFAN, _RFAN, _TFAN, _XFAN.
	record *r,		// record containing FAN: TU, AH, etc
	SI fanFn,		// field number of FAN subrecord in r
	//BOO prohib,	// non-0 to prohibit fan (terminal with no air heat/cool)
	float defVfDs,	// if > 0, default design flow (cfm).  If <= 0, fan->vfDs is required.
	FAN* defCurveFrom )	// NULL or ptr to FAN record whose part load curve coefficients to copy (to dflt rtn same as sup)

// returns RCOK if no error
// on return, fan->fanTy is C_FANTYCH_NONE if fan not present.

// for air handler, call for supply fan before return fan
{
#define REQUIRES(subFn) r->require( fanFn + FAN_##subFn)		// issue error message if field of subrecord not given
#define ERS(subFn)      r->ooer( fanFn + FAN_##subFn,   		// subrecord field err msg: addl arg(s) and ) must follow
#define ERS2(f1,f2)     r->ooer( fanFn + FAN_##f1, fanFn+FAN_##f2,  	// subrecord 2-field error message ditto
#define IDS(subFn)      r->mbrIdTx( fanFn + FAN_##subFn)		// subrecord field name, from CULT search for polymorphism
#define NOTGZS(subFn)   r->notGzEr( fanFn + FAN_##subFn)   		// error message for field <= 0
// default cubic polynomial coefficients: array of 4 floats. uses r, fss, fanFn.
#define DFLCUBIC2( subFn, dflt, k0,k1,k2,k3,x0 )   	\
    if (!(fss[FAN_##subFn+PYCUBIC2_K] & FsSET))  	\
    {  static float dat[]={k0,k1,k2,k3,x0};  		\
       memcpy( r->field(fanFn+FAN_##subFn+PYCUBIC2_K), dflt ? dflt->curvePy.k : dat, sizeof(dat) );	\
    }

	RC rc = RCOK;					// preset return code to "no error"
	UCH* fss = (UCH *)r + r->b->sOff + fanFn;		// start of field status bytes in r for FAN members

// set member flag if autoSizing requested 7-1-95

	ausz = (fss[FAN_VFDS] & FsAS) != 0;			// field status bit set if input contains AUTOSIZE _fanVfDs.
	// Only accepted for SFAN and RFAN (per CULT AS_OK bit).
	// A value is not also accepted for vfDs.

// fan application-dependent checks and initialization

	switch (app)
	{
	default:
		return r->oer( (char *)MH_S0676, (UI)app);  		// "Internal error: cncult5.cpp:ckFan: bad fan app 0x%x"

	case C_FANAPPCH_TFAN:
		break;					// terminal fan (not implemented 10-92)

	case C_FANAPPCH_ZFAN:		// interzone fan
	case C_FANAPPCH_XFAN:		// zone exhaust fan
		if ( fss[FAN_VFDS] & FsSET 		// nz cfm says present
				&&  vfDs > 0 ) 				// (redundant)
			fanTy = C_FANTYCH_EXHAUST;		// type not inputtable; set per presence
		else
			fanTy = C_FANTYCH_NONE;			// fan not active
		break;
	case C_FANAPPCH_DSFAN:		// DOAS supply fan
	case C_FANAPPCH_DEFAN:		// DOAS exhaust fan
		if (defVfDs > 0.f)
		{
			fanTy = C_FANTYCH_EXHAUST;		// type not inputtable; set per presence
		}
		else
		{
			fanTy = C_FANTYCH_NONE;
		}
		break;
	case C_FANAPPCH_SFAN:   		// air handler supply fan.
		if (ausz)					// if autoSizing specified,
		{	((AH *)r)->fanAs.az_active = TRUE;		// set flag in ah's fan AUSZ substruct
			((AH *)r)->asFlow = TRUE;  		// say use open-ended fan/tu models with
		}						// ... const supply temps to find size in part A.
		break;

	case C_FANAPPCH_RFAN:  			// air handler return fan. do supply fan first.
		if (ausz)					// if AUTOSIZE rfanVfDs given
		{	if (fanTy==C_FANTYCH_NONE)
			{	rc = r->oer( "Cannot give \"AUTOSIZE rfanVfDs\" when there is no return fan.");	// NUMS
				ausz = FALSE;
			}
			else if (!((AH *)r)->fanAs.az_active)
			{
				rc = r->oer( "Cannot autoSize return fan without autoSizing supply fan.");		// NUMS
				ausz = FALSE;
			}
			else					/* return fan shares AUSZ fanAs with supply fan.
         						   Already done: ah->fanAs.az_active = ah->asFlow = TRUE. */
				((AH *)r)->asRfan = TRUE;		// say autoSizing return as well as supply fan.
		}
		else 							// not autosizing this return fan
			if ( ((AH *)r)->sfan.ausz				// if autoSizing supply fan (set up b4 rfan)
					&&  fanTy != C_FANTYCH_NONE				// and this return fan present
					&&  !(fss[FAN_VFDS] & FsSET)				// and its vfDs not given
					&&  defVfDs <= 0 )					// and no default vfDs (redudant - default is from sfanVfDs)
				// give more specific message than that for missing vfDs below
				rc = ERS(VFDS) "rfanVfDs must be given when autoSizing supply fan\n"
					 "    but not autoSizing return fan." );				// NUMS
		break;
	}

// if do not have this fan [or fan not allowed in context], issue error for each given member, then return

	if ( fanTy==C_FANTYCH_NONE			// if no fan specified
			/*||  prohib*/ )		// [if caller said no fan allowed]
	{
		for (SI subFn = 0;  subFn < FAN_NFIELDS;  subFn++)  	// for all members of fan subrecord
			if (fss[subFn] & FsSET)   				// if member given in input
			{
				switch (subFn)					// continue if giving this member ok
				{
				case FAN_FANTY:  //if (ty==C_FANTYCH_NONE) 	// non-none can get here when prohib (in terminal)
					continue;			// giving type NONE is always ok if in CULT , to say "no fan"
					//break;

				case FAN_VFDS:
					if (app==C_FANAPPCH_XFAN)	// given design volume flow 0 for zone exhaust fan is ok
						continue;			// ... that's how to say "no fan present"
					break;

				case FAN_CURVEPY + PYCUBIC2_K + 1:		// skip curvePy.k[1..5]: [0] already msg'd if given;
					subFn += 4;			//  addl bits set if > 1 ARRAY values entered, but no CULT entries
					continue;			//  --> mbrIdTx from cantGiveEr might give internal err msg. 6-92.
				}
				char *when = nullptr;					// subtext for input that says no fan present
				switch (app)
				{
				case C_FANAPPCH_SFAN:
					when = (char *)MH_S0677;
					break;	// "[internal error: no supply fan]"
				case C_FANAPPCH_RFAN:
					when = (char *)MH_S0678;
					break;	// "when rfanType is NONE or omitted"
				case C_FANAPPCH_ZFAN:
					when = "when izFanVfDs is 0";
					break;
				case C_FANAPPCH_DSFAN:
				case C_FANAPPCH_DEFAN:
					continue;  // Inputs are ignored instead of issuing an error
				case C_FANAPPCH_XFAN:
					when = (char *)MH_S0679;
					break;	// "when no exhaust fan (xfanVfDs 0 or omitted)"
				case C_FANAPPCH_TFAN:
					when = (char *)MH_S0680;
					break;	// "when tfanType is NONE or omitted"
					/* prohib ? "terminal has no air heat nor cool\n"
						*          "    (none of tuTH, tuTC, tuVfMn given)"
					                     *        : "when tfanType is NONE or omitted" */
				}
				rc = r->cantGiveEr( fanFn + subFn, when);			// cncult2.cpp
			}
			return rc;
	}

// fan present, check members

	switch (app)				// check type per application
	{
	case C_FANAPPCH_SFAN:
		if (fanTy==C_FANTYCH_DRAWTHRU || fanTy==C_FANTYCH_BLOWTHRU)  break;
		goto badFanTy;
	case C_FANAPPCH_RFAN:
		if (fanTy==C_FANTYCH_RETURN   || fanTy==C_FANTYCH_RELIEF)    break;
		goto badFanTy;
	case C_FANAPPCH_TFAN:
		if (fanTy==C_FANTYCH_SERIES   || fanTy==C_FANTYCH_PARALLEL)  break;
		goto badFanTy;
	case C_FANAPPCH_XFAN:
		if (fanTy==C_FANTYCH_EXHAUST)  break;
badFanTy:
		rc = ERS(FANTY) (char *)MH_S0681, 				// "bad fan type '%s' for %s"
			r->getChoiTx( fanFn+FAN_FANTY, 1, fanTy),
			IDS(FANTY) );
	}

	//rc |= REQUIRES(MTRI);					to require fanMtr

	if (!(fss[FAN_VFDS] & FsSET))			// if design flow not given nor autoSized
	{	if (defVfDs > 0.f)
			vfDs = defVfDs;					// if caller gave (calculated) default, use it
		else
			rc = ERS(VFDS) (char *)MH_S0682, IDS(VFDS) );	// else error "'%s' must be given"
	}
	else if (!ausz  &&  vfDs <= 0.f)
		rc = r->notGzEr( fanFn + FAN_VFDS);			// insurance (default/limits shd force)
	if (vfMxF < 1.f)
		rc = ERS(VFMXF) (char *)MH_S0683, IDS(VFMXF) );	// NOT checked elsewhere. "'%s' cannot be less than 1.0"
	if (press < 0.f)
		rc = ERS(PRESS) (char *)MH_S0684, IDS(PRESS) );	// insurance (default/limits shd force).
														// "'%s' cannot be less than 0"
	if (fss[FAN_EFF] & fss[FAN_SHAFTPWR] & FsSET)
		rc = ERS2( EFF, SHAFTPWR) (char *)MH_S0685, IDS(EFF), IDS(SHAFTPWR) );	// "Cannot give both '%s' and '%s'"
	if (fss[FAN_SHAFTPWR] & fss[FAN_ELECPWR] & FsSET)
		rc = ERS2( SHAFTPWR, ELECPWR) (char *)MH_S0685, IDS(SHAFTPWR), IDS(ELECPWR) );	// ditto
	if (fss[FAN_EFF] & fss[FAN_ELECPWR] & FsSET)
		rc = ERS2( EFF, ELECPWR) (char *)MH_S0685, IDS(EFF), IDS(ELECPWR) );	// ditto
	if (motPos != C_MOTPOSCH_INFLO)				// motor in air flow ok for all apps
	{	switch (app)						// check other motor position per app
		{
		case C_FANAPPCH_SFAN:  //if (motPos==C_MOTPOSCH_INRETURN)  break;	supply fan in return air: implement if easy
			// fall thru
		case C_FANAPPCH_RFAN:
			if (motPos==C_MOTPOSCH_EXTERNAL)
				break;		// outside supply or r/r fan not in ok
			// fall thru
		case C_FANAPPCH_TFAN:
		case C_FANAPPCH_XFAN: 					// in flow (above) only
				rc = ERS(MOTPOS) "Bad %s '%s'", IDS(MOTPOS),
						r->getChoiTx( fanFn+FAN_MOTPOS, 1));
		}
	}
	if (rc)
		return rc;			// do not attempt setup with erroneous fields

//------------ fan setup (incl more checks) -----------

	// default fan curve polynomial if not given
	DFLCUBIC2( CURVEPY, defCurveFrom, 0., 1., 0., 0., 0. );			// default is per caller, else linear.

	// check/fix fan curve polynomial for having value 1.0 at relative flow 1.0, 6-92
	rc |= curvePy.normalize( r, fanFn + FAN_CURVEPY, "relative flow", 1.0);		// below

// set up derived numerical values except if NAN in vfDs (as at start autoSize)
	  t = 70;				// init temperature member: setup uses.
#if 1	// support variant input (shaftpwr (aka shaftBhp), elecPwr, ) 8-26-2010
  	  fn_setup2(			// if vfDs not NAN, set outPower, shaftPwr or eff, inPower, airPower, (cMx). cnfan.cpp.
		  ((fss[FAN_SHAFTPWR] & FsSET)!=0) + 2*((fss[FAN_ELECPWR] & FsSET)!=0));
#else
x	  fn_setup2();			// if vfDs not NAN, set outPower, shaftPwr, inPower, airPower, (cMx). cnfan.cpp.
#endif


// check shaft power or efficiency, and motor efficiency
	if (fss[FAN_SHAFTPWR] & FsSET)			// if shaft power (input-converted to Btuh) given
	{
		if (ausz)					// disallow when autoSizing fan, cuz size-dependent -- use eff instead.
			return ERS(SHAFTPWR) "Cannot give '%s' when autoSizing '%s'.\n"
						   "    You may give '%s', or let it default.",
						   IDS(SHAFTPWR), IDS(VFDS), IDS(EFF) );
		if (outPower > shaftPwr * 1.000001)			// (only reason outPower is a member)
			return ERS(SHAFTPWR)  (char *)MH_S0686,		/* "'%s' is less than air-moving output power \n"
								   "      ( = '%s' times '%s' times units conversion factor)" */
					   IDS(SHAFTPWR), IDS(VFDS), IDS(PRESS) );		// omit values (else cvin2ex them!)
		if (shaftPwr <= 0.f)  				// /0 insurance
		   return NOTGZS(SHAFTPWR);

									  // error message missing for giving both eff and shaft power is above in this fcn.
	}
	else						// if efficiency not given, it is defaulted by CULT
	{	if (eff <= 0.f)  				// /0 insurance
			return NOTGZS( EFF);
	}
	if (motEff <= 0.f)
		return NOTGZS( MOTEFF);

	// [note cMx != 0 assumed by code in cncoils.cpp <-- appears no longer true 7-1-95 -- that code if'd out].
	// note cMx != 0 required by app\cnah2:setFrFanOn for main sim, 0 ok for autoSize, 7-95.

	/* POSSIBLE BUG to deal with sometime: no demand on design days --> vfDs = 0.
	  Then if there is demand on ah during main sim, a fatal error may occur if fanCycling. 7-95.*/

	return rc;						// other returns above!

#undef REQUIRES
#undef ERS
#undef ERS2
#undef IDS
#undef NOTGZS
#undef DFLCUBIC2
}		// FAN::fn_setup
//=============================================================================================================================
 RC PYLINEAR::normalize( 			// if poly coefficients are inconsistent, normalize and/or issue message.

	 record *r, 			// non-sub record containing the poly subrecord -- needed re error message
	 SI fn,			// field number of polynomial subrecord in r
	 char *descrip,		// description of argument, eg "relative flow", for use in "at %s = %g"
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
							  (char *)MH_S0687,		/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
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
						 (char *)MH_S0688,		// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"
						 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
						 v, descrip, x );
		return RCOK;
}			// PYLINEAR::normalize
//=============================================================================================================================
	RC PYCUBIC::normalize( 			// if cubic poly coefficients are inconsistent, normalize and/or issue message.

		record *r, 			// non-sub record containing the poly subrecord -- needed re error message
		SI fn,			// field number of polynomial subrecord in r
		char *descrip,		// description of argument, eg "relative flow", for use in "at %s = %g"
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
							  (char *)MH_S0687,	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
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
						 (char *)MH_S0688,		// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"
						 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
						 v, descrip, x );
		return RCOK;
}			// PYCUBIC::normalize
//=============================================================================================================================
RC PYCUBIC2::normalize( 		// if cubic-with-x0 poly coefficients are inconsistent, normalize and/or issue message.

	record *r, 			// non-sub record containing the poly subrecord -- needed re error message
	SI fn,			// field number of polynomial subrecord in r
	char *descrip,		// description of argument, eg "relative flow", for use in "at %s = %g"
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
						  (char *)MH_S0687,	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
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
				 (char *)MH_S0688,		// "Inconsistent '%s' coefficients: value is %g, not 1.0,\n    for %s = %g"
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
	char *descX, char *descY,	// descriptive texts, for use in "for %s=%g and %s=%g"
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
				&&  fabs(v - 1.) > .001 )   		// if much wrong, tell user, at least for now 7-14-92
			return r->oWarn( 			// returns RCOK 7-92
						  (char *)MH_S0689,	/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
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
					 (char *)MH_S0690,		/* "Inconsistent '%s' coefficients: value is %g, not 1.0,\n"
					   "    for %s=%g and %s=%g\n" */
					 r->mbrIdTx( fn),   	// get id text for record/field. slow: do not do til error detected.
					 v, descX, x, descY, y );
	return RCOK;
}			// PYBIQUAD::normalize

// end of cncult5.cpp
