# DHWLOOPBRANCH

DHWLOOPBRANCH constructs one or more objects representing a branch pipe from the preceding DHWLOOPSEG. A DHWLOOPSEG can have any number of DHWLOOPBRANCHs to represent pipe runs with differing sizes, insulation, or surrounding conditions.

### wbName

Optional name of segment; give after the word “DHWLOOPBRANCH” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbMult=*float***

Specifies the number of identical DHWLOOPBRANCHs. Note may be non-integer.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbLength=*float***

Length of branch.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbSize=*float***

Nominal size of pipe. CSE assumes the pipe outside diameter = size + 0.125 in.

{{
  member_table({
    "units": "in",
    "legal_range": "*x* $>$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**wbInsulK=*float***

Pipe insulation conductivity

{{
  member_table({
    "units": "Btuh-ft/ft^2^-^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "0.02167",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbInsulThk=*float***

Pipe insulation thickness

{{
  member_table({
    "units": "in",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbExH=*float***

Combined radiant/convective exterior surface conductance between insulation (or pipe if no insulation) and surround.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "1.5",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wbExCnd=*choice***

Specify exterior conditions.

{{
  csv_table("Choice, Description
ADIABATIC, Adiabatic on other side
AMBIENT, Ambient exterior
SPECT, Specify temperature
ADJZN, Adjacent zone
GROUND, Ground conditions", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "See table above", 
    "default": "SPECT",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbAdjZn=*float***

Boundary conditions for adjacent zones.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "0.0",
    "required": "No",
    "variability": "runly" 
  })
}}

**wbExTX=*float***

External boundary conditions.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "70.0",
    "required": "No",
    "variability": "runly" 
  })
}}

**wbFUA=*float***

Adjustment factor applied to branch UA.  UA is derived (from wbSize, wbLength, wbInsulK, wbInsulThk, and wbExH) and then multiplied by wbFUA.  Used to represent e.g. imperfect insulation.  Note that parent DHWLOOP wlFUA does not apply to DHWLOOPBRANCH (only DHWLOOPSEG)

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wbExT=*float***

Surrounding equivalent temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "*70*",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wbFlow=*float***

Branch flow rate assumed during draw.

{{
  member_table({
    "units": "gpm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "2",
    "required": "No",
    "variability": "hourly" 
  })
}}

**wbFWaste=*float***

Number of times during the hour when the branch volume is discarded.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

### endDHWLOOPBRANCH

Optionally indicates the end of the DHWLOOPBRANCH definition.

{{
  member_table({
    "units": "",
    "legal_range": "*none*", 
    "default": "*none*",
    "required": "No",
    "variability": "" 
  })
}}

**Related Probes:**

- @[DHWLoopBranch][p_dhwloopbranch]
