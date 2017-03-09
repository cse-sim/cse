// Copyright (c) 1997-2016 The CSE Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license
// that can be found in the LICENSE file.

// csevrsn.h: specify current build CSE version number
//    Used by cse.cpp and cse.rc

// convert #defined value to string literal
//   #define NAME BOB
//   MAKE_LIT( NAME) -> "BOB"
#define MAKE_LIT2(s) #s
#define MAKE_LIT(s) MAKE_LIT2(s)

// version # for current build
// change ONLY here
#define CSEVRSN_MAJOR 0
#define CSEVRSN_MINOR 826

// version # as quoted text ("x.xxx")
#define CSEVRSN_TEXT MAKE_LIT(CSEVRSN_MAJOR##.##CSEVRSN_MINOR)

// ONLY comments below here

/* History:
   0.826: ...
   0.825: fix excessive fSunlit; add tp_TDVfName, committed 3-9-2017
   0.824: shading rotation, committed 3-7-2017
   0.823: Initial version with PVARRAY shading calcs, committed 3-1-2017
   0.822: SHADEX and PVARRAY vertices input, committed 2-23-2017
   0.821: BATTERY now only reinitializes at begin of sim if bt_initSOE is set
   0.820: BATTERY now re-initializes at begin of sim (after warmup, etc.)
   0.819: HPWH setpoint-below-water-temp bug fix, 1-22-2017
   0.818: add Battery model; committed 1-16-2017
   0.817: HPWH wsTUse bug fix; enable HPWH CSV dump; committed 1-17-2017
   0.816: support rpCpl=-1; misc. doc-related cleanup; committed 11-30-2016
   0.815: Initial open-source version w/ C++ ASHWAT; committed 9-26-2016
   0.814: ASHWAT C++ conversion test version
   0.813: HPHW 1.3.0 (Ecotope cleanup), non-HPWH whUse bug fix; committed 8-3-2016
   0.812: Accum-during-warmup: fixes use of prior hour results in expressions;
          HPWH 1.2.9 (reworked generic per Ecotope); DHWSYS load share; committed 7-21-2016
   0.811: HPWH generic LTC=45, corrected UA; committed 5-25-2016
   0.810: HPWH generic post-modification method; 2 variants committed 5-24-2016
          (Not merged to master.)
   0.809: HPWH 1.2.8rev2 (generic rev); default whASHPResUse=7.22; committed 5-23-2016
   0.808: default whASHPResUse = 5.4; committed 5-20-2016
   0.807: add'l HPWH backup resistance heating; committed 5-20-2016
   0.806: default whASHPResUse=7.22; central dist loss passed to HPWH; committed 5-19-2016
   0.805: HPWH 1.2.6; whASHPResUse added, committed 5-18-2016
   0.804: test version w/ HPWH Generic, committed 5-17-2016
   0.803: test version w/ HPWH UEF2generic, commited 5-14-2016
   0.802: central DHWSYS; committed 5-12-2016

   0.801: test version with HPWH generic models; partial central DHWSYS; sent 5-11-2016
   0.800: path search; suffixed output files; HPWH 1.2.5; committed 5-7-2016
   0.799: RFI cleanup; fix DHWMTR accum bugs; committed 4-26-2016
   0.798: HPWH 1.2.4; minor cleanups; committed 4-14-2016
   0.797: GAIN/DHWSYS linkage; HPWH 1.2.3; committed 4-7-2016
   0.796: DHWSYS wsTSetpoint and mixdown; incomplete GAIN/DHWSYS linkage; committed 4-6-2016
   0.795: HPWH EF resistance heater model; HPHW mix-down; committed 4-1-2016
   0.794: HPWH 1.1.1 x/0 fix; committed 3-24-2016
   0.793: tp_HPWHVersion; EF=1 ideal DHWHEATER; HPWH 1.1.0; committed 3-23-2016
   0.792: DHWMTR; DHWUSE wuHotF; committed 3-22-2016
   0.791: HPWH improvements, tick-level DHW use fixes; committed 3-15-2016
   0.790: DHWUSE recovery efficiency, string expression crash fix; committed 3-4-2016
   0.789: PVWatts internal calcs; HPWH initial test version; committed 2-25-2016
   0.788: fix potential bad solar values from weather file, committed 2-3-2016
   0.787: PVWatts corrections, committed 1-29-2016
   0.786: test version with PVWatts, committed 1-27-2016
   0.785: fix crash when >1000 surfaces;
          fix crash when dhw load = 0 (due to e.g. SSF >= 1)
		  fix crash when input file name contains '%'
		  internal (inactive) DHW progress
		  committed 1-11-2016
   0.784: DHWPUMP energy calc bug fix; committed 11-11-2015
   0.783: DHWHEATER whElecPar bug fix; wsSSF hourly; committed 11-10-2015
   0.782: DHWTank TSA consistent with App B; misc cleanup; committed 11-9-2015
   0.781: DHWLOOPSEG loss bug fixes; committed 11-2-2015
   0.780: added LDEF limits, DHWLOOPPUMP, DHWHEATER.wh_parElec; committed 10-28-2015
   0.779: added DHWSYS.wsMult, committeed 10-26-2015
   0.778: DHW meter bug fixed, committed 10-26-2015
   0.777: DHW multi-family recirc, initial model implemetnation, committed 10-24-2015
   0.776: DHW multi-family recirc test version, committed 10-22-2015
   0.775: DHWTANK wtVol/wtInsulR; remove LDEF for large storage elec.  Committed 9-10-2015
   0.774: use LDEF for only large storage electric.  Committed 9-8-2015
   0.773: added C_DHWMODELCH_T24DHW; use LDEF for Large Storage. Committed 8-31-2015
   0.772: wtUA=0 OK, 32 bit vrpak, sent 8-23-2015
   0.771: whHPAF, better DHW input checking, screen quiet mode, sent 8-21-2015
   0.770: back to "DHW", DHW LDEF calc, sent 8-18-2015
   0.769: "DHW" in meter report reverted to "SHW", sent 8-14-2015
   0.768: XP comptibility, sent 8-12-2015
   0.767: added DHWSYS, DHWHEATER, DHWTANK, DHWPUMP. sent 8-4-2015
   0.766: completed conversion of end use "SHW" to "DHW", sent 7-8-2015
   0.765: initial DHW version, sent 7-6-2015
   0.764: autosize bug fix, sent 5-8-2015
   0.763: autosize rework, sent 5-8-2015
   0.762: VS2013, sent 4-29-2015
   0.761: added tp_soilDiff, sent 3-4-2015
   0.760: test version with ANTolAbs and ANTolRel
		  labeled and sent, 1-15-2015
   0.759: test version with abbreviated run bug fix and AWtrigXXX
		  sent 1-11-2015
   0.758: test version with abbreviated run scheme (skipDaysStart, skipDaysStep)
		  not labeled, test version only 1-10-2015
   0.757: exman bug fix re more than 0x377 records
		  Ground error checking revised re walls with sfDepthBG=0
		  Sent 1-6-2014
   0.756: XSURF.xs_tLrB[] added, sent 12-11-2014
   0.755: eliminate airnet and masslayer size limits, sent 11-20-2014
   0.754: below grade layer adjustment bug fix, BG RFilm correction, sent 11-19-2014
   0.753: allow znFloorZ < 0, sent 11-18-2014
   0.752: wall/floor below grade model.  Sent 11-17-2014
   0.751: fix rs_IsElecHeat() so hydronic heat pump energy counted as electric
          add top.skymodelLW alternative sky temp models
		  suppress airnet excessive pressure message during early autosize
		  add sfDepthBG to SURFACE (input only, no implementation)
		  sent 11-7-2014
   0.750: correct incorrect taDbAvgYr value for EPW files, sent 10-24-2014
   0.749: rsCapAuxH=0 now OK (previously required >0), revised ASHP autosizing
          sent 10-19-2014
   0.748: bug fix in yacam.cpp, now accepts ".0", sent 10-1-2014
   0.747: EPW support, sent 10-1-2014
   0.746: eliminate zn_floorZ check for AirNet horiz openings, sent 7-14-2014
   0.745: disable buffering on stdout; added iz_amfNom, 6-20-2014
   0.744: changed spool file name allowing parallel runs in same dir, 6-9-2014
   0.743: bug fix re electricity accounting in ASHPHydronic, sent 4-21-2014
   0.742: rsType=ASHPHydronic; infil/vent code cleanup, sent 4-18-2014
   0.741: fix wd_taDbAvg calc bug (wfpak.cpp), sent 4-10-2014
   0.740: use wd_taDbAvg (not wd_taDbAvg01) in rs_OAVAirFlow(), sent 4-9-2014
   0.739: range checks on RSYS fan power, allow 0 IZXFER vent area,
		  E() etc. move to cnglob.h, sent 4-8-2014
   0.738: taDbAvg added, taDbAvg01 fixed, sent 4-4-2014
   0.737: OAVREV2 (revised NightBreeze model); added rsOAVVFMinF, sent 3-27-2014
   0.736: auszTol default = .005; delete old ausz code, sent 2-7-2014
   0.735: bug fix: incorrect autosize, changes made to cnaus.cpp.  Sent 1-31-2014
   0.734: bug fix: door/window area same as parent now does not crash
          bug fix: re-use of report/export file detected.  Sent 11-13-2013
   0.733: OAV setpoint now tDbOut + rsOAVTdiff (was tSupply + rsOAVTdiff)
		  Message for inadvertent OAV vent heating
	      bug fix: OAV relief vent area adjusted subhourly
		  bug fix: correct taDbPvPk for 1st hour of day during daylight savings
		  sent 10-10-2013
   0.732: rsCapAuxH autosize = heating load at design temp
          sfExHCMult now applies to C_EXCNDCH_SPECT (allowing almost setting surface temp)
		  added sb_tSrfls = last step surface temp for probes (no *e).  Sent 10-2-2013
   0.731: fixed bugs associated with RSYS multizone, added version cse.rc w/ version resource
          sent 9-24-2013
   0.730: OAV with AirNet relief air, ZNRES.qvMech, sent 8-20-2013
   0.729: OAV initial version (no relief izxfer)
          duct inside convective resistance curvature fix, 8-16-2013
   0.728: ASHP revised defrost model test, duct inside convective resistance,
          multi-zone air flow bug fix, indendent AHSP heating/cooling 8-9-2013
   0.727: ASHP "match HSPF", rotation fix, sent 7-15-2013
   0.726: ASHP modifications, 6-10-2013
   0.725: improved HERV model, sent 5-17-2013
   0.724: test version with HERV, sent 5-14-2013
   0.723: eliminated unused IZXFERs, DSE disables DUCTSEG (re baseboards), sent 5-10-2013
   0.722: improved UA reporting on ZDD, sent 5-3-2013
   0.721: MASSLAYER merge bug fix, sent 5-1-2013
   0.720: non-airnet infil based on zone air density (was outdoor), sent 4-17-2013
   0.719: wnSMSO/wnSMSC rework, sent 4-12-2013
   0.718: IZXFER error detection bug fix, sent 4-9-2013
   0.717: ASHP backup rework, rsASHPLockOutT; sent 4-4-2013
   0.716: Heating fan power fixes, sent 3-22-2013
   0.715: DSE distrib efficiency; RSYS parasitics, sent 3-20-2013
   0.714: improved ZDD report, sent 2-28-2013
   0.713: added cnrecs.def *e to rs_CapH so report probe works, sent 2-20-2013
   0.712: sent 2-15-2013
	  interzone quick surface no longer cause energy balance errors
	  DUCTSEG conduction calcs moved to ds_FinalizeSh(),
      fix isCallBackAbort initialization bug
	  fix et1 weather file bugs re ground temp
	  added add'l RSYS rsType choices
	  ZDD bug fix (not disabled by autosize)
	  initial work on better U-factor reporting for ZDD
   0.711: ASHP autosize v1, sent 1-14-2013
   0.710: ASHP autosize trial, sent 1-13-2013
   0.709: rs_tdbOut bug fix, sent 1-12-2013
   0.708: bug fixes + initial ASHP aux heat, sent 1-10-2013
   0.707: autosizing improvements, sent 12-31-2012
   0.706: autosizing improvements, sent 12-28-2012
   0.705: autosizing improvements, sent 12-27-2012
   0.704: autosizing progress (not complete), multiple DLL cse() calls, CSEProgInfo(), sent 12-20-2012
   0.703: initial autosizing version, sent 12-13-2012
   0.702: ASHP elec meter bug fix; revised AC model; "last-step" unmet definition, sent 12-10-2012
   0.701: initial version of ASHP model, buffer overrun bug fixes, sent 12-6-2012
   0.700: various test versions, never officially sent
   0.699: fill of some missing weather data items (re CVRH).  Sent 9-28-2012
   0.698: RSYS::rs_Cooling corrections; moisture bal corrections; znHIRatio.  Sent 9-27-2012
   0.697: ZNRES.pz0, ZNRES.qlIz; MoistureBalCR improvements.  Sent 9-26-2012
   0.696: DLL version, RSYS performance map, sent 9-21-2012
   0.695: allow izHD<0; incomplete AUSZ_RSYS; incomplete restructure for DLL.  Sent 8-30-2012
   0.694: RSYS humidity limit, sent 8-22-2012
   0.693: multi-zone RSYS, sent 6-19-2012
   0.692: fixed "Ext" omission for MTR report/export, sent 6-6-2012
   0.691: additional end uses, sent 6-5-2012
   0.690: fix heating fuel reported to RSYS meter; changed qlHvac calc; added rs_outLat; sent 5-17-2012
   0.689: fix af_AccumMoist(), RSYS ducts optional, add zn_hcAirX.  Sent 5-15-2012
   0.688: moisture model, latent cooling, zn_relHum now alters air radiant "air", sent 5-11-2012
   0.687: air radiant "area" adjusted hourly, more AIRSTATE internals, sent 5-5-2012
   0.686: fix duct conduction split, added rsRhIn, AIRSTATEs in DUCTSEG, sent 5-2-2012
   0.685: capacity model for cooling HVAC + bug fixes, sent 4-27-2012, bugfix sent 4-30-2012
   0.684: initial version of equip model (enhanced RSYS), sent 4-25-2012
   0.683: duct model bug fixes, corrected zn_airX, sent 4-19-2012
   0.682: duct model bug fixes, sent 4-16-2012
   0.681: duct model refinements, sent 4-13-2012 (bug-fix sent 4-13-2012, relabelled 4-16-2012)
   0.680: duct model test version 1, sent 4-10-2012
   0.679: fixed stairwell AeHoriz; partial ducts, sent 4-3-2012
   0.678: znRes airX, sent 3-28-2012
   0.677: multizone vent cleanup, vent airnet only when needed, sent 3-27-2012
   0.676: multizone vent control, default convection model = UNIFIED,
          eliminate ZNR.qAirNet, sent 3-23-2012
   0.675: activate IZXFER for convective/radiant, added _ONEWAY, sent 3-12-2012
   0.674: Added sfExRConGrnd, corrected ground model, sent 3-8-2012
   0.673: Derivation of weather values (tGrnd, lagged temps, etc.); sent 3-6-2012
   0.672: SURFACE sfExCnd=GROUND and associated couplings, sent 3-1-2012
   0.671: added znWindFLkg, airnet wind pressure by zone, sent 2-29-2012
   0.670: final? UNIFIED convection (elevation adjustment on all, zn_eaveZ, ) sent 2-27-2012
   0.669: improved UNIFIED convection (terrain, shielding), sent 2-23-2012
   0.668: name length limit increased to 63; preliminary UNIFIED convection, sent 2-17-2012
   0.667: bug fix, energy bal errors for interior massless layers, sent 2-3-2012
   0.666: FDX (using enhanced heat cap) now standard for FD, sent 2-2-2012
   0.665: "enhanced heat capacity" method for FDX light layers, sent 1-27-2012
   0.664: suppress energy balance checks for light mass, sent 1-17-2012
   0.663: sfLThkF=0 means no splitting, sent 1-14-2012
   0.662: automatic layer splitting, sent 1-13-2012
   0.661: convection and UZM.dll bug fixes, sent 1-3-2012
   0.660: generalized convection model, UZX zone model, sent 1-2-2012
   0.659: detect unsupported sfModel per znModel, sent 12-8-2011
   0.658: no-prefix on probe names, 12-6-2011 (not sent)
   0.657: exterior radiant model fix, sent 10-25-2011
   0.656: internal CZM code, sent 10-18-2011
   0.655: solar gain distribution rework, sent 10-10-2011
   0.654: fixes and fiddles, sent 8-11-2011
   0.653: enabled fan power polynomial, sent 3-8-2011
   0.652: CZM capacity limit, including duct efficiency, sent 3-7-2011
   0.651: UZM / airnet bug fix, sent 3-5-2011
   0.650: fan elec bug fix; added flow airnet, sent 2-25-2011
   0.649: IZXFER fan power, sent 2-17-2011
   0.648: comfort model, sent 1-21-2011
   0.647: strncpy0 source copy overrun fixed, sent 1-13-2011
   0.646: rebuild of 0.645 with .pdb/.map, sent 1-13-2011
   0.645: 32 bit dmSize(), sent 1-12-2011
   0.644: ASHWAT single glazing bug fix, sent 1-11-2011
   0.643: Airnet fan variability=subhour; energy bal tols=.0001; sent 1-8-2011
   0.642: duct leakage mass flow returned by UZM; sent 1-7-2011
   0.641: ASHWAT controlled shades; sent 1-6-2011
   0.640: bug fixes; ASHWAT "call when needed"; ASHWAT.dll release build; sent 1-5-2011
   0.639: Fixes: 1) ASHWAT; 2) internal gain bal error; sent 1-4-2011
   0.638: ASHWAT with shades, sent 1-4-2011
   0.637: Frame debug, ASHWAT ZDD, sent 1-2-2011
   0.636: Initial version of ASHWAT frame model, sent 12-31-2010
   0.635: ASHWAT beam-diffuse fix, sent 12-30-2010
   0.634: Yet more energy balance debug, sent 12-27-2010
   0.633: Energy balance debug, sent 12-23-2010
   0.632: ASHWAT partial debug, sent 12-16-2010
   0.631: initial ASHWAT version, sent 12-14-2010
   0.630: qAirNet averaging re stability, sent 11-4-2010
   0.629: external convective coeff model characteristic length
	      now based on eaveZ/4 per Niles 11-2-2010 eMail
		  sent 11-3-2010
   0.628: XSRATs created for all MSRATs (no external effect?)
		  sb_qSrf set for MSRATs re probes; #undef EXTH_FIXED
		  sent 11-2-2010
   0.627: same as 0.626 except #undef EXTH_FIXED, sent 11-1-2010
   0.626: quick surfaces; hard-coded exterior hc and hr (for testing)
          windspeedMin default = 0.5.  Sent 11-1-2010
   0.625: conv/radiant progress; added znRes.tRad, corrected qMass.  Sent 10-19-2010
   0.624: conv/radiant progress; initial "Plan A" implementation for windows, sent 10-18-2010
   0.623: partially complete conv/radiant model (not running)
          MASSBC.surftemp subhour probeable; sent 10-12-2010
   0.622: dref memory clobber fixed, partially complete conv/rad internals, sent 10-1-2010
   0.621: znQmxCRated, znQMxHRated, UZM vwind correction, sent 9-28-2010
   0.620: eliminate vent heating at CZM call, 9-22-2010
   0.619: vent controls ("double AirNet"), sent 9-20-2010
   0.618: AirNet additions: izCpr, izExp, Top.windSpeedMin, sent 9-17-2010
   0.617: UZM interzone surface linkage, sent 9-16-2010
   0.616: UZM bug fix, znQMxH and znQMxC, sent 9-13-2010
   0.615: UZM improvements, sent 9-10-2010
   0.614: inital UZM version, sent 9-9-2010
   0.613: bug fix with CSW weather reader, 9-2-2010
   0.612: prelimary version with CSW weather reader, 9-2-2010
   0.611: hourly xFan, xFanElecPwr, sent 8-30-2010
   0.610: zn setpoints, CZM linkage to zone results, sent 8-27-2010
   0.609: AirNetFan, zone exhaust, sent 8-26-2010
   0.608: MASSMODEL / MASSTYPE combination, misc cleanup, sent 8-23-2010
   0.607: mass debug, sent to BAW, 8-9-2010
   0.606: mass rework, initial version sent to BAW, 8-6-2010
   0.605: test version sent to BAW, 7-12-2010
   0.604: 5-?-2010
   0.603: change export file default extension to .csv, 5-25-2010
   0.602: fix SGDIST crash when no surfaces, 5-25-2010
   0.601: airnet bug fixes, 5-21-2010
   0.600: resurrected for CSE work (3-2010); initial airnet test version 4-30-10
//------
	0.489:
	0.488: Work on autosizing heat convergence when cooling req't forced oversize fan. BUG0090. 6-17-97.
	0.487: addressed BUG0088 (increased tolerances for Dx coil "Inconsistency #4" and "..#5" messages) and
	       BUG0089 ausz noncvg (made antRatTs resize tu vf larger when ah ts on wrong side of setpoint). 6-10-97.
	0.486: fixed .485 bugs. Delivered 5-27-97.
	0.485: resumed work 5-97.
	       -n suppresses warnings on screen (reorganized err/ppMsgI);
	       Fan cfm vs coil warnings +- 25 not 20%;
	       AHSIZE and TUSIZE reports generated even after ausz w/o main sim;
	       condensation modelled in zone and at DX coil entry. Delivered 5-21-97.
	0.484: 0 (autoSized) coil captRat handled; better zone superSaturation messages; 11-1-96.
	0.483: resumed work 10-23-96: ausz bugs: change in cncoil.cpp to fix AH autosize convergence problem;
	       "100%%" in messages changed (crashed win32);
	       fix ah-tu noncvg when ausz'd ah/tu size is 0; delivered 10-24-96.
	0.482: just made & delivered all, 3-28-96.
	0.481: fix bug in .479 change. 2-26-96.
	0.480: CNEW32 change only (fix dropping 1st arg on cmd line). 2-24-96.
	0.479: output package change only: allow bin res rename. 2-22-96.
	0.478: our own icons in win version. 2-9-96.
	0.477: compiled & runs under W95. opk rank/keep changes. 1-22-95.
	0.476: compiler version update: BC4.0 --> BC4.5, 1-6-96. CNEW32 under W95 fixed, 1-18-95. Not delivered.
	0.475: function definition bug fixed in cuparse.cpp, 11-25-95.
	0.474: radiant internal gains 11-7-95.
	0.473: CNEZ() entry added 10-31-95.
	0.472: bug fix: int constant 100 in certain contexts. 10-30-95.
	0.471: math & psychro functions added. 10-30-95.
	0.470: BUG0072 fixed -- exprs in reports when autoSizing+mainSim done. 10-12-95.
	0.469: BUG0071 fixed -- no ZDD reports if autoSizing & main sim. 10-10-95.
	0.468: BUG0069 fixed -- error msg with long file line -- 9-25-95.
	0.467: fix BUG0066 (monthly + hourly expr spuriously made monthly-hourly, thus eval'd only on solar calc days).
	       restore Top.tDbOMM7. Delivered 9-20-95.
	0.466: 32-bit vrpak bug fixed re > 64K of reports. 9-16-95.
	0.465: AutoSizing... DX modeling changes, memos updated. 8-29-95.
	0.464: AutoSizing... BUG0065 fixed. 8-16.95.
	0.463: AutoSizing... Much debugging & robustifying. Delivered to BSG 7-21-95.
	0.462: AUTOSIZING!!! First, partial, delivery to BSG 7-11-95.
	0.461: gtSHGC/SMSO/SMSC wn... grndRefl... wnVfDf...; workdays; more yields. 6-2-95.
	0.460: psychro fcns extended to 300F; bug re eg @Top.runDateTime probe in report/export fixed.
	0.459: more ZN robustness; tolerances in ah converger(); more tests. 4-95.
	0.458: ZN/ZN2 robustness. T13nx and T14nx test groups created. 4-95.
	0.457: Top.jdayST init bug. 3-27-95.
	0.456: wf path bug; add wf path search. 3-8-95.
	0.455: 2 minor bug fixes
	0.454: -ipath on cmd line; file search enhancements; fop.cpp; multiple input files (new cse1 level added). 2-24-95.
	0.453: cgsolar cleanup; solar bugs, iz & adiabatic surface bugs, other bugs; 2-19-95.
	0.452: 8 sgdists, mass ebal bugfix (qd double) 2-13-95.
	0.451: carpet, insolation split 2-11-95.
	0.450: new solar gain targeting, surface changes, no INTMASSes, integers floated, non-debugging versions. 2-9-95.
	0.449: subhour masses; walls massive by default; interpolated weather. 1-19-95.
	0.448: elevation defaults from wthr file; warmup uses days b4 run. 1-7-95.
	0.447: sea lave coil ratings & use rating w for entering air heat capacity; variable subHours removed. 1-5-95.
	0.446: new windows; ZN2 == sp thrash bug fix. delivered 12-12-94.
	0.445: fixed cnloads bug when mass only -- ztuCf not getting set --> ebal errors & wrong answer. Delivered 12-6-94.
	0.444: bin res uses global memory only when return requested, re Win32s OOM problems
	0.443: wfrd.zip (wthr file fcns) delivered 11-23-94 with this version #
	0.442: bugs, help redo & rdt in r/k in opk. delivered 11-10-94.
	0.441: skipped (used for weather programs)
	0.440: weather files in flux due to code shared with utilities
	0.439: cnew32.exe delivered 10-21-94 so Scott could probe runDateTime. Weather files in flux.
	0.438: opk/wincne delivered with this version 10-7-94
	0.437: opk delivered with this version
	0.436: more graphs; footcandles conversion for brf corrected in cnguts.cpp; delivered 9-22-94.
	0.435: more graphs, version++ to match WINCNE (delivered 9-19-94)
	0.434: accidentally made opk.zip after incrementing version, so incremented again.
	0.433: more bin res file additions; opk graphs enhancements. 9-1-94... RESFVER 7. uploaded to bsgs 9-4-94.
	0.432: no such, but a backup saved with this ver 9-1-94
	0.431: additions to bin res files; version under construction, do not distribute. 8-94.
	0.430: 8-94 resumption of work for NREL.

    0.429..423, 2..3-94: 32 bit version, client-server, cneHansFree(), new Phar Lap, yields, cneCleaup().
    0.422..405, 12-91..1-94: import files, compressed bin res.
    0.404..400, 10..12-94: binary results files, simple rates & demand, peaks, multi-run, windows, dll,
                           version jump at start of NREL E10 work.

    0.336:  3-20-93:       Phar Lap RTK not SDK (in CNEP) for distributability.
    0.335:  1-93:          where stopped work in 1-93; some copies distributed.
                           had heatplants, coolplants, towerplants; ahp coils.
    .301.. .311,4..7-92:    furnaces, fan-cycles, heatplants, protected mode, dx coils, ah/tu; version jump.
    .22.. .231,10-91..3-92: Borland; disk texts; name CNE92; virtual reports, report/export files,
                            error messages in input listing.
    chip's work below, rob's above.
    0.21:   10-14-91: 1st working terminal version
    0.20:   10-2-91:  development version including some terminal code
*/

// csevrsn.h end
