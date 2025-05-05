# DHWTANK

DHWTANK constructs an object representing one or more unfired water storage tanks in a DHWSYS. DHWTANK heat losses contribute to the water heating load.

**wtName**

Optional name of tank; give after the word “DHWTANK” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**wtMult=*integer***

Number of identical tanks of this type. Any value $>1$ is equivalent to repeated entry of the same DHWTANK.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

Tank heat loss is calculated hourly (note that default heat loss is 0) --

$$\text{qLoss} = \text{wtMult} \cdot (\text{wtUA} \cdot (\text{wtTTank} - \text{wtTEx}) + \text{wtXLoss})$$

**wtUA=*float***

Tank heat loss coefficient.

{{
  member_table({
    "units": "Btuh/^o^F",
    "legal_range": "x $\\ge$ 0", 
    "default": "Derived from wtVol and wtInsulR",
    "required": "No",
    "variability": "constant" 
  })
}}

**wtVol=*float***

Specifies tank volume.

{{
  member_table({
    "units": "gal",
    "legal_range": "x $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**wtInsulR=*float***

Specifies total tank insulation resistance. The input value should represent the total resistance from the water to the surroundings, including both built-in insulation and additional exterior wrap insulation.

{{
  member_table({
    "units": "ft^2^-^o^F/Btuh",
    "legal_range": "x $\\ge$ 0.01", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**wtZone=*znName***

Zone location of DHWTANK regarding tank loss. The value of zero only valid if wtTEx is being used. Half of the heat losses go to zone air and the other goes to half radiant.

{{
  member_table({
    "units": "",
    "legal_range": "*Name of ZONE*", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**wtTEx=*float***

Tank surround temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "x $\\ge$ 0", 
    "default": "70",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wtTTank=*float***

Tank average water temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "$>$ 32 ^o^F", 
    "default": "Parent DHWSYSTEM wsTUse",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wtXLoss=*float***

Additional tank heat loss. To duplicate CEC 2016 procedures, this value should be used to specify the fitting loss of 61.4 Btuh.

{{
  member_table({
    "units": "Btuh",
    "legal_range": "(any)", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**endDHWTank**

Optionally indicates the end of the DHWTANK definition.

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

- @[DHWTank][p_dhwtank]
