# DHWMETER

A DHWMETER object is a user-defined "device" that records water consumption as simulated by CSE.  The data accumulated by DHWMETERs can be reported at hourly, daily, monthly, and annual (run) intervals by using REPORTs and EXPORTs of type DHWMTR. Water use is reported in gallons.


DHWMETERs account for water use in the following pre-defined end uses.  The abbreviations in parentheses are used in DHWMTR report headings.

- Total water use (Total)
- Unknown end use (Unknown)
- Miscellaneous draws (Faucet)
- Shower (Shower)
- Bathtub (Bath)
- Clothes washer (CWashr)
- Dishwasher (DWashr)

[DHWSYS][dhwsys] items wsWHhwMtr and wsFXhwMtr specify the DHWMETER(s) to which water consumption is accumulated.

**dhwMtrName**

Name of meter: required for assigning water uses to the DHWMETER.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**endDhwMeter**

**Related Probes:**

- @[DHWmeter][p_dhwmeter]
