# BATTERY

BATTERY describes input data for a model of an energy-storage system which is not tied to any specific energy storage technology. The battery model integrates the energy added and removed (accounting for efficiency losses). Note: although we use the term battery, the underlying model is flexible enough to model any energy storage system.

The modeler can set limits and constraints on capacities and flows and the associated efficiencies for this model.

### btName

Name of the battery system. Given after the word BATTERY.

{{
  member_table({
    "units": "",
    "legal_range": "63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**btMeter=*choice***

Name of a METER to which the BATTERY's charge/discharge energy flows are recorded. Battery energy flows are accumulated to meter end use "BT". Battery energy flows are seen from the standpoint of a "load" on the electric grid, so charges to the BATTERY system are positive values while discharges from the BATTERY system are negative values.

Note btMeter also determines the source for the probe value *loadSeen*.  See discussion and example under btChgReq (below).

{{
  member_table({
    "units": "",
    "legal_range": "meter name ", 
    "default": "*none* ",
    "required": "No",
    "variability": "constant" 
  })
}}

**btChgEff=*float***

The charging efficiency of storing electricity into the BATTERY system. A value of 1.0 means that no energy is lost and 100% of charge energy enters and is stored in the battery.

{{
  member_table({
    "units": "",
    "legal_range": "0 < x $\\le$ 1", 
    "default": "0.975",
    "required": "No",
    "variability": "hourly" 
  })
}}

**btDschgEff=*float***

The discharge efficiency for when the BATTERY system is discharging power. A value of 1.0 means that no energy is lost and 100% of discharge energy leaves the system.

{{
  member_table({
    "units": "",
    "legal_range": "0 < x $\\le$ 1", 
    "default": "0.975",
    "required": "No",
    "variability": "hourly" 
  })
}}

**btMaxCap=*float***

This is the maximum amount of energy that can be stored in the BATTERY system in kilowatt-hours. Once the BATTERY has reached its maximum capacity, no additional energy will be stored.

{{
  member_table({
    "units": "kWh",
    "legal_range": "x $\\ge$ 0", 
    "default": "16",
    "required": "No",
    "variability": "constant" 
  })
}}

**btInitSOE=*float***

The initial state of energy of the BATTERY system as a fraction of the total capacity. If `btInitSOE` is specified, the battery state-of-energy at the beginning of the actual simulation will be set to the amount specified, regardless of whether there was a warm-up period or not. If `btInitSOE` is NOT specififed, it will default to 1.0 (i.e., 100%) at the beginning of the warmup period (if any).

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ x $\\le$ 0", 
    "default": "1.0",
    "required": "No",
    "variability": "constant" 
  })
}}

**btInitCycles=*int***

The number of cycles on the battery at the beginning of the run.

<% if show_comments %>

Note: a more robust life model will need not only cycle counts but cycles by depth of discharge to capture "shallow cycling" vs "deep cycling". A further enhancement is to capture "time at temperature". We may want to look into the battery literature and also more into this application to better understand what kind of life model may be a good fit in terms of information requirements vs fidelity.

<% end %>

{{
  member_table({
    "units": "number of cycles",
    "legal_range": "x $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "runly" 
  })
}}

**btMaxChgPwr=*float***

The maximum rate at which the BATTERY can be charged in kilowatts (i.e., energy flowing *into* the BATTERY).

{{
  member_table({
    "units": "kW",
    "legal_range": "x $\\ge$ 0", 
    "default": "4",
    "required": "No",
    "variability": "hourly" 
  })
}}

**btMaxDschgPwr=*float***

The maximum rate at which the BATTERY can be discharged in kilowatts (i.e., energy flowing *out of* the BATTERY).

{{
  member_table({
    "units": "kW",
    "legal_range": "x $\\ge$ 0", 
    "default": "4",
    "required": "No",
    "variability": "hourly" 
  })
}}

**btControlAlg=*choice***

Selects charge/discharge control algorithm.  btChgReq (next) specifies the desired battery charge or discharge rate.  btControlAlg allows selection of alternative algorithms for deriving btChgReq.

{{ csv_table("DEFAULT,        btChgReq is used as input or defaulted (see below)
TDVPEAKSAVE,    btChgReq input (if any) is ignored.  Instead&comma; a California-specific algorithm is used that saves battery charge until peak TDV (Time Dependant Valuation) hours of the day&comma; shifting energy generated on site (e.g. PV) to supply feed the grid during critical periods.  The algorithm requires availability of hourly TDV data&comma; see Top.tdvFName.")
}}

Note btControlAlg has hourly variability, allowing dynamic algorithm selection.  In California compliance applications, TDVPEAKSAVE is typically used only on days with high TDV peaks.

{{
  member_table({
    "units": "",
    "legal_range": "DEFAULT or TDVPEAKSAVE", 
    "default": "DEFAULT",
    "required": "No",
    "variability": "hourly" 
  })
}}

**btChgReq=*float***

The power request to charge (or discharge if negative) the battery. If omitted, the default strategy is used (attempt to satisfy all loads and absorb all available excess power).  btChgReq and the default strategy requested power are literally *requests*: that is, more power will not be delivered than is available; more power will not be absorbed than capacity exits to store; and the battery's power limits will be respected.

btChgReq can be set by an expression to allow complex energy management/dispatch strategies.  These approaches typically involve the BATTERY probe value *loadSeen*, which is the net electricity use (not including the battery) on the meter specified by btMeter (above).  When loadSeen is positive, electricity input (e.g. from the grid) is required; when negative, excess electrical energy is available from photovoltaic generation. loadSeen can be used in btChgReq expressions such as --

    btChgReq = select(
        $hour>=9 && $hour<=20,-min(@BATTERY[ 1].loadSeen, 0), // hrs 9 - 20: charge when surplus energy
        default -max( @BATTERY[ 1].loadSeen, 0))              // else: discharge when energy is required

The sign conventions here are tricky.  min(@BATTERY[ 1].loadSeen, 0) produces a value <=0 that is the negative of the amount of surplus energy available.  A positive btChgReq value requests charging, hence "-" (minus sign) in front of the min().  Conversely, max( @BATTERY[ 1].loadSeen, 0) results in a value >= 0 indicating the net energy needed by the building.  To request discharge, btChgReq must be negative, so "-" is also needed in the discharge expression.  (The @BATTERY[1] references mean "this battery", assuming there is only one battery being modelled.  In multi-battery situations, the current BATTERY's index or name must be included within the "[  ]".)

{{
  member_table({
    "units": "kW",
    "legal_range": "", 
    "default": "btMeter net load",
    "required": "No",
    "variability": "hourly" 
  })
}}

**btUseUsrChg=*choice***

Former yes/no choice that currently has no effect.  Deprecated, will be removed in a future version.

### endBATTERY

Optionally indicates the end of the BATTERY definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none* ",
    "required": "No",
    "variability": "constant" 
  })
}}

<!--
Probes? Control strategies?

SOE

-->

**Related Probes:**

- @[battery][p_battery]
