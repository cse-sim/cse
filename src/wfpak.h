// Copyright (c) 1997-2019 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// wfpak.h -- definitions for weather file reading and decoding

/* The Weather File Story ...

Weather files are efficiently packed binary files with hold up to a full year
of hourly data to drive simulation programs.

These formats are in use for BSGS applications --

   BSGS (or PC or TMP)
   		A "temporary" format put together for the original CALPAS3
		port to MS-DOS systems (in 1984?).  Each record contains only
		the 6 data items required by CALPAS-style simulation.
		A DEMO variant of this format also exists in which the data
		order in the hourly records is scrambled.  This allows free
		distribution of educational/demonstration programs which
		can read only altered (demo) weather data and cannot be used
		professionally with real data.  As of 1990, wfread is
		set up to decode both the standard and demo variants, but a
		demo-only version of wfread could be easily resurrected.

   ET1		Energy-10 version 1, October 1994. Many fields added to header.
		Data for monthly design days added. Hourly data is same as BSGS,
		plus global radiation and cloud cover.

   BSG   	The original format used on Data General systems, with 10
		data items per record.  As of 1990, this format is probably
		defunct, given the complete conversion to PCs.  Record format
		is documented only in obscure notes in Chip's possession.
		Support for this format eliminated from CSE code 10-27-91.

All formats begin with a header which identifies the file.  The header and
record layouts are described below for BSGS format; the header
format for ET1 is given as a C structure after these comments. Data records
follow the header; where used (in ET1) monthly design day data records
follow the regular daily data.

=== BSGS (aka PC or TMP) format header layout ===

A BSGS header is 576 bytes long (4 daily records of 24 * 6 bytes) and is divided
into a number of fields.  All data in the header is ASCII (for human
readability).  Header offsets are found in wfpak:wfOpen and wfpak3:wfBuildHdr
and MUST BE MAINTAINED BY HAND if layout changes, 8-17-90.

Item		Offset beg/end	Description
--------------	--------------	---------------------------------------------
File type	  0   2		Code identifying file format.  All BSGS PC
				files are "TMP"
Pack date/time	  3  12		Year, month, day, hour, minute (2 chars each)
				when file was packed (that is, written in
				packed binary format).
Packer ID	 13  17		Code identifying program which wrote file.
				Offset 13 is always "P", offsets 14 - 17 are
 				used for serial numbering of CALPAS3 files
				(ignored by TK/CR).  A 255 (0xff) at offset
				14 indicates file is DEMO format (see above).
File name	 18  48		For checking.  31 char length derived from
				DG filename length limit.
File origin	 49  52		Code indicating source of file.  All BSGS
				say "BSG"? CECrv2 files say "BSGS"--rob.
Location	 53  82		Text indicating city/state of observation,
				eg "CEC Zone 12 Rev 2".
Location ID	 83  93		ID for location, generally the WBAN number
				of the observation site.  Larger field was
				left to accomodate possible WMO codes etc.
Weather year	 94  95		If real year, value xx indicates data from
				19xx.  "-1" for fabricated years (eg TMYs)
Latitude	 96 101		Site latitude, deg N.
Longitude	102 108		Site longitude, deg W.
Time zone	109 114		Site time zone, hours W of Greenwich.  Note
				this can be non-integral.
Elevation	115 119		Elevation, ft above sea level.  0 on all
				current files?
First day	120 122		Day of year (1-365) of first data record
			        (allows short files for test data etc)
Last day	123 125		Day of year (1-365) of last data record.
(Unused)	126 143
Comment		144 287		Any documenting text


=== BSGS (aka PC or TMP) format data record layout ===

Although hourly data records are independent, they are read in daily groups
of 24 (see wfread).  Each hourly record consists of 3 16 bit words
(6 bytes per hour), organized as follows:

Item		    Word/bits   Description
--------------	----------  ----------------------------------------------
Dry bulb        1   0 -  9	High 10 bits contain DB + 100 to nearest F.
Wet bulb dep	   10 - 15	Low 6 bits contain DB - WB, so wet bulb
				            can be calculated.
Beam irrad      2   0 - 11	High 12 bits contain beam irradiance (solar
                            beam rad incident on tracking surface, Btu).
Wind dir           12 - 15	Low 4 bits contain wind direction in 16-ants,
				            WD = 22.5 * file value, 0 = north, 90 east.
Diff irrad	    3   0 -  8	High 9 bits contain diffuse irradiance (sky
                            radiation incident on horiz surface, Btu).
Wind speed          9 - 15  Low 7 bits contain wind speed, mph.

-- Additional values in ET1 file ---
Glob irrad      4   0 - 11	High 12 bits contain global irradiance incident
                            on a horizontal surface, Btu/ft2.
Cld Cover       4  12 - 15	Low 4 bits contain cloud cover in tenths, 0-10,
                            or 15 for missing data. Opaque cloud cover
                            if available on source, else total cloud cover.

Total size of a full year BSGS file is (365 + 4) * 24 * 6 = 53,136 bytes.


*/	// weather file story comments


//------------------------ FILE FORMAT STRUCTURES ---------------------------

//---- ET1 (Energy-10 version 1) header structure ----

struct ET1Hdr
{
    /* The char fields are NOT terminated with null (0) characters as is usual in C.
       If data uses whole field there is no separator; short fields are expected to be space-filled. */

    char formatCode[3];		// "ET1" for Energy-10 version 1 file (or "TMP" for BSGS)
    char packDateTime[10];	// Year, month, day, hour, minute (2 chars each) when file was written in binary format
    char packerID[20];		// name and version, as text, of program writing file, eg "WFPACK v2.1"
    char fileName[31];		// file name, for checking, eg "Denver.CO". 31 char length historical.
    char fileOrigin[4];		// file origin -- who massaged & packed data, eg "BSGS".
    //68
    char sourceDate[4];  	// date of source file, eg "1977"
    char fileSource[36];	// reference for data source, eg "US National Wthr Service".
    char location1[20];		// location part 1 -- city
    char location2[20];		// location part 2 -- state or country
    char locationID[16];	// location ID, usually WBAN number, eg "WBAN986744".
    char wthrYear[4];		// real year, eg "1977", or "-1  " for fabricated year
    char isLeap[1];		// 'Y' if a leap year file containing Feb 29 data -- possible future use
    //68+101=169
    // lattitude, longitude, and times zone may have + or - sign, decimal point, and as many digits after point as fit field.
    char latitude[6];		// site latitude, deg N, eg "38.52"
    char longitude[7];		// site longitude, deg W, eg "121.50"
    char timeZone[6];		// site time zone, hours W of Greenwich, eg "8.00" or "-11".
    char elevation[5];		// site elevation, feet above sea level, eg "26" or "12790".
    char firstDay[3];		// day of year (1-365) of first data record in file
    char lastDay[3];		// day of year (1-365(or 366 if full leap hear)) of last data record in file
    char firstDdm[2];		// month (1-12) of first monthly design day in file
    char lastDdm[2];		// month (1-12) of last monthly design day in file
    //169+34=203
    char winMOE[3];		// winter median of extremes (deg F)
    char win99TDb[3];		// winter 99% design temp (deg F)
    char win97TDb[3];		// winter 97.5% design temp (deg F)
    char sum1TDb[3];		// summer 1% design temp (deg F)
    char sum1TWb[3];		// summer 1% design coincident WB (deg F)
    char sum2TDb[3];		// summer 2.5% design temp (deg F)
    char sum2TWb[3];		// summer 2.5% design coincident WB (deg F)
    char sum5TDb[3];		// summer 5% design temp (deg F)
    char sum5TWb[3];		// summer 5% design coincident WB (deg F)
    char range[2];  		// mean daily temperature range (deg F)
    char sumMonHi[2];		// month of hottest design day, 1-12
    char unused[22];
    //203+31+22=256
    char unused2[44];
    char clearness[12][4];	// monthly clearness numbers, multipliers, eg 1.05, for design day solar
    char turbidity[12][4];	// monthly average atmospheric turbidity, for daylight calcs
    char atmois[12][4]; 	// monthly aveage atmospheric moisture, for daylight calcs
    char formatCode2[4];	// "ET1!" to check for corrupted header or struct length inadvertently changed
    //256+44+144+4=448
    char comment[128];
    //448+128=576.
};			// struct ET1Hdr

/*---- The rest of the ET1 file ----
   The header is followed by the hourly data records (number of days is
   lastDay - firstDay + 1); these are followed by monthly design day records.
   Record format is binary with bit fields, as described above; there is no
   struct declaration; records are unpacked by code to WFDATA struct (below). */


//-------------------------------- OPTIONS ----------------------------------
#define DOSOLARONREAD

/*-------------------------------- DEFINES --------------------------------*/

// solar radiation components
enum SLRCMP { scDIFF, scBEAM, scCOUNT };

//--- file type dependent definitions
// BSGS (PC, TMP) weather file
#define WFBYTEHR_BSGS  6 			// data bytes per hour
#define WFHDRSZ_BSGS (4*24*WFBYTEHR_BSGS)  	// header same length as 4 BSGS daily rcds (4*24*6=576)

// ET1 weather file
#define WFBYTEHR_ET1  8			// data types per hour
#define WFHDRSZ_ET1 (4*24*WFBYTEHR_BSGS)  	// header same length as 4 BSGS (not ET!) daily rcds (4*24*6=576)

#define WFBYTEHR (WFBYTEHR_ET1)			// LARGEST hourly size for all file types, for buffer sizing
#define WFHDRSZ (WFHDRSZ_BSGS)			// LARGEST header size for all file types, for buffer sizing


//-- Defines for option bits for erOp's in wfpak function calls. EROPn defined in cnglob.h
const int WF_DBGTRACE = EROP2;  // wf_Read: emit debugging tracing messages
const int WF_USELEAP = EROP3;	// wf_Read: use Feb 29 if in file, else Feb 29 if present (not, 10-94) is skipped over.
const int WF_DSNDAY = EROP4;	// wf_Read & callees: read hour from design day info (may or may not be from wthr file)
const int WF_FORCEREAD = EROP5;	// wf_Read: always read from file (don't use cached data)
								//         ignored if WF_DSNDAY
const int WF_DSTFIX = EROP6;	// wf_Read: use jDay+1 value for taDbPvMax if iHr=23 (see code)
const int WF_SAVESLRGEOM = EROP7;	// wfRead: do NOT overwrite solar geometry values
									//  (no effect if WF_DSNDAY or WF_FORCEREAD)

// public functions
float CalcSkyTemp( int skyModelLW, int iHr, float taDb, float taDp, float cldCvr,
	float presAtm, float irHoriz);

// weather-related globals
extern anc<WFILE> WfileR;		// basAnc for the one weather file info record 1-94. Runtime only - no input basAnc.
extern WFILE Wfile;				// the one static WFILE record with overall info and file buffer for 1 day's data
extern anc<WFDATA> WthrR;		// basAnc for hour's weather data record. Runtime only - no corress input basAnc.
extern WFDATA Wthr;				// the one static WFDATA record containing unpacked & adjusted data for hour 1-94
extern anc<WFDATA> WthrNxHrR;	// basAnc for next hour's weather data record ("weatherNextHour")
extern WFDATA WthrNxHr;			// static record for next hour's unpacked & adjusted data for cgwthr.cpp read-ahead

extern anc<WFSTATSDAY> WfStatsDay;	// Weather statistics by day

extern anc<DESCOND> DcR;		// design conditions


// end of wfpak.h
