# DHWUSE

Defines a single hot water draw as part of a DHWDAYUSE.  See discussion and examples under DHWDAYUSE. As noted there, most DHWUSE values have hourly variability, allowing flexible representation

**wuName**

Optional name; give after the word “DHWUSE” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wuStart=*float***

The starting time of the hot water draw.

**Units**   **Legal Range**    **Default**   **Required**  **Variability**
----------- ------------------ ------------- ------------- -------------------------
  hr        0 $\le$ x $\le$ 24       --             Yes            constant

**wuDuration=*float***

Draw duration.  wuDuration = 0 is equivalent to omitting the DHWUSE.
Durations that extend beyond midnight are included in the current day.

**Units**   **Legal Range**        **Default**   **Required**  **Variability**
----------- ---------------------- ------------- ------------- -------------------------
  min         0 $\le$ x $\le$ 1440         0           N          hourly

**wuFlow=*float***

Draw flow rate at the point of use (in other words, the mixed-water flow rate).  wuFlow = 0 is equivalent to omitting the DHWUSE.  There is no enforced upper limit on wuFlow, however, unrealistically large values will cause runtime errors.

**Units**   **Legal Range**      **Default**   **Required**  **Variability**
----------- -------------------  ----------- ------------- -------------------------
  gpm         0 $\le$ x           0              N          hourly

**wuHotF=*float***

Fraction of draw that is hot water.  Cannot be specified with wuTemp or wuHeatRecEF.

  **Units**   **Legal Range**     **Default**   **Required**  **Variability**
  ----------- ------------------- ------------- ------------- -------------------------
    --         0 $\le$ x $\le$ 1         1           N          hourly

**wuTemp=*float***

Mixed-water use temperature at the fixture. Cannot be specified when wuHotF is given.   

  **Units**   **Legal Range**     **Default**   **Required**         **Variability**
  ---------- ------------------- ------------- ------------------    -------------------------
  ^o^F         0 $\le$ x          0             when wuHeatRecEF          hourly
                                                is given

**wuHeatRecEF=*float***

Heat recovery effectiveness.  If non-0, wuHeatRecEF allows modeling of heat recovery devices such as drain water heat exchangers.  If given, wuTemp must also be specified.  

**Units**   **Legal Range**        **Default**   **Required**  **Variability**
----------- --------------------- ------------- ------------- -------------------------
  --         0 $\le$ x $\le$ 0.9          0      when wu          hourly


**wuHWEndUse=*choice***

Hot-water end use: one of Shower, Bath, CWashr, DWashr, or Faucet.  whHWEndUse has two functions --

 * Allocation of hot water use among multiple DHWSYSs (if more than one DHWSYS references a given DHWDAYUSE).
 * DHWMETER end-use accounting (via DHWSYS).

**Units**   **Legal Range**       **Default**                 **Required**  **Variability**
----------- --------------------  --------------------------- ------------- -------------------------
  --        One of above choices   (use allocated to Unknown)           N          constant


**wuEventID=*integer***

User-defined identifier that associates multiple DHWUSEs with a single event or activity.  For example, a dishwasher uses water at several discrete times during a 90 minute cycle and all DHWUSEs would be assigned the same wuEventID.  All DHWUSEs having the same wuEventID should have the same wuHWEndUse.

**Units**   **Legal Range**     **Default**   **Required**  **Variability**
----------- ------------------- ------------- ------------- -------------------------
      --         0 $\le$ x               0              N          constant

**endDHWUSE**

Optionally indicates the end of the DHWUSE definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No  

**Related Probes:**

- [@DHWUse](#p_dhwUse)
