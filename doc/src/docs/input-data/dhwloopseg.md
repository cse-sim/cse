# DHWLOOPSEG

DHWLOOPSEG constructs one or more objects representing a segment of the preceeding DHWLOOP. A DHWLOOP can have any number of DHWLOOPSEGs to represent the segments of the loop with possibly differing sizes, insulation, or surrounding conditions.

### wgName

Optional name of segment; give after the word “DHWLOOPSEG” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### wgTy

Type: *choice*

Specifies the type of segment.  RETURN segments, if any, must follow SUPPLY segments.

{{ csv_table("SUPPLY,    Indicates a supply segment (flow is sum of circulation and draw flow&comma; child DHWLOOPBRANCHs permitted).
  RETURN,    Indicates a return segment (flow is only due to circulation&comma; child DHWLOOPBRANCHs not allowed)")
}}

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### wgLength

Type: *float*

Length of segment.

{{
  member_table({
    "units": "ft",
    "legal_range": "x $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

### wgSize

Type: *float*

Nominal size of pipe. CSE assumes the pipe outside diameter = size + 0.125 in.

{{
  member_table({
    "units": "in",
    "legal_range": "x $>$ 0", 
    "default": "1",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### wgInsulK

Type: *float*

Pipe insulation conductivity

{{
  member_table({
    "units": "Btuh-ft/ft^2^-^o^F",
    "legal_range": "x $>$ 0", 
    "default": "0.02167",
    "required": "No",
    "variability": "constant" 
  })
}}

### wgInsulThk

Type: *float*

Pipe insulation thickness

{{
  member_table({
    "units": "in",
    "legal_range": "x $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### wgExH

Type: *float*

Combined radiant/convective exterior surface conductance between insulation (or pipe if no insulation) and surround.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "x $>$ 0", 
    "default": "1.5",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wgExT

Type: *float*

Surrounding equivalent temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "x $>$ 0", 
    "default": "70",
    "required": "No",
    "variability": "hourly" 
  })
}}

### wgFNoDraw

Type: *float*

Fraction of hour when no draw occurs.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "x $>$ 0", 
    "default": "70",
    "required": "No",
    "variability": "hourly" 
  })
}}

### endDHWLoopSeg

Optionally indicates the end of the DHWLOOPSEG definition.

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

- @[DHWLoopSeg][p_dhwloopseg]
