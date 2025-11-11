# DOOR

DOOR constructs a subobject belonging to the current SURFACE. The azimuth, tilt, ground reflectivity and exterior conditions associated with the door are the same as those of the owning surface, although the exterior surface conductance and the exterior absorptivity can be altered.

### drName

Name of door.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### drArea

Type: float

Overall area of door.

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "x > 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### drModel

Type: choice

Provides user control over how CSE models conduction for this door:

{{
  csv_table("QUICK,                               Surface is modeled using a simple conductance. Heat capacity effects are ignored. Either drCon or drU (next) can be specified.
  DELAYED&comma; DELAYED_HOUR&comma; DELAYED_SUBOUR,  Surface is modeled using a multi-layer finite difference technique which represents heat capacity effects. If the time constant of the door is too short to accurately simulate&comma; a warning message is issued and the Quick model is used. drCon (next) must be specified -- the program cannot use the finite difference model if drU rather than drCon is specified.
  AUTO,                                Program selects Quick or appropriate Delayed automatically according to the time constant of the surface (if drU is specified&comma; Quick is selected).
  FD or FORWARD_DIFFERENCE,           Selects the forward difference model (used with short time steps and the CZM/UZM zone models)")
}}

{{
  member_table({
    "units": "",
    "legal_range": "*choices above*", 
    "default": "AUTO",
    "required": "No",
    "variability": "constant" 
  })
}}

Either drU or drCon must be specified, but not both.

### drU

Type: float

Door U-value, NOT including surface (air film) conductances. Allows direct entry of U-value, without defining a CONSTRUCTION, when no heat capacity effects are to be modeled.

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "x > 0", 
    "default": "Determined from *drCon*",
    "required": "if *drCon* not given",
    "variability": "constant" 
  })
}}

### drCon

Type: conName

Name of construction for door.

{{
  member_table({
    "units": "",
    "legal_range": "name of a *CONSTRUCTION*", 
    "default": "*none*",
    "required": "unless *drU* given",
    "variability": "constant" 
  })
}}

### drLThkF

Type: float

Sublayer thickness adjustment factor for FORWARD_DIFFERENCE conduction model used with drCon surfaces.  Material layers in the construction are divided into sublayers as needed for numerical stability.  drLThkF allows adjustment of the thickness criterion used for subdivision.  A value of 0 prevents subdivision; the default value (0.5) uses layers with conservative thickness equal to half of an estimated safe value.  Fewer (thicker) sublayers improves runtime at the expense of accurate representation of rapid changes.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "0.5",
    "required": "No",
    "variability": "constant" 
  })
}}

### drExAbs

Type: float

Door exterior solar absorptivity. Applicable only if sfExCnd of owning surface is AMBIENT or SPECIFIEDT.

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "x > 0", 
    "default": "same as owning surface",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### drInAbs

Type: float

Door interior solar absorptivity.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": "0.5",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### drExEpsLW

Type: float

Door exterior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": "0.9",
    "required": "No",
    "variability": "constant" 
  })
}}

### drInEpsLW

Type: float

Door interior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": "0.9",
    "required": "No",
    "variability": "constant" 
  })
}}

### drInH

Type: float

Door interior surface (air film) conductance. Ignored if drModel = Forward_Difference

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "x > 0", 
    "default": "same as owning surface",
    "required": "No",
    "variability": "constant" 
  })
}}

### drExH

Type: float

Door exterior surface (air film) conductance. Ignored if drModel = Forward_Difference

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "x > 0", 
    "default": "same as owning surface",
    "required": "No",
    "variability": "constant" 
  })
}}

  When drModel = Forward_Difference, several models are available for calculating inside and outside surface convective coefficients.  Inside surface faces can be exposed only to zone conditions. Outside faces may be exposed either to ambient conditions or zone conditions, based on drExCnd.  Only UNIFIED and INPUT are typically used.  The other models were used during CSE development for comparison.  For details, see CSE Engineering Documentation.

{{
  csv_table("Model,            Exposed to ambient,              Exposed to zone
  UNIFIED,          default CSE model,               default CSE model
  INPUT,            hc = drExHcMult,                 hc = drxxHcMult
  AKBARI,           Akbari model,                    n/a
  WALTON,           Walton model,                    n/a
  WINKELMANN,       Winkelmann model,                n/a
  MILLS,            n/a,                             Mills model
  ASHRAE,           n/a,                             ASHRAE handbook values", True)
}}

### drExHcModel

Type: choice

Selects the model used for exterior surface convection when drModel = Forward_Difference.

{{
  member_table({
    "units": "",
    "legal_range": "*choices above*", 
    "default": "UNIFIED",
    "required": "No",
    "variability": "constant" 
  })
}}

### drExHcLChar

Type: float

Characteristic length of surface, used in derivation of forced exterior convection coefficients in some models when outside face is exposed to ambient (i.e. to wind).

{{
  member_table({
    "units": "ft",
    "legal_range": "x > 0", 
    "default": "10",
    "required": "No",
    "variability": "constant" 
  })
}}

### drExHcMult

Type: float

Exterior convection coefficient adjustment factor.  When drExHcModel=INPUT, hc=drExHcMult.  For other drExHcModel choices, the model-derived hc is multiplied by drExHcMult.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### drExRf

Type: float

Exterior roughness factor.  Typical roughness values:

{{
  csv_table("Roughness Index,	    drExRf,	 Example
1 (very rough),		    2.17,	   Stucco
2 (rough),            1.67, 	 Brick
3 (medium rough),	    1.52, 	 Concrete
4 (Medium smooth),	  1.13,	   Clear pine
5 (Smooth),           1.11,    Smooth plaster
6 (Very Smooth),		  1,		   Glass", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "drExHcModel = WINKELMANN:\n  1.66\nelse\n  2.17",
    "required": "No",
    "variability": "constant" 
  })
}}

### drInHcModel

Type: choice

Selects the model used for the inside (zone) surface convection when drModel = Forward_Difference.

{{
  member_table({
    "units": "",
    "legal_range": "*choices above (see drExHcModel)*", 
    "default": "UNIFIED",
    "required": "No",
    "variability": "constant" 
  })
}}

### drInHcMult

Type: float

Interior convection coefficient adjustment factor.  When drInHcModel=INPUT, hc=drInHcMult.  For other drInHcModel choices, the model-derived hc is multiplied by drInHcMult.

{{
  member_table({
    "units": "",
    "legal_range": "x ≥ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### drInHcFrcCoeffs

Type: *float array*

Specifies 3 coefficients for an alternative inside surface forced convection model (applicable only for drInHcModel=UNIFIED).  When given, the inside surface forced convection coefficient for this surface is derived as follows:

      hcFrc = hConvF * (drInHcFrcCoeffs[ 1] + drInHcFrcCoeffs[ 2] * ACH ^ drInHcFrcCoeffs[ 3])

where hConvF is the convection adjustment factor (derived from elevation, see Top hConvMod) and ACH is the zone air change rate per hour from the prior simulation step (including heat pump water heater evaporator air flow).  This formulation is dangerously flexible, so caution is advised when selecting coefficient values.

The default hcFrc value (used when drInHcFrCoeff is not provided) is hConvF * znHcFrcF * ACH ^ 0.8.

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "", 
    "default": "*inherited from parent surface*",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### endDoor

Indicates the end of the door definition. Alternatively, the end of the door definition can be indicated by the declaration of another object or by END.

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

- @[door][p_door]
- @[xsurf][p_xsurf]
- @[mass][p_mass]
