# SURFACE

Surface constructs a ZONE subobject of class SURFACE that represents a surrounding or interior surface of the zone. Internally, SURFACE generates a QUICK surface (U-value only), a DELAYED (massive) surface (using the finite-difference mass model), interzone QUICK surface, or interzone DELAYED surface, as appropriate for the specified construction and exterior conditions.

### sfName

Name of surface; give after the word SURFACE.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfType

Type: choice

Type of surface:

{{
  csv_table("FLOOR, Surface defines part or all of the 'bottom' of the zone; it is horizontal with inside facing up. The outside of the surface is not adjacent to the current zone.
WALL, Surface defines a 'side' of the zone; its outside is not adjacent to the current zone.
CEILING, Surface defines part or all of the 'top' of the zone with the inside facing down. The outside of the surface is not adjacent to the current zone.")
}}

sfType is used extensively for default determination and input checking, but does not have any further internal effect. The Floor, Wall, and Ceiling choices identify surfaces that form boundaries between the zone and some other condition.

{{
  member_table({
    "units": "",
    "legal_range": "FLOOR WALL CEILING", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### sfArea

Type: float

Gross area of surface. (CSE computes the net area for simulation by subtracting the areas of any windows and doors in the surface.).

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "*x* $>$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### sfTilt

Type: float

Surface tilt from horizontal. Values outside the range 0 to 360 are first normalized to that range. The default and allowed range depend on sfType, as follows:

{{
  csv_table("sfType = FLOOR, *sfTilt*=180&comma; default = 180 (fixed value)
sfType = WALL, 60 $<$ *sfTilt* $<$ 180&comma; default = 90
sfType = CEILING, 0 $\leq$ *sfTilt* $\leq$ 60&comma; default = 0")
}}

{{
  member_table({
    "units": "degrees",
    "legal_range": "Dependent upon *sfType*. See above.", 
    "default": "Dependent upon *sfType*. See above.",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfAzm

Type: float

Azimuth of surface with respect to znAzm. The azimuth used in simulating a surface is bldgAzm + znAzm + sfAzm; the surface is rotated if any of those are changed. Values outside the range 0 to 360 are normalized to that range. Required for non-horizontal surfaces.

{{
  member_table({
    "units": "degrees",
    "legal_range": "unrestricted", 
    "default": "*none*",
    "required": "Required if *sfTilt* $\\neq$ 0 and *sfTilt* $\\neq$ 180",
    "variability": "constant" 
  })
}}

### sfModel

Type: choice

Provides user control over how CSE models conduction for this surface.

{{
  csv_table("QUICK,                               Surface is modeled using a simple conductance. Heat capacity effects are ignored. Either sfCon or sfU (next) can be specified.
DELAYED&comma; DELAYED\_HOUR&comma; DELAYED\_SUBHOUR, Surface is modeled using a multi-layer finite difference technique that represents heat capacity effects. If the time constant of the surface is too short to accurately simulate&comma; a warning message is issued and the Quick model is used. The program **cannot** use the finite difference model if sfU rather than sfCon is specified.
AUTO, Program selects Quick or the appropriate Delayed automatically according to the time constant of the surface (if sfU is specified&comma; Quick is selected).
FD (or FORWARD\_DIFFERENCE),         Selects the forward difference model (used with short time steps and the CZM/UZM zone model).
KIVA,                                Uses a two-dimensional finite difference model to simulate heat flow through foundation surfaces.")
}}

{{
  member_table({
    "legal_range": "QUICK, DELAYED, DELAYED\_HOUR, DELAYED\_SUBOUR, AUTO, 2D_FND",
    "default": "AUTO", 
    "required": "No",
    "variability": "constant" 
  })
}}

<!--
TODO: better sfModel desciptions
-->
Either sfU or sfCon must be specified, but not both.

### sfU

Type: float

Surface U-value (NOT including surface (air film) conductances). For surfaces for which no heat capacity is to be modeled, allows direct entry of U-value without defining a CONSTRUCTION.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "Determined from *sfCon*",
    "required": "if *sfCon* not given and if *sfExCnd* is not GROUND",
    "variability": "constant" 
  })
}}

### sfCon

Type: conName

Name of CONSTRUCTION of the surface.

{{
  member_table({
    "units": "",
    "legal_range": "Name of a *CONSTRUCTION*", 
    "default": "*none*",
    "required": "if *sfU* not given and if *sfExCnd* is not GROUND",
    "variability": "constant" 
  })
}}

Note: When *sfExCnd* is GROUND, *sfCon* may be omitted to indicate:

- exposed earth (when *sfType* is FLOOR), or
- bare foundation wall (when *sfType* is WALL) -- the foundation wall 

### sfLThkF

Type: float

Sublayer thickness adjustment factor for FORWARD\_DIFFERENCE conduction model used with sfCon surfaces.  Material layers in the construction are divided into sublayers as needed for numerical stability.  sfLThkF allows adjustment of the thickness criterion used for subdivision.  A value of 0 prevents subdivision; the default value (0.5) uses layers with conservative thickness equal to half of an estimated safe value.  Fewer (thicker) sublayers improves runtime at the expense of accurate representation of rapid changes.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0.5",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfExCnd

Type: choice

Specifies the thermal conditions assumed at surface exterior, and at exterior of any subobjects (windows or doors) belonging to current surface. The conditions accounted for are dry bulb temperature and incident solar radiation.

{{
  csv_table("AMBIENT,        Exterior surface is exposed to the 'weather' as read from the weather file. Solar gain is calculated using solar geometry&comma; sfAzm&comma; sfTilt&comma; and sfExAbs.
SPECIFIEDT,     Exterior surface is exposed to solar radiation as in AMBIENT&comma; but the dry bulb temperature is calculated with a user specified function (sfExT). sfExAbs can be set to 0 to eliminate solar effects.
ADJZN,          Exterior surface is exposed to another zone&comma; whose name is specified by sfAdjZn. Solar gain is 0 unless gain is targeted to the surface with SGDIST below.
GROUND,         The surface is in contact with the ground. Details of the two-dimensional foundation design are defined by sfFnd. Only floor and wall surfaces may use this option.
ADIABATIC, Exterior surface heat flow is 0. Thermal storage effects of delayed surfaces are modeled.")
}}

### sfExAbs

Type: float

Surface exterior absorptivity.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "0.5",
    "required": "Required if *sfExCnd* = AMBIENT or *sfExCnd* = SPECIFIEDT",
    "variability": "monthly-hourly" 
  })
}}

### sfInAbs

Type: float

Surface interior solar absorptivity.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\le$ 1", 
    "default": "sfType = CEILING, 0.2; sfType = WALL, 0.6; sfType = FLOOR, 0.8",
    "required": "No",
    "variability": "monthly-hourly" 
  })
}}

### sfExEpsLW

Type: float

Surface exterior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "0.9",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfInEpsLW

Type: float

Surface interior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "0.9",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfExT

Type: float

Exterior air temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*unrestricted*", 
    "default": "*none*",
    "required": "Required if *sfExCnd* = SPECIFIEDT",
    "variability": "hourly" 
  })
}}

### sfAdjZn

Type: znName

Name of adjacent zone; used only when sfExCnd is ADJZN. Can be the same as the current zone.

{{
  member_table({
    "units": "",
    "legal_range": "name of a *ZONE*", 
    "default": "*none*",
    "required": "Required when<br/>*sfExCnd* = ADJZN",
    "variability": "constant" 
  })
}}

### sfGrndRefl

Type: float

Ground reflectivity for this surface.

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\\leq$ *x* $\\leq$ 1", 
    "default": "grndRefl",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

### sfInH

Type: float

Inside surface (air film) conductance. Ignored for sfModel = Forward\_Difference. Default depends on the surface type.

{{
  csv_table("sfType = FLOOR or CEILING, 1.32
other, 1.5")
}}

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "*See above*",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfExH

Type: float

Outside combined surface (air film) conductance. Ignored for sfModel = Forward\_Difference. The default value is dependent upon the exterior conditions:

{{
  csv_table("sfExCnd = AMBIENT,        dflExH (Top-level member&comma; described above)
sfExCnd = SPECIFIEDT,     dflExH (described above)
sfExCnd = ADJZN,          1.5
sfExCnd = ADIABATIC,      not applicable")
}}

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "*x* $>$ 0", 
    "default": "see above",
    "required": "No",
    "variability": "constant" 
  })
}}

When sfModel = Forward\_Difference, several models are available for calculating inside and outside surface convective coefficients.  Inside surface faces can be exposed only to zone conditions. Outside faces may be exposed either to ambient conditions or zone conditions, based on sfExCnd.  Only UNIFIED and INPUT are typically used.  The other models were used during CSE development for comparison.  For details, see CSE Engineering Documentation.

{{
  csv_table("Model,            Exposed to ambient,              Exposed to zone
UNIFIED,          default CSE model,               default CSE model
INPUT,            hc = sfExHcMult,                 hc = sfxxHcMult
AKBARI,           Akbari model,                    n/a
WALTON,           Walton model,                    n/a
WINKELMANN,       Winkelmann model,                n/a
DOE2,             DOE2 model,                      n/a
MILLS,            n/a,                             Mills model
ASHRAE,           n/a,                             ASHRAE handbook values
TARP,             n/a,                             TARP model", True)
}}

### sfExHcModel

Type: choice

<!-- TODO: How does this impact Kiva? -->

Selects the model used for exterior surface convection when sfModel = Forward\_Difference.

{{
  member_table({
    "units": "",
    "legal_range": "*choices above*", 
    "default": "UNIFIED",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfExHcLChar

Type: float

Characteristic length of surface, used in derivation of forced exterior convection coefficients in some models when outside surface is exposed to ambient.  See sfExHcModel.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $>$ 0", 
    "default": "10",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfExHcMult

Type: float

Exterior convection coefficient adjustment factor.  When sfExHcModel=INPUT, hc=sfExHcMult.  For other sfExHcModel choices, the model-derived hc is multiplied by sfExHcMult.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### sfExRf

Type: float

Exterior surface roughness factor.  Used only when surface is exposed to ambient (i.e. with wind exposure).  Typical values:

{{
  csv_table("Roughness Index,	   sfExRf,	 Example
1 (very rough),		 2.17,	   Stucco
2 (rough),          1.67, 	 Brick
3 (medium rough),	 1.52, 	 Concrete
4 (Medium smooth),	 1.13,	   Clear pine
5 (Smooth),         1.11,    Smooth plaster
6 (Very Smooth),		 1,		   Glass", True)
}}

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "sfExHcModel = WINKELMANN: 1.66 else 2.17",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfInHcModel

Type: choice

<!-- TODO How does this change for Kiva? -->

  Selects the model used for the inside (zone) surface convection when sfModel = Forward\_Difference.

{{
  member_table({
    "units": "",
    "legal_range": "*choices above (see sfExHcModel)*", 
    "default": "UNIFIED",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfInHcMult

Type: float

  Interior convection coefficient adjustment factor.  When sfInHcModel=INPUT, hc=sfInHcMult.  For other sfInHcModel choices, the model-derived hc is multiplied by sfInHcMult.  When sfInHcModel=UNIFIED, natural (buoyancy-driven) and forced convection coefficient values are combined according to TOP inHcCombinationMethod, then sfInHcMult is applied.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

### sfInHcFrcCoeffs

Type: float array

Specifies 3 coefficients for an alternative inside surface forced convection model (applicable only for sfInHCModel=UNIFIED).  When given, the inside surface forced convection coefficient for this surface is derived as follows:

      hcFrc = hConvF * (sfInHcFrcCoeffs[ 1] + scInHcFrcCoeffs[ 2] * ACH ^ sfInHcFrcCoeffs[ 3])

where hConvF is the convection adjustment factor (derived from elevation, see Top hConvMod) and ACH is the zone air change rate per hour from the prior simulation step (including heat pump water heater evaporator air flow).  This formulation is dangerously flexible, so caution is advised when selecting coefficient values.

The default hcFrc value (used when sfInHcFrCoeff is not provided) is hConvF * znHcFrcF * ACH ^ 0.8.

<%= member_table(
  units: "Btuh/ft^2^-^o^F",
  legal_range: "",
  default: "*see above*",
  required: "No",
  variability: "subhourly") %>


The items below give values associated with CSE's model for below grade surfaces (sfExCnd=GROUND).  See CSE Engineering Documentation for technical details.

### sfFnd

Type: fdName

Name of FOUNDATION applied to ground-contact Floor SURFACEs; used only for Floor SURFACEs when sfExCnd is GROUND.

{{
  member_table({
    "legal_range": "Name of a *Foundation*",
    "default": "*none*", 
    "required": "when<br/>*sfExCnd* = GROUND and <br/>*sfType* = Floor",
    "variability": "constant" 
  })
}}

### sfFndFloor

Type: sfName

Name of adjacent ground-contact Floor SURFACE; used only for Wall SURFACEs when sfExCnd is GROUND.

{{
  member_table({
    "legal_range": "Name of a *Surface*",
    "default": "*none*", 
    "required": "when<br/>*sfExCnd* = GROUND and <br/>*sfType* = Wall",
    "variability": "constant" 
  })
}}

### sfHeight

Type: float

Needed for foundation wall height, otherwise ignored. Maybe combine with sfDepthBG?

{{
  member_table({
    "units": "ft",
    "legal_range": "x $>$ 0", 
    "default": "*none*",
    "required": "when *sfType* is WALL and *sfExtCnd* is GROUND",
    "variability": "constant" 
  })
}}

### sfExpPerim

Type: float

Exposed perimeter of foundation floors.

{{
  member_table({
    "units": "ft",
    "legal_range": "x $\\geq$ 0", 
    "default": "*none*",
    "required": "when *sfType* is FLOOR, *sfFnd* is set, and *sfExtCnd* is GROUND",
    "variability": "constant" 
  })
}}


### sfDepthBG

Type: float

*Note: sfDepthBG is used as part of the simple ground model, which is no longer supported. Use sfHeight with sfFnd instead.*

Depth below grade of surface.  For walls, sfDepthBG is measured to the lower edge.  For floors, sfDepthBG is measured to the bottom face.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

*Note: The following data members are part of the simple ground model, which is no longer supported. Use sfFnd instead.*

### sfExCTGrnd, sfExCTaDbAvg07, sfExCTaDbAvg14, sfExCTaDbAvg31, sfExCTaDbAvgYr

Type: float

Conductances from outside face of surface to the weather file ground temperature and the moving average outdoor dry-bulb temperatures for 7, 14, 31, and 365 days.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "see above",
    "required": "No",
    "variability": "constant" 
  })
}}

### sfExRConGrnd

Type: float

Resistance overall construction resistance.  TODO: full documentation.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### endSURFACE

Optional to indicates the end of the surface definition. Alternatively, the end of the surface definition can be indicated by END, or by beginning another SURFACE or other object definition. If used, should follow the definitions of the SURFACE's subobjects -- DOORs, WINDOWs, SHADEs, SGDISTs, etc.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

<!--
The following tables summarize the defaults and legal ranges of surface members for each sfType. "n.a." indicates "not applicable" and also "not allowed".

  -------------------------------------------------------------------------
  **Member** **WALL**                  **FLOOR**    **CEILING**
  ---------- ------------------------- ------------ -----------------------
  sfTilt     optional, default=90, 60  n.a. (fixed  optional, default=0, 0
             $<$ *sfTilt* $<$ 180      at 180)      $\le$ *sfTilt* $\le$ 60

  sfAzm      **required**              n.a.         **required if sfTilt
                                                    $>$ 0**
  -------------------------------------------------------------------------

  -----------------------------------------------------------------------
  **Member**  **WALL**/**FLOOR**/**CEILING**
  ----------- ------------------------------------------------------------
  sfArea      **required**

  sfCon       **required unless sfU given, or if *sfExCnd* = GROUND**

  sfU         **required unless sfCon given, or if *sfExCnd* = GROUND**

  sfInH       optional, default = 1.5

  sfExH       optional, default per *sfExCnd*

  sfExCnd     optional, default = AMBIENT

  sfExAbs     optional if *sfExCnd* = AMBIENT or *sfExCnd* = SPECIFIEDT
              (default = .5), else n.a.

  sfExT       **required if sfExCnd = SPECIFIEDT; else n.a.**

  sfAdjZn     **required if sfExCnd = ADJZN; else n.a.**

  sfGrndRefl  optional, default to grndRefl
  -----------------------------------------------------------------------
  -->

**Related Probes:**

- @[surface][p_surface]
- @[xsurf][p_xsurf]
- @[mass][p_mass]
