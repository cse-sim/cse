# DHWDAYUSE

Defines an object that represents domestic hot water use for a single day.  A DHWDAYUSE contains a collection of DHWUSE
objects that specify the time, volume, and duration of individual draws.  DHWDAYUSEs are referenced by DHWSYS wsDayUse.  Unreferenced DHWDAYUSEs are allowed.

DHWDAYUSEs and their child DHWUSEs are used to construct minute-by-minute hot water use schedules in addition to aggregated hourly schedules.  The minute-by-minute schedules are used for modeling resistance and heat pump storage water heaters, see DHWHEATER whType=SmallStorage whHeatSrc=ResistanceX or whHeatSrc=ASHPX.

The following illustrates some features of DHWDAYUSE / DHWUSE --

    DHWDAYUSE "Sample"
       // 6 AM: 7 min shower, 2 gpm @ 105 F
       DHWUSE whStart=6.0 wuDuration=7 wuFlow=2 wuTemp=105 wuEndUse=Shower wuEventID=1

       // 7 AM: 1 min faucet draw, 100% hot
       DHWUSE whStart=7.0 wuDuration=1 wuFlow=1 wuHotF=1 whEndUse=Faucet wuEventID=2

       // 12:30 PM: dishwasher start, several draws over 70 mins; note common wuEventID
       DHWUSE whStart=12.5 wuDuration=2 wuFlow=2 wuHotF=1 whEndUse=DWashr wuEventID=3
       DHWUSE whStart=12.8 wuDuration=1.5 wuFlow=2 wuHotF=1 whEndUse=DWashr wuEventID=3
       DHWUSE whStart=13.6 wuDuration=3 wuFlow=2 wuHotF=1 whEndUse=DWashr wuEventID=3

       // 7 PM every 2nd day: clothes washer runs
       //   even days: 0 gpm (no draw)
       //   odd days: 3 gpm, 22% hot
       DHWUSE whStart=19 wuDuration=30 wuFlow = ($dayOfYear%2)*3 whEndUse=CWashr whHotF=.22 wuEventID=4

       // 11:54 PM: 20 min bath, 1.5 gpm, 80% hot water
       // Duration spans midnight: draw is wrapped to beginning of *current* day
       //   In this case a 12 M - 12:14 AM draw is modeled -- before (!) the bath start.
       DHWUSE whStart 23.9 wuDuration=20 wuFlow=1.5  wuHotF=.8 whEndUse=Bath wuEventID=99
    endDHWDAYUSE

    DHWSYS "DHWSYS1"
      ...
      wsDayUse = "Sample"
      ...

During the simulation, DHWUSEs are evaluated each hour.  Many DHWUSE values have hourly variability and this allows complicated schemes to be constructed very flexibly.  For example:

    DHWDAYUSE "HourlyFaucet"
       // Every hour on the half hour: 5 minute, 2 gpm draw
       //   Same as 24 DHWUSEs, one for each hour
       DHWUSE wuStart=$hour+.5 wuDuration=5 wuFlow=2 wuEndUse=Faucet
    endDAYUSE

Some DHWUSE configurations involve mixing to specified wuTemp.  Hot and cold water arriving at the point of use is assumed to be at DHWSYS wsUseTemp and wsMainsTemp respectively.  It is possible to set up situations where wuTemp cannot be achieved (wuTemp > wsUseTemp, for example).  Runtime error messages are produced when impossible conditions are detected.

When more than one DHWSYS references the same DHWDAYUSE, DHWUSEs are allocated to DHWSYSs in wuEventID rotation.  This procedure divides the water heating load approximately equally while retaining the peak demand of individual events.  When detailed information is available about which loads are served by specific systems, separate DHWDAYUSEs should be given.


**dhwDayUseName**

Object name, given after “DHWDAYUSE”.  Required for referencing from DHWSYS.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        Yes            constant

**wduMult=*float***

Scale factor applied to all draws in this DHWDAYUSE.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
             $\ge$ 0              1           No            constant


**endDHWDAYUSE**

Indicates the end of the DHWDAYUSE definition.  endDHWDAYUSE should follow all child DHWUSEs.  Alternatively, the end of the meter definition can be indicated by the declaration of another object or by END.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
