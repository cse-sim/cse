# SHADE

SHADE constructs a subobject associated with the current WINDOW that represents fixed shading devices (overhangs and/or fins). A window may have at most one SHADE and only windows in vertical surfaces may have SHADEs. A SHADE can describe an overhang, a left fin, and/or a right fin; absence of any of these is specified by omitting or giving 0 for its depth. SHADE geometry can vary on a monthly basis, allowing modeling of awnings or other seasonal shading strategies.

<!--
  ??Add figure showing shading geometry; describe overhangs and fins.
-->
### shName

Name of shade; follows the word "SHADE" if given.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ohDepth

Type: *float*

Depth of overhang (from plane of window to outside edge of overhang). A zero value indicates no overhang.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### ohDistUp

Type: *float*

Distance from top of window to bottom of overhang.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### ohExL

Type: *float*

Distance from left edge of window (as viewed from the outside) to the left end of the overhang.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### ohExR

Type: *float*

Distance from right edge of window (as viewed from the outside) to the right end of the overhang.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### ohFlap

Type: *float*

Height of flap hanging down from outer edge of overhang.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### lfDepth

Type: *float*

Depth of left fin from plane of window. A zero value indicates no fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### lfTopUp

Type: *float*

Vertical distance from top of window to top of left fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### lfDistL

Type: *float*

Distance from left edge of window to left fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### lfBotUp

Type: *float*

Vertical distance from bottom of window to bottom of left fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### rfDepth

Type: *float*

Depth of right fin from plane of window. A 0 value indicates no fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### rfTopUp

Type: *float*

Vertical distance from top of window to top of right fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### rfDistR

Type: *float*

Distance from right edge of window to right fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### rfBotUp

Type: *float*

Vertical distance from bottom of window to bottom of right fin.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### endShade

Optional to indicate the end of the SHADE definition. Alternatively, the end of the shade definition can be indicated by END or the declaration of another object.

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

- @[shade][p_shade]
