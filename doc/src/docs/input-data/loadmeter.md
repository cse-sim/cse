# LOADMETER

A LOADMETER object is a user-defined "device" that records heating and cooling loads as computed by CSE. The user defines LOADMETERs and assigns them to ZONEs and/or RSYSs (see ZONE znLoadMtr and RSYS rsLoadMtr).

Loads are accumulated for subhour, hour, day, month, and annual intervals.  All values are in Btu.  Values >0 indicated heat into the process or zone -- thus heating loads are >0 and cooling loads are <0.

LOADMETER results must be reported using user-defined REPORTs or EXPORTs.  For example --

    REPORT rpType=UDT rpFreq=Month rpDayBeg=Jan 1 rpDayEnd=Dec 31
        REPORTCOL colHead="mon" colVal=$Month colWid=3
        REPORTCOL colHead="Heating" colVal=@LoadMeter[ 1].M.qHtg colDec=0 colWid=10
        REPORTCOL colHead="Cooling" colVal=@LoadMeter[ 1].M.qClg colDec=0 colWid=10


### ldMtrName

Name of LOADMETER: required for assigning to ZONEs and RSYSs.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### lmtSubmeters

Type: *list of up to 50 LOADMETERs*

A comma-separate list of LOADMETERs that are accumulated into this LOADMETER with optional multipliers (see lmtSubmeterMults).  Submeters facilitate flexible categorization of loads results.  In addition, use of lmtSubmeterMults allows load results from a representative model to be scaled and included in overall results.  For example, a typical zone could be used to represent 5 similar spaces.  The loads calculated for the typical zone could be assigned to a dedicated LOADMETER and that LOADMETER accumulated to a main LOADMETER with a multiplier of 5.  Rules --

-  A LOADMETER cannot reference itself as a submeter.
-  A given LOADMETER can be referenced only once in the lmtSubmeters list.
-  Circular references are not allowed.

{{
  member_table({
    "units": "",
    "legal_range": "*names of LOADMETERs*", 
    "default": "",
    "required": "No",
    "variability": "constant" 
  })
}}

### lmtSubmeterMults

Type: *list of up to 50 floats*

Submeter multipliers.

A note re default values: if lmtSubmeterMults is omitted, all multipliers are defaulted to 1.  However, when lmtSubmeterMults is included, a multiplier value should be provided for each LOADMETER listed in lmtSubmeters since unspecified values are set to 0.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}


### endLOADMETER

Indicates the end of the meter definition. Alternatively, the end of the meter definition can be indicated by the declaration of another object or by END.

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

- @[loadmeter][p_loadmeter]
