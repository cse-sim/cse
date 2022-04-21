# LOADMETER

A LOADMETER object is a user-defined "device" that records heating and cooling loads as computed by CSE. The user defines LOADMETERs and assigns them to ZONEs and/or RSYSs (see ZONE znLoadMtr and RSYS rsLoadMtr).

Loads are accumulated for subhour, hour, day, month, and annual intervals.  All values are in Btu.  Values >0 indicated heat into the process or zone -- thus heating loads are >0 and cooling loads are <0.

LOADMETER results must be reported using user-defined REPORTs or EXPORTs.  For example --

    REPORT rpType=UDT rpFreq=Month rpDayBeg=Jan 1 rpDayEnd=Dec 31
        REPORTCOL colHead="mon" colVal=$Month colWid=3
        REPORTCOL colHead="Heating" colVal=@LoadMeter[ 1].M.qHtg colDec=0 colWid=10
        REPORTCOL colHead="Cooling" colVal=@LoadMeter[ 1].M.qClg colDec=0 colWid=10


**ldMtrName**

Name of LOADMETER: required for assigning to ZONEs and RSYSs.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**endLOADMETER**

Indicates the end of the meter definition. Alternatively, the end of the meter definition can be indicated by the declaration of another object or by END.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[loadmeter](#p_loadmeter)
