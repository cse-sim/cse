# DHWLOOP

DHWLOOP constructs one or more objects representing a domestic hot water circulation loop. The actual pipe runs in the DHWLOOP are specified by any number of DHWLOOPSEGs (see below). Circulation pumps are specified by DHWLOOPPUMPs (also below).

### wlName

Optional name of loop; give after the word “DHWLOOP” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**wlMult=*integer***

Number of identical loops of this type. Any value $>1$ is equivalent to repeated entry of the same DHWLOOP (and all child objects).

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wlFlow=*float***

Loop flow rate (when operating).

{{
  member_table({
    "units": "gpm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "6",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wlTIn1=*float***

Inlet temperature of first DHWLOOPSEG.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "DHWSYS wsTUse",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wlRunF=*float***

Fraction of hour that loop circulation operates.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wlFUA=*float***

DHWLOOPSEG pipe heat loss adjustment factor.  DHWLOOPSEG UA is derived (from wgSize, wgLength, wgInsulK, wgInsulThk, and wgExH) and multiplied by wlFUA.  Note: does not apply to child DHWLOOPBRANCHs (see wbFUA).

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wlLossMakeupPwr=*float***

Specifies electrical power available to make up losses from DHWLOOPSEGs (loss from DHWLOOPBRANCHs is not included). Separate loss makeup is typically used in multi-unit HPWH systems to avoid inefficiencies associated with high condenser temperatures.  Loss-makeup energy is calculated hourly and is the smaller of loop losses and wlLossMakeupPwr.  The resulting electricity use (including the effect of wlLossMakeupEff) is accumulated to the METER specified by wlElecMtr (end use dhwMFL). No other effect, such as heat gain to surroundings, is modeled.

{{
  member_table({
    "units": "W",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wlLossMakeupEff=*float***

Specifies the efficiency of loss makeup heating if any.  No effect when wlLossMakeupPwr is 0.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wlElecMtr=*mtrName***

Name of METER object, if any, to which DHWLOOP electrical energy use is recorded (under end use dhwMFL).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*Parent DHWSYS wsElecMtr*",
    "required": "No",
    "variability": "constant" 
  })
}}

### endDHWLoop

Optionally indicates the end of the DHWLOOP definition.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "No",
    "variability": "" 
  })
}}

**Related Probes:**

- @[DHWLoop][p_dhwloop]
