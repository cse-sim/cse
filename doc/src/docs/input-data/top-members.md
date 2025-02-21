# TOP Members

The top-level data items (TOP members) control the simulation process or contain data that applies to the modeled building as a whole. No statement is used to begin or create the TOP object; these statements can be given anywhere in the input (they do, however, terminate any other objects being specified -- ZONEs, REPORTs, etc.).

## TOP General Data Items

**doMainSim=_choice_**

Specifies whether the simulation is performed when a Run command is encountered. See also doAutoSize.

<%= member_table(
units: "",
legal_range: "NO, YES",
default: "_YES_",
required: "No",
variability: "constant") %>

**begDay=_date_**

Date specifying the beginning day of the simulation performed when a Run command is encountered. See further discussion under endDay (next).

<%= member_table(
units: "",
legal_range: "_date_",
default: "_Jan 1_",
required: "No",
variability: "constant") %>

**endDay=_date_**

Date specifying the ending day of the simulation performed when a Run command is encountered.

The program simulates 365 days at most. If begDay and endDay are the same, 1 day is simulated. If begDay precedes endDay in calendar sequence, the simulation is performed normally and covers begDay through endDay inclusive. If begDay follows endDay in calendar sequence, the simulation is performed across the year end, with Jan 1 immediately following Dec 31.

<%= member_table(
units: "",
legal_range: "_date_",
default: "_Dec 31_",
required: "No",
variability: "constant") %>

**jan1DoW=_choice_**

Day of week on which January 1 falls. jan1DoW is used in the calculation of the day of the week.

Note that "warm-up" days (see wuDays) occur before the start day specified by begDay. Thus "warm-up" days are often in the prior year. In order to preserve the day-of-week sequence, the effective jan1DoW is shifted back by one day during prior-year warmup.

<%= member_table(
units: "",
legal_range: "SUN,\\ MON,\\ TUE,\\ WED,\\ THU,\\ FRI,\\ SAT",
default: "THU",
required: "No",
variability: "constant") %>

**workDayMask=_int_ TODO**

<%= member_table(
units: "",
legal_range: "",
default: "",
required: "",
variability: "constant") %>

**wuDays=_int_**

Number of "warm-up" days used to initialize the simulator. Simulator initialization is required because thermal mass temperatures are set to arbitrary values at the beginning of the simulation. Actual mass temperatures must be established through simulation of a few days before thermal loads are accumulated. Heavier buildings require more warm-up; the default values are adequate for conventional construction.

<%= member_table(
units: "",
legal_range: "0 $\\le$ _x_ $\\le$ 365",
default: "7",
required: "No",
variability: "constant") %>

**nSubSteps=_int_**

Number of subhour steps used per hour in the simulation. 4 is the time-honored value for models using CNE zones. A value of 30 is typically for CSE zone models.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "4",
required: "No",
variability: "constant") %>

**nSubhrTicks=_int_**

Number of subhour ticks used per _nSubSteps_ for DHWSYS simulation.

_Note:_ This input is currently used only for experimental purposes.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "60/nSubSteps",
required: "No",
variability: "constant") %>

**tol=_float_**

Endtest convergence tolerance for internal iteration in CNE models (no effect for CSE models) Small values for the tolerance cause more accurate simulations but slower performance. The user may wish to use a high number during the initial design process (to quicken the runs) and then lower the tolerance for the final design (for better accuracy). Values other than .001 have not been explored.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.001",
required: "No",
variability: "constant") %>

**humTolF=_float_**

Specifies the convergence tolerance for humidity calculations in CNE models (no effect in for CSE models), relative to the tolerance for temperature calculations. A value of .0001 says that a humidity difference of .0001 is about as significant as a temperature difference of one degree. Note that this is multiplied internally by "tol"; to make an overall change in tolerances, change "tol" only.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "0.0001",
required: "No",
variability: "constant") %>

**ebTolMon=_float_**

Monthly energy balance error tolerance for internal consistency checks. Smaller values are used for testing the internal consistency of the simulator; values somewhat larger than the default may be used to avoid error messages when it is desired to continue working despite a moderate degree of internal inconsistency.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.0001",
required: "No",
variability: "constant") %>

**ebTolDay=_float_**

Daily energy balance error tolerance.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.0001",
required: "No",
variability: "constant") %>

**ebTolHour=_float_**

Hourly energy balance error tolerance.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.0001",
required: "No",
variability: "constant") %>

**ebTolSubhr=_float_**

Sub-hourly energy balance error tolerance.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.0001",
required: "No",
variability: "constant") %>

**unMetTzTol=_float_**

Zone temperature unmet load tolerance. At the end of each subhour, if a conditioned zone temperature is more than unMetTzTol below the current heating setpoint or more than unMetTzTol above the current cooling setpoint, "unmet load" time is accumulated.

<%= member_table(
units: "^o^F",
legal_range: "x $\\geq$ 0",
default: "1 ^o^F",
required: "No",
variability: "constant") %>

**unMetTzTolWarnHrs=_float_**

Unmet load warning threshold. A warning message is issued for each zone having more than unMetTzTolWarnHrs unmet heating or cooling loads.

<%= member_table(
units: "hr",
legal_range: "x $\\geq$ 0",
default: "150",
required: "No",
variability: "constant") %>

**grndMinDim=_float_**

The minimum cell dimension used in the two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "ft",
legal_range: "x $>$ 0",
default: "0.066",
required: "No",
variability: "constant") %>

**grndMaxGrthCoeff=_float_**

The maximum ratio of growth between neighboring cells in the direction away from the near-field area of interest. Used in the two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "",
legal_range: "x $\\geq$ 1.0",
default: "1.5",
required: "No",
variability: "constant") %>

**grndTimeStep=_choice_**

Allows the user to choose whether to calculate foundation conduction on hourly or subhourly intervals. Hourly intervals require less overall computation time, but with less accuracy.

<%= member_table(
units: "",
legal_range: "HOURLY, SUBHOURLY",
default: "HOURLY",
required: "No",
variability: "constant") %>

<!--NOTE: Technically, this can probably change hourly -->

**humMeth=_choice_**

Developmental zone humidity computation method choice for CNE models (no effect for CSE models).

<%= csv_table(<<END, :row_header => false)
ROB, Rob's backward difference method. Works well within limitations of backward difference approach.
PHIL, Phil's central difference method. Should be better if perfected*coma* but initialization at air handler startup is unresolved\*coma and ringing has been observed.
END
%>

<%= member_table(
units: "",
legal_range: "ROB, PHIL",
default: "ROB",
required: "No",
variability: "constant") %>

**dflExH=_float_**

Default exterior surface (air film) conductance used for opaque and glazed surfaces exposed to ambient conditions in the absence of explicit specification.

<%= member_table(
units: "Btuh/ft^2^-^o^F",
legal_range: "_x_ $>$ 0",
default: "2.64",
required: "No",
variability: "constant") %>

**bldgAzm=_float_**

Reference compass azimuth (0 = north, 90 = east, etc.). All zone orientations (and therefore surface orientations) are relative to this value, so the entire building can be rotated by changing bldgAzm only. If a value outside the range 0^o^ $\leq$ _x_ $<$ 360^o^ is given, it is normalized to that range.

<%= member_table(
units: "^o^ (degrees)",
legal_range: "unrestricted",
default: "0",
required: "No",
variability: "constant") %>

**elevation=_float_**

Elevation of the building site. Used internally for the computation of barometric pressure and air density of the location.

<%= member_table(
units: "ft",
legal_range: "_x_ $\\ge$ 0",
default: "0 (sea level)",
required: "No",
variability: "constant") %>

**runTitle=_string_**

Run title for the simulation. Appears in report footers, export headers, and in the title lines to the INP, LOG, and ERR built-in reports (these appear by default in the primary report file; the ERR report also appears in the error message file, if one is created).

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "blank (no title",
required: "No",
variability: "constant") %>

**runSerial=_int_**

Run serial number for the simulation. Increments on each run in a session; appears in report footers. <!-- TODO: in future will be saved to status file. -->

<%= member_table(
units: "",
legal_range: "0 $\leq$ _x_ $\leq$ 999",
default: "0",
required: "No",
variability: "constant") %>

## TOP Daylight Saving Time Items

Daylight savings starts by default at 2:00 a.m. of the second Sunday in March. Internally, hour 3 (2:00-3:00 a.m.) is skipped and reports for this day show only 23 hours. Daylight savings ends by default at 2:00 a.m. of the first Sunday of November; for this day 25 hours are shown on reports. CSE fetches weather data using standard time but uses daylight savings time to calculate variable expressions (and thus all schedules).

**DT=_choice_**

Whether Daylight Savings Time is to be used for the current run.

<%= member_table(
units: "",
legal_range: "YES, NO",
default: "YES",
required: "No",
variability: "constant") %>

**DTbegDay=_date_**

Start day for daylight saving time (assuming DT=Yes)

<%= member_table(
units: "",
legal_range: "_date_",
default: "_second Sunday in March_",
required: "No",
variability: "constant") %>

**DTendDay=_date_**

End day for daylight saving time (assuming DT=Yes)

<%= member_table(
units: "",
legal_range: "_date_",
default: "_first Sunday in November_",
required: "No",
variability: "constant") %>

## TOP Model Control Items

**ventAvail=_choice_**

Indicates availability of outdoor ventilation strategies. CSE cannot model simultaneously-operating alternative ventilation strategies. For example, an RSYS central fan integrated (CFI) OAV system is never modeled while whole house fan ventilation is available. ventAvail controls which ventilation mode, if any, is available for the current hour. Note that mode availability means that the strategy could operate but may not operate due to other control assumptions.

<%= csv_table(<<END, :row_header => true)
Choice, Ventilation Strategy Available
NONE, None
WHOLEBUILDING, IZXFER (window and whole-house fan)
RSYSOAV, RSYS central fan integrated (CFI) outside air ventilation (OAV)
END
%>

As noted, ventAvail is evaluated hourly, permitting flexible control strategy modeling. The following example specifies that RSYSOAV (CFI) ventilation is available when the seven day moving average temperature is above 68 ^o^F, otherwise whole building ventilation is available between 7 and 11 PM, otherwise no ventilation.

    ventAvail = (@weather.taDbAvg07 > 68)    ? RSYSOAV
              : ($hour >= 19 && $hour <= 23) ? WHOLEBUILDING
              :                                NONE

<%= member_table(
units: "",
legal_range: "_Choices above_",
default: "WHOLEBUILDING",
required: "No",
variability: "hourly") %>

**exShadeModel=_choice_**

Specifies advanced exterior shading model used to evaluate shading of [PVARRAYs][pvarray] by [SHADEXs][shadex] or other PVARRAYs. Advanced shading is not implemented for building surfaces and this setting has no effect on walls or windows.

<%= csv_table(<<END, :row_header => true)
**Choice**, **Effect**
PENUMBRA, Calculate shading using the Penumbra model
NONE, Disable advanced shading calculations
END
%>

<%= member_table(
units: "",
legal_range: "_Choices above_",
default: "PENUMBRA",
required: "No",
variability: "constant") %>

**slrInterpMeth=_choice_**

Solar interpolation method.

<%= csv_table(<<END, :row_header => true)
Choice
CSE
TRNSYS
END
%>

<%= member_table(
units: "",
legal_range: "See table above",
default: "CSE",
required: "No",
variability: "constant") %>

**ANTolAbs=_float_**

AirNet absolute convergence tolerance. Ideally, calculated zone air pressures should be such that the net air flow into each zone is 0 -- that is, there should be a perfect mass balance. The iterative AirNet solution techniques are deemed converged when netAirMassFlow < max( ANTolAbs, ANTolRel\*totAirMassFlow).

<%= member_table(
units: "lbm/sec",
legal_range: "_x_ $>$ 0",
default: "0.00125 (about 1 cfm)",
required: "No",
variability: "constant") %>

**ANTolRel=_float_**

AirNet relative convergence tolerance. See AnTolAbs just above.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.0001",
required: "No",
variability: "constant") %>

**ANPressWarn=_float_**

AirNet pressure warning threshold. A warning message is issued when the absolute value of the AirNet-calculated zone pressure exceeds ANPressWarn. Note the default for ANPressWarn conservatively large. 10 lb/ft2 is about 500 pascals -- a pressure that is probably impossible in a building. The intent of this value is to alert the user to incorrect modeling inputs while avoiding excessive messages.

<%= member_table(
units: "lb/ft2",
legal_range: "x $\\gt$ 0",
default: "10",
required: "No",
variability: "constant") %>

**ANPressErr=_float_**

AirNet pressure error threshold. The simulation terminates with a message if the absolute value of any AirNet-calculated zone pressure exceeds ANPressErr. Note the default value for ANPressErr is physically unrealistic. 30 lb/ft2 is about 1500 pascals -- a pressure that would never be possible in a building. The intent of this value is to prevent simulation crashes due to numerical errors in AirNet calculations.

<%= member_table(
units: "lb/ft2",
legal_range: "x $\\gt$ 0",
default: "30",
required: "No",
variability: "constant") %>

The ASHWAT complex fenestration model used when WINDOW wnModel=ASHWAT yields several heat transfer results that are accurate over local ranges of conditions. Several values control when these value are recalculated. If any of the specified values changes more than the associated threshold, a full ASHWAT calculation is triggered. Otherwise, prior results are used. ASHWAT calculations are computationally expensive and conditions often change only incrementally between time steps.

**AWTrigT=_float_**

ASHWAT temperature change threshold -- full calculation is triggered by a change of either indoor or outdoor environmental (combined air and radiant) temperature that exceeds AWTrigT.

<%= member_table(
units: "^o^F",
legal_range: "_x_ $>$ 0",
default: "1",
required: "No",
variability: "constant") %>

**AWTrigSlr=_float_**

ASHWAT solar change threshold -- full calculation is triggered by a fractional change of incident solar radiation that exceeds AWTrigSlr.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.05",
required: "No",
variability: "constant") %>

**AWTrigH=_float_**

ASHWAT convection coefficient change threshold -- full calculation is triggered by a fractional change of inside surface convection coefficient that exceeds AWTrigH.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "0.1",
required: "No",
variability: "constant") %>

## TOP Weather Data Items

The following system variables (4.6.4) are determined from the weather file for each simulated hour:

<%= csv_table(<<END, :row_header => false)
\$radBeam, beam irradiance on tracking surface (integral for hour&comma; Btu/ft^2^).
\$radDiff, diffuse irradiance on a horizontal surface (integral for hour&comma; Btu/ft^2^).
\$tDbO, dry bulb temp (^o^F).
\$tWbO, wet bulb temp (^o^F).
\$wO, humidity ratio
\$windDirDeg, wind direction (degrees&comma; NOT RADIANS; 0=N&comma; 90=E).
\$windSpeed, wind speed (mph).
END
%>

The following are the terms determined from the weather file for internal use, and can be referenced with the probes shown.

        @Top.depressWbWet bulb depression (F).

        @Top.windSpeedSquaredWind speed squared (mph2).

**wfName=_string_**

Weather file path name for simulation. The file should be in the current directory, in the directory CSE.EXE was read from, or in a directory on the operating system PATH. Weather file formats supported are CSW, EPW, and ET1. Only full-year weather files are supported.

Note: Backslash (\\) characters in path names must be doubled to work properly (e.g. "\\\\wthr\\\\mywthr.epw"). Forward slash (/) may be used in place of backslash without doubling.

<%= member_table(
units: "",
legal_range: "file name,path optional",
default: "_none_",
required: "Yes",
variability: "constant") %>

**skyModel=_choice_**

Selects sky model used to determine relative amounts of direct and diffuse irradiance.

<%= csv_table(<<END, :row_header => false)
ISOTROPIC, traditional isotropic sky model
ANISOTROPIC, Hay anisotropic model
END
%>

<%= member_table(
units: "",
legal_range: "_choices above_",
default: "ANISOTROPIC",
required: "No",
variability: "constant") %>

**skyModelLW=_choice_**

Selects the model used to derive sky temperature used in long-wave (thermal) radiant heat exchange calculations for SURFACEs exposed to ambient conditions. See the RACM alorithms documentation for technical details.

<%= csv_table(<<END, :row_header => true)
**Choice**, **Description**
DEFAULT, Default: tSky from weather file if available else Berdahl-Martin
BERDAHLMARTIN, Berdahl-Martin (tSky depends on dew point&comma; cloud cover&comma; and hour)
DRYBULB, tSky = dry-bulb temperature (for testing)
BLAST, Blast model (tSky depends on dry-bulb)
IRHORIZ, Derives tSky from horizonal infrared data from the weather file (available on some EPW files only). Caution: minimal error checking! Missing weather file IR values are not handled correctly.
END
%>

<%= member_table(
units: "",
legal_range: "_choices above_",
default: "DEFAULT",
required: "No",
variability: "constant") %>

The reference temperature and humidity are used to calculate a humidity ratio assumed in air specific heat calculations. The small effect of changing humidity on the specific heat of air is generally ignored in the interests of speed, but the user can control the humidity whose specific heat is used through the refTemp and refRH inputs.

**refTemp=_float_**

Reference temperature (see above paragraph).

<%= member_table(
units: "^o^F",
legal_range: "_x_ $\\ge$ 0",
default: "60^o^",
required: "No",
variability: "constant") %>

**refRH=_float_**

Reference relative humidity (see above).

<%= member_table(
units: "",
legal_range: "0 $\\leq$ _x_ $\\leq$ 1",
default: "0.6",
required: "No",
variability: "constant") %>

**grndRefl=_float_**

Global ground reflectivity, used except where other value specified with sfGrndRefl or wnGrndRefl. This reflectivity is used in computing the reflected beam and diffuse radiation reaching the surface in question. It is also used to calculate the solar boundary conditions for the exterior grade surface in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "",
legal_range: "0 $\\leq$ _x_ $\\leq$ 1",
default: "0.2",
required: "No",
variability: "Monthly-Hourly") %>

The following values modify weather file data, permitting varying the simulation without making up special weather files. For example, to simulate without the effects of wind, use windF = 0; to halve the effects of diffuse solar radiation, use radDiffF = 0.5. Note that the default values for windSpeedMin and windF result in modification of weather file wind values unless other values are specified.

**grndEmit=_float_**

Long-wave emittance of the exterior grade surface used in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "",
legal_range: "0.0 $\le$ x $\le$ 1.0",
default: "0.8",
required: "No",
variability: "constant") %>

<!--NOTE: Could vary more frequently -->

**grndRf**

Ground surface roughness. Used for convection and wind speed corrections in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "ft",
legal_range: "x $\\geq$ 0.0",
default: "0.1",
required: "No",
variability: "constant") %>

<!--NOTE: [Use a better wind speed correction that accounts for this?] Could vary more frequently -->

**windSpeedMin=_float_**

Minimum value for wind speed

<%= member_table(
units: "mph",
legal_range: "_x_ $\\ge$ 0",
default: "0.5",
required: "No",
variability: "constant") %>

**windF=_float_**

Wind Factor: multiplier for wind speeds read from weather file. windF is applied _after_ windSpeedMin. Note that windF does _not_ effect infiltration rates calculated by the Sherman-Grimsrud model (see e.g. ZONE.infELA). However, windF does modify AirNet flows (see IZXFER).

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "0.25",
required: "No",
variability: "constant") %>

**terrainClass=_int_**

Specifies characteristics of ground terrain in the project region.

<%= csv_table(<<END, :row_header => false)
1, ocean or other body of water with at least 5 km unrestricted expanse
2, flat terrain with some isolated obstacles (buildings or trees well separated)
3, rural areas with low buildings&comma; trees&comma; etc.
4, urban&comma; industrial&comma; or forest areas
5, center of large city
END
%>

<%= member_table(
units: "",
legal_range: "1 $\\leq$ _x_ $\\leq$ 5",
default: "4",
required: "No",
variability: "constant") %>

<!--
TODO: document wind speed modification
-->

**radBeamF=_float_**

Multiplier for direct normal (beam) irradiance

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "1",
required: "No",
variability: "constant") %>

**radDiffF=_float_**

Multiplier for diffuse horizonal irradiance.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "1",
required: "No",
variability: "constant") %>

**hConvMod=_choice_**

Enable/disable convection convective coefficient pressure modification factor.

$$0.24 + 0.76 \cdot P_{Location}/P_{SeaLevel}$$

<%= member_table(
units: "",
legal_range: "YES, NO",
default: "YES",
required: "No",
variability: "constant") %>

**soilDiff=_float_**

_Note: soilDiff is used as part of the simple ground model, which is no longer supported. Use soilCond, soilSpHt, and SoilDens instead._

Soil diffusivity, used in derivation of ground temperature. CSE calculates a ground temperature at 10 ft depth for each day of the year using dry-bulb temperatures from the weather file and soilDiff. Ground temperature is used in heat transfer calculations for SURFACEs with sfExCnd=GROUND. Note: derivation of mains water temperature for DHW calculations involves a ground temperature based on soil diffusivity = 0.025 and does not use this soilDiff.

<%= member_table(
units: "ft^2^/hr",
legal_range: "_x_ $>$ 0",
default: "0.025",
required: "No",
variability: "constant") %>

**soilCond=_float_**

Soil conductivity. Used in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "Btuh-ft/ft^2^-^o^F",
legal_range: "_x_ $>$ 0",
default: "1.0",
required: "No",
variability: "constant") %>

<!--NOTE: Could be variable-->

**soilSpHt=_float_**

Soil specific heat. Used in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "Btu/lb-^o^F",
legal_range: "_x_ $>$ 0",
default: "0.1",
required: "No",
variability: "constant") %>

<!--NOTE: Could be variable-->

**soilDens=_float_**

Soil density. Used in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "lb/ft^3^",
legal_range: "_x_ $>$ 0",
default: "115",
required: "No",
variability: "constant") %>

<!--NOTE: Could be variable-->

**farFieldWidth=_float_**

Far-field width. Distance from foundation to the lateral, zero-flux boundary condition. Used in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "ft",
legal_range: "_x_ $>$ 0",
default: "130",
required: "No",
variability: "constant") %>

**deepGrndCnd=_choice_**

Deep-ground boundary condition type. Choices are WATERTABLE (i.e., a defined temperature) or ZEROFLUX.

<%= member_table(
legal_range: "WATERTABLE, ZEROFLUX",
default: "ZEROFLUX",
required: "No",
variability: "constant") %>

**deepGrndDepth=_float_**

Deep-ground depth. Distance from exterior grade to the deep-ground boundary. Used in two-dimensional finite difference calculations for FOUNDATIONs.

<%= member_table(
units: "ft",
legal_range: "_x_ $>$ 0",
default: "130",
required: "No",
variability: "constant") %>

**deepGrndT=_float_**

Deep-ground temperature. Used when deepGrndCnd=WATERTABLE.

<%= member_table(
units: "F",
legal_range: "_x_ $>$ 0",
default: "Annual average drybulb temperature",
required: "No",
variability: "hourly") %>

## TOP TDV (Time Dependent Value) Items

CSE supports an optional comma-separated (CSV) text file that provides hourly TDV values for electricity and fuel. TDV values are read along with the weather file and the values merged with weather data. Several daily statistics are calculated for use via probes. The file has no other effect on the simulation. Only full-year TDV files are supported.

The format of a TDV file is the same as an [IMPORTFILE][importfile] with the proviso that the 4 line header is not optional and certain header items must have specified values. In the following table, non-italic items must be provided as shown (with optional quotes).

<%= csv_table(<<END, :row_header => true)
**Line** **Contents** **Notes**
1, TDV Data (TDV/Btu)&comma; _runNumber_&comma; _runNumber_ is not checked
2, _timestamp_ optionally in quotes accessible via @TOP.TDVFileTimeStamp
3, _title_&comma; hour _title_ (in quotes if it contains commas) accessible via @TOP.TDVFileTitle
4, tdvElec&comma; tdvFuel comma separated column names (optionally in quotes)\ not checked
5 .., _valElec_&comma;_valFuel_ comma separated numerical values (8760 or 8784 rows) tdvElec is always in column 1&comma; tdvFuel always in column 2 column names in row 4 do not determine order
END
%>

Example TDV file --

        "TDV Data (TDV/Btu)","001"
        "Wed 14-Dec-16  12:30:29 pm"
        "BEMCmpMgr 2019.0.0 RV (758), CZ12, Fuel NatGas", Hour
        "tdvElec","tdvFuel"
        7.5638,2.2311
        7.4907,2.2311
        7.4478,2.2311
        7.4362,2.2311
        7.5255,2.2311
        7.5793,2.2311
        7.6151,2.2311
        7.6153,2.2311
        7.5516,2.2311
        (... continues for 8760 or 8784 data lines ...)

Note: additional columns can be included and are ignored.

The table below shows probes available for accessing TDV data in expressions. Except as noted, daily values are updated based on standard time, so they may be inaccurate by small amounts when daylight savings time is in effect.

<%= csv_table(<<END, :row_header => true)
**Probe**, **Variability**, **Description**
@Weather.tdvElec, Hour, current hour electricity TDV
@Weather.tdvFuel, Hour, current hour fuel TDV
@Weather.tdvElecPk, Day, current day peak electricity TDV (includes future hours). Updated at hour 23 during daylight savings.
@Weather.tdvElecAvg, Day, current day average electricity TDV (includes future hours)
@Weather.tdvElecPvPk, Day,previous day peak electricity TDV
@Weather.tdvElecAvg01, Day,previous day average electricity TDV
@weather.tdvElecHrRank[], Day, hour ranking of TDVElec values. tdvElecHrRank[ 1] is the hour having the highest TDVElec&comma; tdvElecHrRank[ 2] is the next highest&comma; etc. The hour values are adjusted when dayight savings time is in effect&comma; so they remain consistent with system variable $hour.
@weatherFile.tdvFileTimeStamp, Constant, TDV file timestamp (line 2 of header)
@weatherFile.tdvFileTitle, Constant, TDV file title (line 3 of header)
@Top.tdvFName, Constant, TDV file full path
END
%>

**TDVfName=_string_**

Note: Backslash (\\) characters in path names must be doubled to work properly (e.g. "\\\\data\\\\mytdv.tdv"). Forward slash (/) may be used in place of backslash without doubling.

<%= member_table(
units: "",
legal_range: "file name, path optional",
default: "(no TDV file)",
required: "No",
variability: "constant") %>

## TOP Report Data Items

These items are used in page-formatted report output files. See REPORTFILE, Section 5.245.21, and REPORT, Section 5.25.

**repHdrL=_string_**

Report left header. Appears at the upper left of each report page unless page formatting (rfPageFmt) is OFF. If combined length of repHdrL and repHdrR is too large for the page width, one or both will be truncated.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

**repHdrR=_string_**

Report right header. Appears at the upper right of each report page unless page formatting (rfPageFmt) is OFF. If combined length of repHdrL and repHdrR is too large for the page width, one or both will be truncated.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

**repLPP=_int_**

Total lines per page to be assumed for reports. Number of lines used for text (including headers and footers) is repLPP - repTopM - repBotM.

<%= member_table(
units: "lines",
legal_range: "_x_ $\\ge$ 50",
default: "66",
required: "No",
variability: "constant") %>

**repTopM=_int_**

Number of lines to be skipped at the top of each report page (prior to header).

<%= member_table(
units: "lines",
legal_range: "0 $\\geq$ _x_ $\\geq$ 12",
default: "3",
required: "No",
variability: "constant") %>

**repBotM=_int_**

Number of lines reserved at the bottom of each report page. repBotM determines the position of the footer on the page (blank lines after the footer are not actually written).

<%= member_table(
units: "lines",
legal_range: "0 $\\geq$ _x_ $\\geq$ 12",
default: "3",
required: "No",
variability: "constant") %>

**repCPL=_int_**

Characters per line for report headers and footers, user defined reports, and error messages. CSE writes simple ASCII files and assumes a fixed (not proportional) spaced printer font. Many of the built-in reports now (July 1992) assume a line width of 132 columns.

<%= member_table(
units: "characters",
legal_range: "78 $\\leq$ _x_ $\\leq$ 132",
default: "78",
required: "No",
variability: "constant") %>

**repTestPfx=_string_**

Report test prefix. Appears at beginning of report lines that are expected to differ from prior runs. This is useful for "hiding" lines from text comparison utilities in automated testing schemes. Note: the value specified with command line -x takes precedence over this input.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

## TOP Autosizing

**doAutoSize=_choice_**

Controls invocation of autosizing phase prior to simulation.

<%= member_table(
units: "",
legal_range: "YES, NO",
default: "NO, unless AUTOSIZE commands in input",
required: "No",
variability: "constant") %>

**auszTol=_float_**

Autosize tolerance. Sized capacity results are deemed final when successive design day calculations produce results within auszTol of the prior iteration.

<%= member_table(
units: "",
legal_range: "",
default: ".005",
required: "No",
variability: "constant") %>

**heatDsTDbO=_float_**

Heating outdoor dry bulb design temperature used for autosizing heating equipment.

<%= member_table(
units: "^o^F",
legal_range: "",
default: "_none_",
required: "No",
variability: "hourly") %>

**heatDsTWbO=_float_**

Heating outdoor design dry bulb temperature used for autosizing heating equipment.

<%= member_table(
units: "^o^F",
legal_range: "_x_ $\\ge$ 0",
default: "derived assuming RH=.7",
required: "No",
variability: "hourly") %>

CSE provides 3 mutually-exclusive methods for specifying cooling design conditions. Each resulting design day is simulated repeatedly until results (e.g. equipment capacities) converge to stable values. Multiple days are typically used to ensure the a range of temperatures and sun positions are considered.

- Design conditions. One or more DESCONDs are specified. DESCOND data is used to generate 24 hour design day weather data.
- Design days (from weather file). One or more dates are specified. Actual days from the weather file are simulated.
- Monthly design data. Deprecated method using conditions found in ET1 format weather files.

**coolDsCond=_list of up to 12 DESCONDs_**

Specifies cooling design conditions for cooling autosizing. A comma-separated list of up to 12 DESCOND names can be provided. Each day will be simulated repeatedly using weather conditions generated from DESCOND values.

<%= member_table(
units: "",
legal_range: "_name of DESCOND_",
default: "_none_",
required: "No",
variability: "constant") %>

**coolDsDay=_list of up to 12 days_**

Specifies cooling design days for cooling autosizing. Each day will be simulated repeatedly using weather file conditions for that day.

<%= member_table(
units: "dates",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

**coolDsMo=_list of up to 12 months_**

Deprecated method for specifying design days for cooling autosizing. Design conditions are taken from ET1 weather file header, however, the limited availale ET1 files do not contain design condition information.

<%= member_table(
units: "months",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

## TOP Debug Reporting

**verbose=_int_**

Controls verbosity of screen remarks. Most possible remarks are generated during autosizing of CNE models. Little or no effect in CSE models. TODO: document options

<%= member_table(
units: "",
legal_range: "0 - 5",
default: "1",
required: "No",
variability: "constant") %>

The following dbgPrintMask values provide bitwise control of addition of semi-formated internal results to the run report file. The values and format of debugging reports are modified as required for testing purposes. <!-- TODO: document options -->

**dbgPrintMaskC=_int_**

Constant portion of debug reporting control.

<%= member_table(
units: "",
legal_range: "",
default: "0",
required: "No",
variability: "constant") %>

**dbgPrintMask=_int_**

Hourly portion of debug reporting control (generally an expression that evaluates to non-0 only on days or hours of interest).

<%= member_table(
units: "",
legal_range: "",
default: "0",
required: "No",
variability: "hourly") %>

**dbgFlag=_int_**

Allows passing an input value to ad-hoc debugging code. No permanent use; no impact on results.

<%= member_table(
units: "",
legal_range: "",
default: "0",
required: "No",
variability: "subhourly") %>

**doCoverage=_choice_**

Enables expression code coverage reporting. Development aid.

<%= member_table(
units: "",
legal_range: "NO, YES",
default: "_NO_",
required: "No",
variability: "constant") %>

**Related Probes:**

- @[top][p_top]
- @[weatherFile][p_weatherfile]
- @[weather][p_weather]
- @[weatherNextHour][p_weathernexthour]
