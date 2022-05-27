**whName**

Optional name of water heater; give after the word “DHWHEATER” if desired.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**whMult=*integer***

Number of identical water heaters of this type. Any value $>1$ is equivalent to repeated entry of the same DHWHEATER.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1",
  required: "No",
  variability: "constant")
  %>

**whType=*choice***

Type of water heater.  This categorization is based on CEC and federal rating standards that change from time to time.

<%= csv_table(<<END, :row_header => false)
  SMALLSTORAGE, A storage water heater having an energy factor (EF) rating.  Generally&comma; a gas-fired storage water heater with input of 75&comma;000 Btuh or less&comma; an oil-fired storage water heater with input of 105&comma;000 Btuh or less&comma; an electric storage water heater with input of 12 kW or less&comma; or a heat pump water heater rated at 24 amps or less.
  LARGESTORAGE,          Any storage water heater that is not SMALLSTORAGE.
  SMALLINSTANTANEOUS,    A water heater that has an input rating of at least 4&comma;000 Btuh per gallon of stored water. Small instantaneous water heaters include: gas instantaneous water heaters with an input of 200&comma;000 Btu per hour or less&comma; oil instantaneous water heaters with an input of 210&comma;000 Btu per hour or less&comma; and electric instantaneous water heaters with an input of 12 kW or less.
  LARGEINSTANTANEOUS, An instantaneous water heater that does not conform to the definition of SMALLINSTANTANEOUS&comma; an indirect fuel-fired water heater&comma; or a hot water  supply boiler.
  INSTANTANEOUSUEF, An instantaneous water heater having a UEF rating (as opposed to EF).
END
%>

  <%= member_table(
  units: "",
  legal_range: "*Codes listed above*",
  default: "SMALLSTORAGE",
  required: "No",
  variability: "constant")
  %>

**whHeatSrc=*choice***

Heat source for water heater.  CSE implements uses efficiency-based models for
all whTypes (as documented in RACM, App. B).  In addition, the detailed Ecotope HPWH model is
available for electric (air source heat pump and resistance) SMALLSTORAGE water heaters.

<%= csv_table(<<END, :row_header => false)
  RESISTANCE,    Electric resistance heating element Deprecated for whType=SMALLSTORAGE (use RESISTANCEX)
  RESISTANCEX,   Electric resistance heating element&comma; detailed HPWH model
  ASHP,          Air source heat pump&comma; EF model Deprecated for whType=SMALLSTORAGE (use ASHPX)
  ASHPX,         Air source heat pump&comma; detailed HPWH model
  FUEL,          Fuel-fired burner
END
%>

<%= member_table(
  units: "",
  legal_range: "*Codes listed above*",
  default: "FUEL",
  required: "No",
  variability: "constant")
  %>

**whResType=*choice***

Resistance heater type, valid only if whHeatSrc is equal to RESISTANCEX, else ignored. These choices are supported by the detailed HPWH model.  Except for Generic, all heater characteristics are set by HPWH based on whResType.

<%= member_table(
  units: "",
  legal_range: "Typical\nSwingTank",
  default: "Typical",
  required: "No",
  variability: "constant") %>

**whHeatingCap=*float***

Nominal heating capacity, available only for a limited HPWH types.

<%= member_table(
  units: "Btuh",
  legal_range: "x $>$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>

**whVol=*float***

Storage tank volume. Must be omitted or 0 for instantaneous whTypes.  Used by HPWH model (whHeatSrc=RESISTANCEX or whHeatSrc=ASHPX). Required when whHeatSrc=RESISTANCEX or whHeatSrc=ASHPX with whASHPType=GENERIC.  For all other configurations, whVol is documentation-only.

<%= member_table(
  units: "gal",
  legal_range: "$\\ge$ 0.1 (caution: small values may cause runtime errors)",
  default: "per whASHPType if HPWH else 50",
  required: "For some HPWH configurations, see above",
  variability: "constant")
  %>

**whVolRunning=*float***

Running storage volume is the volume above aquastat. Requires the total volume based on aquastat position. Ecotope's HPWH tank and heater.

<%= member_table(
  units: "gal",
  legal_range: "x $>$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>


**whEF=*float***

Rated energy factor that specifies DHWHEATER efficiency under test conditions.  Used
by CSE to derive annual water heating efficiency and/or other characteristics as described
below.  Calculation methods are documented in RACM, Appendix B.

<%= csv_table(<<END, :row_header => true)
  Configuration,                 whEF default,    Use
  whType=SMALLSTORAGE&comma; whHeatSrc=RESISTANCE or FUEL, 0.82, Derivation of whLDEF
  whType=SMALLSTORAGE&comma; whHeatSrc=ASHP, 0.82, Derivation of whLDEFnote inappropriate default (deprecated&comma; use ASHPX)
  whType=SMALLSTORAGE&comma; whHeatSrc=ASHPX&comma; whASHPType=GENERIC, (req'd), Tank losses Overall efficiency
  whType=SMALLSTORAGE&comma; whHeatSrc=RESISTANCEX, (req'd), Tank losses Note: maximum whEF=0.98.
  whType=SMALLINSTANTANEOUS&comma; whHeatSrc=RESISTANCE or FUEL, 0.82, Annual efficiency = whEF*0.92
  Any other, (unused)
END
%>

<%= member_table(
  units: "",
  legal_range: "$>$ 0 *Caution: maximum not checked. Unrealistic values will cause runtime errors and/or invalid results*",
  default: "*See above*",
  required: "*See above*",
  variability: "constant")
  %>

**whLDEF=*float***

Load-dependent energy factor for DHWHEATERs with whType=SMALLSTORAGE and whHeatSrc=FUEL
or whHeatSrc=RESISTANCE.  If not given, whLDEF is derived using a preliminary simulation
activated via DHWSYS wsCalcMode=PRERUN.  See RACM Appendix B.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated via DHWSYS PreRun mechanism",
  required: "When whType = SMALLSTORAGE and PreRun not used",
  variability: "constant")
  %>

**whUEF=*float***

Water heater Uniform Energy Factor efficiency rating, used when whType=INSTANTANEOUSUEF.

<%= member_table(
  units: "",
  legal_range: "x $\\ge$ 0",
  default: "*none*",
  required: "when whType= INSTANTANEOUSUEF",
  variability: "constant")
  %>

**whAnnualElec=*float***

Annual electricity use assumed in UEF rating derivation.  Used when whType=INSTANTANEOUSUEF.

<%= member_table(
  units: "kWh",
  legal_range: "x $\\ge$ 0",
  default: "*none*",
  required: "when whType= INSTANTANEOUSUEF",
  variability: "constant")
  %>

**whAnnualFuel=*float***

Annual fuel use assumd in UEF rating derivation, used when whType=INSTANTANEOUSUEF.

<%= member_table(
  units: "therms",
  legal_range: "x $\\ge$ 0",
  default: "*none*",
  required: "when whType= INSTANTANEOUSUEF",
  variability: "constant")
  %>

**whRatedFlow=*float***

Maximum flow rate assumed in UEF rating derivation.  Used when whType=INSTANTANEOUSUEF.

<%= member_table(
  units: "gpm",
  legal_range: "x $>$ 0",
  default: "*none*",
  required: "when whType= INSTANTANEOUSUEF",
  variability: "constant")
  %>

**whStbyElec=*float***

Instantaneous water heater standby power (electricity consumed when heater is not operating).  Used when whType=INSTANTANEOUSUEF.

<%= member_table(
  units: "W",
  legal_range: "x $\\ge$ 0",
  default: "4",
  required: "No",
  variability: "constant")
  %>

**whLoadCFwdF=*float***

Instanteous water heater load carry forward factor -- approximate number of hours the heater is allowed to meet water heating demand that is unmet on a 1 minute basis, used when whType=INSTANTANEOUSUEF.

<%= member_table(
  units: "",
  legal_range: "x $\\ge$ 0",
  default: "1",
  required: "No",
  variability: "constant")
  %>

**whZone=*znName***

Name of zone where water heater is located, used only in detailed HPWH models (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX), otherwise no effect. Zone conditions are used for tank heat loss calculations.  Heat exchanged with the DHWHEATER are included in the zone heat balance.  whZone also provides the default for whASHPSrcZn (see below). whZone and whTEx cannot both be specified.

<%= member_table(
  units: "",
  legal_range: "name of a ZONE",
  default: "Not in a zone (heat losses discarded)",
  required: "No",
  variability: "constant")
  %>

**whTEx=*float***

Water heater surround temperature, used only in detailed HPWH models (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX), otherwise no effect.  whZone and whTEx cannot both be specified.

<%= member_table(
  units: "^o^F",
  legal_range: "x $\\ge$ 0",
  default: "whZone air temperature if specified, else 70 ^o^F",
  required: "No",
  variability: "hourly")
  %>

**whASHPType=*choice***

Air source heat pump type, valid only if whHeatSrc=ASHPX. These choices are supported by the detailed HPWH model.  Except for Generic, all heater characteristics are set by HPWH based on whASHPType.

<%= csv_table(<<END, :row_header => true)
"Choice","Specified type"
"Generic","General generic (parameterized by wh_EF and wh_vol)"
"AOSmithPHPT60","60 gallon Voltex"
"AOSmithPHPT80","80 gallon Voltex"
"AOSmithHPTU50","50 gallon AOSmith HPTU"
"AOSmithHPTU66","66 gallon AOSmith HPTU"
"AOSmithHPTU80","80 gallon AOSmith HPTU"
"AOSmithHPTU80DR","80 gallon AOSmith HPTU (demand reduction variant)"
"AOSmithCAHP120","120 gallon AOSmith"
"Sanden40","Sanden 40 gallon CO2 external heat pump"
"Sanden80","Sanden 80 gallon CO2 external heat pump"
"Sanden120","Sanden 120 gallon CO2 external heat pump"
"SandenGS3","Sanden GS3 compressor CO2 external"
"GE2012","2012 era GeoSpring"
"GE2014","2014 80 gal GE model run in the efficiency mode"
"GE2014_80DR","2014 80 gal GE model run in the efficiency mode (demand reduction variant)"
"GE2014StdMode","2014 50 gal GE run in standard mode"
"GE2014StdMode80","2014 80 gal GE run in standard mode"
"RheemHB50","newish Rheem (2014 model?)"
"RheemHBDR2250","50 gallon, 2250 W resistance Rheem HB Duct Ready"
"RheemHBDR4550","50 gallon, 4500 W resistance Rheem HB Duct Ready"
"RheemHBDR2265","65 gallon, 2250 W resistance Rheem HB Duct Ready"
"RheemHBDR4565","65 gallon, 4500 W resistance Rheem HB Duct Ready"
"RheemHBDR2280","80 gallon, 2250 W resistance Rheem HB Duct Ready"
"RheemHBDR4580","80 gallon, 4500 W resistance Rheem HB Duct Ready"
"Rheem2020Prem40","40 gallon, Rheem 2020 Premium"
"Rheem2020Prem50","50 gallon, Rheem 2020 Premium"
"Rheem2020Prem65","65 gallon, Rheem 2020 Premium"
"Rheem2020Prem80","80 gallon, Rheem 2020 Premium"
"Rheem2020Build40","40 gallon, Rheem 2020 Builder"
"Rheem2020Build50","50 gallon, Rheem 2020 Builder"
"Rheem2020Build65","65 gallon, Rheem 2020 Builder"
"Rheem2020Build80","80 gallon, Rheem 2020 Builder"
"Stiebel220E","Stiebel Eltron (2014 model?)"
"AOSmithSHPT50","AOSmith add'l models (added 3-24-2017)"
"AOSmithSHPT66","AOSmith add'l models (added 3-24-2017)"
"AOSmithSHPT80","AOSmith add'l models (added 3-24-2017)"
"GenericTier1","Generic Tier 1"
"GenericTier2","Generic Tier 2"
"GenericTier3","Generic Tier 3"
"Generic","General generic (parameterized by EF and vol)"
"UEF2Generic","Experimental UEF=2"
"WorstCaseMedium","UEF2Generic alias (supports pre-existing test cases)"
"BasicIntegrated","Typical integrated HPWH"
"ResTank","Resistance heater (no compressor).  Superceded by whHeatSrc=RESITANCEX"
"ResTankNoUA","Resistance heater (no compressor) with no tank losses.  Superseded by whHeatSrc=RESISTANCEX."
"AOSmithHPTU80DR","80 gallon AOSmith HPTU with fixed backup setpoint (experimental for demand response testing)"
"AOSmithSHPT50","50 gal AOSmith SHPT"
"AOSmithSHPT66","66 gal AOSmith SHPT"
"AOSmithSHPT80","80 gal AOSmith SHPT"
"ColmacCxV5_SP","Colmac CxA-xx modular external HPWHs (single pass mode)"
"ColmacCxA10_SP","Colmac CxA-xx modular external HPWHs (single pass mode)"
"ColmacCxA15_SP","Colmac CxA-xx modular external HPWHs (single pass mode)"
"ColmacCxA20_SP","Colmac CxA-xx modular external HPWHs (single pass mode)"
"ColmacCxA25_SP","Colmac CxA-xx modular external HPWHs (single pass mode)"
"ColmacCxA30_SP","Colmac CxA-xx modular external HPWHs (single pass mode)"
"ColmacCxV5_MP","Colmac CxA-xx modular external HPWHs (multi-pass mode)"
"ColmacCxA10_MP","Colmac CxA-xx modular external HPWHs (multi-pass mode)"
"ColmacCxA15_MP","Colmac CxA-xx modular external HPWHs (multi-pass mode)"
"ColmacCxA20_MP","Colmac CxA-xx modular external HPWHs (multi-pass mode)"
"ColmacCxA25_MP","Colmac CxA-xx modular external HPWHs (multi-pass mode)"
"ColmacCxA30_MP","Colmac CxA-xx modular external HPWHs (multi-pass mode)"
"NyleC25A_SP","Nyle Cxx external HPWHs (SP = single pass mode)"
"NyleC60A_SP","Nyle Cxx external HPWHs (SP = single pass mode)"
"NyleC90A_SP","Nyle Cxx external HPWHs (SP = single pass mode)"
"NyleC125A_SP","Nyle Cxx external HPWHs (SP = single pass mode)"
"NyleC185A_SP","Nyle Cxx external HPWHs (SP = single pass mode)"
"NyleC250A_SP","Nyle Cxx external HPWHs (SP = single pass mode)"
"NyleC60A_CWP_SP","Nyle Cxx external SP HPWHs with cold weather package"
"NyleC90A_CWP_SP","Nyle Cxx external SP HPWHs with cold weather package"
"NyleC125A_CWP_SP","Nyle Cxx external SP HPWHs with cold weather package"
"NyleC185A_CWP_SP","Nyle Cxx external SP HPWHs with cold weather package"
"NyleC250A_CWP_SP","Nyle Cxx external SP HPWHs with cold weather package"
"NyleC60A_MP","Nyle Cxx external HPWHs (MP = multi-pass mode)"
"NyleC90A_MP","Nyle Cxx external HPWHs (MP = multi-pass mode)"
"NyleC125A_MP","Nyle Cxx external HPWHs (MP = multi-pass mode)"
"NyleC185A_MP","Nyle Cxx external HPWHs (MP = multi-pass mode)"
"NyleC250A_MP","Nyle Cxx external HPWHs (MP = multi-pass mode)"
"NyleC60A_CWP_MP","Nyle Cxx external MP HPWHs w/ cold weather package"
"NyleC90A_CWP_MP","Nyle Cxx external MP HPWHs w/ cold weather package"
"NyleC125A_CWP_MP","Nyle Cxx external MP HPWHs w/ cold weather package"
"NyleC185A_CWP_MP","Nyle Cxx external MP HPWHs w/ cold weather package"
"NyleC250A_CWP_MP","Nyle Cxx external MP HPWHs w/ cold weather package"
"Rheem_HPHD60HNU_MP","Rheem HPHD60 MP external MP HPWHs"
"Rheem_HPHD60VNU_MP","Rheem HPHD60 MP external MP HPWHs"
"Rheem_HPHD135HNU_MP","Rheem HPHD135 MP external MP HPWHs"
"Rheem_HPHD135VNU_MP","Rheem HPHD135 MP external MP HPWHs"
"Scalable_SP","single pass scalable type for autosized standard design"
"Scalable_MP","multipass scalable type for autosized standard design"
END
%>

<%= member_table(
  units: "",
  legal_range: "*Codes listed above*",
  default: "*none*",
  required: "When whHeatSrc=ASHPX",
  variability: "constant")
  %>

**whASHPSrcZn=*znName***

Name of zone that serves as heat pump heat source used when whHeatSrc=ASHPX.  Used for tank heat loss calculations and default for whASHPSrcZn.  Heat exchanges are included in zone heat balance.  whASHPSrcZn and whASHPSrcT cannot both be specified.

<%= member_table(
  units: "",
  legal_range: "name of a ZONE",
  default: "Same as whZone not specified. If no zone is specified by input or default, heat extracted by ASHP has no effect.",
  required: "No",
  variability: "constant")
  %>

**whASHPSrcT=*float***

Heat pump source air temperature used when whHeatSrc=ASHPX.  Heat removed from this source is added to the heated water but has no other effect.  whASHPSrcZn and whASHPSrcT cannot both be specified.

<%= member_table(
  units: "^o^F",
  legal_range: "x $\\ge$ 0",
  default: "whASHPZn air temperature if specified, else 70 ^o^F",
  required: "No",
  variability: "hourly")
  %>

**whASHPResUse=*float***

  Specifies activation temperature difference for resistance heating, used only when whHeatSrc=ASHPX and whASHPType=GENERIC.  Refer to HPWH engineering documentation for model details.

<%= member_table(
  units: "^o^C",
  legal_range: "x $\\ge$ 0",
  default: "7.22",
  required: "No",
  variability: "constant")
  %>

**whResHtPwr=*float***

  Specifies resistance upper element power, used only with whHeatSrc=RESISTANCEX.

<%= member_table(
  units: "W",
  legal_range: "x $\\ge$ 0",
  default: "4500",
  required: "No",
  variability: "constant")
  %>

**whResHtPwr2=*float***

  Specifies resistance lower element power, used only with whHeatSrc=RESISTANCEX.

<%= member_table(
  units: "W",
  legal_range: "x $\\ge$ 0",
  default: "whResHtPwr",
  required: "No",
  variability: "constant")
  %>

**whUA=*float***

HPWH-type total UA (not per tank)

<%= member_table(
  units: "Btuh/F",
  legal_range: "x $\\geq$ 0",
  default: "HPWH default",
  required: "No",
  variability: "constant") %>

**whInsulR=*float***

Tank insulation resistance for heat pump water heater.

<%= member_table(
  units: "hr-F/Btuh",
  legal_range: "x $>$ 0",
  default: "-1",
  required: "No",
  variability: "constant") %>

**whInHtSupply=*float***\
**whInHtLoopRet=*float***

  Fractional tank height of inlets for supply water and DHWLOOP return, used only with HPWH types (whHeatSrc=RESISTANCEX or whHeatSrc=ASHPX).  0 indicates the bottom of the water heater tank and 1 specifies the top.  Inlet height influences tank layer mixing and can impact heat pump COP and/or heating activation frequency.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 1",
  default: "HPWH default (0?) ",
  required: "No",
  variability: "constant")
  %>

**whtankCount=*float***

Number of storage tanks per DHWHEATER, re built-up whType=Builtup, does *not* reflect wh_mult (wh_mult=2, wh_tankCount=3 -> 6 tanks).

<%= member_table(
  units: "#",
  legal_range: "x $\\geq$ 1",
  default: "1",
  required: "No",
  variability: "constant") %>

**whEff=*float***

Water heating efficiency, used in modeling whType=LARGESTORAGE and whType=LARGEINSTANTANEOUS.

<%= member_table(
  units: "",
  legal_range: "0 $<$ whEff $\\leq$ 1",
  default: ".82",
  required: "No",
  variability: "constant")
  %>

**whSBL=*float***

Standby loss, used in modeling whType=LARGESTORAGE.

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "constant")
  %>

**whPilotPwr=*float***

Pilot light consumption, included in fuel energy use of DHWHEATERs with whHeatSrc=FUEL.

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**whParElec=*float***

Parasitic electricity power, included in electrical energy use of all DHWHEATERs.

<%= member_table(
  units: "W",
  legal_range: "x $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**whElecMtr=*mtrName***

Name of METER object, if any, by which DHWHEATER electrical energy use is recorded (under end use DHW).

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*Parent DHWSYS wsElecMtr*",
  required: "No",
  variability: "constant")
  %>

**whxBUEndUse=*choice***

Specifies the whElecMtr end use, if any, to which extra backup energy is accumulated. In some water heater types, extra backup energy is modeled to maintain output temperature at wsTUse.  By default, extra backup energy is included in end use dhwBU.  whxBUEndUse allows specification of an alternative end use to which extra backup energy is accumulated.

<%= member_table(
  units: "",
  legal_range: "*end use code* ",
  default: "(extra backup accums to dhwBU)",
  required: "No",
  variability: "constant")
  %>

**whFuelMtr =*mtrName***

Name of METER object, if any, by which DHWHEATER fuel energy use is recorded (under end use DHW).

<%= member_table(
  units: "*name of a METER*",
  legal_range: "*Parent DHWSYS wsFuelMtr*",
  default: "",
  required: "No",
  variability: "constant")
  %>

**endDHWHEATER**

Optionally indicates the end of the DHWHEATER definition.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "")
  %>

**Related Probes:**

- @[DHWHeater](#p_dhwheater)
