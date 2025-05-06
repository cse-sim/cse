# LAYER

LAYER constructs a subobject of class LAYER belonging to the current CONSTRUCTION. LAYER is not recognized except immediately following CONSTRUCTION or another LAYER. The members represent one layer (that optionally includes framing) within the CONSTRUCTION.

The layers should be specified in inside to outside order. <!-- TODO: this order is unfortunate! --> A framed layer (lrFrmMat and lrFrmFrac given) is modeled by creating a homogenized material with weighted combined conductivity and volumetric heat capacity. Caution: it is generally preferable to model framed constructions using two separate surfaces (one with framing, one without). At most one framed layer (lrFrmMat and lrFrmFrac given) is allowed per construction.

The layer thickness may be given by lrThk, or matThk of the material, or matThk of the framing material if any. The thickness must be specified at least one of these three places; if specified in more than one place and not consistent, an error message occurs.

### lrName

Name of layer (follows "LAYER"). Required only if the LAYER is later referenced in another object, for example with LIKE or ALTER; however, we suggest naming all objects for clearer error messages and future flexibility.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### lrMat

Type: matName

Name of primary MATERIAL in layer.

{{
  member_table({
    "units": "",
    "legal_range": "name of a *MATERIAL*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### lrThk

Type: float

Thickness of layer.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $>$ 0", 
    "default": "Required if *matThk* not specified in referenced *lrMat*",
    "required": "No",
    "variability": "constant" 
  })
}}

### lrFrmMat

Type: matName

Name of framing MATERIAL in layer, if any. At most one layer with lrFrmMat is allowed per CONSTRUCTION. See caution above regarding framed-layer model.

{{
  member_table({
    "units": "",
    "legal_range": "name of a MATERIAL", 
    "default": "*no framed layer*",
    "required": "No",
    "variability": "constant" 
  })
}}

### lrFrmFrac

Type: float

Fraction of layer that is framing. Must be specified if frmMat is specified. See caution above regarding framed-layer model.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\leq$ *x* $\\leq$ 1", 
    "default": "*no framed layer*",
    "required": "Required if  *lrFrmMat* specified, else disallowed",
    "variability": "constant" 
  })
}}

### endLayer

Optional end-of-LAYER indicator; LAYER definition may also be indicated by "END" or just starting the definition of another LAYER or other object.

**Related Probes:**

- @[layer][p_layer]
