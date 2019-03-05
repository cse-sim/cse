# DHWSYS

DHWSYS constructs an object representing a domestic hot water system consisting of one or more hot water heaters, storage tanks, loops, and pumps (DHWHEATER, DHWTANK, DHWLOOP, and DHWPUMP, see below) and a distribution system characterized by loss parameters. This model is based on Appendix B of the 2016 Residential ACM Reference Manual. This version is preliminary, revisions are expected.

The parent-child structure of DHWSYS components is determined by input order. For example, DHWHEATERs belong to the DHWSYS that precedes them in the input file. The following hierarchy shows the relationship among components. Note that any of the commands can be repeated any number of times.

-   DHWSYS
    -   DHWHEATER
    -   DHWTANK
    -   DHWPUMP
    -   DHWLOOP
        -   DHWLOOPPUMP
        -   DHWLOOPSEG
            -   DHWLOOPBRANCH

No actual controls are modeled. For example, if several DHWHEATERs are included in a DHWSYS, an equal fraction of the required hot water is assumed to be produced by each heater, even if they are different types or sizes. Thus a DHWSYS is in some ways just a collection of components, rather than a physically realistic system.

**dhwsysName**

Optional name of system; give after the word “DHWSYS” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wsCentralDHWSYS=*dhwsysName***

  Name of the central DHWSYS that serves this DHWSYS, allowing representation of multiple units having distinct distribution configurations and/or water use patterns but served by a central DHWSYS.  The child DHWSYS(s) may not include DHWHEATERs -- they are "loads only" systems.  wsCentralDHWSYS and wsLoadShareDHWSYS cannot both be given.

  **Units**   **Legal Range**     **Default**             **Required**   **Variability**
  ----------- ------------------- ----------------------- -------------- -----------------
               *name of a DHWSYS*    DHWSYS is standalone          No           constant

**wsLoadShareDHWSYS=*dhwsysName***

 Name of a DHWSYS that serves the same loads as this DHWSYS, allowing representation of multiple water heating systems within a unit. If given, wsUse and wsDayUse are not allowed, hot water requirements are derived from the referenced DHWSYS.  wsCentralDHWSYS and wsLoadShareDHWSYS cannot both be given.

 **Units**   **Legal Range**      **Default**            **Required**   **Variability**
 ----------- ------------------- ---------------------- -------------- -----------------
              *name of a DHWSYS*   No shared loads                 No           constant

**wsMult=*integer***

Number of identical systems of this type (including all child objects). Any value $> 1$ is equivalent to repeated entry of the same DHWSYSs.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**wsTInlet=*float***

Specifies cold (mains) water temperature supplied to DHWHEATERs in this DHWSYS.

  **Units**   **Legal Range**   **Default**                    **Required**   **Variability**
  ----------- ----------------- ------------------------------ -------------- -----------------
  ^o^F        $>$ 32 ^o^F       Mains temp from weather file   No             hourly

**wsUse=*float***

Hourly hot water use (at the point of use).  See further info under wsDayUse.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  gal         $\ge$ 0            0             No             hourly

**wsDayUse=*dhwdayuseName***

  Name of DHWDAYUSE object that specifies a detailed schedule of hot water use (at point of use).

  The total water use modeled by CSE is the sum of amounts given by wsUse and the DWHDAYUSE schedule.  DHWDAYUSE draws are resolved to minute-by-minute bins compatible with the HPWH model and wsUse/60 is added to each minute bin.  Conversely, the hour total of the DHWDAYUSE amounts is included in the draw applied to non-HPWH DHWHEATERs.

  wsDayUse variability is daily, so it is possible to select different schedules as a function of day type (or any other condition), as follows --

         DHWSYS "DHW1"
           ...
           wsDayUse = choose( $isWeHol, "DUSEWeekday", "DUSEWeHol")
           ...

  Note that while DHWDAYUSE selection is updated daily, the DHWUSE values within the DHWDAYUSE can be altered hourly, providing additional scheduling flexibility.

  **Units**   **Legal Range**        **Default**            **Required**   **Variability**
  ---------- ---------------------- ---------------------- -------------- -----------------
              *name of a DHWDAYUSE*   (no scheduled draws)        No             daily

**wsTUse=*float***

Hot water delivery temperature (at the point of use).  Note that draws defined via DHWDAYUSE / DHWUSE can specify mixing to a lower temperature.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        $>$ 32 ^o^F       120           No             hourly

**wsTSetPoint=*float***

  Specifies the hot water setpoint temperature.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
    ^o^F        $>$ 32 ^o^F         wsTUse           No             hourly

**wsParElec=*float***

Specifies electrical parasitic power to represent recirculation pumps or other system-level electrical devices. Calculated energy use is accumulated to the METER specified by wsElecMtr (end use DHW). No other effect, such as heat gain to surroundings, is modeled.

  **Units**   **Legal Range**   **Default**   **Required**    **Variability**
----------- ------------------ ------------- --------------- -----------------
   W            $\ge$ 0             0             No             hourly

**wsSDLM=*float***

Specifies the standard distribution loss multiplier. See App B Eqn 4. To duplicate CEC 2016 methods, this value should be set according to the value derived with App B Eqn 5.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**wsDSM=*float***

Distribution system multiplier. See RACM App B Eqn 4. To duplicate CEC 2016 methods, wsDSM should be set to the appropriate value from App B Table B-2. Note the NCF (non-compliance factor) included in App B Eqn 4 is *not* a CSE input and thus must be applied externally to wsDSM.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**wsWF=*float***

Waste factor. See RACM App B Eqn 1. wsWF is applied to hot water draws.  The default value (1) reflects the inclusion of waste in draw amounts.  App B specifies wsWF=0.9 when the system has a within-unit pumped loop that reduces waste due to immediate availability of hot water at fixtures.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             hourly

**wsSSF=*float***

Specifies the solar savings fraction.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $\ge$ 0           0             No             hourly

**wsElecMtr=*mtrName***

Name of METER object, if any, to which DHWSYS electrical energy use is recorded (under end use DHW). In addition, wsElecMtr provides the default whElectMtr selection for all DHWHEATERs and DHWPUMPs in this DHWSYS.

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**wsFuelMtr =*mtrName***

Name of METER object, if any, to which DHWSYS fuel energy use is recorded (under end use DHW). DHWSYS fuel use is usually (always?) 0, so the primary use of this input is to specify the default whFuelMtr choice for DHWHEATERs in this DHWSYS.

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**wsWHhwMtr=*dhwmtrName***

Name of DHWMETER object, if any, to which hot water quantities (at water heater) are recorded by hot water end use.

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**wsFXhwMtr =*dhwmtrName***

Name of DHWMETER object, if any, to which mixed hot water use (at fixture) quantities are recorded by hot water end use.  DHWDAYUSE and wsUse input can be verified using DHWMETER results.

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**wsCalcMode=*choice***

  ----------- ---------------------------------------
  PRERUN      Calculate hot water heating load; at
              end of run, derive whLDEF for all child
              DHWHEATERs for which that value is
              required and defaulted. This procedure
              emulates methods used in the T24DHW.DLL
              implementation of CEC DHW procedures.

  SIMULATE    Perform full modeling calculations
  ----------- ---------------------------------------

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              *Codes listed above*   SIMULATE      No             

**endDHWSys**

Optionally indicates the end of the DHWSYS definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWSys](#p_dhwsys)
