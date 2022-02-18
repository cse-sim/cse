// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnguts.h -- Include file for CSE hourly simulator (guts) functions

// include first: ancrec.h for anc<T> template class.
//                rccn.h for record structures used in template class instances: TOPRAT, MAT, ZNI, etc.

/*-------------------------------- OPTIONS --------------------------------*/

#undef SHSZ	// define for sum-of-zones subhour results (untested; KEEP for poss future use), cnguts.cpp.
		// undefined 1) for speed 2) to decouple zones so could later do subhrDur on cluster basis.


/*-------------------------------- DEFINES --------------------------------*/

//----- Timers: used in cse.cpp.
const int TMR_INPUT = 0;	// input decoding
const int TMR_AUSZ = 1;		// autosizing
const int TMR_RUN = 2;		// main simulation run
const int TMR_REP = 3;		// reports at end of run
const int TMR_TOTAL = 4;	// total time.  TMR_INPUT - TMR_TOTAL are
							//   printed to report file
							// additional timers active iff DETAILED_TIMING
const int TMR_AIRNET = 5;	// airnet calcs
const int TMR_AWTOT = 6;	// ASHWAT total time (including setup)
const int TMR_AWCALC = 7;	// ASHWAT calculations (included in AWTOT)
const int TMR_COND = 8;		// opaque surface conduction
const int TMR_BC = 9;		// boundary condition setup
const int TMR_ZONE = 10;	// zone modeling
const int TMR_KIVA = 11;	// kiva foundation modeling

#if defined( DETAILED_TIMING)
#define TMRSTART( tmr) tmrStart( tmr)
#define TMRSTOP( tmr) tmrStop( tmr)
#else
#define TMRSTART( tmr)
#define TMRSTOP( tmr)
#endif

//----- for MASSBC for MSRAT (Masses records)
// Mass boundary condition types for MASSBC.ty (MASSBC: cnrecs.def/rccn.h)
enum MSBC {
	MSBCADIABATIC=1,	// Adiabatic (no heat flow at surface)
	MSBCAMBIENT,		// Ambient (outdoor conditions adjacent to surface)
	MSBCZONE,			// Zone (Zone MASSBC.zi conditions adjacent to surface)
	MSBCSPECT,			// specified adjacent air temp = MASSBC.exT
	MSBCGROUND,			// ground temp (surface exterior coupled to lagged air temp(s))
	MSBCTNODE };		// specified adjacent node temp = MASSBC.exT
						//   formerly used re external UZM linkage, 7-2012
inline int IsMSBCZone( int bcty) { return bcty==MSBCZONE || bcty==MSBCTNODE; }

//----- for ZNRES_IVL_SUB: zone simulation results for 1 interval
// constants for limits for avg and sum loops, cgnguts.cpp & cgenbal.cpp.  Note members are grouped by type.
#define oRes(m) offsetof( ZNRES_IVL_SUB, m)

//----- for AHRES_IVL_SUB: air handler simulation results for 1 interval
// constants for limits for avg and sum loops, cnguts.cpp & cgenbal.cpp.  Members are grouped as floats, then LI's.
#define oaRes(m) offsetof( AHRES_IVL_SUB, m)

//----- for ZNR.mdSeq[].xi: zone hvac mode sequence ZHX subsript (> 0) or --
#define MDS_FLOAT (-1)		// in this mode temp is floating; is no active (tstat-ctrl'd) ZHX (Zone Heat Xfer) record.


// XSURF: structure for one radiant/conductive coupling to ambient

//	Each Zone's ZNR record (entry) points to a chain of XSURFs.
//	Each XSURF represents an exterior opaque surface, window, or perim.
//	An XSURF has a uval from the ambient (or .exT) to the zone air (or to outside surface of mass for CTMXWALL).
//	An XSURF receives radiation per its area, orientation, & absorptivity, and distributes ("targets")
//	        the solar gain to zone air or mass inside or outside surface, per info in the structure.

// Definitions for XSURF.ty.
// Note that code totals UA for all light XSURFs (ie not CTMXWALL); if these defn's change, check that code.
const int CTNONE    = 0;	// unknown (must be 0)
const int CTEXTWALL = 1;	// Light ext surface (wall, ceiling, door, etc) (has uval, not "massive")
const int CTINTWALL = 2;	// Light interzone or adiabatic surface: needed re cavity area, not massive, no ambient exposure, no solar
const int CTWINDOW  = 3;	// Window
const int CTMXWALL  = 4;	// Massive exterior wall: XSURF for it targets solar gain to the mass (cgsolar.cpp).
							// Mass itself is represented in MSRAT.
							// .uval is ambient to outer surface, re solar gain split, not ambient to zone.
const int CTPERIM   = 5;	// Perimeter
const int CTKIVA    = 6;	// KIVA
const int CTMAXVALID= 6;	// maximum legal value (re validation)

// Component solar gain distribution target types.
//		For SGRAT.targTy and XSURF.sgdist[].targTy.  Used 2-95 in cncult3,cgsolar,cgdebug,cnrecs.
const int SGDTTZNAIR=0;	// Zone air
const int SGDTTZNTOT=1;	// Zone accounting total, not used in simulation
const int SGDTTSURFI=2;	// surface inside
const int SGDTTSURFO=3;	// surface outside (ambient or interzone)

// classification by tilt (see xs_tyFromTilt())
//  Note: code depends on values
const int xstyCEILING = 0;
const int xstyWALL = 1;
const int xstyFLOOR = 2;


//----------------------- RUNDATA.CPP PUBLIC VARIABLES ----------------------

// more comments and set/used info in rundata.cpp

//-- cnguts.cpp run record basAncs --
extern anc<TOPRAT> TopR;		// basAnc for Top run record. ancrec.h template class.
extern TOPRAT Top;				// the one static Top runtime record. rccn.h type.
extern anc<WFILE> WfileR;		// basAnc for the one weather file info record 1-94. Runtime only - no input basAnc.
extern WFILE Wfile;				// the one static WFILE record with overall info and file buffer for 1 day's data
extern anc<WFDATA> WthrR;		// basAnc for hour's weather data record. Runtime only - no corress input basAnc.
extern WFDATA Wthr;				// the one static WFDATA record containing unpacked & adjusted data for hour 1-94
extern anc<WFDATA> WthrNxHrR;	// basAnc for next hour's weather data record ("weatherNextHour")
extern WFDATA WthrNxHr;			// static record for next hour's unpacked & adjusted data for cgwthr.cpp read-ahead
extern anc<DESCOND> DcR;		// design conditions

extern anc<ZNR> ZrB;			// zones runtime info
extern anc<ZNRES> ZnresB;      	// Zones simulation results info
extern anc<IZXRAT> IzxR;		// interzone transfers
extern anc<DOAS> doasR;			// DOAS
extern anc<RSYS> RsR;			// residential system
extern anc<RSYSRES> RsResR;		// residential system results
extern anc<DUCTSEG> DsR;		// duct segments
extern anc<DUCTSEGRES> DsResR;	// duct segment results
extern anc<AFMTR> AfMtrR;		// air flow meters
extern anc<DHWMTR> WMtrR;  		// DHW meters
extern anc<DHWSYS> WsR;			// DHW system
extern anc<DHWSYSRES> WsResR;	// DHW system results

extern anc<DHWHEATER> WhR;		// DHW heater
extern anc<DHWHEATER> WlhR;		// DHW loop heater
extern anc<DHWTANK> WtR;		// DHW tank
extern anc<DHWHEATREC> WrR;		// DHW heat recovery
extern anc<DHWPUMP> WpR;		// DHW pump
extern anc<DHWLOOP> WlR;		// DHW loop
extern anc<DHWLOOPSEG> WgR;		// DHW loop segment
extern anc<DHWLOOPBRANCH> WbR;	// DHW loop branch
extern anc<DHWLOOPPUMP> WlpR;	// DHW loop pump
extern anc<DHWDAYUSE> WduR;		// DHW day use
extern anc<DHWUSE> WuR;			// DHW use (single draw)

extern anc<DHWSOLARSYS> SwhR;   // DHW solar system
extern anc<DHWSOLARCOLLECTOR> ScR;   // DHW solar collector

extern anc<PVARRAY> PvR;        // PV array
extern anc<BATTERY> BtR;		// Battery

extern anc<SHADEX> SxR;			// Shade

extern anc<GAIN> GnB;			// (zone) gains (with their fuel use) run records basAnc
extern anc<MTR> MtrB;   		// meters, by name, each containing use by interval H D M Y and end use
extern anc<TU> TuB;   			// terminals
extern anc<AH> AhB;   	    	// air handlers
extern anc<ZHX> ZhxB;			// zone HVAC transfers
extern anc<AHRES> AhresB;		// air handler results
extern anc<HEATPLANT> HpB;  	// heatplants
extern anc<BOILER> BlrB;    	// boilers
extern anc<COOLPLANT> CpB;   	// COOLPLANTs run info
extern anc<CHILLER> ChB;   		// CHILLER run info
extern anc<TOWERPLANT> TpB;   	// TOWERPLANT run info
extern anc<XSRAT> XsB;			// XSURFs
extern anc<WSHADRAT> WshadR;	// Window SHADing info for shaded windows
extern anc<MSRAT> MsR;   		// Masses
extern anc<KIVA> KvR;   		// Kiva Instances
extern anc<SGRAT> SgR;    		// Solar gains
// construction: CON,LR,MAT have input info only.
extern anc<GT> GtB;		// GlazeTypes, 12-94
// note: runtime info for RE/EXPORTFILEs: all there is in UnspoolInfo, below.
// note: runtime info for RE/EXPORTs is in DvriB and RcolB/XcolB, various globals and Top and ZnR members, and UnspoolInfo.
extern anc<DVRI> DvriB;			// Date-dependent Virtual Reports Info
extern anc<COL> RcolB;			// Columns info for user-defined reports
extern anc<COL> XcolB;			// Columns info for user-defined exports (.what changed in cncult.cpp)
extern anc<IMPF> ImpfB;			// IMPORTFILEs runtime info 2-94
extern anc<IFFNM> IffnmB;		// Import File Field Names compile support & runtime info 2-94. No Input Rat.
extern anc<HDAY> HdayB;			// holidays. Default records supplied.

//-- cnguts.cpp Input Globals: set externally
extern const char* cmdLineArgs;			// invoking cmd line argv[ 1 ..]
extern const char* exePath;				// path(s) to .exe file or dll;exe
extern const char* InputDirPath;		// drive/dir path to input file
extern const char* InputFileName;		// Input file name as given by user (ptr to argv[])
										//   (with or w/o drive/dir/ext)
extern const char* InputFilePath;		// Input file full path
extern const char* InputFilePathNoExt;	// input file full pathname without .ext
extern VROUTINFO5 PriRep;	// info about primary output file, for appending final end-session
							// info in cse.cpp (after report file input records have been deleted).
							// members set from cncult (at input) and vrpak (at close);
							// room for user to stuff in up to 5 vrh's.
// run serial number in lieu of future status file, 7-92.
extern SI cnRunSerial;			// incremented in cgInit; copied to Topi.runSerial in cncult2\TopStarPrf2.

// virtual report unspooling specifications for this run
extern VROUTINFO* UnspoolInfo;	// dm array of info re unspooling virtual reports into actual report files.
								//  set up in cncult.cpp; format is for vrUnspool. vrpak.h struct.

//---------------------- CGSOLAR.CPP PUBLIC VARIABLE ------------------------
extern int slrCalcJDays[13]; 	// jDay for slr calcs or ausz DT decision for months 1-12. [0] for heat design day.


//------------------------- FUNCTION DECLARATIONS ---------------------------
// error file path (in TmpStr)
inline const char* ErrFilePath() { return strffix2( InputFilePathNoExt, ".err", 1); }

// cnguts.cpp
void FC cgClean(CLEANCASE cs);
void FC cgPreInit();
void FC cgInit();		// init before all phases
void FC cgDone();		// cleanup after all phases
void FC runFazDataFree();
// BOOL DbDo( DWORD oMsk);	// decl in cnglob.h
RC   FC cgFazInit( int isAusz);	// init done for main sim or autosize -- once before all design days
RC   FC cgRddInit( int isAusz);	// init done for main sim run or each autoSize design day
RC   FC cgRddDone( int isAusz);	// cleanup for main sim run or each autoSize design day
RC   FC cgFazDone( int isAusz);	// cleanup for main sim or autosize -- once after all design days

// cnausz.cpp
RC   FC cgAusz();		// 6-95
RC   FC asFazInit(int isAusz);	// 6-95
RC   FC asRddiInit();		// 6-95
#if 0
  RC   FC asmsAfterWarmup();	// 6-95
#endif

// cnloads.cpp
RC   FC loadsHourBeg();
RC   FC loadsSubhr();
RC   FC loadsAfterSubhr();
RC   FC loadsAfterHour();

// cnztu.cpp. Also many member function declarations, in cnrecs.def/rccn.h.
//RC   FC hvacAfterHour();		when needed, meanwhile:
RC   FC inline hvacAfterHour() { return RCOK; }

// cnah.cpp: [also] see member function declarations, in cnrecs.def/rccn.h.
RC FC hvacIterSubhr();

// cncoil.cpp. [Also] many member function declarations, in cnrecs.def/rccn.h.

// cnfan.cpp. [Also] member function declarations, in cnrecs.def/rccn.h.

// cnhp.cpp: [Also] many member function declarations, in cnrecs.def/rccn.h.
// cncp.cpp: [Also] many member function declarations, in cnrecs.def/rccn.h.
#ifdef DEBUG2		// cnglob.h
 void FC endCp();
// cntp.cpp: [Also] many member function declarations, in cnrecs.def/rccn.h.
 void FC endTp();
#endif

// cgresult.cpp
void FC cgReportsDaySetup();
void FC vpRxportsFinish();
void FC vpRxports( IVLCH freq, BOO auszOnly = FALSE);

// cgenbal.cpp
void cgenbal( SI interval, FLOAT tol);

// cgsolar.cpp
void FC makHrSgt( float * pVerSun = NULL);

// cgdebug.cpp
void FC cgzndump( SI vrh, TI zi);

// dhwcalc.cpp
RC DHWBegIvl(IVLCH ivl);
RC DHWSubhr();
RC DHWEndIvl(IVLCH ivl);


// end of cnguts.h
