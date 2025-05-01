# BOILER

BOILERs are subObjects of HEATPLANTs (preceding Section 5.20). BOILERs supply heat, through their associated HEATPLANT, to HW coils and heat exchangers.

Each boiler has a pump. The pump operates whenever the boiler is in use; the pump generates heat in the water, which is added to the boiler's output. The pump heat is independent of load -- the model assumes a bypass valve keeps the water flow constant when the loads are using less than full flow -- except that the heat is assumed never to exceed the load.

### boilerName

Name of BOILER object, given immediately after the word BOILER. The name is used to refer to the boiler in heat plant stage commands.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**blrCap=*float***

Heat output capacity of this BOILER.

{{
  member_table({
    "units": "Btuh",
    "legal_range": "*x* $\\gt$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**blrEffR=*float***

Boiler efficiency at steady-state full load, as a fraction. 1.0 may be specified to model a 100% efficient boiler.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\lt$ *x* $\\le$ 1.0", 
    "default": "0.8",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrEirR=*float***

Boiler Energy Input Ratio: alternate method of specifying efficiency.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 1.0", 
    "default": "1/*blrEffR*",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrPyEi=*a, b, c, d***

Coefficients of cubic polynomial function of part load ratio (load/capacity) to adjust full-load energy input for part load operation. Up to four floats may be given, separated by commas, lowest order (i.e. constant term) coefficient first. If the given coefficients result in a polynomial whose value is not 1.0 when the input variable, part load ratio, is 1.0, a warning message will be printed and the coefficients will be normalized to produce value 1.0 at input 1.0.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": ".082597, .996764, 0.79361, 0.",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrMtr=*name of a METER***

Meter to which Boiler's input energy is accumulated; if omitted, input energy is not recorded.

{{
  member_table({
    "units": "",
    "legal_range": "name of a METER", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrpGpm=*float***

Boiler pump flow in gallons per minute: amount of water pumped from this boiler through the hot water loop supplying the HEATPLANT's loads (HW coils and heat exchangers) whenever boiler is operating.

{{
  member_table({
    "units": "gpm",
    "legal_range": "*x* $\\gt$ 0", 
    "default": "blrCap/10000",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrpHdloss=*float***

Boiler pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

{{
  member_table({
    "units": "ft H2O",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "114.45\*",
    "required": "No",
    "variability": "constant" 
  })
}}

\* may be temporary value for 10-31-92 version; prior value of 35 may be restored.

**blrpMotEff=*float***

Boiler pump motor efficiency.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\lt$ *x* $\\le$ 1.0", 
    "default": ".88",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrpHydEff=*float***

Boiler pump hydraulic efficiency

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\lt$ *x* $\\le$ 1.0", 
    "default": ".70",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrpMtr=*name of a METER***

Meter to which pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

The following four members permit specification of auxiliary input power use associated with the boiler under the conditions indicated.

{{ csv_table("blrAuxOn=*float*,            Auxiliary power used when boiler is running&comma; in proportion to its subhour average part load ratio (plr).
blrAuxOff=*float*,           Auxiliary power used when boiler is not running&comma; in proportion to 1 - plr.
blrAuxFullOff=*float*,       Auxiliary power used only when boiler is off for entire subhour; not used if the boiler is on at all during the subhour.
blrAuxOnAtAll=*float*,       Auxiliary power used in full value if boiler is on for any fraction of subhour.")
}}

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

The following four allow specification of meters to record boiler auxiliary energy use through blrAuxOn, blrAuxOff, blrFullOff, and blrAuxOnAtAll, respectively. End use category "Aux" is used.

**blrAuxOn=*float***

Additional input energy used in proportion to part load ratio when coil on, as for induced draft fan, hourly variable for unforeseen applications.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**blrAuxOnMtr=*mtrName***

Meter to which to charge *auxOn*.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrAuxOff=*float***

Additional input energy when off for part or all of subhour.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**blrAuxOffMtr=*mtrName***

Meter to which to charge *auxOff*.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrAuxFullOff=*float***

Additional input energy when off for an entire subhour.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**blrAuxFullOffMtr=*mtrName***

Meter to which to charge *blrAuxFullOff*.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**blrAuxOnAtall=*float***

Additional input energy used in coil on for any part of subhour, for unforeseen uses.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**blrAuxOnAtAllMtr=*mtrName***

MTR for "auxOnAtall"

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

### endBoiler

Optionally indicates the end of the boiler definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

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

- @[boiler][p_boiler]
