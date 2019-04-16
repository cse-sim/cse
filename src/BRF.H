// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// brf.h  CSE binary results files structures
//
//         This file is included in the CSE simulation engine
//         AND in the application program reading the results files.
//
//	see also: brfw.h/cpp: writes binary results files (used in CSE)
//		  brfr.h/cpp: reads binary results files (used in E10)


#ifndef _BRF_H
#define _BRF_H

//===========================================================================
// File format version defines
//	   These are compiled into the file-writing and the file-reading programs,
//      and written in the file headers. If the version in the reading program
//      does not match that in the file, then an old version of one program
//	may be in use and the file may be incompatible.
//	   These should be incremented whenever the file structures or meaning
//	of any member is changed.
//---------------------------------------------------------------------------
#define RESFVER    8
	// 8 added workDayMask, 5-95, CSE .461.
	// 7 added dlwv2, wv count & names, 9-94.
	// 7 added Eu and Lt Peaks for zone and building, and ZnRes .monLitDmd/Eu
	// 6 added ResEgyMo, hrly & moav htg/clg/fan eu's & dlwv & packed HresDay,
	//   zone hrly & moav litDmd and litEu, runDateTime,
	//   dropped old version support, did deferred member rearrangements. CSE .431... 8-17-94... .
	// ver 6 code reads no prior versions.
	// 5 added ResZone .monMass, .monIz. CSE .414 1-2-94.
	// 4 packed floats: PACK12, PACK24, -Ram and -Pak versions of structs. 12-20-93 CSE .410.
	// 3 add Res.repHdrL, .repHdrR; ResZone.area, .spare. 12-15-93, CSE .410.
	// 2 .fileSize inserted in ResfHdr, .dowh added to struct Res 12-6-93, CSE .406.
	// 1 initial value, cannot be read by version 2 code.
#define HRESFVER   7
	// 7 added dlwv2, wv count & names, 9-94.
	// 6 htg/clg/fan eu's, ..., . CSE .431... 8-17-94... .  ver 6 code reads no prior versions.
	// 4 packed floats, 12-20-93, CSE .410.  5 never used.
	// 1 initial value. 2,3 never used.
//---------------------------------------------------------------------------
#define TWOWV		// undefined to remove all provisions for 2nd dl weather variable.
			// tested in brfr/w.h/cpp, app\cnguts.cpp, opk\progr.cpp, ... .
//---------------------------------------------------------------------------
#undef PACKRESEGY	// define to pack RESEGY structure, 8-17-94;
			// tentatively undone 8-19-94 when decided not to use that struct monthly.
			// when editing out permanently, search ResEgyRam and ResEgyPak & undo.
//---------------------------------------------------------------------------


//===========================================================================
// Other defines & types
//===========================================================================
#define NEXTEVEN(n) (((n) + 1) & ~1)	// round odd quantities to next even value
//---------------------------------------------------------------------------

//===========================================================================
//  Packing temperatures: temps are packed into single bytes as recevied
//===========================================================================
//--- macros to encode and decode temperatures for/from storage in one byte
#define tempBias (short(32))						// yields range -96 to +160
#define PACKTEMP(t) (char( max( -256, \
                           min( 255,  \
                                ((short)((t)+.5f)-tempBias) ) ) ) )	// .5f: round (t expected to be float)
#define UNPACKTEMP(t) (short(t) + tempBias)				// short() is intended to sign-extend
//---------------------------------------------------------------------------

//===========================================================================
//  Packing floats
//     floats are packed into 10 bits each + a float scale factor for each vector.
//     A vector of 12, 24, or .. floats is packed or unpacked at a time
//     At write, unpacked data is accumulated in ram and packed at write to disk.
//     At read, vectors are unpacked to caller's storage.
//===========================================================================
struct PACK12			// packed vector of 12 floats (monthly values)
{   float scale;
    char v[NEXTEVEN((5*12+3)/4)];		// 10 bit values / .scale. 15 bytes.
    void pack( float *data12);			// packer: brfw.cpp (CSE)
    BOO unpack( float *data12, BOO add);	// unpacker: brfr.cpp (E10)
};						// 19 bytes
//---------------------------------------------------------------------------
#ifdef needed	// 8-18-94 no uses
0 struct PACK16			// packed vector of 16 floats
0 {   float scale;
0     char v[NEXTEVEN((5*16+3)/4)];
0     void pack( float *data16);
0     BOO unpack( float *data16, BOO add);
0 };
#endif
//---------------------------------------------------------------------------
struct PACK24			// packed vector of 24 floats (hourly values)
{   float scale;
    char v[NEXTEVEN((5*24+3)/4)];		// 30 bytes
    void pack( float *data24);
    BOO unpack( float *data24, BOO add);
};						// 34 bytes
//---------------------------------------------------------------------------

//===========================================================================
// ID substructure for making sure have correct file and/or memory block in binary results files
//===========================================================================
struct ResfID			// used in ResfHdr, HresfHdr, HresfMonHdr.
{
    char cneName[8]; 		// name of program that wrote file.
    				// on read, first 3 characters must be "CSE".
    char cneVer[8]; 		// version of writing program, as text.
    // if changing, add cneVariant[12] for progVariant here? 3-94.
    short ver; 			// file format version: RESFVER or HRESFVER, above.
    				// if differs on read, file is from old program.
    				// reading program may adapt per .ver in file, or may reject file.
    char fNamExt[12];		// name.ext of file

    void init( short ver, const char *pNam);		// fcn in brfw.cpp, used when writing

    BOO check( HWND hPar, BOO silent, short minVer, short _ver,
    			      const char *fNam, const char *pNam);	// fcn in brfr.cpp, used when reading
};		// struct ResfID. 30 bytes.
//---------------------------------------------------------------------------

//===========================================================================
// Hourly results substructures for hourly results file, and monthly average data in basic results file.
//===========================================================================
// hourly non-zone results for one day
struct HresDayRam		// form in RAM during data accumulation for write
{   char ver;
    char isSet;
    char tOut[24]; 			// byte-packed hourly outdoor temperatures (temps have no unpacked ram form)
    float dlwv[24];					// one weather variable re daylighting
  #ifdef TWOWV//9-94
    float dlwv2[24];					// second weather variable re daylighting
  #endif
    float euHtg[24], euClg[24], euFan[24], euX[24];	// selected whole-bldg hourly energy uses (X is extra/spare)
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct HresDayPak		// form in disk file and in read buffers
{   char ver;
    char isSet;
    char tOut[24]; 		// byte-packed hourly outdoor temperatures (temps have no unpacked ram form). 24 bytes.
    PACK24 dlwv;			// one weather variable re daylighting. 34 bytes.
  #ifdef TWOWV//9-94
    PACK24 dlwv2;			// second weather variable re daylighting. 34 bytes.
  #endif
    PACK24 euHtg, euClg, euFan, euX;	// selected energy uses (X is spare) in packed vectors of 24 floats. 136 bytes.
    // packing code: loop brfw.cpp:packHresMon() calls HresDay::pack().
    void pack( HresDayRam *src);	// packing fcn, brfw.cpp
};					// 230 bytes
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef HresDayRam ResMoavRam;	// monthly average hourly non-zone results for one month - same
typedef HresDayPak ResMoavPak;	// monthly average hourly non-zone results for one month - same
//---------------------------------------------------------------------------
// hourly results for one zone, one day
struct HresZoneDayRam 			// in RAM during preparation for write
{   char ver;
    char isSet;
    char temp[24];  				// hourly zone temperature
    float slr[24], ign[24], cool[24], heat[24];	// hourly heat flows to/from zone (not at system; not energy use)
    float litDmd[24], litEu[24];		// lighting egy demand (b4 daylighting) and energy use (after DL). added RESFVER 6.
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct HresZoneDayPak 			// packed: on disk and in read buffers
{   char ver;
    char isSet;
    char temp[24];
    PACK24 slr, ign, cool, heat;	// packed vectors of 24 floats for hourly zone heat flows. 136 bytes.
    PACK24 litDmd, litEu;		// lighting egy demand (b4 daylighting) and energy use (after DL). added RESFVER 6. 68 by.
    void pack( HresZoneDayRam *src);	// packing fcn, brfw.cpp
};					// 230 bytes.
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
typedef HresZoneDayRam ResMoavZoneRam;	// monthly average hourly results for one zone for one month - same
typedef HresZoneDayPak ResMoavZonePak;	// monthly average hourly results for one zone for one month - same
//---------------------------------------------------------------------------
// hourly non-zone results for month
struct HresMonRam
{   short ver;
    HresDayRam day[31];		// .ver, .isSet, .tOut[24], .dlwv[24], (dlwv2[23],) .euHtg[24], .euClg[24], .euFan[24], .euX[24].
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct HresMonPak
{   short ver;
    HresDayPak day[31];		// .ver, .isSet, .tOut, .dlwv, (.dlwv2,) .euHtg, .euClg, .euFan, .euX
};				// 2 + 230*31 bytes = 7132, 12-94.
// packing code for HresMonPak: loop brfw.cpp:packHresMon() calls HresDayPak::pack().
//---------------------------------------------------------------------------
// hourly results for one month, one zone. 31 days always allocated
struct HresMonZoneRam		// in ram before write in CSE
{   short ver;
    HresZoneDayRam day[31]; 	// .ver, .isSet, float .temp[24], slr[24], ign[24], cool[24], heat[24]
};
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct HresMonZonePak		// packed: on disk, in ram after read in E10
{   short ver;
    HresZoneDayPak day[31]; 	// .ver, .isSet, PACK24 .temp, slr, ign, cool, heat
};				// 7132 bytes
// packing code for HresMonZonePak: see packHresfMon in brfw.cpp.
//---------------------------------------------------------------------------

//***************************************************************************
// Hourly binary results file structures
//***************************************************************************
/*	Form of hourly binary results file.
 *	Actual varies according to number of zones and months; access using mbr fcns that use info in header.
 *	File is in memory as 13 objects: header (HresfHdr), 12 months (HresfMon(nzones)).
 *
 * struct Hresf			// form of whole file
 * {   HresfHdr  hdr;			// .moOff(), .moSize()  are used by app that load's a month's block at a time
 *     HresfMon  mon[12];
 * };
 * struct HresfMon		// form of each mon[]
 * {   HresfMonHdr     monHdr;		// .nonzoneRam/Pak(), .zoneRam/Pak(zi)
 *     HresMonRam/Pak     nonzone;	// HresDayRam/Pak.day[mdi]
 *     HresMonZoneRam/Pak zone[nZones];	// HresZoneRam/Pak.day[mdi]
 * };
 */
//---------------------------------------------------------------------------
struct HresfHdr 		// header for entire hourly binary results file
{
//identification-verification information
    ResfID id;			// name of program that wrote file, file format version, name of file, etc.
		    // 30 bytes
//info on file layout
    DWORD offMon0;		// file offset to first month data (initially, sizeof this header)
    DWORD offPerMon;		// additional file offset per zone
		    // 30 + 8 = 38 bytes
//info on file contents for this run
    short firstMon;		// first month in file, 0-11: ZERO-BASED!
    short nMon;			// number of months in file
		    // 38 + 4 = 42 bytes
    short spare[8];
    short nZones;		// number of zones in yet another place -- used by packer. 12-19-93.
    BOO isPakd;		// non-0 if packed form (for redundant consistency checks)
    short spare2;
                   // 52 + 16 + 2 + 2 + 2 = 64 bytes

    void init( short ver, const char *pNam, short firstMon, short nMon, short nZones, BOO pak);	// in brfw.cpp

//functions to access parts of file

    DWORD moSize( short mi)		// return # bytes for month, or 0 if not presnt
    {  if (!isGoodMon(mi))
          return 0L;
       return offPerMon;
    }

    DWORD moOff( short mi) 		// file position of month mi 0-11, or -1L if not present
    {  if (!isGoodMon(mi))				// check month carefully, but allowing end year wrap
          return -1L;
       return offMon0 + runMon(mi)*offPerMon;   	// months are stored in run order, unwrapped.
    }

    BOO isGoodMon(short mi)		// check for valid 0-based month in run, allowing for running into next year
    {  if (mi < 0 || mi >= 12)   			// must be 0-11
          return FALSE;
       if (firstMon <= mi && mi < firstMon + nMon)	// can be from first month to last (or to 11 if less - checked above)
          return TRUE;
       return (mi < firstMon + nMon - 12);		// can be from 0 (checked above) to # months run wrapped past end year
    }

    short runMon(short mi)		// return month in run sequence: subtract start, unwrap any end-year wrap
    {
       return mi >= firstMon ? mi - firstMon : mi - firstMon + 12;
    }

    short monDiff( short mi2, short mi1)	// run months difference of two month numbers
    {
       return (mi2 >= mi1  ?  mi2 - mi1  :  mi2 + 12 - mi1);	// if second mi less, interval goes into next hear (dec-->jan)
    }
};		// struct HresfHdr. 64 bytes.
//---------------------------------------------------------------------------
struct HresfMonHdr 		// header for one month's info in hourly results file
{
//identification/verification info
    ResfID id;			// name of program that wrote file, file format version, name of file, etc.
    short month;		// month represented, 0-11, also for verification
		// 32 bytes
//info on contents of this block
    short nZones; 		// number of zones
    short firstDaySet;		// first day for which data is stored (may not be [0] if run starts mid-month)
    short lastDaySet;		// last day ...   NB # days is: last - first PLUS 1.

//info on file layout
    short nDaysAlloc;  		// number of days for which zone info is allocated (currently always 31; could vary)
    DWORD offNonZone;		// offset from start of this header non-zone-specific hourly results (eg sizeof this header)
    DWORD offZone0; 		// offset from start of this header of first zone ( .. plus sizeof(HresMonRam/Pak), or as changed)
    DWORD offPerZone; 		// additional offset per zone (sizeof(HresMonZone), or as changed)
          	// 32 + 20 = 52 bytes
    short spare[5]; 	// 52 + 10 = 62 bytes
    BOO isPakd;		// non-0 if packed form (for redundant consistency checks)

    void init( short ver, const char *pNam, short mon, short nZones, BOO pak); 	// in brfw.cpp

//fcns to access parts of month info, assuming all month info in one contiguous ram block

    // return pointer to non-zone-specific info for month (outdoor temps, some bldg egy use)
    HresMonRam * nonzoneRam()
    {  if (!this)  return NULL;
       if (!offNonZone)  return NULL;
       if (isPakd)  return NULL;			// wrong fcn for this header
       return (HresMonRam *)((char *)this + (WORD)offNonZone);
    }
    HresMonPak * nonzonePak()
    {  if (!this)  return NULL;
       if (!offNonZone)  return NULL;
       if (!isPakd)  return NULL;			// wrong fcn for this header
       return (HresMonPak *)((char *)this + (WORD)offNonZone);
    }

    // return pointer to zone info for 0-based zone index
    HresMonZoneRam* zoneRam(short zi) 			// in ram form, during data accumulation
    {  if (!this)  return NULL;
       if (!offZone0 || !offPerZone)  return NULL;
       if (zi < 0 || zi >= nZones)  return NULL;
       if (isPakd)  return NULL;			// if wrong function called for this header (offsets wrong)
       return (HresMonZoneRam *)((char *)this + (WORD)offZone0 + zi*(WORD)offPerZone);	// assumes whole HresfMon is in ram
    }
    HresMonZonePak* zonePak(short zi) 			// packed disk form, at write, on disk, at read
    {  if (!this)  return NULL;				// (same except for types; offsets differ)
       if (!offZone0 || !offPerZone)  return NULL;
       if (zi < 0 || zi >= nZones)  return NULL;
       if (!isPakd)  return NULL;			// if wrong function called for this header (offsets wrong)
       return (HresMonZonePak *)((char *)this + (WORD)offZone0 + zi*(WORD)offPerZone);	// assumes whole HresfMon is in ram
    }

    BOO isGoodDay(short mdi)  {  return (mdi >= 0 && mdi < nDaysAlloc); }
};			// struct HresfMonHdr. 64 bytes
//---------------------------------------------------------------------------
// layout of hourly results for all zones for one month. comments. substructs actually accessed with .hdr mbr fcns.
//struct HresfMonRam		// before-write ram form
//{   HresfMonHdr hdr; 			// each month has header to facilitate checking when holding one month in RAM.
//    HresMonRam nonzone;		// non-zone info: .day[mdi]. tOut[hi], .dlwv[hi] .euClg[hi] etc
//    HresMonZoneRam zone[2]; 		// info for zones (actual # zones varies): .day[mdi].heat[hi] etc
//};
//struct HresfMonPak		// packed form: on disk and after read
//{   HresfMonHdr hdr; 			// each month has header to facilitate checking when holding one month in RAM.
//    HresMonPak nonzone;		// non-zone info: .day[mdi]. tOut[hi], .PACK24 dlwv .PACK24 euClg etc
//    HresMonZonePak zone[2]; 		// info for zones (actual # zones varies): .day[mdi]. PACK24 heat etc
//};
//---------------------------------------------------------------------------
//--- space to allocate to hold the above for any # zones:
#define SizeHresfMonRam(zones)  ( NEXTEVEN(sizeof(HresfMonHdr)) + NEXTEVEN(sizeof(HresMonRam)) + (zones)*NEXTEVEN(sizeof(HresMonZoneRam)) )
#define SizeHresfMonPak(zones)  ( NEXTEVEN(sizeof(HresfMonHdr)) + NEXTEVEN(sizeof(HresMonPak)) + (zones)*NEXTEVEN(sizeof(HresMonZonePak)) )
//---------------------------------------------------------------------------
//--- block packing functions, brfw.cpp
void packHresfHdr( HresfHdr *src, HresfHdr *dest);
void packHresfMon( HresfMonHdr *src, HresfMonHdr *dest);
//---------------------------------------------------------------------------

//***************************************************************************
// Basic binary results file structures (for all results but hourly results)
//***************************************************************************
/*	Form of basic binary results file.
 *	Actually varies per number of energy sources and zones, but 12 months are always allocated.
 *      Access parts of file with mbr fcns in header.
 *      File is handled in memory as one object.
 * struct Resf
 * {   ResfHdr         hdr;   		// .zoneRam/Pak(zi), .egy(ei), .egyMo(ei), .moav(mi).	128 bytes
 *     Res             nozone;		// non-zone stuff: monthly outdoor temps.  	14 bytes
 *     ResZoneRam/Pak  zone[2];		// zone annual and monthly results.	2*238 = 476 bytes
 *     ResEgy          egy[2];		// year energy use for each meter
 *     ResEgyMoRam/Pak egyMo[2];	// monthly energy use for each meter 8-19-94
 *     resfMoav        moav[12];  	// as shown just below
 * };
 * struct ResfMoav			// 12 of these
 * {   ResfMoavHdr         hdr;		// .nonzoneRam/Pak(), .zoneRam/Pak(zi)
 *     ResMoavRam/Pak      nonzone;	// .tOut[hi], .dlwv[hi], eu stuff
 *     ResMoavZoneRam/Pak  zone[2];	// .heat[hi] etc
 * }; */
//---------------------------------------------------------------------------
struct PeakStr	// substructure to hold energy flow peak value and its date & time. for zone, building, energy demand.
{   float q;  		// the peak energy flow, Btu. May be negative for cooling.
    WORD shoy;		/* julian date/hour at peak occurred: ((jDay 1-365) * 24 + hour 0-23) * 4.
			   2 low bits reserved for poss future subhour. 1-based jDay reserves value 0 for unset. */

    void init() { q = 0.f;  shoy = 0; }
    void set( float _q, WORD _shoy ) { q=_q; shoy = _shoy; }
    //void set( float _q, SI jday, SI hi, SI sh=0) { q=_q;  shoy = sh + 4*(hi+24*jday); }	if needed
    //void set( float _q, SI _mi, SI _mdi, SI _hi);  	implement with conversion if found to be needed
};			// struct PeakStr. 6 bytes.
//---------------------------------------------------------------------------
struct Res		// non-zone annual and monthly results information
{   char ver;			// version of structure
    char isSet;
    char repHdrL[60];		// left top report header. E10 to use for "project name". 12-93 RESFVER 3, moved at 6.
    char repHdrR[60];		// right top report header. E10 to use for "variant". 12-93 RESFVER 3, moved at 6.
    char runDateTime[40];	// run start date & time string eg "Sun 31-Aug-94  12:59:59 pm" (char [27] req'd)
    char monTOut[12];		// monthly average outdoor temperatures, packed (not used by ET, 8-94).
    char dowh[12][31];		/* in file, weekday 1-7 (Sun=1), 8 if Holiday, 0 if unset. Callers see less 1. RESFVER 2.
				   poss future: 9=autoSize heating design day,10=cool.*/
 #ifdef TWOWV
    SI nDlwv;			// # daylighting weather variables with non-0 data in file: 0, 1, or 2. NB 2 always alloc'd.
    char dlwvName[2][32];	// their names -- may depend on weather file type
 #else
    SI nDlwv;			// number of daylighting weather variables with non-0 data in file: 0 or 1. NB 1 always alloc'd.
    char dlwvName[32];		// their names -- may depend on weather file type
 #endif
    // above values will be 0 for months not included in run (determine with getFirstMon, getNMons)
};		// struct res
//---------------------------------------------------------------------------
// information that occurs once per run or monthly, per zone, in binary results file
struct ResZoneRam
{   char ver;			// version of structure
    char isSet;
    char  zoneName[16];
    float area;		// added RESFVER 3, moved 6.
    float spare; 	// added RESFVER 3, moved 6.
    // Peaks: each have max energy flow and its date & time (substruct just above)
     PeakStr  pkCoolHf;		// maximum cooling heat flow for zone during year
     PeakStr  pkHeatHf;		// heating heat flow peak likewise
     PeakStr  pkCoolEuSpare;   	// poss future maximum cooling energy use for zone. eu not now metered by zone.
     PeakStr  pkHeatEuSpare;   	// poss future heating heat egy use peak likewise
     PeakStr  pkLitSvg;		// peak lighting savings (Dmd - Eu)
     PeakStr  pkLitDmd;		// peak lighting demand
    char  monTemp[12];						// monthly average indoor temps, packed (not used by ET, 8-94)
    float monSlr[12], monIgn[12], monCool[12], monHeat[12];	// monthly totals: solar gain, internal gain, heating, cooling
    float monInfil[12], monCond[12];				// monthly totals: net infiltration, net conduction.
    float monMass[12], monIz[12];   	// monthly totals: net mass heat flow and interzone heat flow. Added RESFVER 5.
    float monLitDmd[12], monLitEu[12];	// monthly totals: lighting demand & energy use, re daylighting. Added RESFVER 7.
    // above values will be 0 for months not included in run (determine with getFirstMon/getNMons or isGoodMonth)
};			// struct ResZoneRam
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct ResZonePak
{   char ver;			// version of structure
    char isSet;
    char  zoneName[16];
    float area;		// added RESFVER 3, moved 6.
    float spare;	// added RESFVER 3, moved 6.
    // Peaks: each have max energy flow and its date & time (substruct just above)
     PeakStr  pkCoolHf;		// maximum cooling heat flow for zone during year
     PeakStr  pkHeatHf;		// heating heat flow peak likewise
     PeakStr  pkCoolEuSpare;   	// poss future maximum cooling energy use for zone. eu not now metered by zone.
     PeakStr  pkHeatEuSpare;   	// poss future heating heat egy use peak likewise
     PeakStr  pkLitSvg;		// peak lighting savings (Dmd - Eu)
     PeakStr  pkLitDmd;		// peak lighting demand
    char  monTemp[12];					// monthly average indoor temps, packed
    PACK12 monSlr, monIgn, monCool, monHeat;		// packed monthly totals: solar gain, internal gain, heating, cooling
    PACK12 monInfil, monCond;				// packed monthly totals: net infiltration, net conduction.
    PACK12 monMass, monIz;		// monthly totals: net mass heat flow and interzone heat flow. Added RESFVER 5.
    PACK12 monLitDmd, monLitEu;   	// monthly totals: lighting demand & energy use, re daylighting. Added RESFVER 7.
    // above values will be 0 for months not included in run (determine with getFirstMon/getNMons or isGoodMonth)
};			// struct ResZonePak
// packing code: see packResf in brfw.cpp
//---------------------------------------------------------------------------
#ifdef PACKRESEGY
0 // once-per-run energy use info for one energy source (one meter)
0 struct ResEgyRam
0 {   char ver;			// version of structure: 2, 8-94.
0     char isSet;			// ... CAUTION! brfr.cpp code assumes non-packed version 1 struct is same as version 2 ResEgyRam.
0     char egyName[16];
0     float cost; 		// cost
0     float dmdCost;
0     // next 14 floats, thru dmd.q, match order of CSE MTR_IVL_SUB record
0     float totUse; 		// total use
0      // following array is packed in disk file:
0     float endUse[NENDUSES]; 	// use broken down by end use. subscript: see enum endUses, above.
0     PeakStr dmd;			// peak demand .q, .shoy
0 };		// struct ResEgyRam. 84 bytes
0 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
0 struct ResEgyPak
0 {   char ver;			// version of structure: 2. Version 1 is not packed -- same as ResEgyRam.
0     char isSet;
0     char egyName[16];
0     float cost; 		// cost
0     float dmdCost;
0     // next 14 floats, thru dmd.q, match order of CSE MTR_IVL_SUB record
0     float totUse; 		// total use
0     PACK12 endUsePak; 		// use broken down by end use, 12 floats packed into 19 bytes.
0     PeakStr dmd;			// peak demand .q, .shoy
0     void pack( ResEgyRam *src);			// brfw.cpp
0     BOO unpack( ResEgyRam *dest);		// brfr.cpp
0 };			// struct ResEgyPak. 55 bytes
0 #if (NENDUSES-12)
0   #error NENDUSES no longer 12... May need to change ResEgyPak and its functions.
0 #endif
0 // - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
0 //struct ResEgyOld1		// pre 8-94, pre-RESFH 6 version: not packed; same as version 2 ResEgyRam.
0 //{   char ver;			// version of structure: 1
0 //    char isSet;
0 //    char  egyName[16];
0 //    float cost; 		// cost
0 //    float dmdCost;
0 //    // next 14 floats, thru dmd.q, match order of CSE MTR_IVL_SUB record
0 //    float totUse; 		// total use
0 //    float endUse[NENDUSES]; 	// use broken down by end use. subscript: see enum endUses, above.
0 //    PeakStr dmd;			// peak demand .q, .shoy
0 //};		// struct ResEgyOld1.
0 //---------------------------------------------------------------------------
#else // !PACKRESEGY
// once-per-run energy use info for one energy source (one meter)
struct ResEgy
{   char ver;			// version of structure (1)
    char isSet;			// TRUE if data has been stored in structure
    char  egyName[16];	// name of meter (energy source)
    float cost; 		// cost
    float dmdCost;
    // next floats, thru dmd.q, match order of CSE MTR_IVL_SUB record
    float totUse; 				// total use
    float endUse[ NENDUSES];	// use broken down by end use
								// subscript: reactivate enum endUses
    PeakStr dmd;   		// peak demand .q, .shoy
};		// struct ResEgy. 84 bytes
#endif
//---------------------------------------------------------------------------
// monthly energy use info for one energy source (one meter), added RESFVER 6
struct ResEgyMoRam	// ram form, used while building file
{   char ver;			// version of structure (1)
    char isSet;			// TRUE if data has been stored in structure for at least one month
    //char  egyName[16];	use name in ResEgy for same ei
    // next 14 float[12]'ss, thru dmdQ, match order of float's in CSE MTR_IVL_SUB record.
    float cost[12]; 		// monthly energy cost
    float dmdCost[12];		// monthly demand cost
    float totUse[12]; 		// monthly total use
    float endUse[NENDUSES][12];	// monthly use for each end use. see enum endUses, above.
    float dmdQ[12];		// monthly peak demand power
    WORD dmdShoy[12];		// monthly time of peak demand (subhour of year)
//or? x PeakStr dmd[12];  	// monthly peak demand .q, .shoy
};		// struct ResEgyMoRam
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
struct ResEgyMoPak	// packed form used in binary results file on disk
{   char ver;			// version of structure (1)
    char isSet;			// TRUE if data has been stored in structure for at least one month
    //char  egyName[16];	use meter name in ResEgy
    PACK12 cost; 		// monthly energy cost (packed vector of 12 floats)
    PACK12 dmdCost;		// monthly demand cost
    PACK12 totUse; 		// monthly total use
    PACK12 endUse[NENDUSES];	// monthly use for each end use. see enum endUses, above.
    PACK12 dmdQ;		// monthly peak demand power
    WORD dmdShoy[12];		// monthly time of peak demand (subhour of year)
//or? x PeakStr dmd[12];   	// monthly peak demand .q, .shoy (not packed)
    void pack( ResEgyMoRam *src);	// packing function, brfw.cpp
    //BOO unpack( ResEgyMoRam *dest);	// all-at-once unpacking function, brfr.cpp -- if needed
};		// struct ResEgyMoPak
//---------------------------------------------------------------------------
struct ResfMoavHdr	// header for EACH MONTH'S monthly averages in basic binary results file
{			// contains mbr fcns and info they need to address non-zone and per-zone subobjects

    // id-verification info: none. same file and normally the same memory block as basic binary results file.
    short ver;		// well, none but this.

// info about file layout. off's 0 for (possible future) absent data.
    short nZones;	// number of zones, duplicated from ResfHdr
    DWORD offNonzone;	// offset from 'this' to once per month info:  sizeof(ResfMoavHdr)		or as changed
    DWORD offZone0;	// offset from 'this' to start of zone info:   offNonzone + sizeof(ResMoavRam/Pak)
    WORD  offPerZone;	// additional offset for each zone:            sizeof(ResMoavZoneRam or Pak)
		// 12 bytes
    short spare[3];	// 18 bytes
    BOO isPakd;

    void FC init( short _nZones, BOO pak);	// in brfw.cpp. called for 12 months from ResfHdr.init.

// member functions to access subobjects

    ResMoavRam * nonzoneRam()
    {  if (!this)  return NULL;
       if (!offNonzone)  return NULL;
       if (isPakd)  return NULL;					// called wrong fcn for this file
       return (ResMoavRam *)((char *)this + (WORD)offNonzone);
    }
    ResMoavPak * nonzonePak()
    {  if (!this)  return NULL;
       if (!offNonzone)  return NULL;
       if (!isPakd)  return NULL;					// called wrong fcn for this file
       return (ResMoavPak *)((char *)this + (WORD)offNonzone);
    }

    ResMoavZoneRam * zoneRam(short zi)
    {  if (!this)  return NULL;
       if (!offZone0 || !offPerZone)  return NULL;
       if (zi < 0 || zi >= nZones)  return NULL;
       if (isPakd)  return NULL;					// called wrong fcn for this file
       return (ResMoavZoneRam *)((char *)this + (WORD)offZone0 + zi*offPerZone);
    }
    ResMoavZonePak * zonePak(short zi)			// (same as above except types, check, and values in header)
    {  if (!this)  return NULL;
       if (!offZone0 || !offPerZone)  return NULL;
       if (zi < 0 || zi >= nZones)  return NULL;
       if (!isPakd)  return NULL;					// called wrong fcn for this file
       return (ResMoavZonePak *)((char *)this + (WORD)offZone0 + zi*offPerZone);
    }
};		// struct ResfMoavHdr		20 bytes
//---------------------------------------------------------------------------
// struct resMoavRam/Pak	same as HresDayRam/Pak, typedef'd above
//---------------------------------------------------------------------------
// struct resMoavZoneRam/Pak	same as HresZoneDayRam/Pak, typedef'd above
//---------------------------------------------------------------------------
//struct ResfMoav		example for two zones, actual layout done by code
//{   ResfMoavHdr         hdr;			// .nonzoneRam/Pak(), .zoneRam/Pak(zi)
//    ResMoavRam/Pak      nonzone;		// .tOut[hi], .dlwv[hi], .euClg[hi] etc
//    ResMoavZoneRam/Pak  zone[2];		// .heat[hi] etc
//};
//---------------------------------------------------------------------------
#define SizeResfMoavRam(zones)  (NEXTEVEN(sizeof(ResfMoavHdr)) + NEXTEVEN(sizeof(ResMoavRam)) + (zones)*NEXTEVEN(sizeof(ResMoavZoneRam)) )
#define SizeResfMoavPak(zones)  (NEXTEVEN(sizeof(ResfMoavHdr)) + NEXTEVEN(sizeof(ResMoavPak)) + (zones)*NEXTEVEN(sizeof(ResMoavZonePak)) )
//---------------------------------------------------------------------------
struct ResfHdr
{
// identification/verification info
    ResfID id;			// name of program that wrote file, file format version, name of file, etc.
		// 30 bytes
// once-per-run info
    short startJDay, endJDay;			// start and end Julian days (1-365) of run
    short jan1DoW;				// day of week of Jan 1, 1=Sunday.
		// 36 bytes to here
// Peaks for entire building: each have max energy flow and its date & time (substruct just above)
    PeakStr  pkCoolHf;	// maximum cooling heat flow to all zones during year
    PeakStr  pkHeatHf;	// heating heat flow peak likewise
    PeakStr  pkCoolEu;	// maximum cooling energy use for all zones combined
    PeakStr  pkHeatEu;	// heating heat egy use peak likewise
    PeakStr  pkLitSvg;	// peak lighting savings (Dmd - Eu)
    PeakStr  pkLitDmd;	// peak lighting demand for all zones combined
		// + 36 = 72 bytes
// numbers of indicated things in file
    short firstMon;  short nMon;	// first month (0-11) and # months (determined during write)
    short nZones;  			// number of zones
    short nEgys;  			// number of energy sources - # ResEgy's and # ResEgyMoRam/Pak's.h.
    short spareN1, spareN2;
		// +12 = 84
// file layout info, to facilitate reading old files when file layout has later been changed. Off's 0 if data not present.
    DWORD fileSize;		// offset to end of file
    DWORD resOff;		// offset to nonZone stuff		sizeof(ResfHdr)    or as changed if file rearranged
    WORD  resSize;		// size of nonZone stuff		sizeof(Res)
    DWORD zoneOff;	 	// to zone info for 1st zone:			resOff + resSize
    WORD  perZone;		// add for each zone:				sizeof(ResZoneRam/Pak)
    DWORD egyOff; 		// to year energy use/cost info for 1st egy:  	zoneOff + nZones * perZone
    WORD  perEgy;		// add for each egy source:     		sizeof(ResEgy)
    // egyMo inserted here in file 8-94 but off/per added belo for compat.
    DWORD moavOff;		// to monthly average info for 1st mon:		egyMoOff + nEgys * perEgyMo !
    WORD  perMoav;		// add for each month           		SizeResfMoavRam/Pak(zones)
    DWORD egyMoOff;		// offset to month energy info for 1st egy:	egyOff + nEgys * perEgy  !
    WORD  perEgyMo;		// add for each egy source			sizeof(ResEgyMoRam/Pak)
	    	// +34 = 118
    short spare[13];
    WORD workDayMask;		/* bits set for "work days": Sun=1,Mon=2,Tu=4,W=8,Th=16,F=32,Sat=64,Holiday=128,
				    autoSizing heat design day=256, cooling dsn days=512 (but no dsn days data in brf yet,6-95).
				   Encoding MATCHES: CSE TOP::workDayMask, OPK progr.h/cpp:ProGrSub::dayShowMask, brfr.h default.
				   Added at RESFVER 8, CSE .461, 5-95. */
    BOO isPakd;
		// + 30 = 148 bytes

    void init( short ver, const char *pNam,
                      short firstMon, short nMon, short _nZones, short _nEgys, BOO pak);	// fcn in brfw.cpp

 //member functions to access various items assuming initialized header
			// arguments are ZERO-BASED even where corressponding quantities are stored 1-based in members!
    DWORD getFileSize() { return fileSize; }

    Res * nonzone()
    {   if (!this)  return NULL;
        if (!resOff || !resSize)  return NULL;
        return (Res *)((char *)this + (WORD)resOff);
    }

    ResZoneRam * zoneRam(short zi)
    {   if (!this)  return NULL;
        if (!zoneOff || !perZone)  return NULL;
        if (zi < 0 || zi >= nZones)  return NULL;
        if (isPakd)  return NULL;					// called wrong fcn for this buffer
        return (ResZoneRam *)((char *)this + (WORD)zoneOff + zi*perZone);
    }
    ResZonePak * zonePak(short zi)
    {   if (!this)  return NULL;
        if (!zoneOff || !perZone)  return NULL;
        if (zi < 0 || zi >= nZones)  return NULL;
        if (!isPakd)  return NULL;					// called wrong fcn for this buffer
        return (ResZonePak *)((char *)this + (WORD)zoneOff + zi*perZone);
    }

#ifdef PACKRESEGY
0     ResEgyRam * egyRam(short ei)
0     {   if (!this)  return NULL;
0 	if (!egyOff || !perEgy)  return NULL;
0 	if (ei < 0 || ei >= nEgys)   return NULL;
0 	if (isPakd)  return NULL;					// called wrong fcn for this buffer
0 	return (ResEgyRam *)((char *)this + (WORD)egyOff  + ei*perEgy);
0     }
0     ResEgyPak * egyPak(short ei)
0     {   if (!this)  return NULL;
0 	if (!egyOff || !perEgy)  return NULL;
0 	if (ei < 0 || ei >= nEgys)   return NULL;
0 	if (!isPakd)  return NULL;					// called wrong fcn for this buffer
0 	return (ResEgyPak *)((char *)this + (WORD)egyOff  + ei*perEgy);
0     }
#else // PACKRESEGY undef
   ResEgy * egy(short ei)
   {   if (!this)  return NULL;
       if (!egyOff || !perEgy)  return NULL;
       if (ei < 0 || ei >= nEgys)   return NULL;
       return (ResEgy *)((char *)this + (WORD)egyOff  + ei*perEgy);
   }
#endif

    ResEgyMoRam * egyMoRam(short ei)
    {   if (!this)  return NULL;
        if (!egyMoOff || !perEgyMo)  return NULL;
        if (ei < 0 || ei >= nEgys)   return NULL;
        if (isPakd)  return NULL;					// called wrong fcn for this buffer
        return (ResEgyMoRam *)((char *)this + (WORD)egyMoOff  + ei*perEgyMo);
    }
    ResEgyMoPak * egyMoPak(short ei)
    {   if (!this)  return NULL;
        if (!egyMoOff || !perEgyMo)  return NULL;
        if (ei < 0 || ei >= nEgys)   return NULL;
        if (!isPakd)  return NULL;					// called wrong fcn for this buffer
        return (ResEgyMoPak *)((char *)this + (WORD)egyMoOff  + ei*perEgyMo);
    }

    ResfMoavHdr * moav(short mi)
    {   if (!this)  return NULL;
        if (!moavOff || !perMoav)  return NULL;
        if (!isGoodMon(mi))   return NULL;		// assumes firstMon/nMon already updated when adding a month
        return (ResfMoavHdr *)((char *)this + (WORD)moavOff + mi*perMoav);
    }

    ResfMoavHdr * moavx(short mi) 		// access monthly average header for unrun month, for internal init use
    {   if (!this)  return NULL;
        if (mi < 0 || mi >= 12)   return NULL;
        return (ResfMoavHdr *)((char *)this + (WORD)moavOff + mi*perMoav);
    }

    BOO isGoodMon(short mi)		// check for valid 0-based month in run, allowing for running into next year
    {  if (mi < 0 || mi >= 12)   			// must be 0-11
          return FALSE;
       if (firstMon <= mi && mi < firstMon + nMon)	// can be from first month to last (or to 11 if less - checked above)
          return TRUE;
       return (mi < firstMon + nMon - 12);		// can be from 0 (checked above) to # months run wrapped past end year
    }

    short runMon(short mi)		// return month in run sequence: subtract start, unwrap any end-year wrap
    {
       return mi >= firstMon ? mi - firstMon : mi - firstMon + 12;
    }

    short monDiff( short mi2, short mi1)	// run months difference of two month numbers
    {
       return (mi2 >= mi1  ?  mi2 - mi1  :  mi2 + 12 - mi1);	// if second mi less, interval goes into next hear (dec-->jan)
    }
};		// struct ResfHdr.  128 bytes.
//---------------------------------------------------------------------------
//-- size of basic binary results file
 #define SizeResfRam(zones,egys)  ( NEXTEVEN(sizeof(ResfHdr)) + NEXTEVEN(sizeof(Res)) + (zones) * NEXTEVEN(sizeof(ResZoneRam)) \
 				   + (egys) * NEXTEVEN(sizeof(ResEgy)) + (egys) * NEXTEVEN(sizeof(ResEgyMoRam)) \
 				   + 12 * NEXTEVEN(SizeResfMoavRam(zones)) )
 #define SizeResfPak(zones,egys)  ( NEXTEVEN(sizeof(ResfHdr)) + NEXTEVEN(sizeof(Res)) + (zones) * NEXTEVEN(sizeof(ResZonePak)) \
				   + (egys) * NEXTEVEN(sizeof(ResEgy)) + (egys) * NEXTEVEN(sizeof(ResEgyMoPak)) \
				   + 12 * NEXTEVEN(SizeResfMoavPak(zones)) )
//---------------------------------------------------------------------------
//-- packing function for basic binary results file
void packResf( ResfHdr *src, ResfHdr *dest);	// in brfw.cpp
//---------------------------------------------------------------------------


//===========================================================================
// struct to hold handles for binary results information in fcn arguments and CSE return
//   This structure is defined in cnewin.h.
//===========================================================================
//struct BrHans		// but official copy may be is in cnewin.h
//{
//    HGLOBAL hBrs;  		// 0 or global memory handle for Building Binary Results file
//    HGLOBAL hBrhHdr;  		//   .. building Hourly binary Results file Header block
//    HGLOBAL hBrhMon[12]; 	//   .. .. month blocks
//
//    BrHans() { memset( this, 0, sizeof(*this)); }
//    ~BrHans() { clean(); }
//    void clean();
//};			// struct BrHans
//---------------------------------------------------------------------------

#endif	// ifndef _BRF_H at start file

// end of brf.h
