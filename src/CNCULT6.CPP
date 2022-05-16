// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cncult6.c: cul user interface tables and functions for CSE:
//   part 6: fcns for checking plants, called from cncult2.c:topCkf.


/*------------------------------- INCLUDES --------------------------------*/
#include <CNGLOB.H>	// USI SI LI

#include <ANCREC.H>	// record: base class for rccn.h classes
#include <rccn.h>	// HEATPLANTstr BOILERstr NHPSTAGES HPSTAGESZ
#include <MSGHANS.H>	// MH_S0700

#include <PSYCHRO.H>	// psySatEnthalpy psySatEnthSlope

#include <CNGUTS.H>	// HpB BlrB

//#include <cuparse.h>	// perl
//#include <exman.h>	// ISNANDLE ISUNSET
#include <CUL.H>	// FsSET oer oWarn
//#include <cueval.h>	// cupfree cupIncRef

//declaration for this file:
//#include <cncult.h>	// use classes, globally used functions
#include <IRATS.H>	// declarations of input record arrays (rats)
#include <CNCULTI.H>	// cncult internal functions shared only amoung cncult,2,3,4.c


/*-------------------------------- OPTIONS --------------------------------*/
//none

/*-------------------------------- DEFINES --------------------------------*/
#define CPW  1.0	// heat capacity of water, Btu/lb.

/*---------------------------- LOCAL VARIABLES ----------------------------*/
//none

/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
//none


//=========================================== FUNCTIONS FOR CHECKING PLANTS ===================================================


//-----------------------------------------------------------------------------------------------------------------------------
//  record-copiers & loopers. setup member fcns follow.
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topHp1( void)		// heat plants 1: create run records. AH's & HPLOOPs & BOILERs must be done between topHp1 and 2.
{
	HEATPLANT *hpi, *hp;
	RC rc /*=RCOK*/;	// (redundant rc inits removed 12-94 when BCC 32 4.5 warned)

// copy HEATPLANT input records to run records: do early as other checkers chain HW coils, HX's, BOILERs to hp.

	CSE_E( HpB.al( HpiB.n, WRN) )			// delete (unexpected) old hp's, alloc needed # hp's at once for min fragmentation
	// hp's are not owned (names must be unique), leave .ownB 0.
	RLUP( HpiB, hpi)				// loop heatplant input records (cnglob.h macro)
	{
		HpB.add( &hp, ABT, hpi->ss);   		// add heatplant runtime record.  Error unexpected cuz ratCre'd.
		*hp = *hpi;				// copy entire input record into run record including name
	}

	return rc;
}		// topHp1
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topHp2( void)		// heat plants 2: complete check after loads and BOILERs chained to HEATPLANT run records
{
	RC rc=RCOK;
	HEATPLANT* hp;
	RLUP( HpB, hp)				// loop heatplant run records (cnglob.h macro)
	rc |= hp->setup();			// check/set up one heatplant run record (below)
	return rc;
}		// topHp2
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topBlr( void)		// check/initialize boilers. do between topHp1 and 2.
{
	BOILER *blri, *blr;
	RC rc /*=RCOK*/;

// copy BOILER input records to run records
	CSE_E( BlrB.al( BlriB.n, WRN) )			// delete (unexpected) old blr's, alloc needed # at once for min fragmentation
	// blr's are not owned (names must be unique), leave .ownB 0.
	RLUP( BlriB, blri)				// loop heatplant input records (cnglob.h macro)
	{
		BlrB.add( &blr, ABT, blri->ss);  	// add boiler runtime record.  Error unexpected cuz ratCre'd.
		*blr = *blri;				// copy entire input record into run record including name
	}

// use member fcn to init each record
	RLUP( BlrB, blr)				// loop boiler run records (cnglob.h macro)
	rc |= blr->setup();			// check/set up one boiler run record (below)
	return rc;
}		// topBlr
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topCp1( void)		// coolPlants 1: create run records. AH's & CHILLERs must be done between topCp1 and 2.
{
	COOLPLANT *cpi, *cp;
	RC rc /*=RCOK*/;

// copy COOLPLANT input records to run records: do early as other checkers chain CHW coils, CHILLERs to cp.

	CSE_E( CpB.al( CpiB.n, WRN) )			// delete (unexpected) old cp's, alloc needed # cp's at once for min fragmentation
	// cp's are not owned (names must be unique), leave .ownB 0.
	RLUP( CpiB, cpi)				// loop coolplant input records (cnglob.h macro)
	{
		CpB.add( &cp, ABT, cpi->ss);   		// add coolplant runtime record.  Error unexpected cuz ratCre'd.
		*cp = *cpi;				// copy entire input record into run record including name
	}

	return rc;
}		// topCp1
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topCp2( void)		// coolPlants 2: complete check after loads and CHILLERs chained to COOLPLANT run records
{
	RC rc=RCOK;
	COOLPLANT* cp;
	RLUP( CpB, cp)				// loop coolplant run records (cnglob.h macro)
	rc |= cp->setup();			// check/set up one coolplant run record (below)
	return rc;
}		// topCp2
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topCh( void)		// check/initialize chillers. do between topCp1 and 2.
{
	CHILLER *chi, *ch;
	RC rc /*=RCOK*/;

// copy CHILLER input records to run records
	CSE_E( ChB.al( ChiB.n, WRN) )			// delete (unexpected) old ch's, alloc needed # at once for min fragmentation
	// ch's are not owned (names must be unique), leave .ownB 0.
	RLUP( ChiB, chi)				// loop coolplant input records (cnglob.h macro)
	{
		ChB.add( &ch, ABT, chi->ss);		// add chiller runtime record.  Error unexpected cuz ratCre'd.
		*ch = *chi;				// copy entire input record into run record including name
	}

// use member fcn to init each record
	RLUP( ChB, ch)				// loop chiller run records (cnglob.h macro)
	rc |= ch->setup();			// check/set up one chiller run record (below)
	return rc;
}		// topCh
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topTp1( void)		// towerplants 1: generate run records. topCp2 must be done between topTp1 and 2.
{
	TOWERPLANT *tp, *tpi;
	RC rc /*=RCOK*/;

// copy TOWERPLANT input records to run records: do early as other checkers chain COOLPLANTs to TOWERPLANTs.
	CSE_E( TpB.al( TpiB.n, WRN) )			// delete (unexpected) old tp's, alloc needed # at once for min fragmentation
	// tp's are not owned (names must be unique), leave .ownB 0.
	RLUP( TpiB, tpi)				// loop coolplant input records (cnglob.h macro)
	{
		TpB.add( &tp, ABT, tpi->ss);		// add TOWERPLANT runtime record.  Error unexpected cuz ratCre'd.
		*tp = *tpi;				// copy entire input record into run record including name
	}
	return rc;
}		// topTp1
//-----------------------------------------------------------------------------------------------------------------------------
RC   FC topTp2( void)		// towerplants 2. do after topCp2: uses coolplant chain & info re defaults that topCp2 generates.
{
	RC rc=RCOK;
	TOWERPLANT *tp;
	RLUP( TpB, tp)				// loop towerplant run records (cnglob.h macro)
	rc |= tp->setup();			// check/set up one towerplant run record (below)
	return rc;
}		// topTp2
//-----------------------------------------------------------------------------------------------------------------------------
// heatplants & boilers setup member fcns
//-----------------------------------------------------------------------------------------------------------------------------
RC HEATPLANT::setup()		// finish checking and initializing one HEATPLANT.

// called from topHp2: AFTER setup of BOILERs, AFTER loads (HW coils, HPLOOP HX's) have been checked and chained to HEATPLANT.
{
	RC rc=RCOK;

// checks

	if (!blr1)                 rc |= oer( (char *)MH_S0700);	// "heatPlant has no BOILERs"
	if (!ah1 && !tu1 && !hl1)  oWarn( (char *)MH_S0701);		// "HeatPlant has no loads: no HW coils, no HPLOOP HX's."
	// ******** wording re hx??

#if 1	// default stages in code, not CULT table, 10-92. Also cncult.c.
// default stages: if none given, supply a single all-boiler stage

	//#define NHPSTAGES 7		(or as changed) is put in rccn.h by cnrecs.def
	//#define HPSTAGESZ 8		(or as changed) is put in rccn.h by cnrecs.def
	TI *stg;
	for (SI i = 0; i < NHPSTAGES; i++)					// loop over stage numbers, 0-based internally
	{
		stg = hpStage1 + i * HPSTAGESZ;  				// point hpStage1..hpStage7 for i = 0..6
		if (*stg)  break;						// if value entered for this stage, leave loop
	}
	if (i >= NHPSTAGES)							// if loop completed (no stage data found)
		hpStage1[0] = TI_ALL;						// default stage 1 to ALL boilers

// check/set up stages

	//assumed, because new record is all 0's:  stgN = stgMxQ = 0; stgCap[0..6] = 0.;  all BOILER.used=FALSE.
	BOILER *blr;
	for (i = 0; i < NHPSTAGES; i++)					// loop over stage numbers, 0-based internally
	{
		stg = hpStage1 + i * HPSTAGESZ;  				// point hpStage1..hpStage7 for i = 0..6
#else
x// check/set up stages
x
x    //#define NHPSTAGES 7		(or as changed) is put in rccn.h by cnrecs.def
x    //#define HPSTAGESZ 8		(or as changed) is put in rccn.h by cnrecs.def
x    //assumed, because new record is all 0's:  stgN = stgMxQ = 0; stgCap[0..6] = 0.;  all BOILER.used=FALSE.
x    BOILER *blr;
x    for (SI i = 0; i < NHPSTAGES; i++)					// loop over stage numbers, 0-based internally
x	{
x       TI *stg = hpStage1 + i * HPSTAGESZ;  				// point hpStage1..hpStage7 for i = 0..6
#endif
		char stgNm[20];
		sprintf( stgNm, "hpStage%d", INT(i+1) ); 	// stage variable name text for error messages

		// skip stage if empty. Used stages need not be contiguous (but they should be in order of increasing power).
		if (!stg[0])
			continue;
		stgN = i+1;					// update max+1 used stage subscript / nz indicates found used stage

		// insurance check for ALL, ALL_BUT and list, or just list of valid subcripts of our BOILERs; lists followed by 0.
		if (stg[0]==TI_ALL)				// if ALL, check no more, cuz cul.c defaulter may fill array with TI_ALL.
			;						// runtime (cnhp.c) ignores rest of hpStage array after TI_ALL.
		else
		{
			for (SI j = stg[0]==TI_ALLBUT ? 1 : 0;  j < HPSTAGESZ-1;  j++)   	// skip ALLBUT if present, check list except last
			{
				if (stg[j]==0)  break;						// 0 terminates list, done
				// ALL or ALLBUT after 1st position: cul.c disallows; if did get here, wd yield ckRefPt internal error message.
				rc |= ckRefPt( &BlrB, stg[j], stgNm, NULL, (record**)&blr);	// check for valid BOILER subscript, access record
				if (blr->ownTi != ss)						// boiler named in stage list must be ours
					rc |= oer( (char *)MH_S0702, 			// "%s: boiler '%s' is not in this heatPlant"
					stgNm, blr->name);
				for (SI k = j + 1;  k < NHPSTAGES-1 && stg[k];  k++)	// loop following part of list
					if (stg[k]==stg[j])					// check for duplication
						rc |= oer( (char *)MH_S0703,			// "%s: BOILER '%s' cannot be used twice in same stage"
						stgNm, blr->name );
			}
			if (stg[j])   					// stops at last j whether or not 0
				rc |= oer( (char *)MH_S0704, stgNm); 	// if not 0, error "Internal error: %s not terminated with 0"
		}

		// accumulate stage power and flag used boilers
		for (blr=NULL;  nxBlrStg( blr, i);  )		// loop boilers of stage i (0-6). Interprets TI_ALL and TI_ALLBUT. cnhp.c.
		{
			blr->used = TRUE;
			stgCap[i] += blr->blrCap + blr->blrp.q;   	// accumulate stage power: rated output of each boiler + pump heat
			stgPQ[i] += blr->blrp.q;			// accum stage pump heat (blrp.q set by PUMP::setup from BOILER::setup)
		}

		// remember most powerful stage
		if (stgCap[i] > stgCap[stgMxQ])			// if new most powerful stage
			stgMxQ = i;					// 0-based subscript of most powerful stage
		else						// stage not empty and not more powerful than all preceding stages
			if (i > 0)					// don't warn for hpStage1 vs hpStage1, as when no boilers
				oWarn(
					   (char *)MH_S0705, 	// "stage %s is not more powerful than hpStage%d, \n    and thus will never be used"
					   stgNm, INT(stgMxQ+1) );
	}
	// messages re stage checks
	if (!stgN)   					// insurance: "impossible"
		rc |= oer( (char *)MH_S0706);		// "No non-empty hpStages specified"
	for (blr = NULL;  nxBlr(blr);  )			// loop HEATPLANT's BOILERS as chained by BOILER::setup. cnhp.c.
		if (!blr->used)					// for each owned boiler used in no stage
			blr->oWarn( (char *)MH_S0707);		// warn and continue. "Not used in any hpStage of heatPlant".

// other setup

	qPipeLoss = hpPipeLossF * (stgCap[stgMxQ] - stgPQ[stgMxQ]); 	// pipe heat loss is fraction of max stage boiler capacity
	hpMode = C_OFFONCH_OFF;						// init to OFF; hpEstimate will turn ON when appropriate.
	capF = 1.0;								// init to full capacity of each load avail (no overload)

	return rc;
}		// HEATPLANT::setup
//-----------------------------------------------------------------------------------------------------------------------------
RC BOILER::setup()		// check and initialize one BOILER.
// call before topHp2.
{
	// macro to default cubic polynomial coefficients in record member
#define DFLCUBIC( fname, fn, k0,k1,k2,k3 )	/* args are field name, field #, 4 floats.*/ \
	if (!(sstat[fn+PYCUBIC_K] & FsSET))  		\
	{  static float dat[]={k0,k1,k2,k3};  		\
	   memcpy( fname.k, dat, sizeof(dat) );  	\
	}

	RC rc=RCOK;

// chain boiler to owning heatplant, for fast looping in cnhp.c:HEATPLANT::nxBlr. Used in HEATPLANT::setup.
	HEATPLANT *hp = HpB.p + ownTi;				// point owning heatplant (.ownTi set by CULT table entry)
	nxBlr4hp = hp->blr1;
	hp->blr1 = ss;			// chain (results in reverse order)

// subrecords
	if (!(sstat[BOILER_BLRP+PUMP_GPM] & FsSET))  blrp.gpm = blrCap/10000.;	// default pump flow per boiler capacity
	rc |= blrp.setup( this, BOILER_BLRP);  				// check/init blr's pump, incl setting pump heat .blrp.q.

// user may specify either eir (Energy Input Ratio) or efficiency (1/eir); efficiency is defaulted; runtime uses eir, not eff.

	if (sstat[BOILER_BLREIRR] & FsSET)						// if eir given by user
	{
		if (blrEirR < 1.0)                  oer( (char *)MH_S0708);	// "blrEirR must be >= 1.0"
		if (sstat[BOILER_BLREFFR] & FsSET)  oer( (char *)MH_S0709);	// "Can't give both blrEirR and blrEffR"
	}
	else							// eir not given; if eff not given, CULT table entry defaulted it.
		blrEirR = 1./blrEffR;					// set eir from efficiency, for runtime use.
	// /0 note: field limits check disallows 0.

// default/normalize cubic polynomial for correcting energy input for part load

	// following default BlrPyEi coeff are from Niles' CNR92 Program Specification, VI-B-3, p 162, 8-17-92.
	DFLCUBIC( blrPyEi, BOILER_BLRPYEI, .082597f, .996764f, -.079361f, 0.f);		// default polynomial coefficients if not given
	blrPyEi.normalize( this, BOILER_BLRPYEI, "relative load", 1.0f);		// check/normalize polynomial (cncult5.c)

// aux energy drain checks: warn for meter without power
#define MTRWRN(a,b,c,d) \
       if (sstat[a] & FsSET && !(sstat[b] & FsSET))  oWarn( (char *)MH_S0710, c, d);
	// "'%s' given, but no '%s' to charge to it given"
	MTRWRN( BOILER_AUXONMTRI     , BOILER_AUXON     , "blrAuxOnMtr"     , "blrAuxOn"      );
	MTRWRN( BOILER_AUXOFFMTRI    , BOILER_AUXOFF    , "blrAuxOffMtr"    , "blrAuxOff"     );
	MTRWRN( BOILER_AUXONATALLMTRI, BOILER_AUXONATALL, "blrAuxOnAtAllMtr", "blrAuxOnAtAll" );
	MTRWRN( BOILER_AUXFULLOFFMTRI, BOILER_AUXFULLOFF, "blrAuxFullOffMtr", "blrAuxFullOff" );
#undef MTRWRN

	return rc;

#undef DFLCUBIC
}		// BOILER::setup
//-----------------------------------------------------------------------------------------------------------------------------
// coolplants & chillers
//----------------------------------------------------------------------------------------------------------------------------
RC COOLPLANT::setup()	// check/init/set up one COOLPLANT record
// call after ah setup: uses CHW coil chain to COOLPLANT record.
// call after chiller setup: uses CHILLER chain to COOLPLANT record.
// call after records allocated for towerplants: chains to them.
// call before TOWERPLANT setup: that uses COOLPLANT chain and .mxCondQ.
{
	RC rc=RCOK;

// checks

	if (!ch1)   rc |= oer( (char *)MH_S0711);		// "coolPlant has no CHILLERs"
	if (!ah1)   oWarn( (char *)MH_S0712);			// "CoolPlant has no loads: no CHW coils."

// check towerplant / chain coolplants for towerplant

	TOWERPLANT *tp;
	rc |= ckRefPt( &TpB, cpTowi, "cpTowerplant" ,NULL, (record **)&tp );	// check /point cooling tower plant rec
	nxCp4tp = tp->cp1;
	tp->cp1 = ss;						// this cp to (head of) tp's cp chain

#if 0	// never compiled
x// check/set up stages
x
x    //#define NCPSTAGES 7		(or as changed) is put in rccn.h by cnrecs.def
x    //#define CPSTAGESZ 8		(or as changed) is put in rccn.h by cnrecs.def
x    //assumed, because new record is all 0's:  stgN = stgMxCap = 0; stgPpmpQ[0..6] = 0.;  all CHILLER.used=FALSE; etc.
x    CHILLER *ch;
x    for (SI i = 0; i < NCPSTAGES; i++)					// loop over stage numbers, 0-based internally
x	{
x       TI *stg = cpStage1 + i * CPSTAGESZ;  				// point cpStage1..cpStage7 for i = 0..6
#else	// redo to default in code
// default stages: if no cpStages entered, supply 1 stage of all chillers

	//#define NCPSTAGES 7		(or as changed) is put in rccn.h by cnrecs.def
	//#define CPSTAGESZ 8		(or as changed) is put in rccn.h by cnrecs.def
	TI *stg;
	for (SI i = 0; i < NCPSTAGES; i++)					// loop over stage numbers, 0-based internally
	{
		stg = cpStage1 + i * CPSTAGESZ;  				// point cpStage1..cpStage7 for i = 0..6
		if (*stg)  break;						// leave loop if anything entered for stage
	}
	if (i >= NCPSTAGES)							// if loop finished (no cpStages entered)
		cpStage1[0] = TI_ALL;						// supply default: stage 1 = all chillers

// check/set up stages

	//assumed, because new record is all 0's:  stgN = stgMxCap = 0; stgPpmpQ[0..6] = 0.;  all CHILLER.used=FALSE; etc.
	CHILLER *ch;
	for (i = 0; i < NCPSTAGES; i++)					// loop over stage numbers, 0-based internally
	{
		stg = cpStage1 + i * CPSTAGESZ;  				// point cpStage1..cpStage7 for i = 0..6
#endif
		char stgNm[20];
		sprintf( stgNm, "cpStage%d", INT(i+1) );   	// stage variable name text for error messages

		// skip stage if empty. Used stages need not be contiguous (but they should be in order of increasing power).
		if (!stg[0])
			continue;
		stgN = i+1;					// update max+1 used stage subscript / nz indicates found used stage

		// insurance check for ALL, ALL_BUT and list, or just list of valid subcripts of our CHILLERs; lists followed by 0.
		if (stg[0]==TI_ALL)				// if ALL, check no more, cuz cul.c defaulter may fill array with TI_ALL.
			;						// runtime (cncp.c) ignores rest of cpStage array after TI_ALL.
		else
		{
			for (SI j = stg[0]==TI_ALLBUT ? 1 : 0;  j < CPSTAGESZ-1;  j++)   	// skip ALLBUT if present, check list except last
			{
				if (stg[j]==0)  break;						// 0 terminates list, done
				// ALL or ALLBUT after 1st position: cul.c disallows; if did get here, wd yield ckRefPt internal error message.
				rc |= ckRefPt( &ChB, stg[j], stgNm, NULL, (record**)&ch);	// check for valid CHILLER subscript, access record
				if (ch->ownTi != ss)						// chiller named in stage list must be ours
					rc |= oer( (char *)MH_S0713, 				// "%s: chiller '%s' is not in this coolPlant"
					stgNm, ch->name);
				for (SI k = j + 1;  k < NCPSTAGES-1 && stg[k];  k++)		// loop following part of list
					if (stg[k]==stg[j])						// check for duplication
						rc |= oer( (char *)MH_S0714,			// "%s: CHILLER '%s' cannot be used twice in same stage"
						stgNm, ch->name );
			}
			if (stg[j])   					// stops at last j whether or not 0
				rc |= oer( (char *)MH_S0715, stgNm); 	// if not 0, error "Internal error: %s not terminated with 0"
		}

		// accumulate stage powers, flows, pump heats, flag used chillers
		DBL capDs = 0.;				// this stage's capacity at design cond (neg), corress to stgCap[i] for HEATPLANTs
		DBL pMwOv = 0.;				// this stage's primary pump flow with overrun
		DBL condGpm = 0.;			// this stage's condenser pump flow: user input value, gpm
		DBL condQ = 0.;				// this stage's design rejected (condenser) heat (positive)
		SI  nCh = 0;				// # chillers: check nxChStg.
		for (ch=NULL;  nxChStg( ch, i);  )		// loop chillers of stage i (0-6). Interprets TI_ALL and TI_ALLBUT. cncp.c.
		{
			nCh++;
			ch->used = TRUE;
			capDs += ch->chCapDs + ch->chpp.q;   		// accumulate stage design power: (neg) rated output of chiller + pump heat
			stgPPQ[i] += ch->chpp.q;			// accum stage pri pump heat (chpp.q set by PUMP::setup from CHILLER::setup)
			stgCPQ[i] += ch->chcp.q;			// accum stage condenser pump heat likewise
			stgPMw[i] += ch->chpp.mw;			// accum primary pump flow for stage
			pMwOv += ch->chpp.mw * ch->chpp.ovrunF;	// accum ditto with overrun -- for too many coils check
			stgCMw[i] += ch->chcp.mw;			// accum internal condenser pump flow for stage, lb/hr
			condGpm += ch->chcp.gpm;			// accum user-input condenser pump flow for stage, gpm, re towerplant dflt
			condQ += 					// accum max design rejected heat for stage, for defaulting tower capacity
			-ch->chCapDs 			//  design output capacity (made positive) goes to condenser
			* (1. + ch->chEirDs			//  design input power also goes to condenser
			* ch->chMotEff)		//   except not motor heat if open (lost to air) (motEff 1.0 if hermetic)
			+ ch->chcp.q; 			//  condenser pump heat goes to condenser
		}
		if (nCh < 1 || nCh >= CPSTAGESZ)  		// unused stages don't get here; check on nxChStg.
			rc |= oer( (char *)MH_S0716, 		// "Internal error: bad # chillers (%d) found in stage %s"
			(INT)nCh, stgNm );
		else if (capDs >= 0.)				// errors in CHILLER::setup, should not get here
			rc |= oer( (char *)MH_S0717, 		// "Internal error: nonNegative total capacity %g of chillers in stage %s"
			capDs, stgNm );

		// remember most powerful stage
		if (capDs < mxCapDs)				// if new most (neg) powerful stage
		{
			mxCapDs = capDs;				// design power of most powerful stage
			mxPMwOv = pMwOv;				// primary flow with pump overrun of most powerful stage
			stgMxCap = i;					// 0-based subscript of most design-powerful stage
		}
		else						// stage not empty and not more powerful than all preceding stages
			if (i > 0)					// don't warn for cpStage1 vs cpStage1, as when no chillers
				// for chillers use weak warning as stage might be more powerful at different ts,tCnd.
				oWarn(
				(char *)MH_S0718, 			/* "stage %s is not more powerful (under design conditions) than \n"
							   "    cpStage%d, and thus may never be used" */
				stgNm, INT(stgMxCap+1) );
		if (condQ > mxCondQ)				// remember largest design rejected heat
		{
			mxCondQ = condQ;				// used in defaulting cooling towers capacity
			mxCondGpm = condGpm;     			// condenser pump flow of same stage, gpm, used ditto
		}
		if (stgPMw[i] > mxPMw)				// remember max primary pump flow, possibly used in cncp.c 10-92
			mxPMw = stgPMw[i];
	}
	// messages re stage checks
	if (!stgN)   					// insurance: "impossible" cuz stage1 defaults to TI_ALL
		rc |= oer( (char *)MH_S0719);		// "No non-empty cpStages specified"
	for (ch = NULL;  nxCh(ch);  )			// loop COOLPLANT's CHILLERs as chained by CHILLER::setup. cncp.c.
		if (!ch->used)					// for each owned chiller used in no stage
			ch->oWarn((char *)MH_S0720);			// warn "Not used in any cpStage of coolPlant" and continue
	if (rc)  return rc;					// return if no chillers, error in staging, etc

// error now if connected coils exceed max stage pumping capacity with overrun: can't check during run as flow not simulated.

	if (mwDsCoils > mxPMwOv)				// mwDsCoils: sum of coil design flows, accum by COOLCOIL::setup
		oer( (char *)MH_S0721, mwDsCoils, mxPMwOv );
	/* "Total design flow of connected coils, %g lb/hr, is greater than\n"
	   "    most powerful stage primary pumping capacity (with overrun), %g lb/hr." */

// other setup
	qPipeLoss = cpPipeLossF * (mxCapDs - stgPPQ[stgMxCap]); 		// pipe heat loss is fraction of max stage chiller capacity
	cpMode = C_OFFONCH_OFF;						// init to OFF; cpEstimate will turn ON when appropriate.

	return rc;
}			// COOLPLANT::setup()
//----------------------------------------------------------------------------------------------------------------------------
RC CHILLER::setup()		// check/set up/init one CHILLER record
// call after COOLPLANT records allocated: chains to them.
// call before COOLPLANT::setup: that uses chiller chain, pump.mw's, .
{
	RC rc /*=RCOK*/;

// owning coolplant: check, point, chain chiller to it

	COOLPLANT *cp;
	CSE_E( ckRefPt( &CpB, ownTi, "ownTi", NULL, (record**)&cp) );	// if ownTi is bad, msg & return bad to caller now.
																// Is this check unnecessary?
	nxCh4cp = cp->ch1;
	cp->ch1 = ss;				// nxCh4cp bkwds chillers-for-coolplant chain begins in cp->ch1.

// checks/defaults

	chCapDs = float( -fabs(chCapDs));					// capacity may be entered + or -; make negative internally.
	if (chMinFsldPlr > chMinUnldPlr) 				// min plr for false loading must be <= min plr for unloading
		ooer( CHILLER_CHMINFSLDPLR, CHILLER_CHMINUNLDPLR,
		(char *)MH_S0722, chMinFsldPlr, chMinUnldPlr );	// "chMinFsldPlr (%g) must be <= chMinUnldPlr (%g)"
	if (mtri)							// if meter reference given, check it
		ckRefPt( &MtrB, mtri, "chMtr");

// pump subrecords

	if (!(sstat[CHILLER_CHCP+PUMP_GPM] & FsSET))		// condenser (heat rejection) pump: if gpm not given
		chcp.gpm = fabs(chCapDs)/4000.;				// default based on capacity
	chcp.setup( this, CHILLER_CHCP);				// set up condenser PUMP subrecord: sets .mw, .q, .p.
	if (!(sstat[CHILLER_CHPP+PUMP_GPM] & FsSET))		// primary (CHW coil) loop pump: if gpm not given
		chpp.gpm = fabs(chCapDs)/5000.;				// default based on capacity
	chpp.setup( this, CHILLER_CHPP);				// set up primary PUMP subrecord: sets .mw, .q, .p.
	if (!rc)							// if no error: other error/omission might be cause of following
	{
		if (chpp.q >= -chCapDs)					// coolplant:setup expects net negative
			rc |= oer( (char *)MH_S0723, chpp.q, -chCapDs);	// "Primary pump heat (%g) exceeds chCapDs (%g)"
		else if (chpp.q > -chCapDs * .25) 	  		// intended to help user, rob 10-92. 10%?
			oWarn( (char *)MH_S0724, chpp.q, -chCapDs);	// "Primary pump heat (%g) exceeds 1/4 of chCapDs (%g)."
		if (chcp.q > -chCapDs * .25)   				// intended to help user, rob 10-92. 10%?
			oWarn( (char *)MH_S0725, chcp.q, -chCapDs);	// "Condenser pump heat (%g) exceeds 1/4 of chCapDs (%g)."
		// note 25% or 25%% printed garbage.
	}

// default the polynomials

	BOO capTdf=FALSE /*, eirTdf=FALSE*/;
	if (!(sstat[CHILLER_CHPYCAPT+PYBIQUAD_K] & FsSET))					// default chPyCapT coeffs
	{
		static float dat[] = {-1.742040f,.029292f,-.000067f,.048054f,-.000291f,-.000106f};
		memcpy( chPyCapT.k, dat, sizeof(dat));
		capTdf = TRUE;
	}
	if (!(sstat[CHILLER_CHPYEIRT+PYBIQUAD_K] & FsSET))					// default chPyEirT coeffs
	{
		static float dat[] = {3.117600f,-.109236f,.001389f,.003750f,.000150f,-.000375f};
		memcpy( chPyEirT.k, dat, sizeof(dat));
		/*eirTdf = TRUE;*/
	}
	if (!(sstat[CHILLER_CHPYEIRUL+PYCUBIC_K] & FsSET))					// default chPyEirUl coeffs
	{
		static float dat[] = {.222903f,.313387f,.463710f};
		memcpy( chPyEirUl.k, dat, sizeof(dat));
	}

// check & normalize polynomial subrecords

	// uncomment after seeing the spurous warnings, 10-92:
	////next 2: last arg suppresses normalization warning if defaulted cuz the coeff we have are for ARI, not user's, conditions.
	rc |= chPyCapT.normalize(  this, CHILLER_CHPYCAPT,  "design conditions of chTsDs", "chTcndDs", chTsDs, chTcndDs, capTdf );
	rc |= chPyEirT.normalize(  this, CHILLER_CHPYEIRT,  "design conditions of chTsDs", "chTcndDs", chTsDs, chTcndDs /*,eirTdf*/ );
	rc |= chPyEirUl.normalize( this, CHILLER_CHPYEIRUL, "relative load", 1.0);

// eir (energy input ratio) / chCop (coeff of performance===1/eir): allow only one. cop is defaulted, but eir is used internally.

	if (sstat[CHILLER_CHEIRDS] & FsSET)				// if chEirDs entered
	{
		disallow((char *)MH_S0726, CHILLER_CHCOP);		// disallow cop entry. "when chEirDs is given"
		//any checks for eir? limit range?
	}
	else							// eir not entered. chCop entered, or defaulted by CULT table.
	{
		chEirDs = 1./chCop;					// set eir from cop for internal use
#if 0	// don't know if still should be done. asking Bruce in issues2.txt, 10-3-92.
x	// 1 --> 0 10-23-92 .328: Bruce said no, don't adjust.
x      // adjust default eir from ARI to user's conditions. verify that 44,85 ARE the ARI conditions.
x       if (!(sstat[CHILLER_CHCOP] & FsSET))			// if cop was default value, which is for ARI temps
x          chEirDs /= chPyEirT.val(44,85);  			/* adjust cuz our temp correction polynomial is normalized to 1.0
x          							   @chTsDs, ctTcndDs which may not be the ARI 44,85. (equiv to
x          							   normalizing poly @44,85 except re warning message conditions)*/
#endif
	}

// more setup
	eirMinUnldPlr = chPyEirUl.val(chMinUnldPlr);	/* precompute constant energy input (frac of cap) for false loading plrs
    							   (also used, prorated, for cycling plrs) */
// initialization
	chMode = C_OFFONCH_OFF;					// say chiller not running

// aux energy drain checks: warn for meter without power
#define MTRWRN(a,b,c,d) \
       if (sstat[a] & FsSET && !(sstat[b] & FsSET))  oWarn( (char *)MH_S0727, c, d);
	// "'%s' given, but no '%s' to charge to it given"
	MTRWRN( CHILLER_AUXONMTRI     , CHILLER_AUXON     , "chAuxOnMtr"     , "chAuxOn"      );
	MTRWRN( CHILLER_AUXOFFMTRI    , CHILLER_AUXOFF    , "chAuxOffMtr"    , "chAuxOff"     );
	MTRWRN( CHILLER_AUXONATALLMTRI, CHILLER_AUXONATALL, "chAuxOnAtAllMtr", "chAuxOnAtAll" );
	MTRWRN( CHILLER_AUXFULLOFFMTRI, CHILLER_AUXFULLOFF, "chAuxFullOffMtr", "chAuxFullOff" );
#undef MTRWRN

	return rc;						// at least 1 other return above (E macro)
}			// CHILLER::setup
//-------------------------------------------------------------------------------------------------------------------------
//  Towerplants
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::setup()				// check / default / initialize a TOWERPLANT
// outputs include: ctK, if not given by user; ntuADs.
// must be called after coolplants: uses coolplant chain and COOLPLANT.mxCondQ.
{
	RC rc=RCOK;
	BOO odGiven=FALSE;

	if (!cp1 && !hl1)
		return oer( (char *)MH_S0728);		// "TowerPlant has no loads: no coolPlants, no hpLoop HX's."
	//******* wording re hx??
	/* error return now cuz pumpGpm and qLoadMax will be 0 and might be divided by.
		  (to warn & allow run, flag bad? so RLUP skips it?) */

// sum needed quantities from loads

	DBL qLoadMax = 0.;			// for total connected max load (incl pump heats), for ctCapDs,-Od defaults.
	DBL pumpGpm = 0.;			// for total connected max pump gpm, for ctGpmDs, -Od defaults
	for (COOLPLANT* cp = NULL;  nxCp(cp);  )
	{
		qLoadMax += cp->mxCondQ;		/* full-power rejected heat, positive, of coolPlant's most powerful stage,
       					   at design (supply and condenser) temps, incl condenser pump heat */
		pumpGpm += cp->mxCondGpm;  	// pump gpm of same max heat rejection powerful stage (may not be largest cond gpm)
	}
#if 0 // when implemented
0    for (HPLOOP* hl = NULL;  nxHl(hl); )
0	{
0       qLoadMax += hl-> design max load, incl pump heat.
0       pumpGpm += hl->  corress pump gpm
0	}
#endif
	//add if/when reason found:
	//if (qLoadMax <= 0.)
	//   [return] oer( "Total of connected loads (%g) must be > 0", qLoadMax);
	if (pumpGpm <= 0.)  			/* message, and continue, cuz: 1) this message weak since has no imput member name.
        						                       2) ctGpmDs, -Od checks below prevent /0 if dflt'd.*/
		oer( (char *)MH_S0729, pumpGpm);	// "Total pump gpm of connected loads (%g) must be > 0"

// some defaults & checks

	if (ctN <= 0)  return notGzEr(TOWERPLANT_CTN);			// redundant insurance re /0. cncult2.c fcn.
	if (!(sstat[TOWERPLANT_CTCAPDS] & FsSET))  ctCapDs = qLoadMax/ctN;	// default 1-tower design capacity per load
	if (!(sstat[TOWERPLANT_CTCAPOD] & FsSET))  ctCapOd = qLoadMax/ctN;	// default 1-tower "off-design" capacity per load
	// must differ... if both defaulted, may error below.
	ctCapDs = -fabs(ctCapDs);			// make design capacity negative if entered as or defaulted to positive value
	ctCapOd = -fabs(ctCapOd);			// make off-design capacity negative

#if 1// added defaults 10-28-92. BRUCEDFL.
	if (!(sstat[TOWERPLANT_CTSHAFTPWR] & FsSET))  	// default fan shaft brake horsepower
		ctShaftPwr = qLoadMax/(290000.f*ctN);    		// 290000: Bruce & Steve got by inspection of a catalog, 10-92
	if (!(sstat[TOWERPLANT_CTVFDS] & FsSET))    	// default design air flow === full-speed fan cfm
		ctVfDs = qLoadMax/(51.f*ctN);    			// 51: Bruce & Steve got by inspection of a catalog, 10-92.
	if (!(sstat[TOWERPLANT_CTVFOD] & FsSET))      	// off-design air flow. but defaulting both --> same --> error below.
		ctVfOd = qLoadMax/(51.f*ctN);    			// 51: Bruce & Steve got by inspection of a catalog, 10-92.
#endif

	if (ctTy != C_CTTYCH_TWOSPEED)   disallow( (char *)MH_S0730, TOWERPLANT_CTLOSPD);	// "when ctTy is not TWOSPEED". cncult2.c.

	if (!(sstat[TOWERPLANT_CTGPMDS] & FsSET))  ctGpmDs = pumpGpm/ctN;		// dflt design & off-design water flows per loads
	if (!(sstat[TOWERPLANT_CTGPMOD] & FsSET))  ctGpmOd = pumpGpm/ctN;		// ctN 0-checked above.
	if (ctGpmDs <= 0.)  return notGzEr(TOWERPLANT_CTGPMDS);			// prevent /0 below
	// same message for ctGpmDs is below, if required.

	if (ctVfDs <= 0.)   return notGzEr(TOWERPLANT_CTVFDS);	// redundant protection. ctVfDs=0-->maOverMwDs=0-->log(0) below.

// off-design conditions

	char *when;									// sprintf insert, don't use MH
	if ( (  sstat[TOWERPLANT_CTCAPOD]  | sstat[TOWERPLANT_CTVFOD]    		// if gave any of ctCapOd, ctVfOd,
			| sstat[TOWERPLANT_CTGPMOD]  | sstat[TOWERPLANT_CTTDBOOD]		//   ctGpmOd, ctTDbOOd, ctTWbOOd, ctTwoOd:
			| sstat[TOWERPLANT_CTTWBOOD] | sstat[TOWERPLANT_CTTWOOD]  ) & FsSET )	// merge 6 status bytes then test bit
	{
		// at least one of off-design conditions given. Require usable off-design point input and use it.

		odGiven = TRUE;								// say compute ctK below, from Od values

		when = "when any of the off-design conditions\n"				// error message insert, MH not supported
			   "    (ctCapOd,ctVfOd,ctGpmOd,ctTDbOOd,ctTWbOOd,ctTwoOd) are given";

#if 0	// all now have defaults (BRUCEDFL) 10-92.
x       rc |= requireN( when, /*TOWERPLANT_CTCAPOD,*/    /*TOWERPLANT_CTVFOD*/, 		// require off-design inputs w/o defaults
x                             /*TOWERPLANT_CTGPMOD,*/    /*TOWERPLANT_CTTDBOOD,*/   	// /*commented*/ ones have default ..
x                             /*TOWERPLANT_CTTWBOOD,*/   /*TOWERPLANT_CTTWOOD,*/ 0 );	// .. in CULT table or in code above.
#endif
		// cncult2.c functions.
		rc |= disallow( when, TOWERPLANT_CTK);				// ctK cannot be given: it will be computed from Od inputs

		if (ctGpmOd <= 0.)  						// check ctGpmOd only when input required
			return notGzEr(TOWERPLANT_CTGPMOD);				// prevent /0 below (uses ooer)

		if (ctVfOd <= 0.)   						// check only when input required
			return notGzEr(TOWERPLANT_CTVFOD);				// redundant /0 protection (uses ooer)

		if (ctVfOd==ctVfDs)								// must be different. fuzz?
			ooer( TOWERPLANT_CTVFOD, TOWERPLANT_CTVFDS, 				// msg only if no msg yet for either field
				  (sstat[TOWERPLANT_CTVFOD] | sstat[TOWERPLANT_CTVFDS]) & FsSET		// if either given, use different text
				  ?  (char *)MH_S0731		// "ctVfOd (%g) is same as ctVfDs.\n    These values must differ %s."
				  :  (char *)MH_S0732,		// "ctVfOd and ctVfDs are both defaulted to %g.\n    These values must differ %s."
				  ctVfOd, when );

		if (ctGpmOd==ctGpmDs)								// must be different. fuzz?
			ooer( TOWERPLANT_CTGPMOD, TOWERPLANT_CTGPMDS, 				// message function in cul.c
				  (sstat[TOWERPLANT_CTGPMOD] | sstat[TOWERPLANT_CTGPMDS]) & FsSET		// if either given, use different text
					?  (char *)MH_S0733		// "ctGpmOd (%g) is same as ctGpmDs.\n    These values must differ %s."
					:  (char *)MH_S0734,		//"ctGpmOd and ctGpmDs are both defaulted to %g.\n    These values must differ %s."
				  ctVfOd, when );
	}
	else		// no off-design conditions given
	{
		//odGiven = FALSE	initialized FALSE above
		when = "when none of the off-design conditions \n"
			   "    (ctCapOd,ctVfOd,ctGpmOd,ctTDbOOd,ctTWbOOd,ctTwoOd) are given";
		rc |= disallowN( when, TOWERPLANT_CTCAPOD,    TOWERPLANT_CTVFOD, 	// insurance, believe currently 9-92 redundant
						 TOWERPLANT_CTGPMOD,    TOWERPLANT_CTTDBOOD,	// cncult2.c fcn
						 TOWERPLANT_CTTWBOOD,   TOWERPLANT_CTTWOOD, 0 );
		if (ctK >= 1.)								// limits have checked for 0 < ctK <= 1.
			ooer( TOWERPLANT_CTK, (char *)MH_S0735);			// error msg "ctK must be less than 1.0". cul.c.

		//ctGpmOd may be 0, but ctGpmOd/mwOd not used if !odGiven.
	}

// fan curve polynomials

	switch (ctTy)		// default/check/normalize fan polys for type of (lead) fan used.
	{
	case C_CTTYCH_VARIABLE:
		if (!(sstat[TOWERPLANT_CTFCVAR] & FsSET))		// if ctFcVar not given
		{
			static float dat[]= {0.,0.,0.,1.};
			memcpy( ctFcVar.k, dat, sizeof(dat));		// default cubic coeff to 0, 0, 0, 1
		}
		ctFcVar.normalize( this, TOWERPLANT_CTFCVAR, "relative speed", 1.0);	// check for value 1.0 at speed 1.0, warn, adjust.
		break;

	case C_CTTYCH_ONESPEED:
		if (!(sstat[TOWERPLANT_CTFCONE] & FsSET))		// if ctFcOne not given
		{
			ctFcOne.k[0] = 0.;
			ctFcOne.k[1] = 1.;		// default linear coeff to 0, 1
		}
		ctFcOne.normalize( this, TOWERPLANT_CTFCONE, "relative speed", 1.0);	// check for value 1.0 at speed 1.0, warn, adjust.
		break;

	case C_CTTYCH_TWOSPEED:
		if (!(sstat[TOWERPLANT_CTFCLO] & FsSET))		// if ctFcLo (low speed) linear coefficients not given
		{
			ctFcLo.k[0] = 0.;
			ctFcLo.k[1] = ctLoSpd*ctLoSpd;	// default to 0, ctLoSpd^2 (power proportional to cube of flow)
		}
		if (!(sstat[TOWERPLANT_CTFCHI] & FsSET))			// if ctFcHi (hi speed) linear poly coeff not given
		{
			float tem = ctLoSpd*(ctLoSpd+1.f);		// these values make it ctLoSpd^3 @ ctLoSpd, 1.0 @ 1.0:
			ctFcHi.k[0] = -tem;			//  -ctLoSpd^2 - ctLoSpd
			ctFcHi.k[1] = tem + 1.;			//  ctLoSpd^2 + ctLoSpd + 1
		}
		ctFcHi.normalize( this, TOWERPLANT_CTFCHI, "relative speed", 1.0);	// check for value 1.0 at speed 1.0, warn, adjust.
		// ctFcLo is not used at speed 1.0, can't check it for 1.0 or normalize it, but can check curve junction @ ctLoSpd.
		float lo = ctFcLo.val(ctLoSpd), hi = ctFcHi.val(ctLoSpd);		// power should not drop when start using hi speed
#if 1					// initial temp devel aid 9-92: check defaults by checking for exact meet
		if (fabs(lo - hi) > .00001)						// check that (default) lo, hi curves meet
#else					// this is all we should check in general
*          if (hi < lo - .0001)							// (step rise ok: simulates start-stop overhead.)
#endif
			ooer( TOWERPLANT_CTFCLO, TOWERPLANT_CTFCHI, 			// msg if no msg yet for these fields (cul.c)
				  (char *)MH_S0736, lo, hi );			/* "Inconsistent low and hi speed fan curve polynomials:\n"
     								   "    ctFcLo(ctLoSpd)=%g, but ctFcHi(ctLoSpd)=%g." */
		break;
	}

// disallow inappropriate fan curve polynomials

	if (ctTy != C_CTTYCH_ONESPEED)   disallow((char*)MH_S0737, TOWERPLANT_CTFCONE);	// "when ctType is not ONESPEED"
	if (ctTy != C_CTTYCH_TWOSPEED)   disallowN( (char *)MH_S0738, TOWERPLANT_CTFCLO, 	// "when ctType is not TWOSPEED"
				TOWERPLANT_CTFCHI, 0 );	// cncult2.c fcns.
	if (ctTy != C_CTTYCH_VARIABLE)   disallow((char*)MH_S0739, TOWERPLANT_CTFCVAR);	// "when ctType is not VARIABLE"

// subexpressions and units conversions

	if (ctMotEff <= 0.)  return notGzEr(TOWERPLANT_CTMOTEFF);	// redundant /0 protection. fcn in cncult2.c.
	oneFanP = ctShaftPwr / ctMotEff;				// 1-fan motor input (Btuh) = shaft power (Btuh here) / efficiency
	//convert cfms to lb/hr @ temps. air density is temp-dependent. *60 converts /min to /hr.
	maDs = ctVfDs * Top.airDens(ctTDbODs) * 60.;		// design air flow (1 tower): cfm-->lb/hr, @ design outdoor temp
	maOd = ctVfOd * Top.airDens(ctTDbOOd) * 60.;		// off-design air flow ..., @ off-design outdoor temp
	//convert gpms to lb/hr. 60 is minutes per hour.
#define LBperGAL 8.337 					// @ 62 F, CRC handbook. Note cons.c shows 8.3454 -- for 39F?
	mwDs = ctGpmDs * LBperGAL * 60.;     			// design water flow into 1 tower: gpm-->lb/hr
	mwOd = ctGpmOd * LBperGAL * 60.;     			// off-design ...: gpm-->lb/hr
	maOverMwDs = maDs/mwDs;					// precompute design air flow / water flow, both lb/hr.

	if (rc)   return rc;			// don't attempt following setup with bad inputs

// determine ntuADs: ntu on air side at design conditions: characterizes tower performance at dsgn cond. Niles VI-C-3-b.

	CSE_E( setupNtuADs() )				// function follows. E macro returns to our caller upon error return.

// conditionally determine ctK: Niles "k", for formula  ntuA = const*(mw/ma)^k .  Niles VI-C-3-b continued.

	if (odGiven)				// if off-design conditions given (else ctK given or defaulted per CULT table)
	{
		CSE_E( setupNtuAOd() )						// determine ntuAOd: off-design air side ntu. below.
		if (fabs(maOverMwDs - maOd/mwOd) < 1.e-10)			// prevent divide by 0 (log of 1) in next statement
			return oer( (char *)MH_S0740,				/* "maOd/mwOd must not equal maDs/mwDs.\n"
									   "    maOd=%g  mwOd=%g  maDs=%g  mwDs=%g\n"
									   "    maOd/mwOd = maDs/mwDs = %g" */
						maOd, mwOd, maDs, mwDs,  maOverMwDs );
		ctK = log(ntuAOd/ntuADs) / log(maOverMwDs*mwOd/maOd);		// calcultate k.  log = natural logarithm.
	}  									// this finishes VI-C-3-b.

	if (ctK <= 0. || ctK >= 1.)						// insurance (previously ck'd if input)
		ooer( TOWERPLANT_CTK, (char *)MH_S0741);			// "ctK (%g) did not come out between 0 and 1 exclusive".
	// cul.c function.
	return rc;						// more return(s) above, incl E macros and several /0 protections
}			// TOWERPLANT::setup
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::setupNtuADs()		// compute ntuADs for towerplant setup. # transfer units, air side, design conditions
{
	RC rc=RCOK;
	char *design = "design";	// insert for error messages so same message texts can be used re off-design conditions

// preliminary checks

	if (ctTWbODs >= ctTwoDs)				// can't cool water below outdoor wetbulb. Would make hswoDs <= haiDs.
		return ooer( TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,	// msg if no msg yet for these members
					 (char *)MH_S0742,				/* "%s outdoor wetbulb temperature (ctTWbODs=%g) must be\n"
								   "    less than %s leaving water temperature (ctTwoDs=%g)" */
					 design, ctTWbODs,    design, ctTwoDs );

// entering and exiting air and sat air equiv of water enthalpies, and slope

	DBL ctTwiDs = ctTwoDs - ctCapDs/(mwDs*CPW);  		// entering water temp under design conditions: exit - (neg) q
	DBL haiDs = psySatEnthalpy(ctTWbODs);			// inlet air enthalpy at design outdoor wetbulb
	//Niles:
	//DBL haoDs = haiDs + mwDs*CPW*(ctTwiDs - ctTwoDs)/maDs;	// outlet air enthalpy (heat content) from heat balance
	//Rob's simplification 10-92:
	DBL haoDs = haiDs - ctCapDs/maDs;				// outlet air enthalpy (heat content) from heat balance
	DBL cs = psySatEnthSlope((ctTwiDs + ctTwoDs)/2.);   	// sat dh/dt at average water temp
	DBL hswiDs = psySatEnthalpy(ctTwiDs);			// sat air enth at water inlet temp
	DBL hswoDs = psySatEnthalpy(ctTwoDs);					// sat air enth at outlet inlet temp

// errors for impossible design conditions / protect re /0, log(0), log(<0):

	if (haiDs >= hswoDs)	// if sat air enthalpy at exit water temp colder than entering air enth (impossible water cooling)
		return ooer(  					// should have issued "ctTWbODs not < ctTwoDs" above -->no msg here
					 TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,	// if no msg yet for these members, give this msg
					 (char *)MH_S0743, haiDs, hswoDs);		// "Internal error in setupNtuaADs: haiDs (%g) not < hswoDs (%g)"

	if (haoDs >= hswiDs)	// if exiting air enth > enthalpy of sat water at entering water temp (impossible air heating)
		return ooer( TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,	// msg if no msg yet for these members
					 (char *)MH_S0744,				/* "%s conditions produce impossible air heating:\n"
								   "    enthalpy of leaving air (%g) not less than enthalpy of\n"
								   "    saturated air (%g) at leaving water temp (ctTWbODs=%g).\n"
								   "    Try more air flow (ctVfDs=%g)." */
					 design, haoDs,   hswiDs, ctTWbODs,    ctVfDs );

#if 0	// case added below 10-23-92
x    if (1. - cs*maOverMwDs/CPW==0.)		// would divide by 0 (but + or - may be ok; log in numerator may also change sign).
x       return ooer( this, TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,			// msg if no msg yet for these members
x                    "CSE can't handle %s conditions where heat capacity of water flow\n"
x                    "    is exactly equal to heat capacity of saturated air flow:\n"
x                    "          maDs*dh/dt = mwDs*cw\n"
x                    "    where maDs =  %s air flow (lb/hr) = %g\n"
x                    "          dh/dt = avg slope of sat line on psych chart (Btu/lb-F) = %g\n"
x                    "          mwDs =  %s water flow (lb/hr) = %g\n"
x                    "          cw =    heat capacity of water (Btu/lb-F) = 1.0\n"
x                    "    Contact Phil Niles and ask him to expand the CSE specification\n"
x                    "    to cover this case (Section VI-C-3-b, page 176-177);\n"
x                    "    Meanwhile, change one of the inputs, such as ctVfDs, just a little.",
x                    design,   design, maDs,   cs,   design, mwDs );
#endif

// ok, now compute air side # transfer units

#if 0	// case added below 10-23-92
x    /* Need to add case for maDs*cs==mwDs*CPW: equal heat capacity flows. Memo to Phil 10-14-92. Then remove errMsg above.
x       Following /0's if ==; looks like can handle either > (numerator and denominator change sign together) Rob 10-92 */
#endif

	if (1. - cs*maOverMwDs/CPW==0.)					// if would divide by 0 (add small range?)
		ntuADs = (haoDs - haiDs)/(hswiDs - haoDs);					// compute this way
	else										// normally compute thus
		ntuADs = -log((haoDs - hswiDs)/(haiDs - hswoDs)) / (1. - cs*maOverMwDs/CPW);	//  log = natural logarithm.
	if (ntuADs <= 0.)  									// prevent /0 (by caller)
		rc = oer( (char *)MH_S0745, ntuADs);					// "Internal error: ntuADs (%g) not > 0"
	return rc;										// also error returns above
}			// TOWERPLANT::setupNtuADs
//-------------------------------------------------------------------------------------------------------------------------
RC TOWERPLANT::setupNtuAOd()		// compute ntuAOd for towerplant setup: # transfer units, air side, off-design conditions
// call only if off-design conditions given and ctK is being derived from them.
// same as setupNtuADs except uses off-design inputs.
{
#if 0	// original version 10-92. Unrun, but original setupNtuADs (not kept) ran ok.
x    RC rc=RCOK;
x    DBL ctTwiOd = ctTwoOd + fabs(ctCapOd)/(mwOd*CPW); 			// off-design entering water temp
x    DBL haiOd = psySatEnthalpy(ctTWbOOd);  				// inlet air enthalpy at design outdoor wetbulb
x    DBL haoOd = haiOd + mwOd*CPW*(ctTwiOd - ctTwoOd)/maOd;		/* outlet air enthalpy (heat content) from heat balance
x    									   -- compute from change in heat content of water. */
x    DBL cs = psySatEnthSlope((ctTwiOd + ctTwoOd)/2);			// sat dh/dt at average water temp
x    DBL hswiOd = psySatEnthalpy(ctTwiOd); 				// sat air enth at water inlet temp
x    DBL hswoOd = psySatEnthalpy(ctTwoOd);    					// sat air enth at outlet inlet temp
x    ntuAOd = -log((haoOd - hswiOd)/(haiOd - hswoOd)) / (1. - cs*maOd/(mwOd*CPW));	// log = natural logarithm.
x    //ntuAOd is used only by caller re ctK, could be returned via an arg; is member to keep as debug aid or for reporting.
x    if (ntuAOd <= 0.)  									// prevent log(0) (by caller)
x       rc = oer( "Internal error: ntuAOd (%g) not > 0", ntuAOd);
x    return rc;
#else	// 10-14-92 revision with simplifications and more messages, as for setupNtuADs.
	RC rc=RCOK;
	DBL maOverMwOd = maOd/mwOd;		// (have no -Od member corress to maOverMwDs)
	char *design = "off-design";	// insert for error messages so same message texts can be used re design conditions

// preliminary checks

	if (ctTWbOOd >= ctTwoOd)				// can't cool water below outdoor wetbulb. Would make hswoOd <= haiOd.
		return ooer( TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,	// msg if no msg yet for these members
					 (char *)MH_S0746,				/* "%s outdoor wetbulb temperature (ctTWbOOd=%g) must be\n"
								   "    less than %s leaving water temperature (ctTwoOd=%g)" */
					 design, ctTWbOOd,    design, ctTwoOd );

// entering and exiting air and sat air equiv of water enthalpies, and slope

	DBL ctTwiOd = ctTwoOd - ctCapOd/(mwOd*CPW);  		// entering water temp under off-design conditions: exit - (neg) q
	DBL haiOd = psySatEnthalpy(ctTWbOOd);			// inlet air enthalpy at off-design outdoor wetbulb
	//Niles:
	//DBL haoOd = haiOd + mwOd*CPW*(ctTwiOd - ctTwoOd)/maOd;	// outlet air enthalpy (heat content) from heat balance
	//Rob's simplification 10-92:
	DBL haoOd = haiOd - ctCapOd/maOd;				// outlet air enthalpy (heat content) from heat balance
	DBL cs = psySatEnthSlope((ctTwiOd + ctTwoOd)/2.);   	// sat dh/dt at average water temp
	DBL hswiOd = psySatEnthalpy(ctTwiOd);			// sat air enth at water inlet temp
	DBL hswoOd = psySatEnthalpy(ctTwoOd);					// sat air enth at outlet inlet temp

// errors for impossible off-design conditions / protect re /0, log(0), log(<0):

	if (haiOd >= hswoOd)	// if sat air enthalpy at exit water temp colder than entering air enth (impossible water cooling)
		return ooer(  					// should have issued "ctTWbOOd not < ctTwoOd" above -->no msg here
					 TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,	// if no msg yet for these members, give this msg
					 (char *)MH_S0747, haiOd, hswoOd);		// "Internal error in setupNtuaAOd: haiOd (%g) not < hswoOd (%g)"

	if (haoOd >= hswiOd)	// if exiting air enth > enthalpy of sat water at entering water temp (impossible air heating)
		return ooer( TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,	// msg if no msg yet for these members
					 (char *)MH_S0748,				/* "%s conditions produce impossible air heating:\n"
								   "    enthalpy of leaving air (%g) not less than enthalpy of\n"
								   "    saturated air (%g) at leaving water temp (ctTWbOOd=%g).\n"
								   "    Try more air flow (ctVfOd=%g)." */
					 design, haoOd,   hswiOd, ctTWbOOd,    ctVfOd );

#if 0	// case added below 10-23-92
x    if (1. - cs*maOverMwOd/CPW==0.)		// would divide by 0 (but + or - may be ok; log in numerator may also change sign).
x       return ooer( this, TOWERPLANT_CTTWODS, TOWERPLANT_CTTWBODS,			// msg if no msg yet for these members
x                    "CSE can't handle %s conditions where heat capacity of water flow\n"
x                    "    is exactly equal to heat capacity of saturated air flow:\n"
x                    "          maOd*dh/dt = mwOd*cw\n"
x                    "    where maOd =  %s air flow (lb/hr) = %g\n"
x                    "          dh/dt = avg slope of sat line on psych chart (Btu/lb-F) = %g\n"
x                    "          mwOd =  %s water flow (lb/hr) = %g\n"
x                    "          cw =    heat capacity of water (Btu/lb-F) = 1.0\n"
x                    "    Contact Phil Niles and ask him to expand the CSE specification\n"
x                    "    to cover this case (Section VI-C-3-b, page 176-177);\n"
x                    "    Meanwhile, change one of the inputs, such as ctVfOd, just a little.",
x                    design,   design, maOd,   cs,   design, mwOd );
#endif

// ok, now compute air side # transfer units

#if 0	// case added below 10-23-92
x    /* Need to add case for maOd*cs==mwOd*CPW: equal heat capacity flows. Memo to Phil 10-14-92. Then remove errMsg above.
x       Following /0's if ==; looks like can handle either > (numerator and denominator change sign together) Rob 10-92 */
#endif

	if (1. - cs*maOverMwOd/CPW==0.)					// if would divide by 0 (add small range?)
		ntuAOd = (haoOd - haiOd)/(hswiOd - haoOd);					// compute this way
	else										// normally compute thus
		ntuAOd = -log((haoOd - hswiOd)/(haiOd - hswoOd)) / (1. - cs*maOverMwOd/CPW);	//  log = natural logarithm.
	if (ntuAOd <= 0.)  									// prevent /0 (by caller)
		rc = oer( (char *)MH_S0748, ntuAOd);					// "Internal error: ntuAOd (%g) not > 0"
	return rc;										// also error returns above
#endif
}				// TOWERPLANT::setupNtuAOd
//-----------------------------------------------------------------------------------------------------------------------------
// subrecords
//-----------------------------------------------------------------------------------------------------------------------------
RC PUMP::setup( record *r, SI pumpFn)			// initialize a PUMP subrecord
{
	RC rc=RCOK;

// defaults
	if (ovrunF==0.)  ovrunF = 1.3;		// in case not set by CULT table entry

// checks
	if (ovrunF < 1.0)
		r->oer( (char *)MH_S0749, r->mbrIdTx( pumpFn+PUMP_OVRUNF) );		// "%s must be >= 1.0"

// initialization
#define LBperGAL 8.337 			// @ 62 F, CRC handbook. Note cons.c shows 8.3454 -- for 39F?. also in cncult5.c.
	mw = gpm * LBperGAL * 60.;			// convert gallons-per-minute into pounds per hour
	//mwMx = mw * ovrunF;   			// lb/hr flow including overrun
	// pump power: mw: lb/hr, /62.42x --> ft3/hr.  hdLoss: ft H2O, *62.42x --> lb/ft2. product is ft-lbs/hr; factors cancel!
#define FTLBperBTU 777.97			// (mean Btu -- 1054.8 Joules)
	q = mw * hdLoss / (hydEff * FTLBperBTU);	// shaft power === heat to water (Btuh)
	p = q / motEff;				// electrical input (Btuh). motor heat (to air) is lost.

	return rc;
}		// PUMP::setup
//-----------------------------------------------------------------------------------------------------------------------------

// end of cncult6.cpp