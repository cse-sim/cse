# RSYS

RSYS constructs an object representing an air-based residential HVAC system.

### rsName

Optional name of HVAC system; give after the word “RSYS” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsType

Type: choice

Type of system.

{{
  csv_table("rsType, Description
ACFURNACE, Compressor-based cooling modeled per SEER and EER.  Fuel-fired heating. Primary heating input energy is accumulated to end use HTG of meter rsFuelMtr.
ACPMFURNACE, Compressor-based cooling modeled per PERFORMANCEMAP specified in rsPerfMapClg.  Fuel-fired heating. Primary heating input energy is accumulated to end use HTG of meter rsFuelMtr.
ACRESISTANCE, Compressor-based cooling and electric ('strip') heating. Cooling performance based on SEER and EER.  Primary heating input energy is accumulated to end use HTG of meter rsElecMtr.
ACPMRESISTANCE, Cooling based on PERFORMANCEMAP specified in rsPerfMapClg. Primary heating input energy is accumulated to end use HTG of meter rsElecMtr.
ASHP,  Air-source heat pump (compressor-based heating and cooling). Primary (compressor) heating input energy is accumulated to end use HTG of meter rsElecMtr. Auxiliary and defrost heating input energy is accumulated to end use HPBU of meter rsElecMtr or meter rsFuelMtr (depending on rsTypeAuxH).
ASHPPKGROOM,  Packaged room air-source heat pump.
ASHPHYDRONIC, Air-to-water heat pump with hydronic distribution. Compressor performance is approximated using the air-to-air model with adjusted efficiencies.
ASHPPM, Air-to-air heat pump modeled per PERFORMANCMAPs specified via rsPerfMapHtg and rsPerfMapClg.
WSHP,  Water-to-air heat pump.
AC, Compressor-based cooling; no heating. Required ratings are SEER and capacity and EER at 95 °F outdoor dry bulb.
ACPM, Compressor-based cooling modeled per PERFORMANCEMAP specified in rsPerfMapClg; no heating.
ACPKGROOM, Packaged compressor-based cooling; no heating. Required ratings are capacity and EER at 95 °F outdoor dry bulb.
FURNACE,  Fuel-fired heating. Primary heating input energy is accumulated to end use HTG of meter rsFuelMtr.
RESISTANCE,  Electric heating. Primary heating input energy is accumulated to end use HTG of meter rsElecMtr.
ACPKGROOMFURNACE, Packaged room cooling and (separate) furnace heating.
ACPKGROOMRESISTANCE, Packaged room cooling and electric resistance heating.
COMBINEDHEATDHW,  Combined heating / DHW.  Use rsCHDHWSYS to specify the DHWSYS that provides hot water to the coil in this RSYS.  No cooling.
ACCOMBINEDHEATDHW, Compressor-based cooling; COMBINEDHEATDHW heating.
ACPMCOMBINEDHEATDHW, Compressor-based cooling modeled per PERFORMANCEMAP specified in rsPerfMapClg; COMBINEDHEATDHW heating.
FANCOIL, Coil-based heating and cooling.  No primary (fuel-using) equipment is modeled.  rsLoadMtr&comma; rsHtgLoadMtr&comma; and rsClgLoadMtr are typically used to record loads for linking to an external model.", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "*one of above choices*", 
    "default": "ACFURNACE",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsDesc

Type: string

Text description of system, included as documentation in debugging reports such as those triggered by rsGeneratePerfMap=YES

{{
  member_table({
    "units": "",
    "legal_range": "string", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsModeCtrl

Type: choice

Specifies systems heating/cooling availability during simulation.

{{
  csv_table("OFF,     System is off (neither heating nor cooling is available)
  HEAT,    System can heat (assuming rsType can heat)
  COOL,    System can cool (assuming rsType can cool)
  AUTO,    System can either heat or cool (assuming rsType compatibility). First request by any zone served by this RSYS determines mode for the current time step.")
}}

{{
  member_table({
    "units": "",
    "legal_range": "OFF, HEAT, COOL, AUTO", 
    "default": "AUTO",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsGeneratePerfMap

Type: choice

Generate performance map(s) for this RSYS. Comma-separated text is written to file PM_[rsName].csv. This is a debugging capability that is not necessarily maintained.  The format of the generated csv text file may change and is unrelated to the PERFORMANCEMAP input scheme used via *rsPerfMapHtg* and *rsPerfMapClg*.

{{
  member_table({
    "units": "",
    "legal_range": "YES, NO", 
    "default": "NO",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFanTy

Type: choice

Specifies fan (blower) position relative to primary heating or cooling source (i.e. heat exchanger or heat pump coil for heating and AC coil for cooling).  The blower position determines where fan heat is added to the RSYS air stream and thus influences the coil entering air temperature.

{{
  member_table({
    "units": "",
    "legal_range": "BLOWTHRU, DRAWTHRU", 
    "default": "BLOWTHRU",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFanMotTy

Type: choice

Specifies type of motor driving the fan (blower). This is used in the derivation of the coil-only cooling capacity for the RSYS.

{{
  csv_table("PSC,   Permanent split capacitor
  BPM,   Brushless permanent magnet (aka ECM)")
}}

{{
  member_table({
    "units": "",
    "legal_range": "PSC, BPM", 
    "default": "PSC",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsAdjForFanHt

Type: choice

Fan heat adjustment with two options Yes or no. Yes: fanHtRtd derived from rsFanTy and removed from capacity and input values. No: no rated fan heat adjustments.

### rsElecMtr

Type: mtrName

Name of METER object, if any, by which system’s electrical energy use is recorded (under appropriate end uses).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFuelMtr 

Type: *mtrName*

Name of METER object, if any, by which system’s fuel energy use is recorded (under appropriate end uses).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsLoadMtr, rsHtgLoadMtr, rsClgLoadMtr

Type: *ldMtrName*

Names of LOADMETER objects, if any, to which the system’s heating and/or cooling loads are recorded.  Loads are the gross heating and cooling energy added to (or removed from) the air stream.  Fan heat, auxiliary heat, and duct losses are not included in loads values.

rsLoadMtr accumulates both heating (> 0) and cooling (< 0) loads. rsHtgLoadMtr accumulates only heating loads.  rsClgLoadMtr accumulates only cooling loads.  This arrangement accomodates mixed heating and cooling source configurations.  For example, loads can be tracked appropriately in a building that has multiple cooling sources and a single heating source.

rsLoadMtr should not specify the same LOADMETER as rsHtgLoadMtr or rsClgLoadMtr since this would result in double counting.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a LOADMETER*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsSrcSideLoadMtr, rsHtgSrcSideLoadMtr, rsClgSrcSideLoadMtr

Type: *ldMtrName*

Name of LOADMETER objects, if any, to which the system’s source-side heat transfers are recorded.  For DX systems, this is the outdoor coil heat transfer.  For other types, source-side values are the same as the indoor coil loads reported via rsLoadMtr.

rsSrcSideLoadMtr accumulates both heating (> 0) and cooling (< 0) transfers. rsHtgSrcSideLoadMtr accumulates only heating transfers.  rsClgSrcSideLoadMtr accumulates only cooling transfers.  This arrangement accomodates mixed heating and cooling source configurations.

rsSrcSideLoadMtr should not specify the same LOADMETER as rsHtgSrcSideLoadMtr or rsClgSrcSideLoadMtr since this would result in double counting.

{{
  member_table({
    "units": "",
    "legal_range": "*Name of a LOADMETER*", 
    "default": "",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCHDHWSYS

Type: dhwsysName

DHWSYS hot water source for this RSYS, required when rsType is COMBINEDHEATDHW or ACCOMBINEDHEATDHW.  The specified DHWSYS must include a DHWHEATER of whType=ASHPX or RESISTANCEX.

{{
  member_table({
    "units": "",
    "legal_range": "Name of a DHWSYS", 
    "default": "*none*",
    "required": "if combined heat/DHW",
    "variability": "constant" 
  })
}}

### rsAFUE

Type: float

Heating Annual Fuel Utilization Efficiency (AFUE).

{{
  member_table({
    "units": "",
    "legal_range": "0 $<$ x ≤ 1", 
    "default": "0.9 if furnace, 1.0 if resistance",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCapH

Type: float

Heating capacity, used when rsType is ACFURNACE, ACRESISTANCE, FURNACE, WSHP or RESISTANCE.

If rsType=WSHP, rsCapH is at source fluid temperature = 68 °F.

{{
  member_table({
    "units": "Btu/hr",
    "legal_range": "*AUTOSIZE* or x ≥ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsTdDesH

Type: float

Nominal heating temperature rise (across system, not at zone) used during autosizing (when capacity is not yet known) and to derive heating air flow rate from heating capacity.

{{
  member_table({
    "units": "°F",
    "legal_range": "*x* > 0", 
    "default": "30 °F if heat pump else 50 °F",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFxCapH

Type: float

Heating autosizing capacity factor. If AUTOSIZEd, rsCapH or rsCap47 is set to rsFxCapH $\times$ (peak design-day load). Peak design-day load is the heating capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

{{
  member_table({
    "units": "",
    "legal_range": "*x* > 0", 
    "default": "1.4",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFanPwrH

Type: float

Heating operating fan power. For most rsTypes, heating air flow is calculated from heating capacity and rsTdDesH.  The default value of rsFanPwrH is .365 W/cfm except 0.273 W/cfm is used when rsType=COMBINEDHEATDHW and rsType=ACCOMBINEDHEATDHW.

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "*x* ≥ 0", 
    "default": "see above",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsHSPF

Type: float

For rsType=ASHP, Heating Seasonal Performance Factor (HSPF).

{{
  member_table({
    "units": "Btu/Wh",
    "legal_range": "*x* $>$ 0", 
    "default": "*none*",
    "required": "Yes if rsType=ASHP",
    "variability": "constant" 
  })
}}

### rsCap47

Type: float

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 47 °F.

If rsType=ASHP and both rsCapC and rsCap47 are autosized, both are set to the larger consistent value using rsCapRat9547 (after application of rsFxCapH and rsFxCapC).

{{
  member_table({
    "units": "Btu/Wh",
    "legal_range": "*AUTOSIZE* or *x* $>$ 0", 
    "default": "Calculated from rsCapC",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCap35

Type: float

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 35 °F.  rsCap35 typically reflects reduced capacity due to reverse (cooling) heat pump operation for defrost.

{{
  member_table({
    "units": "Btu/Wh",
    "legal_range": "*x* $>$ 0", 
    "default": "Calculated from rsCap47 and rsCap17",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCap17

Type: float

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 17 °F.

{{
  member_table({
    "units": "Btu/Wh",
    "legal_range": "*x* $>$ 0", 
    "default": "Calculated from rsCap47",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCOP95

Type: float

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 95 °F.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "Calculated from rsCap95",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCOP47

Type: float

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 47 °F. For rsType=WSHP, rated heating coefficient of performance at source fluid temperature = 68 °F.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "Estimated from rsHSPF, rsCap47, and rsCap17",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCOP35

Type: float

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 35 °F.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "Calculated from rsCap35, rsCap47, rsCap17, rsCOP47, and rsCOP17",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCOP17

Type: float

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 17 °F.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "Calculated from rsHSPF, rsCap47, and rsCap17",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCapRat1747

Type: float

For rsType=ASHP, ratio of rsCAP17 over rsCAP47.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "Based on HSPF or\nother correlations",
    "required": "No",
    "variability": "Start of a run" 
  })
}}

### rsFChgH

Type: float

For all heatpump types, heating compressor charge factor.  The heating gross (compressor only) COP is modified by rsFChgH to account for incorrect refrigerant charge.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1 (no effect)",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCapRat9547

Type: float

Ratio of rsCAP95 to rsCAP47.  This ratio is used for inter-defaulting rsCap47 and rsCapC such that they have consistent values as is required given that a heat pump is a single device.  If not given, rsCapRat9547 is determined during calculations using the relationship cap95 = 0.98 * cap47 + 180 (derived via correlation of capacities of a set of real units).

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "See above",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCapRatCH

Type: float

For WSHP only: ratio of rsCapC to rsCapH.  Used to derive capacity during autosizing or when only one capacity is specified.

{{
  member_table({
    "units": "",
    "legal_range": ".3 ≤ x $<$ 2", 
    "default": "0.8",
    "required": "No",
    "variability": "Start of a run" 
  })
}}

### rsPerfMapHtg

Type: performanceMapName

Specifies the heating performance PERFORMANCEMAP for RSYSs having rsType=ASHPPM.  The PERFORMANCEMAP must have grid variables outdoor drybulb and compressor speed (in that order) and lookup values of net capacity ratios and COP.  See example in PERFORMANCEMAP.

{{
  member_table({
    "units": "",
    "legal_range": "Name of a PERFORMANCEMAP", 
    "default": "",
    "required": "if rsType specifies a performance map model",
    "variability": "Start of a run" 
  })
}}

### rsPerfMapClg

Type: performanceMapName

Specifies the cooling performance PERFORMANCEMAP for RSYSs having rsType=ASHPPM, ACPM, ACPMFURNACE, ACPMRESISTANCE, or ACPMCOMBINEDHEATDHW.  The PERFORMANCEMAP must have grid variables outdoor drybulb and compressor speed (in that order) and lookup values of net capacity ratios and COP.  See example in PERFORMANCEMAP.

{{
  member_table({
    "units": "",
    "legal_range": "Name of a PERFORMANCEMAP", 
    "default": "",
    "required": "if rsType specifies a performance map model",
    "variability": "Start of a run" 
  })
}}

### rsTypeAuxH

Type: choice

For rsType=ASHP, type of auxiliary heat.  Auxiliary heating is used when heatpump capacity is insufficient to maintain zone temperature and during reverse-cycle defrost operation (if rsDefrostModel=REVCYCLEAUX).  If rsTypeAuxH=Furnace, energy use for auxiliary heat is accumulated to end use HPBU of meter rsFuelMtr (if specified).  If rsTypeAuxH=Resistance, energy use for auxiliary heat is accumulated to end use HPBU of meter rsElecMtr (if specified).

{{
  csv_table("Choice, Description
NONE, No auxiliary heat
RESISTANCE, Electric resistance (aka strip heat)
FURNACE, Fuel-fired", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "See table above", 
    "default": "RESISTANCE",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsCtrlAuxH

Type: choice

For rsType=ASHP, type of auxiliary heating control.

{{
  csv_table("Choice, Description
LOCKOUT, Compressor locked out if any auxiliary heating control
CYCLE, Compressor runs continuously and auxiliary cycles
ALTERNATE, Alternates between compressor and auxiliary", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "See table above", 
    "default": "ALTERNATE if rsTypeAuxH=FURNACE else CYCLE",
    "required": "No",
    "variability": "Start of a run" 
  })
}}

### rsCapAuxH

Type: float

For rsType=ASHP, auxiliary heating capacity. If AUTOSIZEd, rsCapAuxH is set to the peak heating load evaluated at the heating design temperature (Top.heatDsTDbO).

{{
  member_table({
    "units": "Btu/hr",
    "legal_range": "*AUTOSIZE* or *x* ≥ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsAFUEAuxH

Type: float

For rsType=ASHP, auxiliary heat annualized fuel utilization efficiency.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "0.9 if rsTypeAuxH=FURNACE else 1",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsDefrostModel

Type: choice

  Selects modeling options for ASHP outdoor coil defrosting when 17 °F < TDbO < 45 °F.  In this temperature range, heating capacity and/or efficiency are typically reduced due to frost accumulation on the outdoor coil.  

{{
  csv_table("NONE,       Defrost is not modeled.  When 17 °F < TDbO < 45 °F&comma; capacity and efficiency are determined by interpolation using unmodified 17 °F and 47 °F data.
  REVCYCLE,   Reverse compressor (cooling) operation.  Net capacity and efficiency is derived from rsCap17/rsCOP17 and rsCap35/rsCOP35 using linear interpolation.  Auxiliary heat is not modeled.
  REVCYCLEAUX,  Reverse compressor (cooling) operation with provision of sufficient auxiliary heat to make up the loss of heating capacity.  Auxiliary heating is typically used to prevent cold air delivery to zones during the defrost cycle.")
}}

{{
  member_table({
    "units": "",
    "legal_range": "*one of above choices*", 
    "default": "REVCYCLEAUX",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsSHRtarget

Type: float

Nominal target for sensible heat ratio (for fancoil).

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "0.7",
    "required": "No",
    "variability": "subhour" 
  })
}}

### rsFxCapAuxH

Type: float

  Auxiliary heating autosizing capacity factor. If AUTOSIZEd, rsCapAuxH is set to rsFxCapAuxH $\times$ (peak design-day load). Peak design-day load is the heating capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsSEER

Type: float

Cooling rated Seasonal Energy Efficiency Ratio (SEER).

{{
  member_table({
    "units": "Btu/Wh",
    "legal_range": "*x* $>$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### rsEER

Type: float

Cooling Energy Efficiency Ratio (EER) at standard AHRI rating conditions (outdoor drybulb of 95 °F and entering air at 80 °F drybulb and 67 °F wetbulb). For rsType=WSHP, rated EER at fluid source temperature = 86 °F.

{{
  member_table({
    "units": "Btu/Wh",
    "legal_range": "*x* $>$ 0", 
    "default": "Estimated from SEER unless WSHP",
    "required": "Yes for WSHP else No",
    "variability": "constant" 
  })
}}

### rsCapC

Type: float

Net cooling capacity at standard rating conditions (outdoor drybulb temperature = 95 °F for air source or fluid source temperature = 86 °F for water source).

If rsType=ASHP and both rsCapC and rsCap47 are autosized, both are set to the larger consistent value using rsCapRat9547 (after application of rsFxCapH and rsFxCapC).

If rsType=WSHP and both rsCapC and rsCapH are autosized, both are set to the larger consistent value using rsCapRatCH (after application of rsFxCapH and rsFxCapC).

{{
  member_table({
    "units": "Btu/hr",
    "legal_range": "*AUTOSIZE* or *x* ≤ 0 (x $>$ 0 coverted to $<$ 0)", 
    "default": "*none*",
    "required": "Yes if rsType includes cooling",
    "variability": "constant" 
  })
}}

### rsTdDesC

Type: float

Nominal cooling temperature fall (across system, not zone) used during autosizing (when capacity is not yet known).

{{
  member_table({
    "units": "°F",
    "legal_range": "*x* $<$ 0", 
    "default": "-25",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFxCapC

Type: float

Cooling autosizing capacity factor. rsCapC is set to rsFxCapC $\times$ (peak design-day load). Peak design-day load is the cooling capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1.4",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFChgC, rsFChg

Type: float

Cooling compressor charge factor.  The cooling gross (compressor only) COP is modified by rsFChgC to account for incorrect refrigerant charge.  Note: rsFChg is the prior name for this value and is supported for backwards compatibility.


{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1 (no effect)",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsVFPerTon

Type: float

Standard air volumetric flow rate per nominal ton of cooling capacity.

{{
  member_table({
    "units": "cfm/ton",
    "legal_range": "150 ≤ x ≤ 500", 
    "default": "350",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsFanPwrC

Type: float

Cooling fan power.

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "*x* ≥ 0", 
    "default": "0.365",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsASHPLockOutT

Type: float

  Source air dry-bulb temperature below which the air source heat pump compressor does not operate.

{{
  member_table({
    "units": "°F",
    "legal_range": "", 
    "default": "(no lockout)",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsCdH

Type: float

  Heating cyclic degradation coefficient, valid only for compressor-based heating (heat pumps).

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ x ≤ 0.5", 
    "default": "ASHPHYDRONIC: 0.25 ASHP: derived from rsHSPF",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsCdC

Type: float

Cooling cyclic degradation coefficient, valid for configurations having compressor-based cooling.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ x ≤ 0.5", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsFEffH

Type: float

Heating efficiency factor.  At each time step, the heating efficiency is multiplied by rsFEffH.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### rsFEffAuxHBackup

Type: float

Backup auxiliary heating efficiency factor.  At each time step, the backup heating efficiency is multiplied by rsFEffAuxHBackup. Backup auxiliary heating is typically provided by electric resistance "strip heat" but may be provided by a furnace (see rsTypeAuxH).  If rsTypeAuxH is not "none", backup heat operates when air source heat pump compressor capacity is insufficient to meet heating load.  See also rsFEffAuxHDefrost.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### rsFEffAuxHDefrost

Type: float

Defrost auxiliary heating efficiency factor.  At each time step, the defrost auxiliary heating efficiency is multiplied by rsFEffAuxHDefrost.  Defrost auxiliary heating is  typically provided by electric resistance "strip heat" but may be provided by a furnace (see rsTypeAuxH).  If rsDefrostModel=REVCYCLEAUX, defrost auxiliary heat operates during air source heat pump defrost mode.  Since defrost and backup heating are generally provided by the same equipment, rsFEffAuxHDefrost and rsFEffAuxHBackup are usually set to the same value, but separate inputs are available for special cases.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### rsFEffC

Type: float

Cooling efficiency factor.  At each time step, the cooling efficiency is multiplied by rsEffC.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### rsCapNomH

Type: float

Heating nominal capacity.  Provides type-independent probe source for RSYS heating capacity.  Daily variability is specified to support value changes during AUTOSIZEing.  Values set via input are typically constant.

{{
  member_table({
    "units": "Btu/hr",
    "legal_range": "*x* ≥ 0", 
    "default": "no heating: 0 heat pump: rsCap47 (input or AUTOSIZEd) other: rsCapH (input or AUTOSIZEd)",
    "required": "No",
    "variability": "daily" 
  })
}}

### rsCapNomC

Type: float

Cooling nominal capacity.  Provides type-independent probe source for RSYS cooling capacity.  Daily variability is specified to support value changes during AUTOSIZEing.  Values set via input are typically constant.

{{
  member_table({
    "units": "Btu/hr",
    "legal_range": "*x* ≥ 0", 
    "default": "no cooling: 0 other: rsCap95 (input or AUTOSIZEd)",
    "required": "No",
    "variability": "daily" 
  })
}}

### rsDSEH

Type: float

Heating distribution system efficiency.  If given, (1-rsDSEH) of RSYS heating output is discarded.  Cannot be combined with more detailed DUCTSEG model.


{{
  member_table({
    "units": "",
    "legal_range": "0 < *x* < 1", 
    "default": "(use DUCTSEG model)",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsDSEC

Type: float

Cooling distribution system efficiency.  If given, (1-rsDSEC) of RSYS cooling output is discarded.  Cannot be combined with more detailed DUCTSEG model.


{{
  member_table({
    "units": "",
    "legal_range": "0 < *x* < 1", 
    "default": "(use DUCTSEG model)",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsOAVType

Type: choice

  Type of central fan integrated (CFI) outside air ventilation (OAV) included in this RSYS.  OAV systems use the central system fan to circulate outdoor air (e.g. for night ventilation).

  OAV cannot operate simultaneously with whole building ventilation (operable windows, whole house fans, etc.).  Availability of ventilation modes is controlled on an hourly basis via  [Top ventAvail][top-model-control-items].

{{
  csv_table("NONE,       No CFI ventilation capabilities
FIXED,      Fixed-flow CFI (aka SmartVent). The specified rsOAVVfDs is used whenever the RSYS operates in OAV mode.
VARIABLE,   Variable-flow CFI (aka NightBreeze). Flow rate is determined at midnight based on prior day's average dry-bulb temperature according to a control algorithm defined by the NightBreeze vendor.")
}}

{{
  member_table({
    "units": "",
    "legal_range": "NONE, FIXED, VARIABLE", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsOAVVfDs

Type: float

 Design air volume flow rate when RSYS is operating in OAV mode.


{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* ≥ 0", 
    "default": "*none*",
    "required": "if rsOAVType ≠ NONE",
    "variability": "constant" 
  })
}}

### rsOAVVfMinF

Type: float

 Minimum air volume flow rate fraction when RSYS is operating in OAV mode.  When rsOAVType=VARIABLE, air flow rate is constrained to rsOAVVfMinF * rsOAVVfDs or greater.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ x ≤ 1", 
    "default": "0.2",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsOAVFanPwr

Type: float

 RSYS OAV-mode fan power.

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "0 < x ≤ 5", 
    "default": "per rsOAVTYPE FIXED: rsFanPwrC VARIABLE: NightBreeze vendor curve based on rsOAVvfDs",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsOAVTDbInlet

Type: float

OAV inlet (source) air temperature.  Supply air temperature at the zone is generally higher due to fan heat.  Duct losses, if any, also alter the supply air temperature.

{{
  member_table({
    "units": "°F",
    "legal_range": "*x* ≥ 0", 
    "default": "Dry-bulb temperature from weather file",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsOAVTdiff

Type: float

 OAV temperature differential.  When operating in OAV mode, the zone set point temperature is max( znTD, inletT+rsOAVTdiff).  Small values can result in inadvertent zone heating, due to fan heat.

{{
  member_table({
    "units": "°F",
    "legal_range": "*x* $>$ 0", 
    "default": "5 °F",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsOAVReliefZn

Type: znName

Name of zone to which relief air is directed during RSYS OAV operation, typically an attic zone.  Relief air flow is included in the target zone's pressure and thermal balance.

{{
  member_table({
    "units": "",
    "legal_range": "*name of ZONE*", 
    "default": "*none*",
    "required": "if rsOAVType ≠ NONE",
    "variability": "constant" 
  })
}}

### rsParElec

Type: float

Parasitic electrical power.  rsParElec is unconditionally accumulated to end use AUX of rsElecMtr (if specified) and has no other effect.

{{
  member_table({
    "units": "W",
    "legal_range": "", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsParFuel

Type: float

Parasitic fuel use.  rsParFuel is unconditionally accumulated to end use AUX of sFuelMtr (if specified) and has no other effect.

{{
  member_table({
    "units": "Btuh",
    "legal_range": "", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### rsRhIn

Type: float

Entering air relative humidity (for model testing).

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": "Derived from entering air state",
    "required": "No",
    "variability": "constant" 
  })
}}

### rsTdbOut

Type: float

Air dry-bulb temperature at the outdoor portion of this system.

{{
  member_table({
    "units": "°F",
    "legal_range": "*x* ≥ 0", 
    "default": "From weather file",
    "required": "No",
    "variability": "hourly" 
  })
}}

### endRSYS

Optionally indicates the end of the RSYS definition.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**Related Probes:**

- @[rsys][p_rsys]
- @[RSYSRes][p_rsysres] (accumulated results)
