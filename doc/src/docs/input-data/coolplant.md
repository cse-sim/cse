# COOLPLANT

A COOLPLANT contains one or more CHILLER subobjects (Section 5.21.1). Each COOLPLANT supports one or more CHilled Water (CHW) cooling coils in AIRHANDLERs, and is supported by a TOWERPLANT (Section 5.22). The piping circuit connecting the cold-water (evaporator) side of the CHILLERs to the CHW coils is referred to as the *primary loop*; the piping connecting the warm-water (condenser) side of the CHILLERs to the cooling towers in the TOWERPLANT is referred to as the *secondary loop*. Flows in these loops are established primary and secondary (or heat rejection) by pumps in each CHILLER; these pumps operate when the CHILLER operates.

The modeling of the CHW coils, COOLPLANTs, and CHILLERs includes modeling the supply temperature of the water in the primary loop, that is, the water supplied from the COOLPLANT's operating CHILLER(s) to the CHW coils. If the (negative) heat demanded by the connected coils exceeds the plant's capacity, the temperature rises and the available power is distributed among the AIRHANDLERs according to the operation of the CHW coil model.

The primary water flow through each CHILLER is always at least that CHILLER's specified primary pump capacity -- it is assumed that any flow in excess of that used by the coils goes through a bypass value. When the coils request more flow than the pump's capacity, it is assumed the pressure drops and the pump can deliver the greater flow at the same power input and while imparting the same heat to the water. The primary water flow is not simulated during the run, but an error occurs before the run if the total design flow of the CHW coils connected to a COOLPLANT exceeds the pumping capacity of the CHILLERs in the plant's most powerful stage.

The CHILLERs in the COOLPLANT can be grouped into *STAGES* of increasing capacity. The COOLPLANT uses the first stage that can meet the load. The load is distributed amoung the CHILLERs in the active stage so that each operates at the same fraction of its capacity; CHILLERs not in the active stage are turned off.

For each COOLPLANT, primary loop piping loss is modeled, as a heat gain equal to a constant fraction of the CHILLER capacity of the COOLPLANT's most powerful stage. This heat gain is added to the load whenever the plant is operating; as modeled, the heat gain is independent of load, weather, which stage is operating, or any other variables. No secondary loop piping loss is modeled.

**coolplantName**

Name of COOLPLANT object, given immediately after the word COOLPLANT. This name is used to refer to the coolPlant in *ahhcCoolplant* commands.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**cpSched=*choice***

Coolplant schedule: hourly variable choice of OFF, AVAIL, or ON.

{{ csv_table("OFF, COOLPLANT will not supply chilled water regardless of demand. All loads (CHW coils) should be scheduled off when the plant is off; an error will occur if a coil calls for chilled water when its plant is off.
AVAIL, COOLPLANT will operate when one or more loads demand chilled water.
ON,      COOLPLANT runs unconditionally. When no load wants chilled water&comma; least powerful (first) stage runs anyway.")
}}

{{
  member_table({
    "units": "",
    "legal_range": "OFF, AVAIL, or ON", 
    "default": "AVAIL",
    "required": "No",
    "variability": "hourly" 
  })
}}

**cpTsSp=*float***

Coolplant primary loop supply temperature setpoint: setpoint temperature for chilled water supplied to coils.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* $\\gt$ 0", 
    "default": "44",
    "required": "No",
    "variability": "hourly" 
  })
}}

**cpPipeLossF=*float***

Coolplant pipe loss: heat assumed gained from primary loop piping connecting chillers to loads whenever the COOLPLANT is operating, expressed as a fraction of the chiller capacity of the plant's most powerful stage.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": ".01",
    "required": "No",
    "variability": "constant" 
  })
}}

**cpTowerplant=*name***

TOWERPLANT that cools the condenser water for the chillers in this COOLPLANT.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a TOWERPLANT*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**cpStage1=chillerName, chillerName, chillerName, ...**

**cpStage1=ALL\_BUT, chillerName, chillerName, chillerName, ...**

**cpStage2 through cpStage7 same**

The commands *cpStage1* through *cpStage7* allow specification of up to seven *STAGES* in which chillers are activated as the load increases. CSE will use the first stage that can meet the load; if no stage will meet the load (output the heat requested by the coils at *cpTsSp*), the last COOLPLANT stage is used.

Each stage may be specified with a list of up to seven names of CHILLERs in the COOLPLANT, or with the word ALL, meaning all of the COOLPLANT's CHILLERs, or with the word ALL\_BUT and a list of up to six names of CHILLERs. Each stage should be more powerful than the preceding one. If you have less than seven stages, you may skip some of the commands *cpStage1* through *cpStage7* -- the used stage numbers need not be contiguous.

If none of *cpStage1* through *cpStage7* are given, CSE supplies a single default stage containing all chillers.

A comma must be entered between chiller names and after the word ALL\_BUT.

{{
  member_table({
    "units": "",
    "legal_range": "1 to 7 names; ALL\_BUT and 1 to 6 names; ALL", 
    "default": "*cpStage1* = ALL",
    "required": "No",
    "variability": "constant" 
  })
}}

**endCoolplant**

Optionally indicates the end of the COOLPLANT definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

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

- @[coolPlant][p_coolplant]
