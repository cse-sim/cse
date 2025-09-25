# DHWSYS

DHWSYS constructs an object representing a domestic hot water system consisting of one or more hot water heaters, storage tanks, loops, and pumps (DHWHEATER, DHWTANK, DHWLOOP, and DHWPUMP, see below) and a distribution system characterized by loss parameters. This model is based on Appendix B of the 2019 Residential ACM Reference Manual and the Ecotope HPWHSim air source heat pump water heater model (called HPWH herein).

The parent-child structure of DHWSYS components is determined by input order. For example, DHWHEATERs belong to the DHWSYS that precedes them in the input file. The following hierarchy shows the relationship among components. Note that any of the commands can be repeated any number of times.

- DHWSYS
    - DHWHEATER
    - DHWLOOPHEATER
    - DHWHEATREC
    - DHWTANK
    - DHWPUMP
    - DHWLOOP
        - DHWLOOPPUMP
        - DHWLOOPSEG
            - DHWLOOPBRANCH

Minimal modeling is included for physically realistic controls. For example, if several DHWHEATERs are included in a DHWSYS, an equal fraction of the required hot water is assumed to be produced by each heater, even if they are different types or sizes. Thus a DHWSYS is in some ways a collection of components as opposed to an explicitly connected system.  This approach avoids requiring detailed input that would impose impractical user burden, especially in compliance applications.

### dhwsysName

Optional name of system; give after the word “DHWSYS” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsCalcMode

Type: choice

Enables preliminary simulation that derives values needed for simulation.

{{
  csv_table("PRERUN,      Calculate hot water heating load; at end of run&comma; derive whLDEF for all child DHWHEATERs for which that value is required and defaulted (this emulates methods used in the T24DHW.DLL implementation of CEC DHW procedures). Also derived are average number of draws per day by end use (used in the wsDayWaste scheme).
  SIMULATE,    Perform full modeling calculations")
}}

To use PRERUN efficiently, the recommended input file structure is:

- General input
- DHWSYS(s) and child objects
- RUN
- ALTER DHWSYS input (as needed)
- Building input
- RUN

This order avoids duplicate time-consuming simulation of the full building model.

{{
  member_table({
    "units": "",
    "legal_range": "*Codes listed above*", 
    "default": "SIMULATE",
    "required": "No",
    "variability": "" 
  })
}}

### wsCentralDHWSYS

Type: dhwsysName

  Name of the central DHWSYS that serves this DHWSYS, allowing representation of multiple units having distinct distribution configurations and/or water use patterns but served by a central DHWSYS.  The child DHWSYS(s) may not include DHWHEATERs -- they are "loads only" systems.  wsCentralDHWSYS and wsLoadShareDHWSYS cannot both be given.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a DHWSYS*", 
    "default": "DHWSYS is standalone",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsLoadShareDHWSYS

Type: dhwsysName

 Name of a DHWSYS that serves the same loads as this DHWSYS, allowing representation of multiple water heating systems within a unit. If given, wsUse and wsDayUse are not allowed, hot water requirements are derived from the referenced DHWSYS.  wsCentralDHWSYS and wsLoadShareDHWSYS cannot both be given.

 For example, two DHWSYSs should be defined to model two water heating systems serving a load represented by wsDayUse DayUseTyp.  Each DHWSYS should include DHWHEATER(s) and other components as needed.  DHWSYS Sys1 should specify wsDayUse=DayUseTyp and DHWSYS Sys2 should have wsLoadShareDHWSYS=Sys1 in place of wsDayUse.

 Loads are shared by assigning DHWUSE events sequentially by end use to all DHWSYS with compatible fixtures (determined by wsFaucetCount, wsShowerCount etc., see below) in the group.  This algorithm approximately divides load for each end use by the number of compatible fixtures in the group.  In addition, assigning 0 to a fixture type prevents assignment of an end use load to a DHWSYS -- for example, wsDWashrCount=0 could be provided for a DHWSYS that does not serve a kitchen.


{{
  member_table({
    "units": "",
    "legal_range": "*name of a DHWSYS*", 
    "default": "No shared loads",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsMult

Type: float

Number of identical systems of this type (including all child objects). Any value $> 1$ is equivalent to repeated entry of the same DHWSYS.  A value of 0 is equivalent to omitting the DHWSYS.  Non-integral values scale all results; this may be useful in parameterized models, for example.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsFaucetCount, wsShowerCount, wsBathCount, wsCWashrCount, wsDWashrCount

Type: integer

Specifies the count of fixtures served by this DHWSYS that can accommodate draws of each end use (see DHWUSE).  These counts are used for distributing draws in shared load configurations (multiple DHWSYSs serving the same loads, see wsLoadShareDHWSYS above).

In addition, wsShowerCount participates in assignment of Shower draws to DHWHEATRECs (if any).

Unless this DHWSYS is part of a shared-load group or includes DHWHEATREC(s), these counts have no effect and need not be specified.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsTInlet

Type: float

Specifies cold (mains) water temperature supplying this DHWSYS.  DHWHEATER supply water temperature wsTInlet adjusted (increased) by any DHWHEATREC recovered heat and application of wsSSF (approximating solar preheating).

{{
  member_table({
    "units": "^o^F",
    "legal_range": "> 32 ^o^F", 
    "default": "Mains temp from weather file",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsTInletTest

Type: float

Overides at the substep interval the cold (mains) water temperature supplying this DHWSYS.

CAUTION: wsTInletTest is intended for testing and model validation studies and should not be generally used. It is not fully supported for all DHWSYS configurations.  wsTInletTest is allowed only for configurations using HPWH-based DHWHEATERs (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX).  

{{
  member_table({
    "units": "^o^F",
    "legal_range": "> 32 ^o^F", 
    "default": "",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsTInletDes

Type: float

Cold water inlet design temperature for sizing.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "x > 32 ^o^F", 
    "default": "Annual minimums mains temperature",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsUse

Type: float

Hourly hot water use (at the point of use).  See further info under wsDayUse.

{{
  member_table({
    "units": "gal",
    "legal_range": "x ≥ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsUseTest

Type: float

Additional substep hot water use added to draw(s) specfied by wsHWUse and wsDayUse.

CAUTION: wsUseTest is intended for testing and model validation studies and should not be generally used. It is not fully supported for all DHWSYS configurations.  wsUseTest is allowed only for configurations using HPWH-based DHWHEATERs (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX).  

{{
  member_table({
    "units": "gal",
    "legal_range": "x ≥ 0", 
    "default": "",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsDayUse

Type: dhwdayuseName

  Name of DHWDAYUSE object that specifies a detailed schedule of mixed water use at points of hot water use (that is, "at the tap").  The mixed water amounts are used to derive hot water requirements based on specified mixing fractions or mixed water temperature (see DHWDAYUSE and DHWUSE).

  The total water use modeled by CSE is the sum of amounts given by wsUse and the DWHDAYUSE schedule.  DHWDAYUSE draws are resolved to minute-by-minute bins compatible with the HPWH model and wsUse/60 is added to each minute bin.  Conversely, the hour total of the DHWDAYUSE amounts is included in the draw applied to non-HPWH DHWHEATERs.

  wsDayUse variability is daily, so it is possible to select different schedules as a function of day type (or any other condition), as follows --

         DHWSYS "DHW1"
           ...
           wsDayUse = choose( $isWeHol, "DUSEWeekday", "DUSEWeHol")
           ...

  Note that while DHWDAYUSE selection is updated daily, the DHWUSE values within the DHWDAYUSE can be altered hourly, providing additional scheduling flexibility.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a DHWDAYUSE*", 
    "default": "(no scheduled draws)",
    "required": "No",
    "variability": "daily" 
  })
}}

### wsFaucetDrawDurF

Type: float

Water heater draw duration factor for faucets. Defined as the ratio of the actual draw duration (including time waiting for hot water to arrive at the fixture) to the nominal draw duration (as though hot water was instantly available).

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsShowerDrawDurF

Type: float

Water heater draw duration factor for showers. Defined as the ratio of the actual draw duration (including time waiting for hot water to arrive at the fixture) to the nominal draw duration (as though hot water was instantly available).

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsBathDrawDurF

Type: float

Water heater draw duration factor for baths. Defined as the ratio of the actual draw duration (including time waiting for hot water to arrive at the fixture) to the nominal draw duration (as though hot water was instantly available).

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsCWashrDrawDurF

Type: float

Water heater draw duration factor for clothes washers. Defined as the ratio of the actual draw duration (including time waiting for hot water to arrive at the fixture) to the nominal draw duration (as though hot water was instantly available).

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsDWashrDurF

Type: float

Water heater draw duration factor for dishwashers. Defined as the ratio of the actual draw duration (including time waiting for hot water to arrive at the fixture) to the nominal draw duration (as though hot water was instantly available).

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsUnkDrawDurF

Type: float

Water heater draw duration factor for unknown end use. Defined as the ratio of the actual draw duration (including time waiting for hot water to arrive at the fixture) to the nominal draw duration (as though hot water was instantly available).

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsFaucetDrawWaste

Type: float

Draw water waste for faucets. Specifies additional draw volume per DHWUSE event (at fixture, by end use).  This can be used to account for water discarded during warmup or otherwise adjust the draw volume.  Because the values are at the fixture, the impact on hot water demand additionally depends on DHWUSE parameters.  The value is applied by lengthening (or shortening) the draw duration.

{{
  member_table({
    "units": "gal/draw",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsShowerDrawWaste

Type: float

Draw water waste for showers. Specifies additional draw volume per DHWUSE event (at fixture, by end use).  This can be used to account for water discarded during warmup or otherwise adjust the draw volume.  Because the values are at the fixture, the impact on hot water demand additionally depends on DHWUSE parameters.  The value is applied by lengthening (or shortening) the draw duration.

{{
  member_table({
    "units": "gal/draw",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsBathDrawWaste

Type: float

Draw water waste for baths. Specifies additional draw volume per DHWUSE event (at fixture, by end use).  This can be used to account for water discarded during warmup or otherwise adjust the draw volume.  Because the values are at the fixture, the impact on hot water demand additionally depends on DHWUSE parameters.  The value is applied by lengthening (or shortening) the draw duration.

{{
  member_table({
    "units": "gal/draw",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsCWashrDrawWaste

Type: float

Draw water waste for clothes washers. Specifies additional draw volume per DHWUSE event (at fixture, by end use).  This can be used to account for water discarded during warmup or otherwise adjust the draw volume.  Because the values are at the fixture, the impact on hot water demand additionally depends on DHWUSE parameters.  The value is applied by lengthening (or shortening) the draw duration.

{{
  member_table({
    "units": "gal/draw",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsDWashrDrawWaste

Type: float

Draw water waste for dishwashers. Specifies additional draw volume per DHWUSE event (at fixture, by end use).  This can be used to account for water discarded during warmup or otherwise adjust the draw volume.  Because the values are at the fixture, the impact on hot water demand additionally depends on DHWUSE parameters.  The value is applied by lengthening (or shortening) the draw duration.

{{
  member_table({
    "units": "gal/draw",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsUnkDrawWaste

Type: float

Draw water waste for unknown end use. Specifies additional draw volume per DHWUSE event (at fixture, by end use).  This can be used to account for water discarded during warmup or otherwise adjust the draw volume.  Because the values are at the fixture, the impact on hot water demand additionally depends on DHWUSE parameters.  The value is applied by lengthening (or shortening) the draw duration.

{{
  member_table({
    "units": "gal/draw",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Hourly" 
  })
}}

### wsTRLTest

Type: float

Circulation loop return temperature for testing and validation.

{{
  member_table({
    "units": "F",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Subhourly" 
  })
}}

### wsVolRLTest

Type: float

Circulation loop volume flow rate for testing and validation.

{{
  member_table({
    "units": "gpm",
    "legal_range": "x ≥ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "Subhourly" 
  })
}}


### wsBranchModel

Type: choice

Branch model selection.

{{
  csv_table("wsBranchModel,Description
T24DHW,Model in appendix B of the Alternative Compliance Manual
DRAWWASTE,Draw duration increase per draw waste
DAYWASTE,draw duration increase per day waste", True)
}}

### wsDayWasteVol

Type: float

Average amount of waste per day.

{{
  member_table({
    "units": "gal/day",
    "legal_range": "x ≥ 0", 
    "default": "wsDayWasteBranchVolF * (Total DHWLOOPBRANCH vol)",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsDayWasteBranchVolF

Type: float

Day waste scaling factor.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsFaucetDayWasteF

Type: float

Relative faucet water draw for day of waste scheme.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsShowerDayWasteF

Type: float

Relative shower water draw for day of waste scheme.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsBathDayWasteF

Type: float

Relative bath water draw for day of waste scheme.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsCWashrDayWasteF

Type: float

Relative clothes washer water draw for day of waste scheme.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsDWashrDayWasteF

Type: float

Relative dish washer water draw for day of waste scheme.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsUnkDayWasteF

Type: float

Unknown relative water draw for day of waste scheme.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsTUse

Type: float

Hot water delivery temperature (at output of water heater(s) and at point of use).  Delivered water is mixed down to wsTUSe (with cold water) or heated to wsTUse (with extra electric resistance backup, see DHWHEATER whXBUEndUse).  Note that draws defined via DHWDAYUSE / DHWUSE can specify mixing to a lower temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "> 32 ^o^F", 
    "default": "120",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsTUseTest

Type: float

Overides at the substep interval the hot water delivery temperature.

CAUTION: wsTUseTest is intended for testing and model validation studies and should not be generally used. It is not fully supported for all DHWSYS configurations.  wsTUseTest is allowed only for configurations using HPWH-based DHWHEATERs (whHeatSrc=ASHPX or whHeatSrc=RESISTANCEX).  

{{
  member_table({
    "units": "^o^F",
    "legal_range": "> 32 ^o^F", 
    "default": "",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### wsTSetPoint

Type: float

  Specifies the hot water setpoint temperature for all child DHWHEATERs.  Used only for HPWH-based DHWHEATERs (HPWH models tank temperatures and heating controls), otherwise has no effect.  wsTSetpoint can be modified hourly to achieve load-shifting effects.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "> 32 ^o^F", 
    "default": "wsTUse",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsTSetPointLH

Type: float

  Specifies the hot water set point temperature for all child DHWLOOPHEATERs.  Used only for HPWH-based DHWHLOOPEATERs (HPWH explicitly models tank temperatures and heating controls), otherwise has no effect.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "> 32 ^o^F", 
    "default": "wsTSetPoint",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsTSetpointDes

Type: float

Specifies the design (sizing) set point temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "x > 32 ^o^F", 
    "default": "wsTUse",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsVolRunningDes

Type: float

Running volume for design. Active volume (above aquastat) equals to a full tank volume, defaults from EcoSizer at end of prerun if not input. No direct use, must be passed to DHWHEATER via ALTER.

{{
  member_table({
    "units": "gal",
    "legal_range": "x > 0", 
    "default": "0.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsASHPTSrcDes

Type: float

Design (sizing) source air temperature for HPWH DHWHEATERs.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "x > 32 ^o^F", 
    "default": "Heating design temperature",
    "required": "No",
    "variability": "At the start and at the end of interval" 
  })
}}

### wsFxDes

Type: float

Excess size factor for domestic hot water design. wsFxDes is applied when wsHeatingCapDes and/or wsVolRunningDes are defaulted from EcoSizer at the end of the prerun. There is no effect if those values are input.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "1.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsDRMethod

Type: choice

Selects alternative control schemes for HPWH-based DHWHEATERs.  These allow shifting primary heater (compressor or resistance element) operation to times of day that have load-management advantages.

{{
  csv_table("wsDRMethod, Description
NONE, None (default setpoint-based control)
SCHEDULE, Demand response schedule (see wsDRSignal)
STATEOFCHARGE, State-of-charge (see wsTargetSOC)", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "See table above", 
    "default": "NONE",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsDRSignal

Type: choice

When (and only when) wsDRMethod=SCHEDULE, wsDRSignal allows hourly specification of modified control schemes.  Available signals are:

{{
  csv_table("wsDRSignal, Description
ON, Normal operation following the water heater's internal control logic.
TOO, Tops off the tank once by engaging the all the available heating sources (compressor and resistive elements) in the water heater to heat the tank to setpoint (regardless of the current condition).
TOOLOR, Tops off the tank once and locks out the resistance elements (only the compressor is used to heat the tank to setpoint).
TOOLOC, Tops off the tank once and locks out the compressor (only the resistance elements are used to heat the tank to setpoint).
TOT, Tops off the tank on a timer using all the available heating sources (compressor and resistive elements) in the water heater. The tank starts a timer and heating to setpoint when the call is received and repeats the heating call when the timer reaches zero.
TOTLOR, Tops of the tank on a timer and locks out the resistance elements (only the compressor is used to heat the tank to setpoint).
TOTLOC, Tops of the tank on a timer and locks out the compressor (only the resistance elements are used to heat the tank to setpoint).
LOC, Locks out the compressor from the water heater's normal internal control logic.
LOR, Locks out the resistive elements from the water heater's normal internal control logic.
LOCLOR, Locks out the compressor and resistive elements from the water heater's normal internal control logic.", True)
}}

Scheduling functions can be used to construct control strategies of interest, for example:

```
wsDRSignal = $isWeHol
  ? hourval( on,  on,  on,  on,  on,  on,  on,  on,  on,  on,  on,  on,
            on,  on,  on,  on,  on,  on, TOO, LOC, LOR,  on,  on,  on)
  : hourval( on,  on,  on,  on,  on,     on,     on,  on,  on,  on,  on,  on,
            on,  on,  on,  on,  TOOLOR, TOOLOR, LOC, LOR, LOR, LOR,  on,  on)
```

Note also that wsTSetpoint can be also be modified hourly to achieve load-shifting effects.

{{
  member_table({
    "units": "",
    "legal_range": "See Table above", 
    "default": "ON",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsTargetSOC

Type: float

When (and only when) wsDRMethod=STATEOFCHARGE, wsTargetSOC specifies the target fraction of maximum tank heat content.  The tank is deemed fully charged when its entire contents is at wsTSetpoint and 0 charged at 110 ^o^F.  Schedules are used to indicate anticipated heat requirements. The STATEOFCHARGE method can be used in combined heat / DHW systems (see RSYS rsType=COMBINEDHEATDHW) when there is excess capacity during summer months, as shown in the following:

```
wsTargetSOC = select(
  $month > 11 || $month < 3,
      hourval(.70,.70,.70,.70,.70,.70,.70,.30,.95,.95,.95,.95,
              .95,.95,.95,.95,.95,.70,.70,.70,.70,.70,.70,.70),
  $month==3 || $month== 4,
      hourval(.50,.50,.50,.50,.50,.50,.50,.30,.95,.95,.95,.95,
              .95,.95,.95,.95,.95,.50,.50,.50,.50,.50,.50,.50),
  default
      hourval(.15,.15,.15,.15,.15,.15,.15,.15,.15,.15,.15,.15,
              .15,.60,.60,.60,.15,.15,.15,.15,.15,.15,.15,.15))
```

{{
  member_table({
    "units": "",
    "legal_range": "0 < x ≤ 1", 
    "default": "0.9",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsSDLM

Type: float

Specifies the standard distribution loss multiplier. See App B Eqn 4. To duplicate CEC 2019 methods, this value should be set according to the value derived with App B Eqn 5.

{{
  member_table({
    "units": "",
    "legal_range": "> 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsDSM

Type: float

Distribution system multiplier. See RACM App B Eqn 4. To duplicate CEC 2016 methods, wsDSM should be set to the appropriate value from App B Table B-2. Note the NCF (non-compliance factor) included in App B Eqn 4 is *not* a CSE input and thus must be applied externally to wsDSM.

{{
  member_table({
    "units": "",
    "legal_range": "> 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsWF

Type: float

Waste factor. See RACM App B Eqn 1. wsWF is applied to hot water draws.  The default value (1) reflects the inclusion of waste in draw amounts.  App B specifies wsWF=0.9 when the system has a within-unit pumped loop that reduces waste due to immediate availability of hot water at fixtures.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "1",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsSSF

Type: float

NOTE: Deprecated. Use wsSolarSys instead.

Specifies the solar savings fraction, allowing recognition of externally-calculated solar water heating energy contributions.  The contributions are modeled by deriving an increased water heater feed temperature --

$$
tWHFeed = tInletAdj + wsSSF*(wsTUse-tInletAdj)
$$

where tInletAdj is the source cold water temperature *including any DHWHEATREC tempering* (that is, wsTInlet + heat recovery temperature increase, if any).  This model approximates the diminishing returns associated with combined preheat strategies such as drain water heat recovery and solar.


{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ x ≤ 0.99", 
    "default": "",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsSolarSys

Type: dhwSolarSys

Name of DHWSOLARSYS object, if any, that supplies pre-heated water to this DHWSYS.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a DHWSOLARSYS*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsParElec

Type: float

Specifies electrical parasitic power to represent recirculation pumps or other system-level electrical devices. Calculated energy use is accumulated to the METER specified by wsElecMtr (end use DHW). No other effect, such as heat gain to surroundings, is modeled.

{{
  member_table({
    "units": "W",
    "legal_range": "x ≥ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wsDrawMaxDur

Type: integer

Maximum draw duration for the sizing window.

{{
  member_table({
    "units": "Hr",
    "legal_range": "x ≥ 0", 
    "default": "4",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsLoadMaxDur

Type: integer

Maximum load duration for the sizing window.

{{
  member_table({
    "units": "Hr",
    "legal_range": "x ≥ 0", 
    "default": "12",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsElecMtr

Type: mtrName

Name of METER object, if any, to which DHWSYS electrical energy use is recorded (under end use DHW). In addition, wsElecMtr provides the default whElectMtr selection for all DHWHEATERs and DHWPUMPs in this DHWSYS.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsFuelMtr 

Type: *mtrName*

Name of METER object, if any, to which DHWSYS fuel energy use is recorded (under end use DHW). DHWSYS fuel use is usually (always?) 0, so the primary use of this input is to specify the default whFuelMtr choice for DHWHEATERs in this DHWSYS.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsWHhwMtr

Type: dhwmtrName

Name of DHWMETER object, if any, to which hot water quantities (at water heater) are recorded by hot water end use.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsFXhwMtr 

Type: *dhwmtrName*

Name of DHWMETER object, if any, to which mixed hot water use (at fixture) quantities are recorded by hot water end use.  DHWDAYUSE and wsUse input can be verified using DHWMETER results.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "",
    "required": "No",
    "variability": "constant" 
  })
}}

### wsWriteDrawCSV

Type: choice

  If Yes, a comma-separated file is generated containing 1-minute interval hot water draw values for testing or linkage purposes.


{{
  member_table({
    "units": "",
    "legal_range": "*Yes or No*", 
    "default": "No",
    "required": "No",
    "variability": "constant" 
  })
}}

### endDHWSys

Optionally indicates the end of the DHWSYS definition.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "No",
    "variability": "" 
  })
}}

**Related Probes:**

- @[DHWSys][p_dhwsys]
