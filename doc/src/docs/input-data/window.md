# WINDOW

WINDOW defines a subobject belonging to the current SURFACE that represents one or more identical windows. The azimuth, tilt, and exterior conditions of the window are the same as those of the surface to which it belongs. The total window area (*wnHt* $\cdot$ *wnWid* $\cdot$ *wnMult*) is deducted from the gross surface area. A surface may have any number of windows.

Windows may optionally have operable interior shading that reduces the overall shading coefficient when closed.

**wnName**

Name of window: follows the word "WINDOW" if given.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnHeight=*float***

Overall height of window (including frame).

{{
  member_table({
    "units": "ft",
    "legal_range": "x $>$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**wnWidth=*float***

Overall width of window (including frame).

{{
  member_table({
    "units": "ft",
    "legal_range": "x $>$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**wnArea=*float***

Overall area of window (including frame).

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "x $>$ 0", 
    "default": "*wnHeight* \* *wnWidth*",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnMult=*float***

Area multiplier; can be used to represent multiple identical windows.

{{
  member_table({
    "units": "",
    "legal_range": "x $>$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnModel=*choice***

Selects window model

{{
  member_table({
    "units": "",
    "legal_range": "SHGC, ASHWAT", 
    "default": "SHGC",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnGt=*choice***

GLAZETYPE for window. Provides many defaults for window properties as cited below.

**wnU=*float***

Window conductance (U-factor without surface films, therefore not actually a U-factor but a C-factor).

Preferred Approach: To use accurately with standard winter rated U-factor from ASHRAE or NFRC enter as:

        wnU = (1/((1/U-factor)-0.85)

Where 0.85 is the sum of the interior (0.68) and exterior (0.17) design air film resistances assumed for rating window U-factors. Enter wnInH (usually 1.5=1/0.68) instead of letting it default. Enter the wnExH or let it default. It is important to use this approach if the input includes gnFrad for any gain term. Using approach 2 below will result in an inappropriate internal gain split at the window.

Approach 2. Enter wnU=U-factor and let the wnInH and wnExH default. Tnormally this approach systematically underestimates the window U-factor because it adds the wnExfilm resistance to 1/U-factor thereby double counting the exterior film resistance. This approach will also yield incorrect results for gnFrad internal gain since the high wnInH will put almost all the gain back in the space.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "x $>$ 0", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**wnUNFRC=*float***

Fenestration system (including frame) U-factor evaluated at NFRC heating conditions.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "x $>$ 0", 
    "default": "gtUNFRC",
    "required": "Required when *wnModel* = ASHWAT",
    "variability": "constant" 
  })
}}

**wnExEpsLW=*float***

Window exterior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "0.84",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnInEpsLW=*float***

Window interior long wave (thermal) emittance.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "0.84",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnInH=*float***

Window interior surface (air film) conductance.

Preferred Approach: Enter the appropriate value for each window, normally:

        wnInH = 1.5

        where 1.5 = 1/0.68 the standard ASHRAE value.

The large default value of 10,000 represents a near-0 resistance, for the convenience of those who wish to include the interior surface film in wnU according to approach 2 above.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "x $>$ 0", 
    "default": "10000",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnExH=*float***

Window exterior surface (air film) conductance.

{{
  member_table({
    "units": "Btuh/ft^2^-^o^F",
    "legal_range": "x $>$ 0", 
    "default": "same as owning surface",
    "required": "No",
    "variability": "constant" 
  })
}}

Several models are available for calculating inside and outside surface convective coefficients.  Inside surface faces can be exposed only to zone conditions. Outside faces may be exposed either to ambient conditions or zone conditions, based on wnExCnd.  Only UNIFIED and INPUT are typically used.  The other models were used during CSE development for comparison.  For details, see CSE Engineering Documentation.

<%= csv_table(<<END, :row_header => true)
Model, Exposed to ambient, Exposed to zone
UNIFIED,          default CSE model, default CSE model
INPUT  ,          hc = wnExHcMult, hc = wnxxHcMult
AKBARI  ,         Akbari model, n/a
WALTON   ,        Walton model, n/a
WINKELMANN,       Winkelmann model,                n/a
MILLS    ,        n/a      ,                       Mills model
ASHRAE    ,       n/a       ,                      ASHRAE handbook values
END
%>

**wnExHcModel=*choice***

Selects the model used for exterior surface convection when wnModel = Forward\_Difference.

{{
  member_table({
    "units": "",
    "legal_range": "*choices above*", 
    "default": "UNIFIED",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnExHcLChar=*float***

Characteristic length of surface, used in derivation of forced exterior convection coefficients in some models when outside face is exposed to ambient (i.e. to wind).

{{
  member_table({
    "units": "ft",
    "legal_range": "x $>$ 0", 
    "default": "10",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnExHcMult=*float***

Exterior convection coefficient adjustment factor.  When wnExHcModel=INPUT, hc=wnExHcMult.  For other wnExHcModel choices, the model-derived hc is multiplied by wnExHcMult.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**wnInHcModel=*choice***

Selects the model used for the inside (zone) surface convection when wnModel = Forward\_Difference.

{{
  member_table({
    "units": "",
    "legal_range": "*choices above (see wnExHcModel)*", 
    "default": "UNIFIED",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnInHcMult=*float***

Interior convection coefficient adjustment factor.  When wnInHcModel=INPUT, hc=wnInHcMult.  For other wnInHcModel choices, the model-derived hc is multiplied by wnInHcMult.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "1",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**wnSHGC=*float***

Rated Solar Heat Gain Coefficient (SHGC) for the window assembly.

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 < x < 1", 
    "default": "gtSHGC",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnFMult=*float***

Frame area multiplier = areaGlaze / areaAssembly

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 < x < 1", 
    "default": "gtFMult or 1",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnSMSO=*float***

SHGC multiplier with shades open. Overrides gtSMSO.

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\\leq$ *x* $\\leq$ 1", 
    "default": "gtSMSO or 1",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

**wnSMSC=*float***

SHGC multiplier with shades closed. Overrides gtSMSC

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\leq$ *x* $\leq$ 1", 
    "default": "wnSMSO or gtSMSC",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

**wnNGlz=*int***

Number of glazings in the window (bare glass only, not including any interior or exterior shades).

{{
  member_table({
    "units": "",
    "legal_range": "0 $<$ *x* $\leq$ 4", 
    "default": "gtNGLZ",
    "required": "Required when *wnModel* = ASHWAT",
    "variability": "constant" 
  })
}}

**wnExShd=*choice***

Exterior shading type (ASHWAT only).

{{
  member_table({
    "units": "",
    "legal_range": "NONE, INSCRN", 
    "default": "gtExShd",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnInShd=*choice***

Interior shade type (ASHWAT only).

{{
  member_table({
    "units": "",
    "legal_range": "NONE, DRAPEMED", 
    "default": "gtInShd",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnDirtLoss=*float***

Glazing dirt loss factor.

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\\leq$ *x* $\\leq$ 1", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**wnGrndRefl=*float***

Ground reflectivity for this window.

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\\leq$ *x* $\\leq$ 1", 
    "default": "sfGrndRefl",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

**wnVfSkyDf=*float***

View factor from this window to sky for diffuse radiation. For the shading effects of an overhang, a wnVfSkyDf value smaller than the default would be used

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\\leq$ x $\\leq$ 1", 
    "default": "0.5 - 0.5 cos(tilt) = .5 for vertical surface",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

**wnVfGrndDf=*float***

View factor from this window to ground for diffuse radiation. For the shading effects of a fin(s), both wnVfSkyDf and wnVfGrndDf would be used.

{{
  member_table({
    "units": "fraction",
    "legal_range": "0 $\\leq$ x $\\leq$ 1", 
    "default": "0.5 + 0.5 .5 for vertical surface",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

**endWINDOW**

Optionally indicates the end of the window definition. Alternatively, the end of the window definition can be indicated by END or the declaration of another object. END or endWindow, if used, should follow any subobjects of the window (SHADEs and/or SGDISTs).

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

- @[window][p_window]
- @[xsurf][p_xsurf]
