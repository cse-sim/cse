# TOP Members

The top-level data items (TOP members) control the simulation process or contain data that applies to the modeled building as a whole. No statement is used to begin or create the TOP object; these statements can be given anywhere in the input (they do, however, terminate any other objects being specified -- ZONEs, REPORTs, etc.).

## TOP General Data Items

**doMainSim=*choice***

Specifies whether the simulation is performed when a Run command is encountered. See also doAutoSize.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              NO,YES            YES           No             constant

**begDay=*date***

Date specifying the beginning day of the simulation performed when a Run command is encountered. See further discussion under endDay (next).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *date*            Jan 1         No             constant

**endDay=*date***

Date specifying the ending day of the simulation performed when a Run command is encountered.

The program simulates 365 days at most. If begDay and endDay are the same, 1 day is simulated. If begDay precedes endDay in calendar sequence, the simulation is performed normally and covers begDay through endDay inclusive. If begDay follows endDay in calendar sequence, the simulation is performed across the year end, with Jan 1 immediately following Dec 31.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *date*            Dec 31        No             constant

**jan1DoW=*choice***

Day of week on which January 1 falls.

  ------------------------------------------------------------------------
  **Units**  **Legal Range**  **Default**  **Required**   **Variability**
  ---------- ---------------- ------------ ------------- -----------------
             SUN\             THU          No                constant
             MON\                                        
             TUE\                                        
             WED\                                        
             THU\                                        
             FRI\                                        
             SAT                                         
  ------------------------------------------------------------------------

**workDayMask=*int* TODO**

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                Mon-fri?      No             constant

**wuDays=*int***

Number of "warm-up" days used to initialize the simulator. Simulator initialization is required because thermal mass temperatures are set to arbitrary values at the beginning of the simulation. Actual mass temperatures must be established through simulation of a few days before thermal loads are accumulated. Heavier buildings require more warm-up; the default values are adequate for conventional construction.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\geq$ 0      7             No             constant

**nSubSteps=*int***

Number of subhour steps used per hour in the simulation. 4 is the time-honored value for models using CNE zones. A value of 30 is typically for CSE zone models.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         4             No             constant

**tol=*float***

Endtest convergence tolerance for internal iteration in CNE models (no effect for CSE models) Small values for the tolerance cause more accurate simulations but slower performance. The user may wish to use a high number during the initial design process (to quicken the runs) and then lower the tolerance for the final design (for better accuracy). Values other than .001 have not been explored.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         .001          No             constant

**humTolF=*float***

Specifies the convergence tolerance for humidity calculations in CNE models (no effect in for CSE models), relative to the tolerance for temperature calculations. A value of .0001 says that a humidity difference of .0001 is about as significant as a temperature difference of one degree. Note that this is multiplied internally by "tol"; to make an overall change in tolerances, change "tol" only.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         .0001         No             

**ebTolMon=*float***

Monthly energy balance error tolerance for internal consistency checks. Smaller values are used for testing the internal consistency of the simulator; values somewhat larger than the default may be used to avoid error messages when it is desired to continue working despite a moderate degree of internal inconsistency.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         0.0001        No             constant

**ebTolDay=*float***

Daily energy balance error tolerance.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         0.0001        No             constant

**ebTolHour=*float***

Hourly energy balance error tolerance.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         0.0001        No             constant

**ebTolSubhr=*float***

Sub-hourly energy balance error tolerance.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         0.0001        No             constant

**humMeth=*choice***

Developmental zone humidity computation method choice for CNE models (no effect for CSE models).

  ------------ ---------------------------------------------------------
  ROB          Rob's backward difference method. Works well within
               limitations of backward difference approach.

  PHIL         Phil's central difference method. Should be better if
               perfected, but initialization at air handler startup is
               unresolved, and ringing has been observed.
  ------------ ---------------------------------------------------------

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              ROB, PHIL         ROB           No             constant

**dflExH=*float***

Default exterior surface (air film) conductance used for opaque and glazed surfaces exposed to ambient conditions in the absence of explicit specification.

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         2.64          No             constant

**bldgAzm=*float***

Reference compass azimuth (0 = north, 90 = east, etc.). All zone orientations (and therefore surface orientations) are relative to this value, so the entire building can be rotated by changing bldgAzm only. If a value outside the range 0^o^ $\leq$ *x* $<$ 360^o^ is given, it is normalized to that range.

  **Units**     **Legal Range**   **Default**   **Required**   **Variability**
  ------------- ----------------- ------------- -------------- -----------------
  ^o^ (degrees)   unrestricted      0             No             constant

**elevation=*float***

Elevation of the building site. Used internally for the computation of barometric pressure and air density of the location.

  **Units**   **Legal Range**   **Default**     **Required**   **Variability**
  ----------- ----------------- --------------- -------------- -----------------
  ft          *x* $\ge$ 0       0 (sea level)   No             constant

**runTitle=*string***

Run title for the simulation. Appears in report footers, export headers, and in the title lines to the INP, LOG, and ERR built-in reports (these appear by default in the primary report file; the ERR report also appears in the error message file, if one is created).

  **Units**   **Legal Range**   **Default**        **Required**   **Variability**
  ----------- ----------------- ------------------ -------------- -----------------
              *63 characters*   blank (no title)   No             constant

**runSerial=*int***

Run serial number for the simulation. Increments on each run in a session; appears in report footers. <!-- TODO: in future will be saved to status file. -->

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 999   0             No             constant

## TOP Daylight Saving Time Items

Daylight savings starts by default at 2:00 a.m. of the second Sunday in March. Internally, hour 3 (2:00-3:00 a.m.) is skipped and reports for this day show only 23 hours. Daylight savings ends by default at 2:00 a.m. of the first Sunday of November; for this day 25 hours are shown on reports. CSE fetches weather data using standard time but uses daylight savings time to calculate variable expressions (and thus all schedules).

**DT=*choice***

Whether Daylight Savings Time is to be used for the current run.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              YES, NO           YES           No             constant

**DTbegDay=*date***

Start day for daylight saving time (assuming DT=Yes)

  **Units**   **Legal Range**   **Default**                **Required**   **Variability**
  ----------- ----------------- -------------------------- -------------- -----------------
              *date*            *second Sunday in March*   No             constant

**DTendDay=*date***

End day for daylight saving time (assuming DT=Yes)

  **Units**   **Legal Range**   **Default**                  **Required**   **Variability**
  ----------- ----------------- ---------------------------- -------------- -----------------
              *date*            *first Sunday in November*   No             constant

## TOP Model Control Items

**ventAvail=*choice***

Indicates availability of outdoor ventilation strategies.  CSE cannot model simultaneously-operating alternative ventilation strategies.  For example, an RSYS central fan integrated (CFI) OAV system is never modeled while whole house fan ventilation is available.  ventAvail controls which ventilation mode, if any, is available for the current hour.  Note that mode availability means that the strategy could operate but may not operate due to other control assumptions.

**Choice**    **Ventilation Strategy Available**
------------- ---------------------------------
NONE          None
WHOLEBUILDING IZXFER (window and whole-house fan)
RSYSOAV       RSYS central fan integrated (CFI) outside air ventilation (OAV)
------------- ---------------------------------

As noted, ventAvail is evaluated hourly, permitting flexible control strategy modeling.  The following example specifies that RSYSOAV (CFI) ventilation is available when the seven day moving average temperature is above 68 ^o^F, otherwise whole building ventilation is available between 7 and 11 PM, otherwise no ventilation.

    ventAvail = (@weather.taDbAvg07 > 68)    ? RSYSOAV
              : ($hour >= 19 && $hour <= 23) ? WHOLEBUILDING
              :                                NONE

  **Units**   **Legal Range**         **Default**    **Required**   **Variability**
  ----------- ----------------------- -------------- -------------- -----------------
              *Choices above*          WHOLEBUILDING           No         hourly

**dhwModel=*choice***

Modifies aspects of DHW calculations.

**Choice**    **Effect**
------------- ---------------------------------
T24DHW          Matches results from T24DHW.DLL
2013            Corrected CEC 2013 methods with 2016 updates
------------- ---------------------------------

  **Units**   **Legal Range**         **Default**    **Required**   **Variability**
  ----------- ----------------------- -------------- -------------- -----------------
              *Choices above*          2013             No            constant

**exShadeModel=*choice***

Specifies advanced exterior shading model used to evaluate shading of [PVARRAYs](#pvarray) by [SHADEXs](#shadex) or other PVARRAYs.  Advanced shading is not implemented for building surfaces and this setting has no effect on walls or windows.

**Choice**    **Effect**
------------- ---------------------------------
PENUMBRA        Calculate shading using the Penumbra model
NONE            Disable advanced shading calculations
------------- ---------------------------------

  **Units**   **Legal Range**         **Default**    **Required**   **Variability**
  ----------- ----------------------- -------------- -------------- -----------------
              *Choices above*          PENUMBRA           No            constant

**ANTolAbs=*float***

AirNet absolute convergence tolerance. Ideally, calculated zone air pressures should be such that the net air flow into each zone is 0 -- that is, there should be a perfect mass balance.  The iterative AirNet solution techniques are deemed converged when netAirMassFlow < max( ANTolAbs, ANTolRel*totAirMassFlow).

**Units**   **Legal Range** **Default**            **Required**   **Variability**
----------- --------------- --------------------- -------------- -----------------
 lbm/sec        x > 0       0.00125 (about 1 cfm)       No             constant

**ANTolRel=*float***

AirNet relative convergence tolerance.  See AnTolAbs just above.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
               x > 0            .0001         No             constant


The ASHWAT complex fenestration model used when WINDOW wnModel=ASHWAT yields several heat transfer results that are accurate over local ranges of conditions.  Several values control when these value are recalculated.  If any of the specified values changes more than the associated threshold, a full ASHWAT calculation is triggered.  Otherwise, prior results are used.  ASHWAT calculations are computationally expensive and conditions often change only incrementally between time steps.

**AWTrigT=*float***

ASHWAT temperature change threshold -- full calculation is triggered by a change of either indoor or outdoor environmental (combined air and radiant) temperature that exceeds AWTrigT.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
  ^o^F         x > 0            1              No             constant

**AWTrigSlr=*float***

ASHWAT solar change threshold -- full calculation is triggered by a fractional change of incident solar radiation that exceeds AWTrigSlr.

**Units**   **Legal Range**   **Default** **Required** **Variability**
----------- ----------------- ----------- ------------ ---------------
            x > 0             .05          No           constant

**AWTrigH=*float***

ASHWAT convection coefficient change threshold -- full calculation is triggered by a fractional change of inside surface convection coefficient that exceeds AWTrigH.

**Units** **Legal Range** **Default** **Required** **Variability**
--------- --------------- ----------- ------------ ---------------
          x > 0           .1          No           constant

## TOP Weather Data Items

The following system variables (4.6.4) are determined from the weather file for each simulated hour:

  ---------------- -----------------------------------------------------
  \$radBeam        beam irradiance on tracking surface (integral for
                   hour, Btu/ft^2^).

  \$radDiff        diffuse irradiance on a horizontal surface (integral
                   for hour, Btu/ft^2^).

  \$tDbO           dry bulb temp (^o^F).

  \$tWbO           wet bulb temp (^o^F).

  \$wO             humidity ratio

  \$windDirDeg     wind direction (degrees, NOT RADIANS; 0=N, 90=E).

  \$windSpeed      wind speed (mph).
  ---------------- -----------------------------------------------------

The following are the terms determined from the weather file for internal use, and can be referenced with the probes shown.

        @Top.depressWbWet bulb depression (F).

        @Top.windSpeedSquaredWind speed squared (mph2).

**wfName=*string***

Weather file path name for simulation. The file should be in the current directory, in the directory CSE.EXE was read from, or in a directory on the operating system PATH.  Weather file formats supported are CSW, EPW, and ET1.  Only full-year weather files are supported.

Note: Backslash (\\) characters in path names must be doubled to work properly (e.g. "\\\\wthr\\\\mywthr.epw").  Forward slash (/) may be used in place of backslash without doubling.

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
              file name,path optional                   Yes            constant

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
              file name,path optional                   Yes            constant

**skyModel=*choice***

Selects sky model used to determine relative amounts of direct and diffuse irradiance.

  ------------- ---------------------------------
  ISOTROPIC     traditional isotropic sky model
  ANISOTROPIC   Hay anisotropic model
  ------------- ---------------------------------

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              *choices above*          ANISOTROPIC   No             constant

**skyModelLW=*choice***

Selects the model used to derive sky temperature used in long-wave (thermal) radiant heat exchange calculations for SURFACEs exposed to ambient conditions.  See the RACM alorithms documentation for technical details.

  Choice         Description
  -------------- ---------------------------------------------
  DEFAULT        Default: tSky from weather file if available else Berdahl-Martin
	BERDAHLMARTIN  Berdahl-Martin (tSky depends on dew point, cloud cover, and hour)
	DRYBULB   	   tSky = dry-bulb temperature (for testing)
	BLAST          Blast model (tSky depends on dry-bulb)
  ------------- ---------------------------------

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
               *choices above*          DEFAULT        No             constant


The reference temperature and humidity are used to calculate a humidity ratio assumed in air specific heat calculations. The small effect of changing humidity on the specific heat of air is generally ignored in the interests of speed, but the user can control the humidity whose specific heat is used through the refTemp and refRH inputs.

**refTemp=*float***

Reference temperature (see above paragraph).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* $\geq$ 0      60^o^         No             constant

**refRH=*float***

Reference relative humidity (see above).

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1   0.6           No             constant

**grndRefl=*float***

Global ground reflectivity, used except where other value specified with sfGrndRefl or wnGrndRefl. This reflectivity is used in computing the reflected beam and diffuse radiation reaching the surface in question.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1   0.2           No             Monthly-Hourly

The following values modify weather file data, permitting varying the simulation without making up special weather files. For example, to simulate without the effects of wind, use windF = 0; to halve the effects of diffuse solar radiation, use radDiffF = 0.5. Note that the default values for windSpeedMin and windF result in modification of weather file wind values unless other values are specified.

**windSpeedMin=*float***

Minimum value for wind speed

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  mph         *x* $\geq$ 0      0.5           No             constant

**windF=*float***

Wind Factor: multiplier for wind speeds read from weather file. windF is applied *after* windSpeedMin. Note that windF does *not* effect infiltration rates calculated by the Sherman-Grimsrud model (see e.g. ZONE.infELA). However, windF does modify AirNet flows (see IZXFER).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\geq$ 0      0.25          No             constant

**terrainClass=*int***

Specifies characteristics of ground terrain in the project region.

  ------------ ---------------------------------------------------------
  1            ocean or other body of water with at least 5 km
               unrestricted expanse

  2            flat terrain with some isolated obstacles (buildings or
               trees well separated)

  3            rural areas with low buildings, trees, etc.

  4            urban, industrial, or forest areas

  5            center of large city
  ------------ ---------------------------------------------------------

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              1 $\leq$ *x* $\leq$ 5   4             No             constant

<!--
TODO: document wind speed modification
-->
**radBeamF=*float***

Multiplier for direct normal (beam) irradiance

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\geq$ 0      1             No             constant

**radDiffF=*float***

Multiplier for diffuse horizonal irradiance.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\geq$ 0      1             No             constant

**soilDiff=*float***

Soil diffusivity, used in derivation of ground temperature.  CSE calculates a ground temperature at 10 ft depth for each day of the year using dry-bulb temperatures from the weather file and soilDiff.  Ground temperature is used in heat transfer calculations for SURFACEs with sfExCnd=GROUND.  Note that derivation of mains water temperature for DHW calculations involves a ground temperature based on soil diffusivity = 0.025 and does not use this soilDiff.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
    ft^2^/hr      x > 0          0.025             No             constant

## TOP TDV (Time Dependent Value) Items

CSE supports an optional comma-separated (CSV) text file that provides hourly TDV values for electricity and fuel.  TDV values are read along with the weather file and the values merged with weather data.  Several daily statistics are calculated for use via probes.  The file has no other effect on the simulation.  Only full-year TDV files are supported.

The format of a TDV file is the same as an [IMPORTFILE](#importfile) with the proviso that the 4 line header is not optional and certain header items must have specified values.  In the following table, non-italic items must be provided as shown (with optional quotes).

  -----------------------------------------------------------------------------------------------------
  Line      Contents                          Notes
  --------- ------------------------------    --------------------------------------------------------------
  1         TDV Data (TDV/Btu), *runNumber*   *runNumber* is not checked

  2         *timestamp*                       optionally in quotes\
                                              accessible via @TOP.TDVFileTimeStamp

  3         *title*, hour                     *title* (in quotes if it contains commas)\
                                              accessible via @TOP.TDVFileTitle

  4         tdvElec, tdvFuel                  comma separated column names (optionally in quotes)\
                                              not checked

  5 ..      *valElec*,*valFuel*               comma separated numerical values (8760 or 8784 rows)\
                                              tdvElec is always in column 1, tdvFuel always in column 2\
                                              column names in row 4 do not determine order
 -------------------------------------------------------------------------------------------------------

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

The following probes are available for accessing TDV data in expressions --

 Probe                         Variability         Description
 --------------                ------------        ------------------
 @Weather.tdvElec               Hour               current hour electricity TDV
 @Weather.tdvFuel               Hour               current hour fuel TDV
 @Weather.tdvElecPk             Day                current day peak electricity TDV (includes future hours)
 @Weather.tdvElecAvg            Day                 current day average electricity TDV (includes future hours)
 @Weather.tdvElecPvPk           Day                previous day peak electricity TDV
 @Weather.tdvElecAvg01          Day                previous day average electricity TDV
 @weatherFile.tdvFileTimeStamp  Constant           TDV file timestamp (line 2 of header)
 @weatherFile.tdvFileTitle      Constant           TDV file title (line 3 of header)
 @Top.tdvFName                  Constant           TDV file full path


**TDVfName=*string***

Note: Backslash (\\) characters in path names must be doubled to work properly (e.g. "\\\\data\\\\mytdv.tdv").  Forward slash (/) may be used in place of backslash without doubling.

  **Units**   **Legal Range**           **Default**    **Required**   **Variability**
  ----------- ------------------------- -------------- -------------- -----------------
              file name, path optional    (no TDV file)          No           constant

## TOP Report Data Items

These items are used in page-formatted report output files. See REPORTFILE, Section 5.245.21, and REPORT, Section 5.25.

**repHdrL=*string***

Report left header. Appears at the upper left of each report page unless page formatting (rfPageFmt) is OFF. If combined length of repHdrL and repHdrR is too large for the page width, one or both will be truncated.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *blank*       No             constant??

**repHdrR=*string***

Report right header. Appears at the upper right of each report page unless page formatting (rfPageFmt) is OFF. If combined length of repHdrL and repHdrR is too large for the page width, one or both will be truncated.

  **Units**   **Legal Range**   **Default**                **Required**   **Variability**
  ----------- ----------------- -------------------------- -------------- -----------------
                                *blank*(no right header)   No             constant??

**repLPP=*int***

Total lines per page to be assumed for reports. Number of lines used for text (including headers and footers) is repLPP - repTopM - repBotM.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  lines       *x* $\geq$ 50     66            No             constant??

**repTopM=*int***

Number of lines to be skipped at the top of each report page (prior to header).

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
  lines       0 $\geq$ *x* $\geq$ 12   3             No             constant

**repBotM=*int***

Number of lines reserved at the bottom of each report page. repBotM determines the position of the footer on the page (blank lines after the footer are not actually written).

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
  lines       0 $\leq$ *x* $\leq$ 12   3             No             constant

**repCPL=*int***

Characters per line for report headers and footers, user defined reports, and error messages. CSE writes simple ASCII files and assumes a fixed (not proportional) spaced printer font. Many of the built-in reports now (July 1992) assume a line width of 132 columns.

  **Units**    **Legal Range**            **Default**   **Required**   **Variability**
  ------------ -------------------------- ------------- -------------- -----------------
  characters   78 $\leq$ *x* $\leq$ 132   78            No             constant

**repTestPfx=*string***

Report test prefix. Appears at beginning of report lines that are expected to differ from prior runs. This is useful for "hiding" lines from text comparison utilities in automated testing schemes. Note: the value specified with command line -x takes precedence over this input.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *blank*       No             constant??

## TOP Autosizing

**doAutoSize=*choice***

Controls invocation of autosizing phase prior to simulation.

  **Units**   **Legal Range**   **Default**                            **Required**   **Variability**
  ----------- ----------------- -------------------------------------- -------------- -----------------
              YES, NO           NO, unless AUTOSIZE commands in input            No           constant

**auszTol=*float***

Autosize tolerance.  Sized capacity results are deemed final when successive design day calculations produce results within auszTol of the prior iteration.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                .005           No             constant

**heatDsTDbO=*float***

Heating outdoor dry bulb design temperature used for autosizing heating equipment.

  **Units**   **Legal Range**   **Default**   **Required**    **Variability**
  ----------- ----------------- ------------- --------------- -----------------
  ^o^F                                          if autosizing          hourly

**heatDsTWbO=*float***

Heating outdoor Whether bulb design temperature used for autosizing heating equipment.

  **Units**   **Legal Range**   **Default**              **Required**   **Variability**
  ----------- ----------------- ------------------------ -------------- -----------------
   ^o^F                          derived assuming RH=.7            No          hourly

**coolDsDay=*list of up to 12 days***

Specifies cooling design days for autosizing.  Each day will be simulated repeatedly using weather file conditions for that day.

 **Units**   **Legal Range**   **Default**   **Required**   **Variability**
 ----------- ----------------- ------------- -------------- -----------------
    dates                           *none*           No             constant

**coolDsMo=*list of up to 12 months***

Deprecated method for specifying cooling autosizing days.  Design conditions are taken from ET1 weather file header, however, the limited availale ET1 files do not contain design condition information.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
     months                         *none*           No           constant



## TOP Debug Reporting

**verbose=*int***

Controls verbosity of screen remarks. Most possible remarks are generated during autosizing of CNE models. Little or no effect in CSE models. TODO: document options

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                0- 5 ?           1             No             constant

The following dbgPrintMask values provide bitwise control of addition of semi-formated internal results to the run report file. The values and format of debugging reports are modified as required for testing purposes. <!-- TODO: document options -->

**dbgPrintMaskC=*int***

Constant portion of debug reporting control.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                0             No             constant

**dbgPrintMask=*int***

Hourly portion of debug reporting control (generally an expression that evaluates to non-0 only on days or hours of interest).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                0             No             hourly

**Related Probes:**

- @[top](#p_top)
- @[weatherFile](#p_weatherfile)
- @[weather](#p_weather)
- @[weatherNextHour](#p_weathernexthour)
