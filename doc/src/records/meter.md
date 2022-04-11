# METER

A METER object is a user-defined "device" that records energy consumption of equipment as simulated by CSE. The user defines METERs with the desired names, then assigns energy uses of specific equipment to the desired meters using commands described under each equipment type's class description (AIRHANDLER, TERMINAL, etc.). Additional energy use from equipment not simulated by CSE (except optionally for its effect on heating and cooling loads) can also be charged to METERs (see GAIN). The data accumulated by meters can be reported at hourly, daily, monthly, and annual (run) intervals by using REPORTs and EXPORTs of type MTR.

Meters account for energy use in the following pre-defined categories, called *end uses*. The abbreviations in parentheses are used in MTR report headings (and for gnMeter input, below). You also get a column for the net total on the meter (abbreviated "Tot").

<%= insert_file('doc/src/enduses.md') %>

The user has complete freedom over how many meters are defined and how equipment is assigned to them. At one extreme, a single meter "Electricity" could be defined and have all of electrical uses assigned to it. On the other hand, definition of separate meters "Elect\_Fan1", "Elect\_Fan2", and so forth allows accounting of the electricity use for individual pieces of equipment. Various groupings are possible: for example, in a building with several air handlers, one could separate the energy consumption of the fans from the coils, or one could separate the energy use by air handler, or both ways, depending on the information desired from the run.

The members that assign energy use to meters include:

-   GAIN: gnMeter, gnEndUse
-   ZONE: xfanMtr
-   IZXFER: izfanMtr
-   RSYS: rsElecMtr, rsFuelMtr
-   DHWSYS: wsElecMtr, wsFuelMtr
-   DHWHEATER: whElectMtr, whFuelMtr
-   DHWPUMP: wpElecMtr
-   DHWLOOPPUMP: wlpElecMtr
-   PVARRAY: pvElecMeter
-   TERMINAL: tuhcMtr, tfanMtr
-   AIRHANDLER: sfanMtr, rfanMtr, ahhcMtr, ahccMtr, ahhcAuxOnMtr, ahhcAuxOffMtr, ahhcAuxFullOnMtr, ahhcAuxOnAtAllMtr, ahccAuxOnMtr, ahccAuxOffMtr, ahccAuxFullOnMtr, ahccAuxOnAtAllMtr
-   BOILER: blrMtr, blrpMtr, blrAuxOnMtr, blrAuxOffMtr, blrAuxFullOnMtr, blrAuxOnAtAllMtr
-   CHILLER: chMtr, chppMtr, chcpMtr, chAuxOnMtr, chAuxOffMtr, chAuxFullOnMtr, chAuxOnAtAllMtr
-   TOWERPLANT: tpMtr

The end use can be specified by the user only for GAINs and PVARRAYs; in other cases it is hard-wired to Clg, Htg, FanC, FanH, FanV, Fan, or Aux as appropriate.

**mtrName**

Name of meter: required for assigning energy uses to the meter elsewhere.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        Yes            constant

**mtrDemandRate=*float***

DmdCost per Btu of demand, for a month.

<%= member_table(
  units: "",
  legal_range: "",
  default: "N/A",
  required: "No",
  variability: "constant") %>

**mtrRate=*float***

Cost of energy use per Btu.

<%= member_table(
  units: "",
  legal_range: "",
  default: "N/A",
  required: "No",
  variability: "constant") %>

**endMeter**

Indicates the end of the meter definition. Alternatively, the end of the meter definition can be indicated by the declaration of another object or by END.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[meter](#p_meter)
