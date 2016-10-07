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

## TOP Weather Data Items

<!--
TODO ??Add material about STD files?  Probably not functional, don’t include?
-->
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

Weather file path name for simulation. The file should be in the current directory, in the directory CSE.EXE was read from, or in a directory on the operating system PATH. (?? Temporarily, backslash (\\) characters in path names must be doubled to work properly (e.g. "\\\\wthr\\\\cz01.cec"). This will be corrected.).

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
              file name,path optional                 Yes            constant

**skyModel=*choice***

Selects sky model used to determine relative amounts of direct and diffuse irradiance.

  ------------- ---------------------------------
  ISOTROPIC     traditional isotropic sky model
  ANISOTROPIC   Hay anisotropic model
  ------------- ---------------------------------

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              ISOTROPIC ANISOTROPIC   ANISOTROPIC   No             constant

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
               unrestriced expanse

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

Report test prefix. Appears at beginning of report lines that are expected to differ from prior runs. This is useful for “hiding” lines from text comparison utilities in automated testing schemes. Note: the value specified with command line -x takes precedence over this input.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *blank*       No             constant??

## TOP Autosizing

**doAutoSize=*choice***

Controls invocation of autosizing step prior to simulation.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              YES, NO           NO            No             constant

**auszTol=*float***

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                .01           No             constant

**heatDsTDbO=*float***

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F                          0             No             hourly

**heatDsTWbO=*float***

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  F                             0             No             hourly

**coolDsMo=*??***

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  F                             .01           No             hourly

## TOP Debug Reporting

**verbose=*int***

Controls verbosity of screen remarks. Most possible remarks are generated during autosizing of CNE models. Little or no effect in CSE models. TODO: document options

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              0 – 5 ?           1             No             constant

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


