# GLAZETYPE

GLAZETYPE constructs an object of class GLAZETYPE that represents a glazing type for use in WINDOWs.

### gtName

Name of glazetype. Required for reference from WINDOW objects, below.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### gtModel

Type: choice

Selects model to be used for WINDOWs based on this GLAZETYPE.

{{
  member_table({
    "units": "",
    "legal_range": "SHGC, ASHWAT", 
    "default": "SHGC",
    "required": "No",
    "variability": "constant" 
  })
}}

### gtU

Type: float

Glazing conductance (U-factor without surface films, therefore not actually a U-factor but a C-factor). Used as wnU default; an error message will be issued if the U value is not given in the window (wnU) nor in the glazeType (gtU). <!-- TODO: rename gtC? (Also wnU s/b wnC?) 7-2011 --> Preferred Approach: To use accurately with standard winter rated U-factor from ASHRAE or NFRC enter as:

        gtU = (1/((1/U-factor)-0.85)

Where 0.85 is the sum of the interior (0.68) and exterior (0.17) design air film resistances assumed for rating window U-factors. Enter wnInH (usually 1.5=1/0.68) instead of letting it default. Enter the wnExH or let it default. It is important to use this approach if the input includes gnFrad for any gain term. Using approach 2 below will result in an inappropriate internal gain split at the window.

Approach 2. Enter gtU=U-factor and let the wnInH and wnExH default. This approach systematically underestimates the window U-factor because it adds the wnExfilm resistance to 1/U-factor thereby double counting the exterior film resistance. This approach will also yield incorrect results for gnFrad internal gain since the high wnInH will put almost all the gain back in the space.

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "*x* > 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### gtUNFRC

Type: float

Fenestration system (including frame) U-factor evaluated at NFRC heating conditions. For ASHWAT windows, a value for the NFRC U-factor is required, set via gtUNFRC or wnUNFRC.

{{
  member_table({
    "units": "Btuh/ft^2^-°F",
    "legal_range": "*x* > 0", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### gtSHGC

Type: float

Glazing Solar Heat Gain Coefficient: fraction of normal beam insolation which gets through glass to space inside. We recommend using this to represent the glass normal transmissivity characteristic only, before shading and framing effects

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### gtSMSO

Type: float

SHGC multiplier with shades open. May be overriden in the specific window input.

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "1.0",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

### gtSMSC

Type: float

SHGC multiplier with shades closed. May be overriden in the specific window input.

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "gtSMSO (no shades)",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

### gtFMult

Type: float

Framing multiplier used if none given in window, for example .9 if frame and mullions reduce the solar gain by 10%. Default of 1.0 implies frame/mullion effects allowed for in gtSHGC's or always specified in Windows.

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "gtSHGCO",
    "required": "No",
    "variability": "Monthly - Hourly" 
  })
}}

### gtPySHGC 

Type: *float*

Four float values separated by commas. Coefficients for incidence angle SHGC multiplier polynomial applied to gtSHGC to determine beam transmissivity at angles of incidence other than 90 degrees. The values are coefficients for first through fourth powers of the cosine of the incidence angle; there is no constant part. An error message will be issued if the coefficients do not add to one. They are used in the following computation:

angle = incidence angle of beam radiation, measured from normal to glass.

cosI = cos(angle)

angMult = a\*cosI + b\*cosI^2^ + c\*cosI^3^ + d\*cosI^4^

beamXmisvty = gtSHGCO * angMult (shades open)

{{
  member_table({
    "units": "float",
    "legal_range": "*any*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### gtDMSHGC

Type: float

SHGC diffuse multiplier, applied to gtSHGC to determine transmissivity for diffuse radiation.

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### gtDMRBSol

Type: float

SHGC diffuse multiplier, applied to qtSHGC to determine transmissivity for diffuse radiation reflected back out the window. Misnamed as a reflectance. Assume equal to DMSHGC if no other data available.

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### gtNGlz

Type: int

Number of glazings in the Glazetype (bare glass only, not including any interior or exterior shades).

{{
  member_table({
    "units": "",
    "legal_range": "*0* < *x* ≤ *4*", 
    "default": "2",
    "required": "No",
    "variability": "constant" 
  })
}}

### gtExShd

Type: choice

Exterior shading type (ASHWAT only).

{{
  member_table({
    "units": "",
    "legal_range": "NONE INSCRN", 
    "default": "NONE",
    "required": "No",
    "variability": "constant" 
  })
}}

### gtInShd

Type: choice

Interior shade type (ASHWAT only).

{{
  member_table({
    "units": "",
    "legal_range": "NONE DRAPEMED", 
    "default": "NONE",
    "required": "No",
    "variability": "constant" 
  })
}}

### gtDirtLoss

Type: float

Glazing dirt loss factor.

{{
  member_table({
    "units": "fraction",
    "legal_range": "*0* ≤ *x* ≤ *1*", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

### endGlazeType

Optional to indicates the end of the Glazetype. Alternatively, the end of the GLAZETYPE definition can be indicated by "END" or by beginning another object

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

- @[glazeType][p_glazetype]
