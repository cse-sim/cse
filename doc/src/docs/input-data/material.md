# MATERIAL

MATERIAL constructs an object of class MATERIAL that represents a building material or component for later reference a from LAYER (see below). A MATERIAL so defined need not be referenced. MATERIAL properties are defined in a consistent set of units (all lengths in feet), which in some cases differs from units used in tabulated data. Note that the convective and air film resistances for the inside wall surface is defined within the SURFACE statements related to conductances.

### matName

Name of material being defined; follows the word "MATERIAL".

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### matThk

Type: float

Thickness of material. If specified, matThk indicates the discreet thickness of a component as used in construction assemblies. If omitted, matThk indicates that the material can be used in any thickness; the thickness is then specified in each LAYER using the material (see below).

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* > 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### matCond

Type: float

Conductivity of material. Note that conductivity is *always* stated for a 1 foot thickness, even when matThk is specified; if the conductance is known for a specific thickness, an expression can be used to derive matCond.

{{
  member_table({
    "units": "Btuh-ft/ft^2^-^o^F",
    "legal_range": "*x* > 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### matCondT

Type: float

Temperature at which matCond is rated. See matCondCT (next).

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* > 0", 
    "default": "70 ^o^F",
    "required": "No",
    "variability": "constant" 
  })
}}

### matCondCT

Type: float

Coefficient for temperature adjustment of matCond in the forward difference surface conduction model. Each hour (not subhour), the conductivity of layers using this material are adjusted as followslrCond = matCond * (1 + matCondCT*(T~layer~ – matCondT))

{{
  member_table({
    "units": "^o^F^-1^",
    "legal_range": "", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

Note: A typical value of matCondCT for fiberglass batt insulation is 0.00418 F^-1^

### matSpHt

Type: float

Specific heat of material.

{{
  member_table({
    "units": "Btu/lb-^o^F",
    "legal_range": "*x* ≥ 0", 
    "default": "0 (thermally massless)",
    "required": "No",
    "variability": "constant" 
  })
}}

### matDens

Type: float

Density of material.

{{
  member_table({
    "units": "lb/ft^3^",
    "legal_range": "*x* ≥ 0", 
    "default": "0 (massless)",
    "required": "No",
    "variability": "constant" 
  })
}}

### matRNom

Type: float

Nominal R-value per foot of material. Appropriate for insulation materials only and *used for documentation only*. If specified, the current material is taken to have a nominal R-value that contributes to the reported nominal R-value for a construction. As with matCond, matRNom is *always* stated for a 1 foot thickness, even when matThk is specified; if the nominal R-value is known for a specific thickness, an expression can be used to derive matRNom.

{{
  member_table({
    "units": "ft^2^-^o^F/Btuh",
    "legal_range": "*x* > 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### endMaterial

Optional to indicate the end of the material. Alternatively, the end of the material definition can be indicated by "END" or simply by beginning another object.

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

- @[material][p_material]
