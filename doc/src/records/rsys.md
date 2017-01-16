# RSYS

RSYS constructs an object representing an air-based residential HVAC system.

**rsName**

Optional name of HVAC system; give after the word “RSYS” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**rsType=*choice***

Type of system.

--------------------------------------------------------------------
**rsType**      **Description**
--------------  --------------------------------------------------
ACFURNACE       Compressor-based cooling and fuel-fired
                heating.\
                Primary heating input energy
                is accumulated to end use HTG of meter
                rsFuelMtr.

ACRESISTANCE    Compressor-based cooling and electric
                (“strip”) heating. Primary heating
                input energy is accumulated to end use
                HTG of meter rsElecMtr.

ASHP            Air-source heat pump (compressor-based
                heating and cooling). Primary
                (compressor) heating input energy is
                accumulated to end use HTG of meter
                rsElecMtr. Auxiliary heating input
                energy is accumulated to end use HPHTG
                of meter rsElecMtr.

ASHPHYDRONIC    Air-to-water heat pump with hydronic distribution.
                Compressor performance is approximated using
                the air-to-air model with adjusted
                efficiencies.

AC              Compressor-based cooling; no heating.

FURNACE         Fuel-fired heating. Primary heating
                input energy is accumulated to end use
                HTG of meter rsFuelMtr.

RESISTANCE      Electric heating. Primary heating input
                energy is accumulated to end use HTG of
                meter rsElecMtr
------------------------------------------------------------------



  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              *one of above choices*   ACFURNACE        No             constant


**rsDesc=*string***

Text description of system, included as documentation in debugging reports such as those triggered by rsPerfMap=YES

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              string            *blank*       No             constant

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

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              OFF, HEAT, COOL, AUTO    AUTO          No             hourly

**rsPerfMap=*choice***

Generate performance map(s) for this RSYS. Comma-separated text is written to file PM\_[rsName].csv. This is a debugging capability that is not necessarily maintained.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              NO, YES           NO            No             constant

**rsFanTy=*choice***

Specifies fan (blower) position relative to cooling coil.

  **Units**   **Legal Range**      **Default**   **Required**   **Variability**
  ----------- -------------------- ------------- -------------- -----------------
              BLOWTHRU, DRAWTHRU   BLOWTHRU      No             constant

**rsFanMotTy=*choice***

Specifies type of motor driving the fan (blower). This is used in the derivation of the coil-only cooling capacity for the RSYS.

  ----- --------------------------------------
  PSC   Permanent split capacitor
  BPM   Brushless permanent magnet (aka ECM)
  ----- --------------------------------------

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              PSC, BPM          PSC           No             constant

**rsElecMtr=*mtrName***

Name of METER object, if any, by which system’s electrical energy use is recorded (under appropriate end uses).

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**rsFuelMtr =*mtrName***

Name of METER object, if any, by which system’s fuel energy use is recorded (under appropriate end uses).

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**rsAFUE=*float***

Heating Annual Fuel Utilization Efficiency (AFUE).

  **Units**   **Legal Range**   **Default**                         **Required**   **Variability**
  ----------- ----------------- ----------------------------------- -------------- -----------------
              0 $<$ x $\le$ 1   0.9 if furnace, 1.0 if resistance   No             constant

**rsCapH=*float***

Heating capacity, used when rsType is ACFURNACE, ACRESISTANCE, FURNACE, or RESISTANCE.

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
  Btu/hr      *AUTOSIZE* or x $\ge$ 0   0             No             constant

**rsTdDesH=*float***

Nominal heating temperature rise (across system, not zone) used during autosizing (when capacity is not yet known).

  **Units**   **Legal Range**   **Default**          **Required**   **Variability**
  ----------- ----------------- -------------------- -------------- -----------------
  ^o^F        *x* $>$ 0         30 if ASHP else 50   No             constant

**rsFxCapH=*float***

Heating autosizing capacity factor. If AUTOSIZEd, rsCapH or rsCap47 are set to rsFxCapH $\times$ (peak design-day load). Peak design-day load is the heating capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         1.4           No             constant


**rsFanPwrH=*float***

Heating fan power. Heating air flow is estimated based on a 50 ^o^F temperature rise.

<!--
  TODO – need input for temp rise?
-->
  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  W/cfm       *x* $\ge$ 0       .365          No             constant

**rsHSPF=*float***

For rsType=ASHP, Heating Seasonal Performance Factor (HSPF).

  **Units**   **Legal Range**   **Default**   **Required**         **Variability**
  ----------- ----------------- ------------- -------------------- -----------------
  Btu/Wh      *x* $>$ 0                       Yes if rsType=ASHP   constant

**rsCap47=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 47 ^o^F. If both rsCap47 and rsCapC are autosized, both are set to the larger consistent value.

  **Units**   **Legal Range**           **Default**            **Required**   **Variability**
  ----------- ------------------------- ---------------------- -------------- -----------------
  Btu/Wh      *AUTOSIZE* or *x* $>$ 0   Calculated from rsCapC   no             constant

**rsCap35=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 35 ^o^F.

  **Units**   **Legal Range**   **Default**                          **Required**   **Variability**
  ----------- ----------------- ------------------------------------ -------------- -----------------
  Btu/Wh      *x* $>$ 0         Calculated from rsCap47 and rsCap17   no             constant

**rsCap17=*float***

For rsType=ASHP, rated heating capacity at outdoor dry-bulb temperature = 17 ^o^F.

  **Units**   **Legal Range**   **Default**              **Required**   **Variability**
  ----------- ----------------- ------------------------ -------------- -----------------
  Btu/Wh      *x* $>$ 0         Calculated from rsCap47   no             constant

**rsCOP47=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 47 ^o^F.

  **Units**   **Legal Range**   **Default**                                   **Required**   **Variability**
  ----------- ----------------- --------------------------------------------- -------------- -----------------
              *x* $>$ 0         Estimated from rsHSPF, rsCap47, and rsCap17   no             constant

**rsCOP35=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 35 ^o^F.

  ------------------------------------------------------------------
  **Units** **Legal** **Default**       **Required** **Variability**
            **Range**                                        
  --------- --------- ----------------- ------------ ---------------
            *x* $>$ 0 Calculated from    no           constant
                      rsCap35, rsCap47,
                      rsCap17, rsCOP47,
                      and rsCOP17

  ------------------------------------------------------------------

**rsCOP17=*float***

For rsType=ASHP, rated heating coefficient of performance at outdoor dry-bulb temperature = 17 ^o^F.

  **Units**   **Legal Range**   **Default**                                   **Required**   **Variability**
  ----------- ----------------- --------------------------------------------- -------------- -----------------
              *x* $>$ 0          Calculated from rsHSPF, rsCap47, and rsCap17   no             constant

**rsCapAuxH=*float***

For rsType=ASHP, auxiliary electric (“strip”) heating capacity. If autosized, rsCapAuxH is set to the peak heating load in excess of heat pump capacity evaluated at the heating design temperature (Top.heatDsTDbO).

  **Units**   **Legal Range**             **Default**   **Required**   **Variability**
  ----------- --------------------------- ------------- -------------- -----------------
  Btu/hr      *AUTOSIZE* or *x* $\ge$ 0   0             no             constant

**rsFxCapAuxH=*float***

  Auxiliary heating autosizing capacity factor. If AUTOSIZEd, rsCapAuxH is set to rsFxCapAuxH $\times$ (peak design-day load). Peak design-day load is the heating capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0             1           No             constant

**rsCOPAuxH=*float***

For rsType=ASHP, auxiliary electric (“strip”) heating coefficient of performance. Energy use for auxiliary heat is accumulated to end use HPHTG of meter rsElecMtr (that is, auxiliary heat is assumed to be electric).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 0       1.0           no             constant

**rsSEER=*float***

Cooling rated Seasonal Energy Efficiency Ratio (SEER).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btu/Wh      *x* $>$ 0                       Yes            constant

**rsEER=*float***

Cooling Energy Efficiency Ratio (EER) at standard AHRI rating conditions (outdoor drybulb of 95 ^o^F and entering air at 80 ^o^F drybulb and 67 ^o^F wetbulb).

  **Units**   **Legal Range**   **Default**           **Required**   **Variability**
  ----------- ----------------- --------------------- -------------- -----------------
  Btu/Wh      *x* $>$ 0         Estimated from SEER   no             constant

**rsCapC=*float***

Cooling capacity at standard AHRI rating conditions. If rsType=ASHP and both rsCapC and rsCap47 are autosized, both are set to the larger consistent value.

  -----------------------------------------------------------------------
  **Units**  **Legal**       **Default**  **Required**    **Variability**
             **Range**                                    
  ---------- --------------- ------------ --------------- ---------------
  Btu/hr     *AUTOSIZE* or                Yes if rsType   constant
             *x* $\le$ 0 (x               includes        
             $>$ 0 coverted               cooling         
             to $<$ 0)                                    
  -----------------------------------------------------------------------

**rsTdDesC=*float***

Nominal cooling temperature fall (across system, not zone) used during autosizing (when capacity is not yet known).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* $<$ 0         -25           No             constant

**rsFxCapC=*float***

Cooling autosizing capacity factor. rsCapC is set to rsFxCapC $\times$ (peak design-day load). Peak design-day load is the cooling capacity that holds zone temperature at the thermostat set point during the *last substep* of all hours of all design days.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         1.4           No             constant

**rsFChg=*float***

Refrigerant charge adjustment factor.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         1             no             constant

**rsFSize=*float***

Compressor sizing factor.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $>$ 0         1             no             constant

**rsVFPerTon=*float***

Standard air volumetric flow rate per nominal ton of cooling capacity.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
  cfm/ton     150 $\le$ x $\le$ 500   350           no             constant

**rsFanPwrC=*float***

Cooling fan power.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  W/cfm       *x* $\ge$ 0       .365          No             constant

**rsASHPLockOutT=*float***

  Source air dry-bulb temperature below which the air source heat pump compressor does not operate.

  **Units**   **Legal Range**   **Default**         **Required**   **Variability**
  ----------- ----------------- ------------------- -------------- -----------------
  ^o^F                          (no lockout)          No             hourly


**rsCdH=*float***

  Heating cyclic degradation coefficient, valid only for compressor-based heating (heat pumps).

  ---------------------------------------------------------------------------------------------
  **Units**   **Legal Range**       **Default**                **Required**   **Variability**
  ----------- --------------------- -------------------------- -------------- -----------------
                0 $\le$ x $\le$ 0.5  ASHPHYDRONIC: 0.25\           No             hourly
                                     ASHP: derived from rsHSPF
  ---------------------------------------------------------------------------------------------

  **rsCdC=*float***

  Cooling cyclic degradation coefficient, valid for configurations having compressor-based cooling.


  **Units**   **Legal Range**       **Default**                **Required**   **Variability**
  ----------- --------------------- -------------------------- -------------- -----------------
                0 $\le$ x $\le$ 0.5  0                          No             hourly


  **rsDSEH=*float***

  Heating distribution system efficiency.  If given, (1-rsDSEH) of RSYS heating output is discarded.  Cannot be combined with more detailed DUCTSEG model.


  **Units**   **Legal Range**       **Default**                **Required**   **Variability**
  ----------- --------------------- -------------------------- -------------- -----------------
                0 < x < 1             (use DUCTSEG model)            No             hourly

  **rsDSEC=*float***

  Cooling distribution system efficiency.  If given, (1-rsDSEC) of RSYS cooling output is discarded.  Cannot be combined with more detailed DUCTSEG model.


  **Units**   **Legal Range**       **Default**                **Required**   **Variability**
  ----------- --------------------- -------------------------- -------------- -----------------
                0 < x < 1             (use DUCTSEG model)            No             hourly  


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

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
               NONE, FIXED, VARIABLE    NONE          No             constant

 **rsOAVVfDs=*float***

 Design air volume flow rate when RSYS is operating in OAV mode.


 **Units**   **Legal Range**       **Default**    **Required**               **Variability**
 ----------- --------------------- ------------ ------------------------ -----------------
    cfm         $\ge$ 0                         if rsOAVType $\ne$ NONE         constant  

 **rsOAVVfMinF=*float***

 Minimum air volume flow rate fraction when RSYS is operating in OAV mode.  When rsOAVType=VARIABLE, air flow rate is constrained to rsOAVVfMinF * rsOAVVfDs or greater.

 **Units**   **Legal Range**       **Default**                **Required**   **Variability**
 ----------- --------------------- -------------------------- -------------- -----------------
               0 $\le$ x $\le$ 1          0.2                         No             constant  


 **rsOAVFanPwr=*float***

 RSYS OAV-mode fan power.

-------------------------------------------------------------------------------------------------------
 **Units**   **Legal Range**       **Default**                        **Required**   **Variability**
 ----------- --------------------- ---------------------------------- -------------- -----------------
    W/cfm       0 < x $\le$ 5       per rsOAVTYPE\                         No             constant  
                                    \ \ FIXED: rsFanPwrC\
                                    \ \ VARIABLE: NightBreeze vendor\
                                    \ \ \ \ curve based on rsOAVvfDs
----------- --------------------- ---------------------------------- -------------- -----------------


**rsOAVTDbInlet=*float***

OAV inlet (source) air temperature.  Supply air temperature at the zone is generally higher due to fan heat.  Duct losses, if any, also alter the supply air temperature.

---------------------------------------------------------------------------
**Units** **Legal Range** **Default**          **Required** **Variability**
--------- --------------- -------------------- ------------ ---------------
  ^o^F                    Dry-bulb temperature No           hourly  
                          from weather file
---------------------------------------------------------------------------

**rsOAVTdiff=*float***

 OAV temperature differential.  When operating in OAV mode, the zone set point temperature is max( znTD, inletT+rsOAVTdiff).  Small values can result in inadvertent zone heating, due to fan heat.

 **Units**   **Legal Range**       **Default**                **Required**   **Variability**
 ----------- --------------------- -------------------------- -------------- -----------------
    ^o^F         > 0                      5 ^o^F                    No             hourly  

**rsOAVReliefZn=*znName***

Name of zone to which relief air is directed during RSYS OAV operation, typically an attic zone.  Relief air flow is included in the target zone's pressure and thermal balance.

 **Units**   **Legal Range**       **Default**  **Required**               **Variability**
 ----------- --------------------- ----------  -------------------------- -----------------
               *name of ZONE*                   if rsOAVType $\ne$ NONE       constant

**rsParElec=*float***

Parasitic electrical power.  rsParElec is unconditionally accumulated to rsElecMtr (if specified) and has no other effect.

**Units**   **Legal Range**       **Default**                **Required**   **Variability**
----------- --------------------- -------------------------- -------------- -----------------
  W                                0                           No             hourly  


**rsParFuel=*float***

Parasitic fuel use.  rsParFuel is unconditionally accumulated to rsFuelMtr (if specified) and has no other effect.


**Units**   **Legal Range**       **Default**                **Required**   **Variability**
----------- --------------------- -------------------------- -------------- -----------------
Btuh                                 0                          No             hourly  


**rsRhIn=*float***

Entering air relative humidity (for model testing).

  **Units**   **Legal Range**       **Default**                       **Required**   **Variability**
  ----------- --------------------- --------------------------------- -------------- -----------------
  W/cfm       0 $\le$ *x* $\le$ 1   Derived from entering air state   No             constant

**rsTdbOut=*float***

Air dry-bulb temperature at the outdoor portion of this system.

  **Units**   **Legal Range**   **Default**         **Required**   **Variability**
  ----------- ----------------- ------------------- -------------- -----------------
  ^o^F                          From weather file   No             hourly

**endRSYS**

Optionally indicates the end of the RSYS definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
