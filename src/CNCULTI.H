// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// cnculti.h: internal declarations shared only amoung cncult,2,3,4,5,6.cpp.

	// see also: cncult.h

/*--------------------------------- TYPES ---------------------------------*/

// shorthand member offsets into SFI.x
#define SFX(m)  (SFI_X+XSURF_##m)
#define SFXI(m) (SFI_X+XSURF_SBCI+SBC_##m)
#define SFXO(m) (SFI_X+XSURF_SBCO+SBC_##m)

// shorthand member offsets into DUCTSEG.ds_sbcO
#define DSBC( m) (DUCTSEG_SBCO+DBC_##m)

/*---------------------------- OTHER VARIABLES ----------------------------*/

/*------------------------- FUNCTION DECLARATIONS -------------------------*/

// cncult.cpp
void FC iRatsFree();			// cleanup fcn: free all input basAnc record storage

// many functions in cncult.cpp are called only via ptrs in CULT tables pointed to by cnTopCult, & have no decls.
// cncult.cpp... check-fcns recalled from cncultn.cpp
RC FC gtStarCkf( CULT* c, /*GT* */  void *p, void *p2, void *p3);
RC FC wnStarCkf( CULT* c, /*SFI* */ void *p, void *p2, void *p3);
RC FC drStarCkf( CULT* c, /*SFI* */ void *p, void *p2, void *p3);
RC FC sfStarCkf( CULT* c, /*SFI* */ void *p, void *p2, void *p3);
RC FC rfStarCkf( CULT* c, /*RFI* */ void *p, void *p2, void *p3);
RC FC xfStarCkf( CULT* c, /*RFI* */ void *p, void *p2, void *p3);
RC FC rpStarCkf( CULT* c, /*RI*  */ void *p, void *p2, void *p3);

// cncult2.cpp -- fcns called by top table
RC FC topStarItf( CULT* c, void *p, void *p2, void *p3);
RC FC topStarPrf( CULT* c, void *p, void *p2, void *p3); 	// also used in place of 'topClearPrf'
RC FC topStarPrf2( CULT* c, void *p, void *p2, void *p3);	// also called from topStarPrf, thus from clear. 7-92.
RC FC topClearItf( CULT* c, void *p, void *p2, void *p3);
RC FC topCkf( CULT* c, void *p, void *p2, void *p3);		// finish/check/set up for run
//RC FC topReCkf() -- see cncult.h

// cncult2.cpp... supporting fcns called from other cncult files
RC   FC ckSet( record *e, SI fn);
RC   FC ckOwnRefPt( BP toBase, record * fromRec, record **pp=NULL, RC *prc=NULL );

// cncult3.cpp -- surfaces etc, called from cncult2.cpp
void FC topPr();
RC   FC topSf1();
RC   FC topMsCon();
RC   FC topSg();
RC   FC topSh();
RC	 FC topZn2();
RC	 FC topZn3();
RC   FC topSf2();
RC   FC topSg2();

// cncult4.cpp...  report/export and holiday input setup/checking fcns, called by cncult2.cpp
RC   FC topPrfRep();		// make default reportfile, exportfile, and report rat entries, before input
RC   FC topRep();		// check rep___ members, set up header & footer 11-91
RC   FC topRf();		// check REPORTFILEs 11-91
RC   FC topXf();		// check EXPORTFILEs
RC   FC topCol( SI isExport);
RC   FC topRxp();		// check REPORTS and EXPORTS / build dvri info
RC   FC buildUnspoolInfo();	// build unspooling specifications
RC   FC clearImpf();		// Import stuff special clear function
RC   FC topImpf();		// check/process ImportFiles at end of input
RC   FC topPrfHday();		// make default HDAY (holiday) rat entries, before input
RC   FC topHday();		// check HDAY input info / build run info

// cncult5.cpp  terminals and air handlers
RC   FC topAh1();
RC   FC topTu();
RC   FC topAh2();

// cncult6.cpp  plants
RC   FC topHp1();		// heatplants 1: create run records. AH's & HPLOOPs & BOILERS must be done between topHp1 and 2.
RC   FC topCp1();		// coolplants 1: create run records. AH's & CHILLERs must be done between topCp1 and 2.
RC   FC topTp1();		// towerplants 1: create run records. topCp2 must be done between topTp1 and 2.
RC   FC topBlr();		// boilers
RC   FC topCh();		// chillers
RC   FC topHp2();		// heatplants 2: complete check after loads & boilers chained to records
RC   FC topCp2();		// coolplants 2: complete check after loads & chillers chained to records; do b4 towerplants
RC   FC topTp2();		// towerplants 2: do after topCp2, which chains cps to tps & generates default info

// end of cnculti.h
