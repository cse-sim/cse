# DUCTSEG

DUCTSEG defines a duct segment. Each RSYS has at most one return duct segment and at most one supply duct segment. That is, DUCTSEG input may be completely omitted to eliminate duct losses.

### dsName

Optional name of duct segment; give after the word “DUCTSEG” if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsTy

Type: choice

Duct segment type.

{{
  member_table({
    "units": "",
    "legal_range": "SUPPLY, RETURN", 
    "default": "",
    "required": "Yes",
    "variability": "constant" 
  })
}}

The surface area of a DUCTSEG depends on its shape. 0 surface area is legal (leakage only). DUCTSEG shape is modeled either as flat or round --

-   dsExArea specified: Flat. Interior and exterior areas are assumed to be equal (duct surfaces are flat and corner effects are neglected).
-   dsExArea *not* specified: Round. Any two of dsInArea, dsDiameter, and dsLength must be given. Insulation thickness is derived from dsInsulR and dsInsulMat and this thickness is used to calculate the exterior surface area. Overall inside-to-outside conductance is also calculated including suitable adjustment for curvature.

### dsBranchLen

Type: float

Average branch length.

{{
  member_table({
    "units": "ft",
    "legal_range": "x > 0", 
    "default": "-1.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsBranchCount

Type: integer

Number of branches.

{{
  member_table({
    "units": "",
    "legal_range": "x > 0", 
    "default": "-1",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsBranchCFA

Type: float

Floor area served per branch

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "x > 0", 
    "default": "-1.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsAirVelDs

Type: float

Specified air velocity design.

{{
  member_table({
    "units": "fpm",
    "legal_range": "x > 0", 
    "default": "-1.0",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsExArea

Type: float

Duct segment surface area at outside face of insulation for flat duct shape, see above.

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "x ≥ 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsInArea

Type: float

Duct segment inside surface area (at duct wall, duct wall thickness assumed negligible) for round shaped duct.

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "x ≥ 0", 
    "default": "Derived from dsDiameter and dsLength",
    "required": "(see above reduct shape)",
    "variability": "constant" 
  })
}}


### dsDiameter

Type: float

Duct segment round duct diameter (duct wall thickness assumed negligible)

{{
  member_table({
    "units": "ft",
    "legal_range": "x ≥ 0", 
    "default": "Derived from dsInArea and dsLength",
    "required": "(see above reduct shape)",
    "variability": "constant" 
  })
}}

### dsLength

Type: float

Duct segment length.

{{
  member_table({
    "units": "ft",
    "legal_range": "x ≥ 0", 
    "default": "Derived from dsInArea and dsDiameter",
    "required": "(see above reduct shape)",
    "variability": "constant" 
  })
}}

### dsExCnd

Type: choice

Conditions surrounding duct segment.

{{
  member_table({
    "units": "",
    "legal_range": "ADIABATIC, AMBIENT, SPECIFIEDT, ADJZN", 
    "default": "ADJZN",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsAdjZn

Type: znName

Name of zone surrounding duct segment; used only when dsExCon is ADJZN. Can be the same as a zone served by the RSYS owning the duct segment.

{{
  member_table({
    "units": "",
    "legal_range": "name of a *ZONE*", 
    "default": "*none*",
    "required": "Required when *dsExCon* = ADJZN",
    "variability": "constant" 
  })
}}

### dsEpsLW

Type: float

Exposed (i.e. insulation) outside surface exterior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": "0.9",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsExT

Type: float

Air dry-bulb temperature surrounding duct segment. <!-- TODO: what is humidity? -->

{{
  member_table({
    "units": "°F",
    "legal_range": "*unrestricted*", 
    "default": "*none*",
    "required": "Required if *sfExCnd* = SPECIFIEDT",
    "variability": "hourly" 
  })
}}

### dsInsulR

Type: float

Insulation thermal resistance *not including* surface conductances. dsInsulR and dsInsulMat are used to calculate insulation thickness (see below).  Duct insulation is modeled as a pure conductance (no mass).

{{
  member_table({
    "units": "ft^2^-°F-hr / Btu",
    "legal_range": "x ≥ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsInsulMat

Type: matName

Name of insulation MATERIAL. The conductivity of this material at 70 °F is combined with dsInsulR to derive the duct insulation thickness. If omitted, a typical fiberglass material is assumed having conductivity of 0.025 Btu/hr-ft^2^-F at 70 °F and a conductivity coefficient of .00418 1/F (see MATERIAL). In addition, insulation conductivity is adjusted during the simulation in response its average temperature.  As noted with dsInsulR, duct insulation is modeled as pure conductance -- MATERIAL matDens and matSpHt are ignored.

{{
  member_table({
    "units": "",
    "legal_range": "name of a *MATERIAL*", 
    "default": "fiberglass",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsLeakF

Type: float

Duct leakage. Return duct leakage is modeled as if it all occurs at the segment inlet. Supply duct leakage is modeled as if it all occurs at the outlet.

{{
  member_table({
    "units": "",
    "legal_range": "0 $<$ x ≤ 1", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### dsExH

Type: float

Outside (exposed) surface convection coefficient.

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "x ≥ 0", 
    "default": ".54",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### endDuctSeg

Optionally indicates the end of the DUCTSEG definition.

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

- @[ductSeg][p_ductseg]
- @[izXfer][p_izxfer] (generated as "\<Zone Name\>-DLkI" for supply or "\<Zone Name\>-DLkO" for return)
