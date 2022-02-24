// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// irats.h  declarations of input record basAncs in cncult.cpp


// include first: ancrec.h for anc<T> template class;
//                rccn.h for record structures used in template class instances: TOPRAT, MAT, ZNI, etc.


//--------------------------- INPUT VARIABLES cncult.cpp ---------------------------

// Topi: Top input record and its basAnc: record contains top-level / once-instance-only input members.
// set via cnTopCult, accessible as Topi.member or TopiR.p->member, copied to run rat TopR/Top (cnguts.cpp) */
extern anc<TOPRAT> TopiR;	// basAnc for Topi.  ancrec.h template class type.
extern TOPRAT Topi;    		// the single static Top input record. rccn.h typ

extern anc<INVERSE> IvB;

// other input records basAncs, cncult.cpp, ref'd only in other cncult_.cpp files
extern anc<GT>  GtiB;			// glazeTypes input records, 12-94
extern anc<FOUNDATION> FndiB;  		// foundation input records
extern anc<FNDBLOCK> FbiB;  		// foundation component input records
extern anc<CON> ConiB;  		// constructions input records
extern anc<LR>  LriB;    		// layers input records
extern anc<MAT> MatiB;			// materials input records

extern anc<ZNI> ZiB;			// zones input records

extern anc<SFI> SfiB;  			// surfaces input records
extern anc<SFI> DriB;  			// doors input records
extern anc<SFI> GziB; 			// windows input records

extern anc<WSHADRAT> ShiB;		// shades (fins, ovh's) input records
extern anc<SGI> SgiB; 			// solar gain distributions input records

extern anc<PRI> PriB;  			// perimeters input records

extern anc<IZXRAT> IzxiB;		// interzone transfer input records


extern anc<TU> TuiB;   			// terminals input
extern anc<AH> AhiB;    		// air handlers
extern anc<HEATPLANT> HpiB;		// HEATPLANTs input info
extern anc<BOILER> BlriB;   	// BOILERs input info
extern anc<COOLPLANT> CpiB;   	// COOLPLANTs input info
extern anc<CHILLER> ChiB;   	// CHILLER input info
extern anc<TOWERPLANT> TpiB;   	// TOWERPLANT input info

extern anc<MTR> MtriB;			// meters input records
extern anc<DHWMTR> WMtriB;		// DHW meter input records
extern anc<AFMTR> AfMtriB;		// air flow meter input records
extern anc<LOADMTR> LdMtriB;	// Load meter input records

extern anc<GAIN> GniB;			// (zone) gains input records
extern anc<RSYS> RSiB;			// residential HVAC system
extern anc<DUCTSEG> DsiB;		// duct segment input records

extern anc<DHWSYS> WSiB;		// DHW system input records
extern anc<DHWHEATER> WHiB;		// DHW heater input records
extern anc<DHWHEATER> WLHiB;	// DHW loop heater input records
extern anc<DHWHEATREC> WRiB;	// DHW heat rec input records
extern anc<DHWTANK> WTiB;		// DHW tank input records
extern anc<DHWPUMP> WPiB;		// DHW pump input records
extern anc<DHWLOOP> WLiB;		// DHW loop input records
extern anc<DHWLOOPSEG> WGiB;	// DHW loop segement input records
extern anc<DHWLOOPBRANCH> WBiB;	// DHW loop branch input records
extern anc<DHWLOOPPUMP> WLPiB;	// DHW loop pump input records
extern anc<DHWDAYUSE> WDUiB;	// DHW daily use input records
extern anc<DHWUSE> WUiB;		// DHW use input records

extern anc<DHWSOLARSYS> SWHiB;  // DHW solar system input records
extern anc<DHWSOLARCOLLECTOR> SCiB;  // DHW solar collector input records

extern anc<PVARRAY> PViB;		// PV array input records
extern anc<BATTERY> BTiB;		// Battery input records

extern anc<SHADEX> SXiB;		// SHADEX (external shading) input records

extern anc<RFI> RfiB; 			// REPORTFILE input info
extern anc<RFI> XfiB; 			// EXPORTFILE input info
extern anc<RI> RiB;    			// REPORT input info
extern anc<RI> XiB;   			// EXPORT input info.  cnPreInit changes .what from "report" to "export".
extern anc<COL> RcoliB;			// REPORTCOL input info for UDT reports
extern anc<COL> XcoliB;			// EXPORTCOL input info for UDT exports
extern anc<IMPF> ImpfiB;		// IMPORTFILE input info 2-94

extern anc<HDAY> HdayiB;		// HDAY (holidays) input info
extern anc<DESCOND> DCiB;		// DESCOND (design conditions) input info

// end of irats.h
