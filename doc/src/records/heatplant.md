# HEATPLANT

A HEATPLANT contains one or more BOILER subobjects (Section 5.20.1) and supports one or more Hot Water (HW) coils in TERMINALs and/or AIRHANDLERs, and/or heat exchangers in HPLOOPs (HPLOOPs are not implemented as of September 1992.). There can be more than one HEATPLANT in a simulation.

BOILERs, HW coils, and heat exchangers are modeled with simple heat-injection models. There is no explicit modeling of circulating hot water temperatures and flows; it is always assumed the temperature and flow at each load (HW coil or heat exchanger) are sufficient to allow the load to transfer any desired amount of heat up to its capacity. When the total heat demand exceeds the plant's capacity, the model reduces the capacity of each load until the plant is not overloaded. The reduced capacity is the same fraction of rated capacity for all loads on the HEATPLANT; any loads whose requested heat is less than the reduced capacity are unaffected.

The BOILERs in the HEATPLANT can be grouped into *STAGES* of increasing capacity. The HEATPLANT uses the first stage that can meet the load. The load is distributed among the BOILERs in the stage so that each operates at the same fraction of its rated capacity.

For each HEATPLANT, piping loss is modeled, as a constant fraction of the BOILER capacity of the heatPlant's most powerful stage. This heat loss is added to the load whenever the plant is operating; as modeled, the heat loss is independent of load, weather, or any other variables.

**heatplantName**

Name of HEATPLANT object, given immediately after the word HEATPLANT. This name is used to refer to the heatPlant in *tuhcHeatplant* and *ahhcHeatplant* <!-- and *_____* (for heat exchanger) --> commands.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 Yes            constant

**hpSched=*choice***

Heat plant schedule: hourly variable choice of OFF, AVAIL, or ON.

  ------- --------------------------------------------------------------
  OFF     HEATPLANT will not supply hot water regardless of demand. All
          loads (HW coils and heat exchangers) should be scheduled off
          when the plant is off; an error will occur if a coil calls for
          heat when its plant is off.

  AVAIL   HEATPLANT will operate when one or more loads demand heat.

  ON      HEATPLANT runs unconditionally. When no load wants heat, least
          powerful (first) stage runs.
  ------- --------------------------------------------------------------

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
              OFF, AVAIL, or ON   AVAIL         No             hourly

**hpPipeLossF=*float***

Heat plant pipe loss: heat assumed lost from piping connecting boilers to loads whenever the HEATPLANT is operating, expressed as a fraction of the boiler capacity of the plant's most powerful stage.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1   .01           No             constant

**hpStage1=boilerName, boilerName, boilerName, ...**

**hpStage1=ALL\_BUT, boilerName, boilerName, boilerName, ...**

**hpStage2 through hpStage7 same**

The commands *hpStage1* through *hpStage7* allow specification of up to seven *STAGES* in which BOILERs are activated as the load increases. Each stage may be specified with a list of up to seven names of BOILERs in the HEATPLANT, or with the word ALL, meaning all of the HEATPLANT's BOILERs, or with the word ALL\_BUT and a list of up to six names of BOILERs. Each stage should be more powerful than the preceding one. If you have less than seven stages, you may skip some of the commands *hpStage1* through *hpStage7* -- the used stage numbers need not be contiguous.

If none of *hpStage1* through *hpStage7* are given, CSE supplies a single default stage containing all boilers.

A comma must be entered between boiler names and after the word ALL\_BUT.

  **Units**   **Legal Range**                              **Default**        **Required**   **Variability**
  ----------- -------------------------------------------- ------------------ -------------- -----------------
              1 to 7 names;ALL\_BUT and 1 to 6 names;ALL   *hpStage1* = ALL   No             constant

**endHeatplant**

Optionally indicates the end of the HEATPLANT definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[heatPlant](#p_heatplant)
