# RSYS

RSYS constructs an object representing an air-based residential HVAC system.

**rsName**

Optional name of HVAC system; give after the word “RSYS” if desired.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "*none*",
  required: "No",
  variability: "constant") %>

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**rsType=*choice***

Type of system.

-------------------------------------------------------------------------
**rsType**           **Description**
-------------------  ----------------------------------------------------
ACFURNACE             Compressor-based cooling and fuel-fired
                      heating.\
                      Primary heating input energy
                      is accumulated to end use HTG of meter
                      rsFuelMtr.

ACRESISTANCE          Compressor-based cooling and electric
                      (“strip”) heating. Primary heating
                      input energy is accumulated to end use
                      HTG of meter rsElecMtr.

ASHP                  Air-source heat pump (compressor-based
                      heating and cooling). Primary
                      (compressor) heating input energy is
                      accumulated to end use HTG of meter
                      rsElecMtr. Auxiliary and defrost resistance
                      ("strip") heating input energy is accumulated to
                      end use HPBU of meter rsElecMtr.

ASHPKGROOM            Packaged air-source heat pump.

ASHPHYDRONIC          Air-to-water heat pump with hydronic distribution.
                      Compressor performance is approximated using
                      the air-to-air model with adjusted
                      efficiencies.

VCHP2                 Air-to-air heat pump with variable speed
                      compressor
 
AC                    Compressor-based cooling; no heating.
                      Required ratings are SEER and capacity and EER at 95 ^o^F
                      outdoor dry bulb.

ACPKGROOM             Packaged compressor-based cooling; no heating.
                      Required ratings are capacity and EER at 95 ^o^F
                      outdoor dry bulb.

FURNACE               Fuel-fired heating. Primary heating
                      input energy is accumulated to end use
                      HTG of meter rsFuelMtr.

RESISTANCE            Electric heating. Primary heating input
                      energy is accumulated to end use HTG of
                      meter rsElecMtr

ACPKGROOMFURNACE

ACPKGROOMRESISTANCE
--------------------------------------------------------------------------



<%= member_table(
  units: "",
  legal_range: "*one of above choices*",
  default: "ACFURNACE",
  required: "No",
  variability: "constant") %>

**rsDesc=*string***

Text description of system, included as documentation in debugging reports such as those triggered by rsPerfMap=YES

<%= member_table(
  units: "",
  legal_range: "string",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**rsModeCtrl=*choice***

Specifies systems heating/cooling availability during simulation.

  ------- ---------------------------------------
  OFF     System is off (neither heating nor
          cooling is available)

  HEAT    System can heat (assuming rsType can
          heat)

  COOL    System can cool (assuming rsType can
          cool)

  AUTO    System can either heat or cool
          (assuming rsType compatibility). First
          request by any zone served by this RSYS
          determines mode for the current time
          step.
  ------- ---------------------------------------

<%= member_table(
  units: "",
  legal_range: "OFF, HEAT, COOL, AUTO",
  default: "AUTO",
  required: "No",
  variability: "hourly") %>

**rsPerfMap=*choice***

Generate performance map(s) for this RSYS. Comma-separated text is written to file PM\_[rsName].csv. This is a debugging capability that is not necessarily maintained.

<%= member_table(
  units: "",
  legal_range: "YES, NO",
  default: "NO",
  required: "No",
  variability: "constant") %>

**rsFanTy=*choice***

Specifies fan (blower) position relative to cooling coil.

<%= member_table(
  units: "",
  legal_range: "BLOWTHRU, DRAWTHRU",
  default: "BLOWTHRU",
  required: "No",
  variability: "constant") %>

**rsFanMotTy=*choice***

Specifies type of motor driving the fan (blower). This is used in the derivation of the coil-only cooling capacity for the RSYS.

  ----- --------------------------------------
  PSC   Permanent split capacitor
  BPM   Brushless permanent magnet (aka ECM)
  ----- --------------------------------------

<%= member_table(
  units: "",
  legal_range: "PSC, BPM",
  default: "PSC",
  required: "No",
  variability: "constant") %>

**rsAdjForFanHt=*NOYESCH***

Fan heat adjustment with two options Yes or no. Yes: fanHtRtd derived from rsFanTy and removed from capacity and input values. No: no rated fan heat adjustments.

**rsElecMtr=*mtrName***

Name of METER object, if any, by which system’s electrical energy use is recorded (under appropriate end uses).

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**rsFuelMtr =*mtrName***

Name of METER object, if any, by which system’s fuel energy use is recorded (under appropriate end uses).

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**rsLoadMtr =*ldmtrName***

Name of a LOADMETER object, if any, to which the system’s heating and cooling loads are recorded.  Loads are the gross heating and cooling energy added to (or removed from) the air stream.  Fan heat, auxiliary heat, and duct losses are not included in loads values.

<%= member_table(
  units: "",
  legal_range: "*name of a LOADMETER*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**rsAFUE=*float***

Heating Annual Fuel Utilization Efficiency (AFUE).

<%= member_table(
  units: "",
  legal_range: "0 $<$ x $\\le$ 1",
  default: "0.9 if furnace, 1.0 if resistance",
  required: "No",
  variability: "constant") %>

**rsCapH=*float***

Heating capacity, used when rsType is ACFURNACE, ACRESISTANCE, FURNACE, or RESISTANCE.

<%= member_table(
  units: "Btu/hr",
  legal_range: "*AUTOSIZE* or x $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>

**rsTdDesH=*float***

Nominal heating temperature rise (across system, not at zone) used during autosizing (when capacity is not yet known) and to derive heating air flow rate from heating capacity.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $>$ 0",
  default: "30 ^o^F if ASHP or ASHPHYDRONIC else 50 ^o^F",
  required: "No",
  variability: "constant") %>

**rsFxCapH=*float***

Heating autosizing capacity factor. If AUTOSIZEd, rsCapH or rsCap47 is set to rsFxCapH $\times$ (peak design-day load). Peak design-day load is the heating capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1.4",
  required: "No",
  variability: "constant") %>

**rsFanPwrH=*float***

Heating fan power. Heating air flow is calculated from heating capacity and rsTdDesH.

<%= member_table(
  units: "W/cfm",
  legal_range: "*x* $\\ge$ 0",
  default: "0.365",
  required: "No",
  variability: "constant") %>

**rsHSPF=*float***

For rsType=ASHP, Heating Seasonal Performance Factor (HSPF).

<%= member_table(
  units: "Btu/Wh",
  legal_range: "*x* $>$ 0",
  default: "*none*",
  required: "Yes if rsType=ASHP",
  variability: "constant") %>

**rsCAP115=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 115 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCAP95",
  required: "No",
  variability: "constant") %>

**rsCAP82=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 82 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCAP95",
  required: "No",
  variability: "constant") %>

**rsCap47=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 47 ^o^F.

If both rsCap47 and rsCapC are autosized, they are set to consistent values based on the relative values of heating and cooling loads.  If the autosized capC is greater than 75% of the autosized cap47, then rsCapC is set to autosized capC and rsCap47 is derived from rsCapC.  Otherwise, rsCap47 is set to 75% of autosized cap47 and rsCapC is derived from rsCap47.

<%= member_table(
  units: "Btu/Wh",
  legal_range: "*AUTOSIZE* or *x* $>$ 0",
  default: "Calculated from rsCapC",
  required: "No",
  variability: "constant") %>

**rsCap35=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 35 ^o^F.  rsCap35 typically reflects reduced capacity due to reverse (cooling) heat pump operation for defrost.

<%= member_table(
  units: "Btu/Wh",
  legal_range: "*x* $>$ 0",
  default: "Calculated from rsCap47 and rsCap17",
  required: "No",
  variability: "constant") %>

**rsCap17=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 17 ^o^F.

<%= member_table(
  units: "Btu/Wh",
  legal_range: "*x* $>$ 0",
  default: "Calculated from rsCap47",
  required: "No",
  variability: "constant") %>

**rsCAP05=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 5 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCAP47",
  required: "No",
  variability: "constant") %>

**rsCOP115=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 115 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCap115",
  required: "No",
  variability: "constant") %>

**rsCOP95=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 95 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCap95",
  required: "No",
  variability: "constant") %>

**rsCOP82=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 82 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCap82",
  required: "No",
  variability: "constant") %>

**rsCOP47=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 47 ^o^F.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "Estimated from rsHSPF, rsCap47, and rsCap17",
  required: "No",
  variability: "constant") %>

**rsCOP35=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 35 ^o^F.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "Calculated from rsCap35, rsCap47, rsCap17, rsCOP47, and rsCOP17",
  required: "No",
  variability: "constant") %>

**rsCOP17=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 17 ^o^F.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "Calculated from rsHSPF, rsCap47, and rsCap17",
  required: "No",
  variability: "constant") %>

**rsCOP05=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 5 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCap05",
  required: "No",
  variability: "constant") %>

**rsCapRat1747=*float***

Ratio of rsCAP17 over rsCAP47.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Based on HSPF or\nother correlations",
  required: "No",
  variability: "Start of a run") %>

**rsCapRat9547=*float***

Ratio of rsCAP95 over rsCAP47.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Based on rsCap95",
  required: "No",
  variability: "constant") %>

**rsCapRat0547=*float***

Ratio of rsCAP05 over rsCAP47.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "In development",
  required: "No",
  variability: "constant") %>

**rsCapRat11595=*float***

Ratio of rsCAP115 over rsCAP95.

<%= member_table(
  units: "",
  legal_range: "0 $<$ x $\\geq$ 1",
  default: "0.9155",
  required: "No",
  variability: "constant") %>

**rsCapRat8295=*float***

Ratio of rsCAP82 over rsCAP95.

<%= member_table(
  units: "",
  legal_range: "1 $\\leq$ x $<$ 2",
  default: "1.06",
  required: "No",
  variability: "Start of a run") %>

**rsCOPMin115=*float***

Coefficient of performance at outdoor dry-bulb temperature of 115 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "Calculated from rsCAP115, rsCOP115",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsCOPMin95=*float***

Coefficient of performance at outdoor dry-bulb temperature of 95 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "Calculated from rsCAP95, rsCOP95",
  default: "0.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsCOPMin82=*float***

Coefficient of performance at outdoor dry-bulb temperature of 82 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "Calculated from rsCAP82, rsCOP82",
  default: "0.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsCOPMin47=*float***

Coefficient of performance at outdoor dry-bulb temperature of 47 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "Calculated from rsCAP47, rsCOP47",
  default: "0.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsCOPMin35=*float***

Coefficient of performance at outdoor dry-bulb temperature of 35 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "Calculated from rsCAP35, rsCOP35",
  default: "0.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsCOPMin17=*float***

Coefficient of performance at outdoor dry-bulb temperature of 17 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "Calculated from rsCAP17, rsCOP17",
  default: "0.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsCOPMin05=*float***

Coefficient of performance at outdoor dry-bulb temperature of 5 ^o^F, minimun speed.

<%= member_table(
  units: "",
  legal_range: "Calculated from rsCAP05, rsCOP05",
  default: "0.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsloadFMin115=*float***

Fraction of a clg minimum load at 115 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsloadFMin95=*float***

Fraction of a clg minimum load at 95 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsloadFMin82=*float***

Fraction of a clg minimum load at 82 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsloadFMin47=*float***

Fraction of a htg minimum load at 47 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsloadFMin17=*float***

Fraction of a htg minimum load at 17 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsloadFMin05=*float***

Fraction of a htg minimum load at 05 ^o^F.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "Before set up or at the end of interval") %>

**rsTypeAuxH=*choice***

Type of auxiliary heat. The functions have C_AUXHEATTY as a prefix.

<%= csv_table(<<END, :row_header => true)
Choice
C_AUXHEATTY_NONE
C_AUXHEATTY_RES
C_AUXHEATTY_FURN
END
%>

<%= member_table(
  units: "",
  legal_range: "See table above",
  default: "C_AUXHEATTY_RES",
  required: "No",
  variability: "constant") %>

**rsCtrlAuxH=*choice***

Type of auxiliary heating control.

<%= csv_table(<<END, :row_header => true)
Choice, Description
C_AUXHEATCTRL_LO, Compressor locked out if any auxiliary heating control
C_AUXHEATCTRL_CYCLE, Compressor runs continuously auxiliary cycles
C_AUXHEATCTRL_ALT, Alternates between compressor auxiliary
END
%>

<%= member_table(
  units: "",
  legal_range: "See table above",
  default: "C_AUXHEATCTRL_CYCLE",
  required: "No",
  variability: "Start of a run") %>

**rsCapAuxH=*float***

For rsType=ASHP, auxiliary electric (“strip”) heating capacity. If AUTOSIZEd, rsCapAuxH is set to the peak heating load evaluated at the heating design temperature (Top.heatDsTDbO).

<%= member_table(
  units: "Btu/hr",
  legal_range: "*AUTOSIZE* or *x* $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>

**rsAFUEAuxH=*float***

Auxiliary furnace heating for annualized fuel utilization efficiency.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "0.9",
  required: "No",
  variability: "constant") %>

**rsDefrostModel=*choice***

  Selects modeling options for ASHP outdoor coil defrosting when 17 ^o^F < TDbO < 45 ^o^F.  In this temperature range, heating capacity and/or efficiency are typically reduced due to frost accumulation on the outdoor coil.  

  ---------  ---------------------------------------------------------------------------
  REVCYCLE   Reverse compressor (cooling) operation.  Net capacity and efficiency is derived from rsCap17/rsCOP17 and rsCap35/rsCOP35 using linear interpolation.  Auxiliary heat is not modeled.

  REVCYCLEAUX  Reverse compressor (cooling) operation with provision of sufficient auxiliary heat to make up the loss of heating capacity.  Auxiliary heating is typically used to prevent cold air delivery to zones during the defrost cycle.

  ---------  ----------------------------------------------------------------------------

<%= member_table(
  units: "",
  legal_range: "*one of above choices*",
  default: "REVCYCLEAUX",
  required: "No",
  variability: "constant") %>

**rsFxCapAuxH=*float***

  Auxiliary heating autosizing capacity factor. If AUTOSIZEd, rsCapAuxH is set to rsFxCapAuxH $\times$ (peak design-day load). Peak design-day load is the heating capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1",
  required: "No",
  variability: "constant") %>

**rsSEER=*float***

Cooling rated Seasonal Energy Efficiency Ratio (SEER).

<%= member_table(
  units: "Btu/Wh",
  legal_range: "*x* $>$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**rsEER=*float***

Cooling Energy Efficiency Ratio (EER) at standard AHRI rating conditions (outdoor drybulb of 95 ^o^F and entering air at 80 ^o^F drybulb and 67 ^o^F wetbulb).

<%= member_table(
  units: "Btu/Wh",
  legal_range: "*x* $>$ 0",
  default: "Estimated from SEER",
  required: "No",
  variability: "constant") %>

**rsCapC=*float***

Cooling capacity at standard AHRI rating conditions. If rsType=ASHP and both rsCapC and rsCap47 are autosized, both are set to the larger consistent value.

<%= member_table(
  units: "Btu/hr",
  legal_range: "*AUTOSIZE* or *x* $\le$ 0 (x $>$ 0 coverted to $<$ 0)",
  default: "*none*",
  required: "Yes if rsType includes cooling",
  variability: "constant") %>

**rsTdDesC=*float***

Nominal cooling temperature fall (across system, not zone) used during autosizing (when capacity is not yet known).

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $<$ 0",
  default: "-25",
  required: "No",
  variability: "constant") %>

**rsFxCapC=*float***

Cooling autosizing capacity factor. rsCapC is set to rsFxCapC $\times$ (peak design-day load). Peak design-day load is the cooling capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1.4",
  required: "No",
  variability: "constant") %>

**rsFChg=*float***

Cooling compressor capacity factor.  The gross cooling capacity is adjusted by the factor rsFChg as specified by California Title 24 procedures.


<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1 (no effect)",
  required: "No",
  variability: "constant") %>

**rsVFPerTon=*float***

Standard air volumetric flow rate per nominal ton of cooling capacity.

<%= member_table(
  units: "cfm/ton",
  legal_range: "150 $\\le$ x $\\le$ 500",
  default: "350",
  required: "No",
  variability: "constant") %>

**rsFanPwrC=*float***

Cooling fan power.

<%= member_table(
  units: "W/cfm",
  legal_range: "*x* $\\ge$ 0",
  default: "0.365",
  required: "No",
  variability: "constant") %>

**rsASHPLockOutT=*float***

  Source air dry-bulb temperature below which the air source heat pump compressor does not operate.

<%= member_table(
  units: "^o^F",
  legal_range: "",
  default: "(no lockout)",
  required: "No",
  variability: "hourly") %>

**rsCdH=*float***

  Heating cyclic degradation coefficient, valid only for compressor-based heating (heat pumps).

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 0.5",
  default: "ASHPHYDRONIC: 0.25 ASHP: derived from rsHSPF",
  required: "No",
  variability: "hourly") %>

**rsCdC=*float***

Cooling cyclic degradation coefficient, valid for configurations having compressor-based cooling.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 0.5",
  default: "0",
  required: "No",
  variability: "hourly") %>

**rsFEffH=*float***

Heating efficiency factor.  At each time step, the heating efficiency is multiplied by rsFEffH.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1",
  required: "No",
  variability: "subhourly") %>

**rsFEffC=*float***

Cooling efficiency factor.  At each time step, the cooling efficiency is multiplied by rsEffC.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1",
  required: "No",
  variability: "subhourly") %>

**rsCapNomH=*float***

Heating nominal capacity.  Provides type-independent probe source for RSYS heating capacity.  Daily variability is specified to support value changes during AUTOSIZEing.  Values set via input are typically constant.

<%= member_table(
  units: "Btu/hr",
  legal_range: "*x* $\\ge$ 0",
  default: "no heating: 0 heat pump: rsCap47 (input or AUTOSIZEd) other: rsCapH (input or AUTOSIZEd)",
  required: "No",
  variability: "daily") %>

**rsCapNomC=*float***

Cooling nominal capacity.  Provides type-independent probe source for RSYS cooling capacity.  Daily variability is specified to support value changes during AUTOSIZEing.  Values set via input are typically constant.

<%= member_table(
  units: "Btu/hr",
  legal_range: "*x* $\\ge$ 0",
  default: "no cooling: 0 other: rsCap95 (input or AUTOSIZEd)",
  required: "No",
  variability: "daily") %>

**rsDSEH=*float***

Heating distribution system efficiency.  If given, (1-rsDSEH) of RSYS heating output is discarded.  Cannot be combined with more detailed DUCTSEG model.


<%= member_table(
  units: "",
  legal_range: "0 < *x* < 1",
  default: "(use DUCTSEG model)",
  required: "No",
  variability: "hourly") %>

**rsDSEC=*float***

Cooling distribution system efficiency.  If given, (1-rsDSEC) of RSYS cooling output is discarded.  Cannot be combined with more detailed DUCTSEG model.


<%= member_table(
  units: "",
  legal_range: "0 < *x* < 1",
  default: "(use DUCTSEG model)",
  required: "No",
  variability: "hourly") %>

  **rsOAVType=*choice***

  Type of central fan integrated (CFI) outside air ventilation (OAV) included in this RSYS.  OAV systems use the central system fan to circulate outdoor air (e.g. for night ventilation).

  OAV cannot operate simultaneously with whole building ventilation (operable windows, whole house fans, etc.).  Availability of ventilation modes is controlled on an hourly basis via  [Top ventAvail](#top-model-control-items).

  ---------  ---------------------------------------------------------------------------
  NONE       No CFI ventilation capabilities

  FIXED      Fixed-flow CFI (aka SmartVent).  The specified rsOAVVfDs is used whenever
             the RSYS operates in OAV mode.

  VARIABLE   Variable-flow CFI (aka NightBreeze).  Flow rate is determined at midnight
             based on prior day's average dry-bulb temperature according to a control
             algorithm defined by the NightBreeze vendor.
  ---------  ----------------------------------------------------------------------------

<%= member_table(
  units: "",
  legal_range: "NONE, FIXED, VARIABLE",
  default: "*none*",
  required: "No",
  variability: "constant") %>

 **rsOAVVfDs=*float***

 Design air volume flow rate when RSYS is operating in OAV mode.


<%= member_table(
  units: "cfm",
  legal_range: "*x* $\\ge$ 0",
  default: "*none*",
  required: "if rsOAVType $\ne$ NONE",
  variability: "constant") %>

 **rsOAVVfMinF=*float***

 Minimum air volume flow rate fraction when RSYS is operating in OAV mode.  When rsOAVType=VARIABLE, air flow rate is constrained to rsOAVVfMinF * rsOAVVfDs or greater.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 1",
  default: "0.2",
  required: "No",
  variability: "constant") %>

 **rsOAVFanPwr=*float***

 RSYS OAV-mode fan power.

<%= member_table(
  units: "W/cfm",
  legal_range: "0 < x $\\le$ 5",
  default: "per rsOAVTYPE FIXED: rsFanPwrC VARIABLE: NightBreeze vendor curve based on rsOAVvfDs",
  required: "No",
  variability: "constant") %>

**rsOAVTDbInlet=*float***

OAV inlet (source) air temperature.  Supply air temperature at the zone is generally higher due to fan heat.  Duct losses, if any, also alter the supply air temperature.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $\\ge$ 0",
  default: "Dry-bulb temperature from weather file",
  required: "No",
  variability: "hourly") %>

**rsOAVTdiff=*float***

 OAV temperature differential.  When operating in OAV mode, the zone set point temperature is max( znTD, inletT+rsOAVTdiff).  Small values can result in inadvertent zone heating, due to fan heat.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $>$ 0",
  default: "5 ^o^F",
  required: "No",
  variability: "hourly") %>

**rsOAVReliefZn=*znName***

Name of zone to which relief air is directed during RSYS OAV operation, typically an attic zone.  Relief air flow is included in the target zone's pressure and thermal balance.

<%= member_table(
  units: "",
  legal_range: "*name of ZONE*",
  default: "*none*",
  required: "if rsOAVType $\ne$ NONE",
  variability: "constant") %>

**rsParElec=*float***

Parasitic electrical power.  rsParElec is unconditionally accumulated to rsElecMtr (if specified) and has no other effect.

<%= member_table(
  units: "W",
  legal_range: "",
  default: "0",
  required: "No",
  variability: "hourly") %>

**rsParFuel=*float***

Parasitic fuel use.  rsParFuel is unconditionally accumulated to rsFuelMtr (if specified) and has no other effect.

<%= member_table(
  units: "Btuh",
  legal_range: "",
  default: "0",
  required: "No",
  variability: "hourly") %>

**rsRhIn=*float***

Entering air relative humidity (for model testing).

<%= member_table(
  units: "W/cfm",
  legal_range: "0 $\\le$ *x* $\\le$ 1",
  default: "Derived from entering air state",
  required: "No",
  variability: "constant") %>

**rsTdbOut=*float***

Air dry-bulb temperature at the outdoor portion of this system.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $\\ge$ 0",
  default: "From weather file",
  required: "No",
  variability: "hourly") %>

**endRSYS**

Optionally indicates the end of the RSYS definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[rsys](#p_rsys)
- @[RSYSRes](#p_rsysres) (accumulated results)
