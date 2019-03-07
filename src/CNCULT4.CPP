// Copyright (c) 1997-2017 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// search NUMS for new texts to extract, 6-95

// rpCond needs check like colVal. search ???. 12-9-91.

// cncult4.cpp: cul user interface tables and functions for CSE:
//   part 4: fcns re reports/exports, and holidays, called from top table, & their subfcns.  used from cncult2.cpp.


/*------------------------------- INCLUDES --------------------------------*/
#include <cnglob.h>	// USI SI LI

#include <ancrec.h>	// record: base class for rccn.h classes
#include <rccn.h>	// TOPRATstr ZNRstr SFIstr SDIstr
#include <msghans.h>	// MH_S0540

#include <xiopak.h>	// xfExist xfdelete
#include <tdpak.h>	// tdHoliDate
#include <rmkerr.h>	// openErrVr  openLogVr

#include <vrpak.h>	// vrOpen VR_FMT VR_FINEWL
#include <cnguts.h>	// Top
#include <impf.h>	// IMPF::close;  impFcn declaration.

#include <pp.h>   	// ppFindFile openInpVr
#include <cutok.h>	// cuEr
#include <cuparse.h>	// TYFL TYSTR evfTx
#include <cuevf.h>	// evf's & variabilities: VMHLY VSUBHRLY
#include <exman.h>	// exInfo
#include <cul.h>	// FsSET FsVAL ratCpy oer oWarn
#include <cueval.h>	// cupfree cupIncRef

//declaration for this file:
#include <cncult.h>	// use classes, globally used functions
#include <irats.h>	// declarations of input record arrays (rats)
#include <cnculti.h>	// cncult internal functions shared only amoung cncult,2,3,4.cpp


/*-------------------------------- DEFINES --------------------------------*/


/*---------------------------- LOCAL VARIABLES ----------------------------*/

// formatted texts for report titles, header, footer.  Access by calling getErrTitle, getHeaderText, etc below.
// these locations are NULL'd (with dmfree) whenever text should be regenerated becuase inputs may have changed
// (most are subject to use during input, possibly b4 runTitle, repHdrL, and repHdrR strings have been decoded).

LOCAL char* errTitle = NULL;	// NULL or dm pointer for title text for ERR report, used from rmkerr.cpp at 1st msg
LOCAL char* logTitle = NULL;	// NULL or dm pointer for title text for LOG report, used from rmkerr.cpp at 1st msg
LOCAL char* inpTitle = NULL;	// NULL or dm pointer for title text for INP report, used from ?? .cpp at 1st msg
LOCAL char* header = NULL;		// NULL or dm pointer to formatted page header, including TopM newlines
LOCAL char* footer = NULL;		// NULL or .. footer .. .  Insert page number at each use
LOCAL char* footerPageN = NULL;	// NULL or ptr into footer at which to store current page number \r\n\0.


/*---------------------- LOCAL FUNCTION DECLARATIONS ----------------------*/
LOCAL RC   addRep( TI rfi, char *name, RPTYCH rpTy, TI zi, IVLCH freq, SI putAtEnd);
LOCAL void addTx( const char* s, int spc, char **pp, int* pr, int rReserve=0);
LOCAL void addHdayDate( char *name, DOY date);
LOCAL void addHdayRule( char *name, HDAYCASECH hdCase, DOWCH dow, MONTH mon);



///////////////////////////////////////////////////////////////////////////////
// REPORTS / EXPORTS							      *
///////////////////////////////////////////////////////////////////////////////
RC topPrfRep()	// REPORT/EXPORT PRE-INPUT FCN called by cncult2.cpp:topStarPrf

// generate primary/default REPORTFILE and EXPORTFILE rat entries, and default REPORT entries.

// called from topStarPrf, which is called after data cleared & b4 start of data input,
//  at startup and after CLEAR. 1-92.

// returns non-RCOK if error.  err ret must be propogated via to cul:cuf to stop run in case error did not errCount++.
{
	TI priRfi=0;
	UCH *fs;
	RC rc;

	/* These files are used for REPORTs and EXPORTs specifying no file or file "Primary".
	   File names are derived from input file name.
	   User can change record with "ALTER" and (i think) "DELETE". */

	if (RfiB.n==0)					// if no report files yet -- insurance: recall not expected
	{
		RFI *rfp;
		E( RfiB.add( &rfp, WRN) )   			// add report file record for PRIMARY REPORTFILE / return if error
		fs = (UCH *)rfp + RfiB.sOff;			// point record's field status bytes (sstat[])
		strcpy( rfp->name, "Primary" );   		// record name
		rfp->fileName = strsave( strffix2( InputFilePathNoExt, ".rep", 1));
												// file name: use input file name w/o extension (includes path)
												//   InputFilePathNoExt known to not have extension, always add ".rep"
		rfp->fileStat = C_FILESTATCH_OVERWRITE;  	// overwriting existing file. rfStarCkf changes to APPEND if not 1st run.
		rfp->pageFmt = C_NOYESCH_YES;			// page formatting on for default report file
		fs[RFI_FILENAME] |= FsSET|FsVAL; 		// flag set fields as "set", else required fields get error messages
		fs[RFI_PAGEFMT]  |= FsSET|FsVAL; 		// ... in our check functions later.
		//fs[RFI_FILESTAT] |= FsSET|FsVAL; 		do not flag set, so no warning on OVERWRITE-->APPEND if not 1st run.
		priRfi = rfp->ss;				// subscript (1) used below
	}
	if (XfiB.n==0)					// if no export files yet -- insurance: recall not expected
	{
		RFI *xfp;
		E( XfiB.add( &xfp, WRN) )    			// add export file record for PRIMARY EXPORTFILE / return if error
		fs = (UCH *)xfp + XfiB.sOff;			// point record's field status bytes (sstat[])
		strcpy( xfp->name, "Primary" );  		// record name
		xfp->fileName = strsave( strffix2( InputFilePathNoExt, ".csv", 1));
												// file name: use input file name & path
												//   InputFilePathNoExt known to not have extension, always add ".csv"
		xfp->fileStat = C_FILESTATCH_OVERWRITE;	// overwriting existing file. xfStarCkf changes to APPEND if not 1st run.
		fs[RFI_FILENAME] |= FsSET|FsVAL; 		// flag set field as "set", so no error message in topXf.
		//fs[RFI_FILESTAT] |= FsSET|FsVAL; 		do not flag: would get warning on OVERWRITE-->APPEND if not 1st run.
	}

// generate default REPORT records, in output order.  User can change record with "ALTER" and (i think) "DELETE".  11-91.

	// hmm... if user deletes primary report file and defines another, do these reports end up in it? 11-91

	if (RiB.n==0)			// if no report entries yet -- recall insurance
	{
		E( addRep( priRfi, "Err",  C_RPTYCH_ERR,  0, 0, 0) ) 		// ERR report: Error messages
		E( addRep( priRfi, "Sum",  C_RPTYCH_SUM,  0, 0, 0) ) 		// SUM report: Summary
		E( addRep( priRfi, "eb",   C_RPTYCH_ZEB, TI_SUM, C_IVLCH_M, 0) )	// ZEB: Zones Energy Balance, sum of all zones, monthly, all months
		// user-specified reports appear here
		E( addRep( priRfi, "Log",  C_RPTYCH_LOG,  0, 0, 1) )		// LOG report: run messages
		E( addRep( priRfi, "Inp",  C_RPTYCH_INP,  0, 0, 1) )		// INP report: input echo, including some or all errMsgs
	}

	// note: there are no default exports.

	return RCOK;			// also E macros have error returns
}			// topPrfRep
//===========================================================================
LOCAL RC addRep( TI rfi, char *name, RPTYCH rpTy, TI zi, IVLCH freq, SI putAtEnd)

// add non-zone-dependent report record for given report file (rfi).  for topStarPrf.

{
	RI *rp;
	UCH *fs;
	RC rc;

	E( RiB.add( &rp, WRN) ) 			// add report input record (ancrec.cpp) / return if error
	strncpy( rp->name, name, sizeof(ANAME)-1 );	// record name, for like/alter/delete
	rp->zi = zi;				// 0, TI_SUM, or TI_ALL (or reference to a zone)
	rp->ownTi = rfi;       			// reference to report file
	rp->rpTy = rpTy;     			// type of report
	rp->rpFreq = freq;  			// 0 or frequency of report
	rp->rpBtuSf = 1e6f;				// energy print scale factor SHOULD MATCH DEFAULT IN CNCULT.C:rpT[] and exT[].
	rp->rpCond = 1;					// make report print
	rp->isExport = 0;   			// is report, not an export
	rp->putAtEnd = putAtEnd;			// put-after-user's-reports request. Only place set, 11-91.
	// dayBeg, dayEnd are left 0: will default to entire run for MONTHLY frequency.

	fs = (UCH *)rp + RiB.sOff;  		// point record's field status bytes (sstat[])
	fs[RI_OWNTI] |= FsSET|FsVAL;    		// flag fields just set as "set" and "value is stored" ...
	fs[RI_RPTY]  |= FsSET|FsVAL;    		// ... because our check functions may check later.
	if (zi)
		fs[RI_ZI]    |= FsSET|FsVAL;
	if (freq)   				// ... if non-0 freq given (else don't flag: rpFreq entry can be an error)
		fs[RI_RPFREQ] |= FsSET|FsVAL;
	//rpCond is a default: don't flag.  isExport, putAtEnd: not user-enterable, need no flags.

	return RCOK;
}		// addRep



//============================== REPORT/EXPORT CHECK/FINISH FCNS called by cncult2.cpp:topCkf ================================
RC topRep()  	// check Top rep___ members, set up page header and footer 11-91

{
#define HDRROWS 3		// number of lines on page to reserve for page header
#define FTRROWS 3		// .. footer

	RC rc = RCOK;

// check page dimensions

	if (Top.repCpl < 72 || Top.repCpl > 132)
		rc = Top.oer( (char *)MH_S0540, Top.repCpl);  	// "repCpl %d is not between 78 and 132"
	if (Top.repLpp < 48)
		rc = Top.oer( (char *)MH_S0541, Top.repLpp);  	// "repLpp %d is less than 50"
	if (Top.repTopM < 0 || Top.repTopM > 12)
		rc = Top.oer( (char *)MH_S0542, Top.repTopM); 	// "repTopM %d is not between 0 and 12"
	if (Top.repBotM < 0 || Top.repBotM > 12)
		rc = Top.oer( (char *)MH_S0543, Top.repBotM); 	// "repBotM %d is not between 0 and 12"

	return rc;
}			// topRep
//===========================================================================
RC topRf( )  	// check REPORTFILEs 11-91

// must also call buildUnspoolInfo (after processing REPORTs).
{
	RFI *rfp;
	RC rc;

	RLUP( RfiB, rfp)
		E( rfp->rf_CkF2( 0));	// 0: report (as opposed to export)
	return RCOK;
}			// topRf
//===========================================================================
RC topXf()   	// check EXPORTFILEs 11-91

// must also call buildUnspoolInfo
{
	RFI *xfp;
	RC rc;

	RLUP( XfiB, xfp)
		E( xfp->rf_CkF2( 1))	// 1: export (as opposed to report)
	return RCOK;
}			// topXf
//===========================================================================
RC topCol( SI isExport)

// check REPORTCOLs or EXPORTCOLs / build run rat RcolB or XcolB / links columns for each report/export

// call before topRxp
// returns RCOK if ok.  CAUTION: be sure non-RCOK return propogates back to cul:cuf to insure errCount++.
{
	RC rc;				// used in E macros

#define RPDEFWID 10 	// report default column width
#define EXDEFWID 13		// export default "max column width" before deblanking: enuf for "-1.23456e+123".
						//   coordinate changes with defaulted # sig digits (6, cgresult.cpp:vpUdtExRow)
						//   and with built-in export format (cgresult.cpp:vpRxRow)

	anc<COL> * coliB = isExport ? &XcoliB : &RcoliB;    	// column input rat
	anc<COL> * colB = isExport ? &XcolB : &RcolB; 		// column run rat
	anc<RI> * rxB = isExport ? &XiB : &RiB;			// report/export input rat
	const char* exrp = isExport ? "ex" : "rp";    			// to insert in member names in error messages
	const char* exrePort = isExport ? "export" : "report";	// to insert in errmsgs


// init info set here in EXPORTs/REPORTs input RAT
	RI* rp;
	RLUP( *rxB, rp)
	{
		rp->coli = 0;     // no 1st column, no columns, no width
		rp->nCol = 0;
		rp->wid = 0;
	}


// clear columns runtime RAT.  A separate run RAT is required re expressions.

	E( colB->al( coliB->n, WRN) )	// delete any old records, alloc needed # COL records now now for min fragmentation.
	// E: return if error.  CAUTION: ratCre may not errCount++ on error.
	// note: prior run if any was cleared, with dmfree's, in cnguts.cpp.
	// CAUTION: run .ownB's also set in cnguts:cgPreInit; if changed here, change there for showProbeNames.
	colB->ownB = rxB;			/* columns are owned by reports and report run subscripts match report input subscripts.
					   Setting run rat .ownB enables use of report subscript from input context
					   for resolution of ambiguous references by name at runtime (probes) 1-92.
					   (currently 1-7-92 only used for nz test and .what display;
					   have no report run rat so use input rat). */

// copy input rat entries to run rat entries, check, init, set info in owning report
	COL* colip;
	COL* colp;
	RLUP( *coliB, colip)				// loop input column RAT records
	{
		// copy data from input rat to run rat, same subscript (needed in this case?)
		colB->add( &colp, ABT, colip->ss);		// add run COL record, ret ptr to it.  err unexpected cuz al'd.
		*colp = *colip;					/* copy entire entry incl name, and incr ref count of heap pointers
       							   (.colHead; colVal.val if constant (unlikely!)) */

		// check references
		if (ckRefPt( rxB, colip, colip->ownTi,  isExport ? "colExport" : "colReport",  NULL, (record **)&rp ) )
			// check report / get ptr to it / if bad
			continue;								// if no or bad report, done with this one
		if (rp->rpTy  &&  rp->rpTy != C_RPTYCH_UDT)
			colp->oer( (char *)MH_S0545,  exrePort, exrePort, exrp);	// "%sCol associated with %s whose %sType is not UDT"

		// check variability of end-of-interval eval'd expression vs frequency of report
		/* problems of too-frequent reporting can include:
		   1) may not be eval'd when first printed ("?" or warmup value may show)
		   2) results values: may be eg last month's value until last day of month.
		   3) results avg values may have working totals not yet divided by # values
		   (if another operand in expr forces faster probing). */
		if ( ISNANDLE( colip->colVal.val)	// if column value is expression (not constant)
		 &&  rp->rpFreq > C_IVLCH_Y )		// if rpfreq set and more often than at end run (ASSUMES _Y is smallest C_IVLCH_)
											//   (yearly reports can use any variability)
		{
			USI rpfreqEvf, colEvf = 0;
			exInfo( EXN(colip->colVal.val), &colEvf, NULL, NULL);   		// get evf of expr; leave 0 if bad exn (exman.cpp)
			switch (rp->rpFreq)							// get evf bits corresponding to rpFreq
			{
			case C_IVLCH_M:
				rpfreqEvf = EVFMON;
				break;
			case C_IVLCH_D:
				rpfreqEvf = EVFDAY;
				break;
			case C_IVLCH_H:
				rpfreqEvf = EVFHR;
				break;
				// case C_IVLCH_S:  case C_IVLCH_HS:
			default:
				rpfreqEvf = EVFSUBHR;
				break;
			}
			if ( colEvf					// if exInfo() call above ok (fail leaves colExf 0, no msg)
			 &&  rpfreqEvf > colEvf 	// and rpFreq faster than colVal varies (leftmost bit farther left)
			 &&  (colEvf & (EVXBEGIVL)) )	// and colVal evaluated at end/post interval (results probe)
				colip->oWarn(
					(char *)MH_S0546,		// "End-of-interval varying value is reported more often than it is set:\n"
					exrp, exrePort, rp->name,	// "    %sFreq of %s '%s' is '%s',\n"		eg "rpFreq of report 'foo' is 'day'
												// "    but colVal for colHead '%s' varies (is given a value) only at end of %s."
					rp->getChoiTx( RI_RPFREQ, 1),	// text for rpfreq choice
					colip->colHead,
					evfTx( colEvf,2) ); 				// text for evf bits,cuparse.cpp,2=noun eg "each hour"
		}

		// translate cuparse data types to cvpak data types for use at runtime, in string-or-float value VALNDT structure.
		switch (colip->colVal.ty)
		{
		case TYFL:
			colp->colVal.ty = DTFLOAT;
			break;		// set data type to use when converting to print during run
		case TYSTR:
			colp->colVal.ty = DTCHP;
			break;
		default:
			colip->oer( (char *)MH_S0547, colip->colVal.ty); 	// "Bad data type (colVal.dt) %d"
		}

		// default width if not given.  Note: gap is defaulted to 1 per CULT table, and is limit-checked for nonNegative.
		if (!colp->colWid)
			colp->colWid = isExport ? EXDEFWID : RPDEFWID;	/* tentative -- presumably type-dependent etc etc
								   (if not, just put default in CULT tables) */
		// count columns and accumulate width for report, including gaps
		rp->nCol++;
		rp->wid += colp->colGap + colp->colWid;

		// add this column to end of column list for its report
		SI* pNxColi;
		for (pNxColi = &rp->coli;  *pNxColi;  pNxColi = &colB->p[*pNxColi].nxColi )	// find end of list so far
			;
		*pNxColi = colp->ss;			// set rp->coli (1st column) or prev column->nxColi
		colp->nxColi = 0;				// this column is current end of list for report.  insurance -- pre-0'd.

	}	// RLUP coliB
	return RCOK;			// also each E macro contains conditional error return
}			// topCol
//=======================================================================================
RC RI::ri_CkF()
// check function automatically called at end of REPORT and EXPORT object entry (per tables * lines).
// also called by code in TopCkf.
{
	RC rc=RCOK;
	UCH* fs = fStat();			// ptr to field status byte array
	int isEx = isExport;		// 0 for report, nz for export: prefetch for several uses
	const char * exrp = isEx ? "ex" : "rp"; 		// to insert in member names in error messages
	const char * exrePort = isEx ? "export" : "report";	// to insert in errmsgs
// get field texts for errMsgs, issue no errmsg (except in returned text) if value out of range.
	const char* tyTx = getChoiTx( RI_RPTY, 1);	// rpTy/exTy as text
	const char* fqTx = getChoiTx( RI_RPFREQ, 1);	// rpFreq/exFreq

// note: file, zone, meter references not checked here as not stored till input complete (to permit forward references)

// check: type ok for export (all types ok for report)
//        rpFreq given or not per rpType.
//	  immediately return errors that would cause additional spurious messages in this function.

	switch (rpTy)			// a TYCH with evf==0 (no deferred exprs) member
	{
	case 0:
		break;			// if omitted, let cul.cpp do its msg (RQD field; calls here first)

		// types not allowing rpFreq nor dates and also not allowed as exports.
	case C_RPTYCH_ERR:		// "Err" (error messages)
	case C_RPTYCH_LOG:		// "Log" (run messages)
	case C_RPTYCH_INP:		// "Inp" (input echo, with some or all error messages)
	case C_RPTYCH_SUM:		// "Summary"
	case C_RPTYCH_ZDD:		// "Zone Data Dump"
		if (isEx)
			return ooer( RI_RPTY, "Invalid exType '%s'", tyTx);		// bad type for export
		// fall thru
		// types not allowing rpFreq nor dates but as exports
	case C_RPTYCH_AHSIZE:					// 6-95
	case C_RPTYCH_AHLOAD:					// 6-95
	case C_RPTYCH_TUSIZE:					// 6-95
	case C_RPTYCH_TULOAD:					// 6-95
		if (fs[ RI_RPFREQ ] & FsSET)
			rc = ooer( RI_RPFREQ, (char *)MH_S0428, exrp, exrp, tyTx);	// "%sFreq may not be given with %sType=%s"
		if (fs[ RI_RPDAYBEG ] & FsSET)
			rc = ooer( RI_RPDAYBEG, (char *)MH_S0429, exrp, exrp, tyTx);  	// "%sDayBeg may not be given with %sType=%s"
		if (fs[ RI_RPDAYEND ] & FsSET)
			rc = ooer( RI_RPDAYEND, (char *)MH_S0430, exrp, exrp, tyTx);  	// "%sDayEnd may not be given with %sType=%s"
		break;

		// types requiring rpFreq and allowing or requiring start and end day depending on rpFreq

	case C_RPTYCH_ZST:		// "Zone Statistics"
	case C_RPTYCH_ZEB:		// "Zone Energy Balance"
	case C_RPTYCH_MTR:  	// "Meter"
	case C_RPTYCH_DHWMTR:  	// "DHWMeter"
	case C_RPTYCH_AH:		// "Air Handler"
	case C_RPTYCH_UDT:		// "User-defined Table"
		switch (rpFreq)		// rpFreq is TYCH with evf==0 ---> no exprs
		{
		case 0:
			return oer( (char *)MH_S0431, exrp, exrp, tyTx);    // "No %sFreq given: required with %sType=%s"

			// dates not allowed: year
		case C_IVLCH_Y:
			if (fs[ RI_RPDAYBEG ] & FsSET)
				rc = ooer( RI_RPDAYBEG, (char *)MH_S0432, 	// "%sDayBeg may not be given with %sFreq=%s"
						   exrp, exrp, fqTx);
			if (fs[ RI_RPDAYEND ] & FsSET)
				rc = ooer( RI_RPDAYEND, (char *)MH_S0433, 	// "%sDayEnd may not be given with %sFreq=%s"
						   exrp, exrp, fqTx);
			break;
			// date optional: month
		case C_IVLCH_M:
			break;

			// start date required: day, hour, both, subhour
		case C_IVLCH_HS:
		case C_IVLCH_S:
			if ( rpTy==C_RPTYCH_MTR			// have no subhourly meter info
			 ||  rpTy==C_RPTYCH_DHWMTR		// have no subhourly DHW meter info
			 ||  rpTy==C_RPTYCH_ZST )		// have no subhourly statistics info, right?
				return ooer( RI_RPFREQ, (char *)MH_S0434, 		// "%sFreq=%s not allowed with %sty=%s"
							 exrp, fqTx, exrp, tyTx );
			{
				char *badSumOf=NULL;			// check for types without subhour sum info
				if      (rpTy==C_RPTYCH_ZEB  &&  (fs[RI_ZI] & FsSET) && zi==TI_SUM)   badSumOf = "zones";
				else if (rpTy==C_RPTYCH_AH  &&  (fs[RI_AHI] & FsSET) && ahi==TI_SUM)  badSumOf = "ahs";
				if (badSumOf)
					return ooer( RI_RPFREQ, (char *)MH_S0435, 	// "%sFreq=%s not allowed in Sum-of-%s %s"
								 exrp, fqTx, badSumOf, exrePort );
			}
			/*lint -e616 fall thru */
		case C_IVLCH_H:
		case C_IVLCH_D:
			if (!rpDayBeg)
				rc = oer( (char *)MH_S0436, 	// "No %sDayBeg given: required with %sFreq=%s"
						  exrp, exrp, fqTx );
			break;
			/*lint +e616 */

		default:
			return ooer( RI_RPFREQ, (char *)MH_S0437,  // "Internal error in RI::ri_CkF: bad rpFreq %d"
						 (INT)rpFreq);
		}
		break;

	default:
		return ooer( RI_RPTY, (char *)MH_S0438, 	// "Internal error in RI::ri_CkF: bad rpTy %d"
					 (INT)rpTy);
	}  // switch (rpTy)


// disallow title and cpl if not UDT

	if (rpTy != C_RPTYCH_UDT)
	{
		if (fs[RI_RPCPL] & FsSET)
			rc = ooer( RI_RPCPL, (char *)MH_S0439, exrp, exrp);	// "%sCpl cannot be given unless %sTy is UDT"
		if (fs[RI_RPTITLE] & FsSET)
			rc = ooer( RI_RPTITLE, (char *)MH_S0440, exrp, exrp);	// "%sTitle cannot be given unless %sTy is UDT"
	}

	// disallow export-only header options for report
	if (!isEx && (   rpHeader == C_RPTHDCH_COL
		          || rpHeader == C_RPTHDCH_COLIF
		          || rpHeader == C_RPTHDCH_YESIF))
	{	const char* rpHdTyTx = getChoiTx(RI_RPHEADER, 1);
		rc |= oWarn("Reports cannot have rpHeader=%s, using rpHeader=Yes instead", rpHdTyTx);
		rpHeader = C_RPTHDCH_YES;
	}

	return rc;				// and several error returns above
}		// RI::ri_CkF
//===========================================================================
RC topRxp()			// check REPORTS and EXPORTS / build dvri info

// checks, and sets vrh's in globals, zones, Top and DvriB as pertinent for each report and export type.
// topZn must be called first.
// buildUnspoolInfo must also be called.

// returns non-RCOK if error.
{
	RI *rp;
	RC rc;

// clear outputs of this fcn

	VrLog = 0;						// not opened elsewhere, 0 here for insurance.
	Top.vrSum = 0;   					// insurance: also 0'd at each unspool and by copying Topi to Top
	E (DvriB.al( RiB.n+XiB.n, WRN) )		/* (redundantly) clear Date-dependent Virtual Reports Info records
    						   & alloc now a generous # records to minimize fragmentation */
	DvriB.ownB = &ZrB;		/* Dvri's belong to zones & zone run subscripts match input subscripts,
    				   so enable possible use of zone input subscript in resolving ambiguous name in probe. 1-92. */
	/**** DvriB owned by zone getting silly with addition of meters, 1-92...
	      can't own by report/export cuz 2 input rats.  make Unowned? */
	// error virtual report (rmkerr.cpp) expected already opened in cse.cpp; do not close here (would loose messages).
	// pp.cpp:VrInp is now open -- input listing report already being written -- 0ing here would loose information.
	// vrh's in RATs eg ZrB: RATs are cleared between runs; new records are all 0s.

// process each requested report and export

	RLUP( RiB, rp)		// loop reports input records
		rp->ri_oneRxp();		// just below

	RLUP( XiB, rp)		// loop exports input records
		rp->ri_oneRxp();

	return RCOK;
}			// topRxp
//===========================================================================
RC RI::ri_oneRxp()		// process one report or export for topRxp

// checks record, and sets vrh in a global, in a zone, or in Top,
// or builds DvriB entry, as pertinent for each report or export type.

// topZn and topCol must be called first; buildUnspoolInfo must be called afterwards.
{
	RC rc=RCOK;

	int isEx = isExport;					// 1 for export, 0 for report
	const char* exrp = isEx ? "ex" : "rp"; 				// to insert in member names in error messages
	const char* exrePort = isEx ? "export" : "report";	// to insert in errmsgs

// get field texts for errMsgs.  No errmsg (except in returned text) if value out of range.

	const char* tyTx = getChoiTx( RI_RPTY, 1);
	const char* whenTy = strtprintf( "when %sType=%s", exrp, tyTx);

// check RQD members set -- else can bomb with FPE

	if (CkSet( RI_RPTY))
		return RCBAD;					// if not set, no run, terminate checking this report/export now

// recall entry time check function 1) as it has not been called for pre-stuffed default reports entries;
// 2) to recheck references (DELETE given after entry?)(if it checks any); and 3) general paranoia.

	if (!errCount())	// but if already have errors (hence no run), do not recall:
    					//   would issue duplicate messages for any errors it detected.
		if (ri_CkF())	// note this is the ckf for exports as well as reports
			return RCBAD;			// bad. done with entry.

// default start and/or end days of report.  Don't set nz b4 other checks as not allowed with some rpt types & freq's.
//					     and, doing here sets dates for monthly default reports.

	if ( rpFreq==C_IVLCH_M				// monthly frequency: default days to start and end of run
	 ||  rpFreq==C_IVLCH_Y )			// annual frequency: this sets dates
	{
		if (!rpDayBeg)   rpDayBeg = Topi.tp_begDay;
		if (!rpDayEnd)   rpDayEnd = Topi.tp_endDay;
	}
	else if (!rpDayEnd)				// other frequencies; rpDayBeg is 0 if here and dates not used.
		rpDayEnd = rpDayBeg;			// end day defaults to start day

// disallow condition for types without repeated conditional lines

	int rpCondGiven = IsSet( RI_RPCOND);			// 1 if rpCond entered by user, 0 if rpCond defaulted (to TRUE).
	switch (rpTy)
	{
	case C_RPTYCH_SUM:
	case C_RPTYCH_LOG:
	case C_RPTYCH_ERR:
	case C_RPTYCH_INP:
	case C_RPTYCH_ZDD:
	case C_RPTYCH_AHSIZE:
	case C_RPTYCH_AHLOAD:
	case C_RPTYCH_TUSIZE:
	case C_RPTYCH_TULOAD:			// 6-95
		if (rpCondGiven)
			rc = oer( (char *)MH_S0548, exrp, exrp, tyTx);			// "%sCond may not be given with %sType=%s"
		break;
	default:
		;
	}
	// also: rpCond should be checked for too-slow-varying end-of-interval expr, like colVal. see topCol. <<<<<<< ??? 12-91.

// require or disallow zone, meter, dhwmeter, ah per report type.
// Return on error that might produce spurious messages in following checks.
	switch (rpTy)
	{
	case C_RPTYCH_MTR:							// "Meter" report/export requires rp/exMeter
		rc |= require( RI_MTRI, whenTy);
		rc |= disallowN( whenTy, RI_ZI, RI_AHI, RI_TUI, RI_DHWMTRI, 0);
		break;

	case C_RPTYCH_DHWMTR:							// "DHWMTR" report/export requires rp/exDHWMeter
		rc |= require( RI_DHWMTRI, whenTy);
		rc |= disallowN( whenTy, RI_ZI, RI_AHI, RI_TUI, RI_MTRI, 0);
		break;

	case C_RPTYCH_AHSIZE:	// AH-specific reports
	case C_RPTYCH_AHLOAD:
	case C_RPTYCH_AH:
		rc |= require( RI_AHI, whenTy);
		rc |= disallowN( whenTy, RI_ZI, RI_TUI, RI_MTRI, RI_DHWMTRI, 0);
		break;

	case C_RPTYCH_TUSIZE:	// TU-specific reports
	case C_RPTYCH_TULOAD:
		rc |= require( RI_TUI, whenTy);
		rc |= disallowN( whenTy, RI_ZI, RI_AHI, RI_MTRI, RI_DHWMTRI, 0);
		break;

	case C_RPTYCH_ZDD:	// zone-specific reports
	case C_RPTYCH_ZEB:
	case C_RPTYCH_ZST:
		rc |= require( RI_ZI, whenTy);
		rc |= disallowN( whenTy, RI_TUI, RI_AHI, RI_MTRI, RI_DHWMTRI, 0);
		break;

	case C_RPTYCH_SUM:	// non- zone -ah -tu -meter reports/exports
	case C_RPTYCH_LOG:
	case C_RPTYCH_ERR:
	case C_RPTYCH_INP:
	case C_RPTYCH_UDT:
		rc |= disallowN( whenTy, RI_ZI, RI_TUI, RI_AHI, RI_MTRI, RI_DHWMTRI, 0);
		break;

	default:
		if (!errCount())					// if other error has occurred, suppress msg: may be consequential
			rc = oer( (char *)MH_S0555, exrp, rpTy);  	// "cncult:topRp: Internal error: Bad %sType %d"
	}

// default/check file reference. Defaulted to rp/exfile in which nested, else default here to "Primary" (supplied by TopStarPrf)

	if (!ownTi)					// if no file given & not defaulted (note default does not set FsSET bit)
	{
		anc<RFI> * fb = isEx ? &XfiB : &RfiB;			// ptr to reportfile or exportfile input ratbase
		if (fb->findRecByNm1( "Primary", &ownTi, NULL))	// find first record by name (ancrec.cpp) / if not found
			if (fb->n)						// if not found, if there are ANY r/xport files,
				ownTi = 1;					// use first one: is probably Primary renamed with ALTER
			else							// no r/xport files at all
				rc |= ooer( RI_OWNTI, 					// issue error once (cul.cpp), no run
					(char *)(isEx ? MH_S0556 : MH_S0557) );	// "No exExportfile given" or "No rpReportfile given"
	}
	RFI* rfp=NULL;
	if (ownTi)
		if (ckRefPt( isEx ? &XfiB : &RfiB, ownTi,  isEx ? "exFile" : "rpFile",  NULL, (record **)&rfp ) )	// ck ref
			return RCBAD;

// check zone reference or ALL or SUM
	int isAll = 0;		// set nz iff ALL
	ZNR* zp=NULL;
	ZNI* zip=NULL;
	if (zi > 0)						// if specific zone given (error'd above if omitted when rqd)
	{
		if (ckRefPt( &ZrB, zi, isEx ? "exZone" : "rpZone", NULL, (record **)&zp ) )
			return RCBAD;
		if (ckRefPt( &ZiB, zi, isEx ? "exZone" : "rpZone", NULL, (record **)&zip ) )	// & pt to zn INPUT rec for ZDD
			return RCBAD;
	}
	else							// zi <= 0: none given, or SUM or ALL.
	{
		char *znTx /*=""*/;					// (redundant init removed 12-94 when BCC 32 4.5 warned)
		if (zi==TI_SUM)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_ZST:					// ZST: statistics report
			case C_RPTYCH_ZEB:
				break;			// ZEB: energy balance report: "SUM" allowed
			default:
				znTx = "sum";
				goto badZn4ty;
			}
		else if (zi==TI_ALL)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_ZST:
			case C_RPTYCH_ZEB:
				isAll++;
				break;
			default:
				znTx = "all";
badZn4ty:
				return ooer( RI_RPTY, (char *)MH_S0558, 	// "rpZone=='%s' cannot be used with %sType '%s'"
									znTx, exrp, tyTx );
			}
		//else: zi is 0 (or negative garbage).  error'd above if omitted when rqd.
		//zp = zip = NULL;		init NULL at fcn entry.		// say no zone, including for SUM and ALL.
	}

// check meter reference or ALL or SUM

	if (mtri > 0)						// if specific meter given (error'd above if omitted when rqd)
	{
		if (ckRefPt( &MtrB, mtri, isEx ? "exMeter" : "rpMeter" ) )
			return RCBAD;
	}
	else							// no meter given, or ALL or SUM.
	{
		char *mtrTx /*=""*/;					// (redundant init removed 12-94 when BCC 32 4.5 warned)
		if (mtri==TI_SUM)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_MTR:
				break;   			// MTR: meter report: SUM allowed.
			default:
				mtrTx = "sum";
				goto badMtr4ty;
			}
		else if (mtri==TI_ALL)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_MTR:
			case C_RPTYCH_DHWMTR:
				isAll++;
				break;		// MTR: ALL allowed
			default:
				mtrTx = "all";
badMtr4ty:
				return ooer( RI_RPTY, (char *)MH_S0559, 	// "rpMeter=='%s' cannot be used with %sType '%s'"
								mtrTx, exrp, tyTx );
			}
		//else: mtri is 0 (or negative garbage).  error'd above if omitted when rqd.
	}

// check DHW meter reference or ALL

	if (ri_dhwMtri > 0)						// if specific meter given (error'd above if omitted when rqd)
	{
		if (ckRefPt( &WMtriB, ri_dhwMtri, isEx ? "exMeter" : "rpMeter" ) )
			return RCBAD;
	}
	else							// no DHW meter given, or ALL
	{	char *mtrTx /*=""*/;
		if (ri_dhwMtri==TI_SUM)
		{	mtrTx = "sum";
			goto badDHWMtr4ty;
		}
		else if (ri_dhwMtri==TI_ALL)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_DHWMTR:
				isAll++;
				break;		// DHWMTR: ALL allowed
			default:
				mtrTx = "all";
badDHWMtr4ty:
				return ooer( RI_RPTY, "rpDHWMeter=='%s' cannot be used with %sType '%s'",
								mtrTx, exrp, tyTx );
			}
		//else: ri_dhwMtri is 0 (or negative garbage).  error'd above if omitted when rqd.
	}

// check air handler reference or ALL or SUM

	if (ahi > 0)					// if specific air handler given (error'd above if omitted when rqd)
	{
		if (ckRefPt( &AhiB, ahi, isEx ? "exAh" : "rpAh" ) )
			return RCBAD;
	}
	else							// no air handler given, or ALL or SUM.
	{
		char *ahTx /*=""*/;					// (redundant init removed 12-94 when BCC 32 4.5 warned)
		if (ahi==TI_SUM)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_AH:
				break;   			// AH: air handler report: SUM allowed.
			default:
				ahTx = "sum";
				goto badAh4ty;
			}
		else if (ahi==TI_ALL)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_AHSIZE:				// 6-95
			case C_RPTYCH_AHLOAD:				// 6-95
			case C_RPTYCH_AH:
				isAll++;
				break;		// AH: ALL allowed
			default:
				ahTx = "all";
badAh4ty:
				return ooer( RI_RPTY, (char *)MH_S0560, 	// "rpAh=='%s' cannot be used with %sType '%s'"
								ahTx, exrp, tyTx );
			}
		//else: ahi is 0 (or negative garbage).  error'd above if omitted when rqd.
	}

// check terminal reference or ALL (or SUM) 6-95

	if (tui > 0)					// if specific terminal given (error'd above if omitted when rqd)
	{
		if (ckRefPt( &TuiB, tui, isEx ? "exTu" : "rpTu" ) )
			return RCBAD;
	}
	else							// no air handler given, or ALL or SUM.
	{
		char *tuTx /*=""*/;					// (redundant init removed 12-94 when BCC 32 4.5 warned)
		if (tui==TI_SUM)
			switch (rpTy)
			{
			case 0:
				break;			// rpTy 0: not set, no message here
				//case C_RPTYCH_...:  break;   			add for future reports that permit SUM of terminals
			default:
				tuTx = "sum";
				goto badTu4ty;
			}
		else if (tui==TI_ALL)
			switch (rpTy)
			{
			case 0:						// rpTy 0: not set, no message here
			case C_RPTYCH_TUSIZE:				// TUSIZE 6-95
			case C_RPTYCH_TULOAD:
				isAll++;
				break;		// TULOAD: ALL allowed. 6-95.
			default:
				tuTx = "all";
badTu4ty:
				return ooer( RI_RPTY, "rpTu=='%s' cannot be used with %sType '%s'",	// NUMS
						tuTx, exrp, tyTx );
			}
		//else: tui is 0 (or negative garbage).  error'd above if omitted when rqd.
	}


// user-defined report checks

	if (rpTy)					// if good type not given, skip these type-dependent checks
		if (rpTy != C_RPTYCH_UDT)			// not user defined
		{
			if (rpTitle)
				oer( (char *)MH_S0561, exrp, exrp);		// "%sTitle may only be given when %sType=UDT"
			if (coli)
				oer( (char *)MH_S0562, exrp, exrePort, exrp);	// "%sport has %sCols but %sType is not UDT"
		}
		else						// is a user-defined report
		{
			//rpTitle is optional

			if (!coli)
				oer( (char *)MH_S0563, exrp, exrePort);   	// "no %sCols given for user-defined %s"

			if (!isEx  &&  coli)			// no width check for exports or if no columns
			{
				int cpl = rpCpl ? rpCpl : getCpl();		// line width
#if 1	// rpCpl<0 -> "as wide as needed", 11-28-2016
				if (wid > cpl)
				{	if (cpl > 0)
						oInfo( "%s width %d is greater than line width %d.\n"
							"    Line width has been adjusted.",
							exrePort, wid, cpl);
					rpCpl = wid;
				}
#else
x				if (wid > cpl )
x					oer( (char *)MH_S0564,
x						exrePort, wid, cpl,		// "%s width %d greater than line width %d.\n"
x						exrp, wid );			// "    Override this message if desired with %sCpl = %d."
#endif
			}
		}


// set up for runtime per report/export type

	// Each uses existing vrh if duplicate report or export.
	// all virtual reports of monthly or less frequency are always spooled with page formating for simplicity.

	// int vrh = 0;		// virtual report handle
	switch (rpTy)
	{

		// global report types: vrh's in Top or globals.  Err and Inp pre-opened in cse.cpp to get complete reports.

	case C_RPTYCH_LOG:
		vrh = openLogVr();
		break;  		// open log vr if not open; get handle anyway. rmkerr.cpp
	case C_RPTYCH_INP:
		vrh = openInpVr();
		break;  		// open input listing vr if not open; get handle. pp.cpp.
	case C_RPTYCH_ERR:
		vrh = openErrVr();
		break;  		// open err vr if not open; get handle anyway. rmkerr.cpp
	case C_RPTYCH_SUM:
		if (!Top.vrSum)
			vrOpen( &Top.vrSum, "Summary", VR_FMT | VR_FINEWL);
		vrh = Top.vrSum;  					// to vrh after switch
		break;

		// report type with info in zone

	case C_RPTYCH_ZDD:
		if (!zp)   break;			// insurance -- rpZone req'd already checked
		if (!zp->i.vrZdd)
			vrOpen( &zp->i.vrZdd, "Zone Description", VR_FMT);	// no VR_FINEWL needed: \f supplied in cse.cpp
		vrh = zp->i.vrZdd;
		// copy vrh to zone INPUT record to persist thru resetup between autoSize and main sim
		//   -- otherwise ZDD reports omitted if both ausz & main sim done, 10-10-95.
		//   (rpt stuff not cleared/resetup tween phases; believe vrZdd is only rpt vbl in a resetup basAnc.)
		zip->i.vrZdd = zp->i.vrZdd;
		// Note zip->i.vrZdd=0 in topZn (vrClear() done after run, all VRs must be re-vrOpen()ed 8-23-2012
		break;

		// non-date-dependent reports that use DVRI mechanism anyway -- to get to cgresult's nice formatting stuff, 6-95

	case C_RPTYCH_AHSIZE:
	case C_RPTYCH_AHLOAD:
	case C_RPTYCH_TUSIZE:
	case C_RPTYCH_TULOAD:
		rpFreq = C_IVLCH_Y;			// say issue report once at end (rpFreq input disallowed)
		rpDayBeg = Topi.tp_begDay;			// set dates to entire run to make report appear
		rpDayEnd = Topi.tp_endDay;			// ..
		// fall thru

		// date dependent re/export types: use a DVRI record for daily enabling/disabling by cgReportsDaySetup, link to zone or Top

	case C_RPTYCH_ZST:
	case C_RPTYCH_ZEB:
	case C_RPTYCH_MTR:
	case C_RPTYCH_DHWMTR:
	case C_RPTYCH_AH:
	case C_RPTYCH_UDT:
	  {
		int optn = 0;					// for vr open option bits
		if (!isEx || rpFooter != C_NOYESCH_NO)	// exFooter=NO suppresses... (rpt has footer text to suppress, so keep \n)
			optn |= VR_FINEWL;				// option to add newline to end of report/export at unspool
		if (!isEx && rfp->pageFmt==C_NOYESCH_YES)	// if a report to a formatted report file
			optn |= VR_FMT;				// spool with formatting; else unformatted for speed.
		SI rpHeaderLocal = rpHeader;	// modifiable copy of rpHeader
		if (isEx)
		{	// export file conditional header options
			//   write header only when destination file is empty
			int bAddToExisting = !rfp->overWrite && rfp->wasNotEmpty;
			if (rpHeader == C_RPTHDCH_COLIF)
				rpHeaderLocal = bAddToExisting ? C_RPTHDCH_NO : C_RPTHDCH_COL;
			else if (rpHeader == C_RPTHDCH_YESIF)
				rpHeaderLocal = bAddToExisting ? C_RPTHDCH_NO : C_RPTHDCH_YES;
			// else unconditional yes/no/col, do not alter
		}
		int found = 0;
		DVRI* dvrip;
		RLUP (DvriB, dvrip)					// seek matching DVRI record
		{
			if ( isExport==dvrip->isExport   &&  rpTy==dvrip->rpTy  &&  rpFreq==dvrip->rpFreq
					&&  rpDayBeg==dvrip->rpDayBeg   &&  rpDayEnd==dvrip->rpDayEnd
					&&  rpCondGiven==dvrip->rpCondGiven &&  rpCond==dvrip->rpCond 	// (expect != if rpCond exprs given. ok.)
					&&  rpHeaderLocal==dvrip->rpHeader   &&  rpFooter==dvrip->rpFooter
					&&  zi==dvrip->ownTi &&  mtri==dvrip->mtri  && ri_dhwMtri == dvrip->dv_dhwMtri
					&&  tui==dvrip->tui	&&  ahi==dvrip->ahi
					&&  rpBtuSf==dvrip->rpBtuSf
					&&  rpTy != C_RPTYCH_UDT )						// don't combine UDTs: many addl details
			{
				found++;
				vrh = dvrip->vrh;				// use existing vrh
				vrChangeOptn( vrh, optn, optn);	// if page-formatted file, set formatted option bit for vr.
		   										// (if also in unformatted file, unspool will de-format)
				break;
			}
		}
		if (!found)    					// if not yet in DVRI
		{
			char *sname, buf[100];				// re name of vr in spool, for error messages
			switch (rpTy)
			{
			case C_RPTYCH_AHSIZE:
				sname = "AH Size";
				break;
			case C_RPTYCH_AHLOAD:
				sname = "AH Load";
				break;
			case C_RPTYCH_TUSIZE:
				sname = "TU Size";
				break;
			case C_RPTYCH_TULOAD:
				sname = "TU Load";
				break;
			case C_RPTYCH_MTR:
				sname = "Meter";
				break;
			case C_RPTYCH_DHWMTR:
				sname = "DHW Meter";
				break;
			case C_RPTYCH_AH:
				sname = "Air Handler";
				break;
			case C_RPTYCH_UDT:
				sname = "User-defined";
				break;
			case C_RPTYCH_ZST:
				sname = "Statistics";
				break;
			default:
				sname = "Energy Balance";
				break;		// C_RPTYCH_ZEB
			}
			sprintf( buf, "%s %s %s", sname, exrePort, name);	// eg "Statistics report userName1", for errmsgs
			vrOpen( &vrh, buf, optn);					// open virtual report, get handle (vrh).
			if (DvriB.add( &dvrip, WRN)==RCOK)   		// add record to DVRI / if ok (fail unlikely after al above)
			{
				strcpy( dvrip->name, name);				// fill entry.  name: for errMsgs, UDT default.
				dvrip->ownTi    = zi;
				dvrip->mtri     = mtri;
				dvrip->dv_dhwMtri = ri_dhwMtri;
				dvrip->ahi      = ahi;
				dvrip->tui      = tui;					// 6-95
				dvrip->isExport = isExport;
				dvrip->isAll    = isAll;
				dvrip->rpTy     = rpTy;
				dvrip->rpFreq   = rpFreq;
				dvrip->rpDayBeg = rpDayBeg;
				dvrip->rpDayEnd = rpDayEnd;
				dvrip->rpBtuSf  = rpBtuSf;
				dvrip->rpCond   = rpCond;
				dvrip->rpCondGiven = rpCondGiven; 				// may change report title
				dvrip->rpTitle  = rpTitle;
				cupIncRef( DMPP( rpTitle));				/* ++ ref count for copied pointer unless UNSET
	        								   or ptr to inline constant in pseudo-code */
				dvrip->rpCpl    = rpCpl ? rpCpl : getCpl();		// default characters per line now
				dvrip->rpHeader = rpHeaderLocal;
				dvrip->rpFooter = rpFooter;
				dvrip->coli     = coli;
				dvrip->nCol     = nCol;
				dvrip->wid      = wid;
				dvrip->vrh      = vrh;						// vrh is used from here for output
				dvrip->nextNow  = 0;						// insurance; shd be pre-0'd; also, set b4 used.
			}
		}
	  }
	  break;

	default:
		if (!errCount())  						// avoid repetition
			err( PWRN, (char *)MH_S0565, exrp, rpTy);		// "cncult:topRp: unexpected %sTy %d"

	}  // switch (rpTy)

	return rc;				// also 4+ error returns above
}			// RI::ri_oneRxp
//===========================================================================
RC buildUnspoolInfo()

// Build array of info to drive vrUnpsoolInd to distribute virtual reports to report files

// uses data in input rats as processed by topRf, topXf, topRp, topXp.
// input rats are then discarded.

// returns non-RCOK if error; be sure bad return propogated to stop run where message does not errCount++.
{
	RFI *rfp;
	RI *rp;
	SI nVrh, atEnd;
	VROUTINFO *p;
	RC rc;

// allocate dm block for unspooling specifications in vrUnspool format, set pointer in cnguts.cpp.

	LI nbytes = sizeof(VROUTINFO)  		// bytes per file, without any vrh's except terminating 0
			* (RfiB.n + XfiB.n + 1)		// number of files. + 1 allows for terminating NULL.
				+ sizeof(int)			// bytes per virtual report to unspool
				* (RiB.n + XiB.n);		// # virtual report arguments, including duplicates
	E( dmral( DMPP( UnspoolInfo), nbytes, DMZERO|WRN) )	/* (re)alloc memory, dmpak.cpp.  Realloc prevents loss of memory
	    							   if prior was not free'd (but main program does release it).*/
	// UnspoolInfo freeing: fileNames free'd as used in vrUnspool;
	// block is free'd in cnguts.cpp:cgReInit.

// fill dm block with name-options for each file, followed by 0-terminated list of vrh's that go into it

	p = UnspoolInfo;
	RLUP( RfiB, rfp)   					// loop report files
	{
		// find and get vrh's for file, if any
		nVrh = 0;
		for (atEnd = 0;  atEnd < 2;  atEnd++)			// do 2 passes to put .putAtEnd-flagged reports last
			/*lint -e731 "Boolean arg to ==" */
			RLUP( RiB, rp)						// loop reports, find those for this file
				if ( rp->ownTi==rfp->ss				// if report is for this file
				 &&  (!rp->putAtEnd)==(!atEnd)		// if it is for end or not per pass
				 &&  rp->vrh )						// if report has non-0 virt rpt handle (insurance)
					p->vrhs[nVrh++] = rp->vrh;  	// add report's vrh to those to put in this file
		/*lint +e731 */
		p->vrhs[nVrh] = 0;				// 0 after last vrh terminates list (vbl length array)
										// note no ++nVrh as sizeof(VROUTOUT) includes .vrhs[1].
		// complete entry only if vrh's found, or if overwrite
		if (nVrh						// if any reports were found for file
		 || p->optn & VR_OVERWRITE )	// or file is to be overwritten: erase even if no reports.
		{
			p->fName = rfp->fileName;				// copy fileName pointer
			cupIncRef( DMPP( rfp->fileName));		// dmIncRef unless ptr into pseudocode (or NANDLE), cueval.cpp.
			p->optn = (rfp->pageFmt==C_NOYESCH_YES ? VR_FMT : 0)	// translate page formatting to vrpak option bit
			        | (rfp->overWrite ? VR_OVERWRITE : 0); 	// translate erase existing file flag to vrpak optn bit
			rfp->overWrite = 0;					// only overWrite once in session, then append!
			IncP( DMPP( p), sizeof(VROUTINFO) + nVrh * sizeof(int));	// point past file output info incl vbl # vrh's
		}
		// no-report overwrite files are transmitted so unspooler can erase them unless input error supresses run.
		// no-report append files are dropped here, by not completing entry and incrementing p.
	}
	RLUP( XfiB, rfp)					// loop export files (separate ancBase of same type as report files)
	{
		// find and get the vrh's for file, if any
		nVrh = 0;
		RLUP( XiB, rp)   			// loop exports, find those for this file
		if (rp->ownTi==rfp->ss  	// if export is for this file
		 && rp->vrh )				// if export has non-0 virt rpt handle (insurance)
			p->vrhs[nVrh++] = rp->vrh;		// add export's vrh to those to put in this output file
		p->vrhs[nVrh] = 0;			// 0 after last vrh terminates list (vbl length array)
									// note no ++nVrh as sizeof(VROUTOUT) includes .vrhs[1].
		// complete entry only if vrh's found, or if overwrite
		if ( nVrh						// if any exports were found for file
		||  p->optn & VR_OVERWRITE )    			// or file is to be overwritten
		{
			p->fName = rfp->fileName;			// copy fileName pointer
			cupIncRef( DMPP( rfp->fileName));	// dmIncRef unless ptr into in pseudocode (or NANDLE), cueval.cpp.
			p->optn = 							// page formatting option bit off for exports
				(rfp->overWrite ? VR_OVERWRITE : 0)     // translate erase existing file flag to vrpak optn bit
				| VR_ISEXPORT;   						// is this tested?
			rfp->overWrite = 0;					// only overWrite once in session, then append!
			IncP( DMPP( p), sizeof(VROUTINFO) + nVrh * sizeof(int));	// point past file output info incl vbl # vrh's
		}
		// no-report overwrite files are transmitted so unspooler can erase them unless input error supresses run.
		// no-report append files are dropped here, by not completing entry and incrementing p.
	}
	*(char **)p = NULL;				// terminate unspooling specs

	return RCOK;				// also error returns in E macros
}		// buildUnspoolInfo

//===========================================================================
/*virtual*/ void RFI::Copy(const record* pSrc, int options/*=0*/)
{
// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten. dmfree: lib\dmpak.cpp.
	cupfree( DMPP( fileName));		// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.
// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED
	record::Copy( pSrc, options);				// verfies that src and this are same record type. lib\ancrec.cpp.
// increment reference count for pointer(s) just copied. dmIncRec: lib\dmpak.cpp.
	cupIncRef( DMPP( fileName));		// dmIncRef unless NANDLE or constant inline in pseudocode, cueval.cpp.
}			// RFI::Copy
//===========================================================================
RFI::~RFI()
{
// free (or decrement reference count of) derived class heap pointers in record being destroyed. dmfree:lib\dmpak.cpp.
	cupfree( DMPP( fileName));			// dmfree unless NANDLE or constant inline in pseudocode, cueval.cpp.
	//record::~record() (automatically called) zeroes .gud to mark space unused.
}			// RFI::~RFI
//-----------------------------------------------------------------------------
RC RFI::rf_CkF(			// REPORTFILE / EXPORTFILE check
	int isExport)	// 1: export, else report
{
	const char* fileExt = isExport ? ".csv" : ".rep";
	const char* what = isExport ? "export" : "report";

	if (IsVal( RFI_FILENAME))		// if filename value stored (not an uneval'd expression)
	{
		// standardize filename and default extension

		char* s = strffix( fileName, fileExt);  		// uppercase, deblank, append ext if none
		if (!*strpathparts( s, STRPPDRIVE|STRPPDIR))	// if contains no drive nor directory
			s = strtPathCat( InputDirPath, s);			// default to INPUT FILE path (rundata.cpp variable) 2-95

		// check if file can be written
		//   attempt alias if not
		//   handles file unwriteable due to e.g. csv file left open in Excel
		char* sAlias;
		const char* msg;
		int ckA = rf_CheckAccessAndAlias( s, sAlias, &msg);
		if (ckA == 0)	// error
			return oer( "bad file name '%s' -- %s", s, msg);
		else if (ckA == 2)
		{	// alias file name successfully found
			const char* tMsg = strtprintf( "%s\n    ",
						IsBlank( msg) ? "Perhaps read-only or open in another application?" : msg);
			oInfo( "Cannot write to file '%s'.\n    %sUsing alternative file '%s'", s, tMsg, sAlias);
			s = sAlias;
		}

		if (strcmpi( s, fileName))					// store only if different (may get here twice) to reduce fragmentation
		{
			cupfree( DMPP( fileName));
			fileName = strsave(s);
		}

		// check against ExportFiles and ReportFiles for duplicate filename
		// Does not check for different expressions of same path; will (we hope) get open error later.
		if (rf_CheckForDupFileName())
			return RCBAD;
		// existence of file is checked from topRf/topXf, once-only overwrite flag also set there.

		// disallow erase and overwrite if file already used in (earlier run of) this session
		if (isUsedVrFile( fileName))		// if name used previously in session, even b4 CLEAR
		{
			if (IsSet( RFI_FILESTAT))		// if fileStat given (incl set by topPrfRep)
			{	const char* was = NULL;
				if (fileStat==C_FILESTATCH_OVERWRITE)
					was = "OVERWRITE";
				else if (fileStat==C_FILESTATCH_NEW)
					was = "NEW";
				if (was)
					oWarn( "Changing xfFileStat from %s to APPEND because %s(s)\n"
						   "    were written to that file in an earlier run this session",
						   was, what);
			}
			fileStat = C_FILESTATCH_APPEND;		// change fileStat of previously used file to "append":
												// fix after warning or silently change default.
		}
	}
	return RCOK;
}	// RFI::rf_CkF
//-----------------------------------------------------------------------------
RC RFI::rf_CkF2(			// start-of-run REPORTFILE / EXPORTFILE check
	int isExport)	// 1: export, else report
{
	RC rc = RCOK;
	// check all RQD members set -- else can bomb with FPE
	E( CkSet( RFI_FILENAME) )			// if not set, no run, terminate checking now

	// recall entry time check function 1) as it has not been called for pre-stuffed "Primary" entry;
	// to recheck references (DELETE after entry?)(if it checks any); and 3) general paranoia.

	E( rf_CkF( isExport))		// stop here if error

	// ONCE ONLY, check file per fileStat, possibly error or set "overwrite" flag.  Later uses always append.

	// (after CLEAR, this mechanism does not work to cause append;
	// instead fileStat of previously used files is forced to APPEND via isUsedVrFile() tests)

	int xfx = xfExist( fileName);	// 0=not found, 1=empty file, 2=non-empty file, 3=dir, -1=err
	wasNotEmpty = xfx == 2;
	if (!fileStatChecked				// not if checked before this run or this session
	 && fileStat != C_FILESTATCH_APPEND )		// append requires no check nor processing
	{	if (xfx)
		{	if (fileStat==C_FILESTATCH_OVERWRITE)
				overWrite++;		// say overwrite when opening (cleared when
	        						// passed to vrPak, so any addl uses append)
									// note: dir overwrite detected later
			else				// assume C_FILESTATCH_NEW
				oer( (char *)MH_S0544, fileName);	// "File %s exists". Text also used below. Message stops run.
		}
		fileStatChecked++;   		// say don't repeat: don't issue error due to prior run's output;
									// don't set overwrite again (would erase prior output).
	}

	if (!isExport)
	{	// capture info on "Primary" report output file in global variable.
			//	cse.cpp may append final info eg timing or addl error msgs at end session
			//       after reg unspool, poss even after input error.
			//  cse.cpp may pre-init this to <name>.REP in case input error prevents getting here.
			//	note must capture before run -- input info conditionally deleted on return from cul().

			if (!stricmp( name, "Primary"))
			{
				cupfree( DMPP( PriRep.f.fName));	// decref/free any value from a prior run, unless a
          										// ptr to "text" inline in pseudocode. cueval.cpp. */
				PriRep.f.fName = fileName;		// set cnguts.cpp global to file name
				cupIncRef( DMPP( fileName));	// dmIncRef unless ptr into pscode (or NANDLE), cueval.cpp.
				PriRep.f.optn = (pageFmt==C_NOYESCH_YES ? VR_FMT : 0)	// translate page formatting to vrpak option bit
								| (overWrite ? VR_OVERWRITE : 0);		// translate erase existing file flag to vrpak optn bit
			}
			// vrUnspool clears PriRep overwrite bit upon unspooling to this file.
		// note: info is propogated to runtime data structures in topRp and buildUnspoolInfo
	}

	return RCOK;
}	// RFI::rf_Ckf2
//-----------------------------------------------------------------------------
RC RFI::rf_CheckForDupFileName()		// make sure this RFI is only user of its file name
{
	// check against standard files
	// note: does not check #include'd input files (caveat user)
	const char* msg;
	if (Topi.tp_CheckOutputFilePath( fileName, &msg))
		return ooer( RFI_FILENAME, "Illegal %s '%s'\n    %s",
						mbrIdTx( RFI_FILENAME), fileName, msg);

	RFI* fip;
	RLUP( RfiB, fip)
	{	if (fip->ss >= ss)		// only check smaller-subscripted ones vs this -- else get multiple messages
			break;
		if (!stricmp( fip->fileName, fileName))
			return ooer( RFI_FILENAME, (char *)MH_S0441, mbrIdTx( RFI_FILENAME), fileName, fip->name );
				// "Duplicate %s '%s' (already used in ReportFile '%s')"
	}
	RLUP( XfiB, fip)
	{
		if (fip->ss >= ss)		// only check smaller-subscripted ones vs this -- else get multiple messages
			break;
		if (!stricmp( fip->fileName, fileName))
			return ooer( RFI_FILENAME, (char *)MH_S0442, mbrIdTx( RFI_FILENAME), fileName, fip->name);
						// "Duplicate %s '%s' (already used in ExportFile '%s')"
	}
	return RCOK;
}	// RFI::rf_CheckForDupFileName
//----------------------------------------------------------------------------
int RFI::rf_CheckAccessAndAlias(
	const char* fName,		// file name to check
	char* &fNameAlias,		// returned: modified file name
	const char** ppMsg /*=NULL*/)	// optionally returned
									//   tmpstr msg explaining cause
									//     0: e.g. path does not exist
									//     1: always ""
									//     2: e.g. fName could not be opened
// check report/export file writeability
// attempt to generate aliased name (with "(n)" suffix

// returns: 2 if fName successfully changed
//          1 if fName OK as is
//          0 if failure
{
	int ret = 0;
	if (ppMsg)
		*ppMsg = strtmp("");	// insurance
	char* fNameTry = strtemp( _MAX_PATH);
	strpathparts( fName, STRPPDRIVE|STRPPDIR|STRPPFNAME, fNameTry);
	int lenBase = strlen( fNameTry);
	char fExt[ _MAX_PATH];
	_splitpath( fName, NULL, NULL, NULL, fExt);
	for (int iTry=0; iTry<100; iTry++)
	{	const char* suffix=fExt;
		if (iTry > 0)
			suffix = strtprintf( "(%d)%s", iTry,fExt);
		strcpy( fNameTry+lenBase, suffix);
		const char* msg;
		int wCk = xfWriteable( fNameTry, &msg);
		// wCk -1: error (bad path), 0: can't write, 1: OK
		if (wCk <= 0)
		{	if (ppMsg)
				*ppMsg = msg;
			if (wCk == 0)
				continue;	// try next suffix
							//   *ppMsg sez why this try is bad
		}
		fNameAlias = fNameTry;
		if (wCk > 0)
			ret = 1 + (iTry > 0);
		// else ret = 0
		break;
	}
	return ret;
}		// RFI_CheckAccessAndAlias
//===========================================================================
/*virtual*/ void RI::Copy( const record* pSrc, int options /*=0*/)
{
// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten
	cupfree( DMPP( rpTitle));
// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED
	record::Copy( pSrc, options);				// verfies that src and this are same record type. lib\ancrec.cpp.
// increment reference count for pointer(s) just copied
	cupIncRef( DMPP( rpTitle));
}			// RI::Copy
//===========================================================================
RI::~RI()
{
// free (or decrement reference count of) derived class heap pointers in record being destroyed
	cupfree( DMPP( rpTitle));
}			// RI::~RI
//===========================================================================
COL::COL( basAnc* b, TI i, SI noZ /*=0*/)
	: record( b, i, noZ)
{
}	// COL::COL
//-----------------------------------------------------------------------------
COL::~COL()
{
// free (or decrement reference count of) derived class heap pointers in record being destroyed
	cupfree( DMPP( colHead));
	if (colVal.ty==TYSTR || colVal.ty==DTCHP)  			// if colVal.val contains a string (not a float or nothing)
		cupfree( DMPP( colVal.val));				/* if not UNSET or NANDLE, and if not a pointer to string
       								   constant inline in pseudo-code, dmfree it.  cueval.cpp. */
}			// COL::~COL
//-----------------------------------------------------------------------------
/*virtual*/ void COL::Copy( const record* pSrc, int options/*=0*/)
{
// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten
	cupfree( DMPP( colHead));
	if (colVal.ty==TYSTR || colVal.ty==DTCHP)  	// if contains a string, not a float
		cupfree( DMPP( colVal.val));			/* if not UNSET or NANDLE, and if not a pointer to string constant
       								   inline in pseudo-code, dmfree it.  cueval.cpp. */
// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED
	record::Copy( pSrc, options);				// verfies that src and this are same record type. lib\ancrec.cpp.
// increment reference count for pointer(s) just copied
	cupIncRef( DMPP( colHead));
	if (colVal.ty==TYSTR || colVal.ty==DTCHP)
		cupIncRef( DMPP( colVal.val));	// if not UNSET or NANDLE, and if not a pointer to string constant
}			// COL::Copy
//-----------------------------------------------------------------------------
/*virtual*/ record& COL::CopyFrom( record* pSrc, int copyName/*=1*/, int dupPtrs/*=0*/)
{
// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten
	cupfree( DMPP( colHead));
	if (colVal.ty==TYSTR || colVal.ty==DTCHP)  	// if contains a string, not a float
		cupfree( DMPP( colVal.val));			/* if not UNSET or NANDLE, and if not a pointer to string constant
       								   inline in pseudo-code, dmfree it.  cueval.cpp. */
// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED
	record::CopyFrom( pSrc, copyName, dupPtrs);			// verfies that src and this are same record type. lib\ancrec.cpp.
// increment reference count for pointer(s) just copied
	cupIncRef( DMPP( colHead));
	if (colVal.ty==TYSTR || colVal.ty==DTCHP)
		cupIncRef( DMPP( colVal.val));	// if not UNSET or NANDLE, and if not a pointer to string constant
	return *this;
}			// COL::CopyFrom
//---------------------------------------------------------------------------------------
DVRI::~DVRI()
{
// free (or decrement reference count of) derived class heap pointers in record being destroyed
	cupfree( DMPP( rpTitle));			// dmfree unless UNSET or inline "text" in pseudocode, cueval.cpp.
}			// DVRI::~DVRI
//---------------------------------------------------------------------------------------
/*virtual*/ void DVRI::Copy( const record* pSrc, int options/*=0*/)	// overrides record::Copy. declaration must be same.

{
// free (or decr ref count for) derived class heap pointer(s) in record about to be overwritten
	cupfree( DMPP( rpTitle));			// dmfree unless UNSET or inline "text" in pseudocode, cueval.cpp.

// use base class Copy.  Copies derived class members too, per record type (.rt): RECORD MUST BE CONSTRUCTED
	record::Copy( pSrc, options);				// verfies that src and this are same record type. lib\ancrec.cpp.

// increment reference count for pointer(s) just copied
	cupIncRef( DMPP( rpTitle));
}			// DVRI::Copy
//====================================================================================



//================================= REPORT/EXPORT FORMATTING externally called functions ===================================


//---------------------------------------------------------------------------------------------------------------------------
char* getErrTitleText() 			// get "ERR" report title text -- public function

{
	// caller 11-91: rmkerr.cpp.  used when text first put into ERR virtual report

	if (!errTitle)			// if not ready to use (NULL'd whenever runTitle etc may have changed)
	{
		// get/default/improvise input data, to work as well as practical when called before input completed

		TOPRAT* tp;
		int repCpl = bracket( 78, getCpl( &tp), 132);	// chars per line: get best value yet avail
														// default if Top.repCpl unset. below.
		// format title text

		if (dmal( DMPP( errTitle), repCpl + 11, PWRN)) 			// +11 for up to 5 crlf's, and \0
			return "";							// if failed, return value that will fall thru code
		int m = sprintf( errTitle, "\n\nError Messages for Run %03d:",	// title
					 tp ? tp->runSerial : 0 );  		// run serial number, or 000 early in session
		// (or cd default to cnRunSerial & init that sooner?
		// note 3 more uses in this file. 7-92) */
		char* p = errTitle + m;
		int r = repCpl - m + 2;					// remaining space on line after the 2 \n's
		if (tp)
			addTx( tp->runTitle, 1, &p, &r);			// 1 space and run title text if any, update p
		strcpy( p, "\n\n");
	}
	return errTitle;				// return pointer to buffer with formatted title text
}			// getErrTitleText
//---------------------------------------------------------------------------------------------------------------------------
char* getLogTitleText() 			// get "LOG" report title text -- public function
// called at first addition of text to LOG report
{

	if (!logTitle)			// if not ready to use (NULL'd whenever runTitle etc may have changed)
	{
		// get/default/improvise input data, to work as well as practical when called before input completed

		TOPRAT* tp;
		int repCpl = bracket( 78, getCpl( &tp), 132);	// chars per line: get best value yet avail
														// default if Top.repCpl unset. below.

		// format title text
		if (dmal( DMPP( logTitle), repCpl + 11, PWRN))     	// +11 for up to 5 crlf's, and \0
			return "";						// if failed, return value that will fall thru code
		int m = sprintf( logTitle, "\n\n%sLog for Run %03d:",
					tp ? tp->tp_RepTestPfx() : "",	// test prefix (hides runDateTime re testing text compare)
					tp ? tp->runSerial : 0 );  		// run serial number, or 000 early in session (unexpected here).
		char* p = logTitle + m;
		int r = repCpl - m + 2;					// remaining space on line after the 2 \n's
		if (tp)
			addTx( tp->runTitle, 1, &p, &r);			// 1 space and run title text if any, update p
		strcpy( p, "\n\n");
	}
	return logTitle;				// return pointer to buffer with formatted title text
}			// getLogTitleText
//---------------------------------------------------------------------------------------------------------------------------
char* getInpTitleText() 			// get "INP" report title text -- public function

{
	// caller 11-91: pp.cpp, at first addition of text to INP report (input listing)

	if (!inpTitle)			// if not ready to use (NULL'd whenever runTitle etc may have changed)
	{
		// get/default/improvise input data, to work as well as practical when called before input completed

		TOPRAT* tp;
		int repCpl = bracket( 78, getCpl( &tp), 132);	// chars per line: get best value yet avail
														// default if Top.repCpl unset. below.
		// format title text
		if (dmal( DMPP( inpTitle), repCpl + 11, PWRN))    	// +11 for up to 5 crlf's, and \0
			return "";						// if failed, return value that will fall thru code
		int m = sprintf( inpTitle, "\n\nInput for Run %03d:", 	// title
					 tp ? tp->runSerial : 0 );  	// run serial number, or 000 early in session (unexpected here).
		char* p = inpTitle + m;
		int r = repCpl - m + 2;					// remaining space on line after the 2 \n's
		if (tp)
			addTx( tp->runTitle, 1, &p, &r);			// 1 space and run title text if any, update p
		strcpy( p, "\n\n");
	}
	return inpTitle;				// return pointer to buffer with formatted title text
}			// getInpTitleText
//---------------------------------------------------------------------------------------------------------------------------
const int HFBUFSZ = 2*132 + 6 + 1 + 100;	// 2 lines of text up to 132, 3 crlf's, null, insurance
//---------------------------------------------------------------------------------------------------------------------------
char* getHeaderText( int pageN) 			// get header text -- public function

// (currently no page # in header; argument is to localize changes if one is put there)
{
	if (!header)			// if not ready to use.  note NULL'd at start new run and at completion of input.
	{

		// get/default/improvise input data, in case called before input decoding complete

		TOPRAT* tp;
		int repCpl = bracket( 78, getCpl( &tp), 132);	// chars per line: get best value yet avail
														// default if Top.repCpl unset. below.

		// allocate header storage
		if (dmal( DMPP( header), HFBUFSZ, PWRN))  	// header
			return "";								// if failed, return value that will fall thru ok

		// header: start by putting top margin crlf's in header buffer

		char* p = header;
		int m = tp ? tp->repTopM : 3;			// default 3 if no initialized TOPRAT record available
		for (int i = 0;  i < m  &&  i < 12;  i++)		// apply limit in case using garbage
		{
			*p++ = '\r';
			*p++ = '\n';
		}

		// header: text line: left and right parts

		int r = repCpl;				// space available on line
		if (tp)						// if found a useable TOPRAT record
		{
			addTx( tp->repHdrL,  0, &p, &r);		// if ptr nonNULL, copy text to p, update p, r.  local fcn.
			addTx( tp->repHdrR, -1, &p, &r);   		// -1: right-adjust this text in r cols, do leading blanks
		}
		*p++ = '\r';
		*p++ = '\n';   			// end line with crlf

		// header: row of ----- and blank line

		memsetPass( (void **)&p, '-', repCpl);  		// row of -'s / point past (strpak.cpp fcn)
		strcpy( p, "\r\n\r\n");				// 2 crlf's (1 blank line) and null
	}
	return header;				// return pointer to buffer with 3 lines of formatted header text & TopM.
}			// getHeaderText
//---------------------------------------------------------------------------------------------------------------------------
char* getFooterText( int pageN) 			// get footer text for specified page number -- public function

// caller is expected to print leading blank line before printing this text.

{
	if (!footer)   				// if not ready to use. note NULL'd at start new run and at completion of input.
	{
		// get/default/improvise input data, in case called before input decoding complete
		TOPRAT* tp;
		int repCpl = bracket( 78, getCpl( &tp), 132);	// chars per line: get best value yet avail
														// default if Top.repCpl unset. below.
		// allocate footer storage
		if (dmal( DMPP( footer), HFBUFSZ, PWRN))	// footer. +3 for page number overflow (if 32767 not 1-99)
			return "";					// failed, return value that will fall thru. msg issued.

		// footer: leading blank line: generated by space-down code

		// footer: line of ----------

		char* p = footer;
		memsetPass( (void **)&p, '-', repCpl);  	// line of -'s. truncate if nec to allow full text row. pt after. strpak.cpp.
		*p++ = '\r';
		*p++ = '\n';		// end line with crlf

		// footer text line: left part

		int r = repCpl;			// remaining space on line
		int rReserve = 30;		// reserved space, default = insurance
		if (tp)
		{	addTx( tp->tp_RepTestPfx(), 0, &p, &r);	// add test prefix to footer (hides runDateTime re testing text compare)
			rReserve = strlen( tp->runDateTime) + 5 + 9;
			if (!IsBlank( tp->runTitle))
				rReserve += strlen( tp->runTitle) + 2;
		}
		addTx( InputFileName, 0, &p, &r, rReserve);		// add user-entered input file name (rundata.cpp); updates p and r.
		// or InputFilePath if full path and defaulted extension desired
		if (tp)
		{ 	if (r > 5)
			{	int m = sprintf( p, "  %03d", tp->runSerial);  	// run serial number
				p += m;
				r -= m;
			}
			addTx( tp->runTitle, 2, &p, &r);			// put run title after serial number

			// footer text line: right part

			r -= 9;						// reserve 9 columns for "  Page 99"
			addTx( tp->runDateTime, -1, &p, &r);  	// if nonNULL, right-adjust run date & time string in this space;
       											// always blank-fills unused part of space; updates p.
		}
		strcpy( p, "  Page 00\r\n");    		// add "  Page "; rest will be overwritten
		footerPageN = p + 7;					// save ptr to where to put page number
		// before footer is used, getFooterText puts page number, \r\n\0 at footerPageN.
		// note that footer[] has 3 extra bytes in case page # is 32767 not the expected 1-99.
	}
	if (footerPageN  &&  footer  &&  *footer)	// insurance
		sprintf( footerPageN, "%2d\r\n", pageN);	// generate text for page number and final \r\n\0 in place in footer
	// CAUTION fix this code if page # no longer at end of footer.
	return footer;				// return pointer to buffer with 2 lines of formatted header text
}			// getFooterText
//---------------------------------------------------------------------------------------------------------------------------
int getCpl(			// get characters per line, public function callable before input complete
	TOPRAT** pTp /*=NULL*/)	// optionally returned: where value found
{
	TOPRAT* tp =
		Top.ck5aa5==0x5aa5   ? &Top		// source for user values: Top if it has been set,
      : Topi.ck5aa5==0x5aa5  ? &Topi 	// or Top input record (input may be in progress if here)
	  :                        NULL;   	// else none. NULL or 0L both bother lint here.
	if (pTp)
		*pTp = tp;
	return tp && tp->repCpl 	// if found a good TOPRAT, with non-0 value in it
				?  tp->repCpl 	// use that value
				:  78;			// else use default value of 78
}			// getCpl
//---------------------------------------------------------------------------------------------------------------------------
int getBodyLpp()		// get report body lines per page, public function callable before input complete

{
	TOPRAT *tp;
	getCpl( &tp);		// get tp (ignore return)

	return ( tp && tp->repLpp 			// if found a good TOPRAT, with non-0 lines per page value in it

			 ?  max( tp->repLpp - tp->repTopM - tp->repBotM - HDRROWS - FTRROWS,	/* compute bodyLpp therefrom */
					 20 )								// apply a minimum to not screw up code

			 :  66 -3 -3 -HDRROWS -FTRROWS );	// else use default for 66 lpp, 3 tm, 3 bm, 3 header, 3 footer lines 11-91.

}				// getBodyLpp
//---------------------------------------------------------------------------------------------------------------------------
void freeHdrFtr()		// free header, footer, and report titles texts

// make this free any other cncult dm not directly accessible, as added

// useful re chasing unaccounted dm (if not called, space is reused here)

// to free memory, call only after report generation complete -- else texts will regenerate

{
	// caller: app\cse.cpp; cncult2.cpp:cncultClean(); freeRepTexts (next)

	dmfree( DMPP( header));			// free pointed to memory block, and set pointer NULL.  dmpak.cpp.
	dmfree( DMPP( footer));			//  header and footer are local variables at start of this file.
	dmfree( DMPP( errTitle));    		// ERR report title
	dmfree( DMPP( logTitle));    		// LOG report title
	dmfree( DMPP( inpTitle));    		// INP report (input listing) title
}				// freeHdrFtr
//---------------------------------------------------------------------------------------------------------------------------
RC freeRepTexts()

// say various report heading etc texts should be (re)generated b4 use as may have new, more, or better inputs
// called from cncult2:topStarPrf (b4 data input) and also from cncult2:setTop (after data input)
{
	freeHdrFtr();		// currently 1-92 same as fcn above
	return RCOK;
}			// freeRepTexts
//---------------------------------------------------------------------------------------------------------------------------
LOCAL void addTx( 		// conditionally add text to line being formed
	const char* s,		// NULL or text to add (blank string treated as NULL)
	int spc,			// number of spaces to add first, or -1 to right-adjust with leading blanks (min 2)
	char** pp, 			// ptr to char * ptr, returned updated
	int* pr,			// ptr to remaining space on line, updated, truncates text to not exceed.
	int rReserve /*=0*/)	// reserved space on line (truncate s if needed)

// if s is NULL: left-adjusted: NOP; right-adjusted: outputs r spaces.

// if spc + 1 char won't fit: left-adjusted: NOP: no spaces output; right-adjusted: outputs r spaces.

{
	// internal fcn for getErrTitle, getHeader, etc
	char* p = *pp;
	int r = *pr;			// fetch args
#if 1
	int m = s && !IsBlank( s) ? strlen( s) : 0;
	if (spc < 0)
		spc = max( r - m, 2);		// # leading spaces to right-adjust text / always separate with 2+ spaces
	if (r > spc && m)
	{	while (spc-- > 0  &&  r > 0)
		{	r--;     // leading / separating blanks
			*p++ = ' ';
		}
		if (m > r - rReserve)
			m = r - rReserve;
		if (m > 0)
		{	memcpyPass( (void **)&p, s, m);
			r -= m;	// copy, advance p past, lib\strpak.cpp
		}
		*pp = p;
		*pr = r;				// return updated args
	}
	*p = 0;					// put null after
#else
		int m = s ? strlen( s) : 0;

	if (spc < 0)
		spc = max( r - m, 2);		// # leading spaces to right-adjust text / always separate with 2+ spaces
	else if (r <= spc || !s)
		return;
	while (spc-- > 0  &&  r > 0)
	{
		r--;     // leading / separating blanks
		*p++ = ' ';
	}
	if (m > r)
		m = r;					// truncate to space available
		if (m > 0)
	{	memcpyPass( (void **)&p, s, m);
		r -= m;	// copy, advance p past, lib\strpak.cpp
	}
	*p = 0;					// put null after
	*pp = p;
	*pr = r;				// return updated args
#endif
}			// addTx
//---------------------------------------------------------------------------------------------------------------------------

///////////////////////////////////////////////////////////////////////////////
// HOLIDAYS
///////////////////////////////////////////////////////////////////////////////

// HOLIDAY PRE-INPUT FCN called by cncult2.cpp:topStarPrf

RC topPrfHday()		// add default HDAY (holiday) records, before input
{
	addHdayDate( "New Year's Day",          1  );
	addHdayDate( "M L King Day",            15 );				// jan 15
	addHdayDate( "Fourth of July",  182-1 + 4  );				// july 4
	addHdayDate( "Veterans Day",    305-1 + 11 );				// nov 11
	addHdayDate( "Christmas",       335-1 + 25 );				// dec 25
	addHdayRule( "President's Day", C_HDAYCASECH_THIRD,  C_DOWCH_MON,  2 );	// 3rd monday in feb
	addHdayRule( "Memorial Day",    C_HDAYCASECH_LAST,   C_DOWCH_MON,  5 );	// last monday in may
	addHdayRule( "Labor Day",       C_HDAYCASECH_FIRST,  C_DOWCH_MON,  9 );	// 1st monday in sep
	addHdayRule( "Columbus Day",    C_HDAYCASECH_SECOND, C_DOWCH_MON, 10 );	// 2nd monday in oct
	addHdayRule( "Thanksgiving",    C_HDAYCASECH_FOURTH, C_DOWCH_THU, 11 );	// 4th thursday in nov
	return RCOK;								// unexpected memory full errors not returned
}		// topPrfHday
//---------------------------------------------------------------------------------------------------------------------------
LOCAL void addHdayDate( char *name, DOY date) 		// add holiday celebrated on specified date or monday after
{
	// for topPrfHday
	HDAY *hdi;
	if (HdayiB.add( &hdi, WRN))  return; 		// add holiday input record (ancrec.cpp) / return if error (msg issued)
	strncpy( hdi->name, name, sizeof(ANAME)-1 );	// record name, for like/alter/delete and error messages
	hdi->hdDateTrue = date;				// store true date
	hdi->hdOnMonday = C_NOYESCH_YES;			// say observe on following monday if falls on weekend
	// topHday will set hdDateObs.
}					// addHdayDate
//---------------------------------------------------------------------------------------------------------------------------
LOCAL void addHdayRule( char *name, HDAYCASECH hdCase, DOWCH dow, MONTH mon)
// add holiday celbrated on <n>th <weekday> of <month>
{
	// for topPrfHday
	HDAY *hdi;
	if (HdayiB.add( &hdi, WRN))  return; 		// add holiday input record (ancrec.cpp) / return if error (msg issued)
	strncpy( hdi->name, name, sizeof(ANAME)-1 );	// record name, for like/alter/delete
	hdi->hdCase = hdCase;				// store arguments
	hdi->hdDow = dow;
	hdi->hdMon = mon;
	// topHday will set hdDateTrue and hdDateObs.
}				// addHdayRule


//================================ HOLIDAY CHECK/FINISH FCN called by cncult2.cpp:topCkf =======================================

RC topHday()		// check HDAY input info / build run info
{
	HDAY *hdi, *hd;
	RC rc;
	E( HdayB.al( HdayiB.n, WRN) )		// delete old holiday run records, alloc needed # now for min fragmentation.
	// E: return if error.  CAUTION: ratCre may not errCount++ on error.
	RLUP( HdayiB, hdi)				// loop over holiday input records
	{
		// check for valid holiday

		// name may be required (just for understandable error msgs) per flag in cncult:topT, enforced by cul.cpp.
		// all hday members contain 0 if not entered: no nz defaults, no runtime expressions.

		// valid input is  hDateTrue with hDateObs or hdOnMonday or neither  OR  hdCase + hdDow + hdMon.

		if (hdi->hdDateTrue || hdi->hdDateObs || hdi->hdOnMonday)		// if specified by date
		{
			if (hdi->hdCase || hdi->hdDow || hdi->hdMon)
			{
				hdi->oer( (char *)MH_S0566);		// "Can't intermix use of hdCase, hdDow, and hdMon\n"
				continue;							// "    with hdDateTrue, hdDateObs, and hdOnMonday for same holiday"
			}
			if (!hdi->hdDateTrue)
			{
				hdi->oer( (char *)MH_S0567);     // "No hdDateTrue given"
				continue;
			}

			if (hdi->hdDateTrue < 1  ||  hdi->hdDateTrue > 365)
			{
				hdi->oer( (char *)MH_S0568); 					// "hdDateTrue not a valid day of year"
				continue; 								// only 1 err msg per holiday
			}
			if (hdi->hdDateObs)
			{
				if (hdi->hdDateObs < 1  ||  hdi->hdDateObs > 365)
				{
					hdi->oer( (char *)MH_S0569); 					// "hdDateObs not a valid day of year"
					continue;
				}
				if (hdi->hdOnMonday)
				{
					hdi->oer( (char *)MH_S0570);     // "Can't give both hdDateObs and hdOnMonday"
					continue;
				}
			}
		}
		else if (hdi->hdCase || hdi->hdDow || hdi->hdMon)			// else must be spec'd by case
		{
			if (!hdi->hdCase || !hdi->hdDow || !hdi->hdMon)
			{
				hdi->oer( (char *)MH_S0571);  		// "If any of hdCase, hdDow, hdMon are given, all three must be given"
				continue;
			}
			if (hdi->hdDow < 1 || hdi->hdDow > 7)
			{
				hdi->oer( (char *)MH_S0572);     // "hdDow not a valid day of week"
				continue;
			}
			if (hdi->hdMon < 1 || hdi->hdMon > 12)
			{
				hdi->oer( (char *)MH_S0572);     // "hdDow not a valid month"
				continue;
			}
		}
		else
		{
			hdi->oer( (char *)MH_S0573);
			continue;			// "Either hdDateTrue or hdCase+hdDow+hdMon must be given"
		}

		// ok, copy data from input records to run records

		HdayB.add( &hd, ABT, hdi->ss);				// add HDAY run record, ret ptr to it.  err unexpected cuz al'd.
		*hd = *hdi;						// copy entire record including name; incRefs any heap pointers.

		// set derived fields in run record (not in input record: some would make errors on next RUN)

		if (hd->hdDateTrue)					// if specified by date, not case
		{
			if (!hd->hdDateObs)
			{
				hd->hdDateObs = hd->hdDateTrue;
				if (hd->hdOnMonday==C_NOYESCH_YES)  		// YES means change Sat or Sun to next Monday
					switch ( (hd->hdDateTrue - 1 			// day of year, 0-364
							  + Top.jan1DoW - 1) 			// plus day of week of jan 1, 0-6, 0 = Sunday
							 % 7 )					// make 0 if Holiday true date falls on Sunday, 1 on Monday, etc
					{
					case 6:
						hd->hdDateObs++;			// true date falls on Saturday: add 2 for Monday (fall thru case)
					case 0:
						hd->hdDateObs++;			// true date falls on Sunday: add 1 for Monday
					}
			}
		}
		else							// hd->hdCase, hdDow, hdMon given
			hd->hdDateTrue = hd->hdDateObs = tdHoliDate( Top.year, hd->hdCase, hd->hdDow-1, hd->hdMon);	// tdpak.cpp
	}
	return RCOK;
}			// topHday

// end of cncult4.cpp
