# IZXFER

IZXFER constructs an object that represents an interzone or zone/ambient heat transfer due to conduction and/or air transfer. The air transfer modeled by IZXFER transfers heat only; humidity transfer is not modeled as of July 2011. Note that SURFACE is the preferred way represent conduction between ZONEs.

The AIRNET types are used in a multi-cell pressure balancing model that finds zone pressures that produce net 0 mass flow into each zone. The model operates in concert with the znType=CZM or znType=UZM to represent ventilation strategies. During each time step, the pressure balance is found for two modes that can be thought of as “VentOff” (or infiltration-only) and “VentOn” (or infiltration+ventilation). The zone model then determines the ventilation fraction required to hold the desired zone temperature (if possible). AIRNET modeling methods are documented in the CSE Engineering Documentation.

Note that fan-driven types assume pressure-independent flow.  That is, the specified flow is included in the zone pressure balance but the modeled fan flow does not change with zone pressure.
The assumption is that in realistic configurations, zone pressure will generally be close to ambient pressure.  Unbalanced fan ventilation in a zone without relief area will result in runtime termination due to excessively high or low pressure.

**izName**

Optional name of interzone transfer; give after the word "IZXFER" if desired.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**izNVType=*choice***

Choice specifying the type of ventilation or leakage model to be used.

<%= csv_table(<<END, :row_header => false)
  NONE,             No interzone ventilation
  ONEWAY,           Uncontrolled flow from izZn1 to izZn2 when izZn1 air temperature exceeds izZn2 air temperature (using ASHRAE high/low vent model).
  TWOWAY,           Uncontrolled flow in either direction (using ASHRAE high/low vent model).
  AIRNETIZ,         Single opening to another zone (using pressure balance AirNet model).  Flow is driven by buoyancy.
  AIRNETEXT,        Single opening to ambient (using pressure balance AirNet model).  Flow is driven by buoyancy and wind pressure.
  AIRNETHORIZ,      Horizontal (large) opening between two zones&comma; used to represent e.g. stairwells.  Flow is driven by buoyancy; simultaneous up and down flow is modeled.
  AIRNETEXTFAN,     Fan from exterior to zone (flow either direction).
  AIRNETIZFAN,      Fan between two zones (flow either direction).
  AIRNETEXTFLOW,    Specified flow from exterior to zone (either direction). Behaves identically to AIRNETEXTFAN except no electricity is consumed and no fan heat is added to the air stream.
  AIRNETIZFLOW,     Specified flow between two zones (either direction). Behaves identically to AIRNETIZFAN except no electricity is consumed and no fan heat is added to the air stream.
  AIRNETHERV,       Heat or energy recovery ventilator. Supply and exhaust air are exchanged with the exterior with heat and/or moisture exchange between the air streams. Flow may or may not be balanced.
  AIRNETDOAS,      Air supplied from and/or exhausted to a centralized DOAS fans.
END
%>

Note that optional inputs izTEx, izWEx, and izWindSpeed can override the outside conditions assumed for ivNVTypes that are connected to ambient (AIRNETEXT, AIRNETEXTFAN, AIRNETEXTFLOW, and AIRNETHERV).

{{
  member_table({
    "units": "",
    "legal_range": "*choices above*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**izAFCat=*choice***

Choice indicating air flow category used *only* for recording air flow results to an AFMETER.  izAFCat has no effect for non-AIRNET IZXFERs.  izAFCat is not used unless the associated ZONE(s) specify znAFMtr.

Choices are:

<%= csv_table(<<END, :row_header => false)
  InfilEx,      Infiltration from ambient
  VentEx,       Natural ventilation from ambient
	FanEx,        Forced ventilation from ambient
	InfilIz,      Interzone infiltration
	VentIz,       Interzone natural ventilation
	FanIz,        Interzone forced ventilation
	DuctLk,       Duct leakage
	HVAC,         HVAC air
END
%>

Default values for izAFCat are generally adequate *except* that natural ventilation IZXFERs are by default categorized as infiltration.  It is thus recommended that izAfCat be omitted except that ventilation IZXFERs (e.g. representing openable windows) should include izAfCat=VentEx (or VentIz).

{{
  member_table({
    "units": "",
    "legal_range": "*choices above*", 
    "default": "derived from IZXFER characteristics",
    "required": "No",
    "variability": "constant" 
  })
}}

**izZn1=*znName***

Name of primary zone. Flow rates $>$ 0 are into the primary zone.

{{
  member_table({
    "units": "",
    "legal_range": "name of a ZONE", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**izZn2=*znName***

Name of secondary zone.

{{
  member_table({
    "units": "",
    "legal_range": "name of a ZONE", 
    "default": "*none*",
    "required": "required unless constant izNVType = AIRNETEXT, AIRNETEXTFAN, AIRNETEXTFLOW, or AIRNETHERV",
    "variability": "constant" 
  })
}}

**izDOAS=*oaName***

Name of DOAS where air is supplied from (**izVfMin** > 0), or exhausting to (**izVfMin** < 0).

{{
  member_table({
    "units": None,
    "legal_range": "name of a DOAS", 
    "default": None,
    "required": "when izNVType = AIRNETDOAS",
    "variability": "constant" 
  })
}}

**izLinkedFlowMult=*float***

Specifies a multiplier applied to air flow to/from any associated DOAS.  This supports use of a single modeled zone to represent multiple actual zones while preserving the total DOAS air flow and energy consumption.

For example, consider a DOAS-linked IZXFER with izVfMin = 100 and izLinkedFlowMult = 5.  The zone specified by izZn1 receives 100 cfm while the DOAS specified by izDOAS is modeled as if the supply flow is 500 cfm.  Thus the DOAS behavior (fan energy use etc.) approximates that of a DOAS serving 5 zones, but only one zone is simulated.

Note izLinkedFlowMult has no effect on the air flow to or from the zone specified by izZn1.

{{
  member_table({
    "units": "--",
    "legal_range": "*x* $\\gt$ 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}


Give izHConst for a conductive transfer between zones. Give izNVType other than NONE and the following variables for a convective (air) transfer between the zones or between a zone and outdoors. Both may be given if desired. Not known to work properly as of July 2011

**izHConst=*float***

Conductance between zones.

{{
  member_table({
    "units": "Btu/^o^F",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**izALo=*float***

Area of low or only vent (typically VentOff)

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "hourly" 
  })
}}

**izAHi=*float***

Additional vent area (high vent or VentOn). If used in AIRNET, izAHi &gt; izALo typically but this is not required.

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "izALo",
    "required": "No",
    "variability": "hourly" 
  })
}}

**izTEx=*float***

Alternative exterior air dry bulb temperature for this vent.  Allowed only with izNVTypes that use outdoor air (AIRNETEXT, AIRNETEXTFAN, AIRNETEXTFLOW, and AIRNETHERV).  If given, izTEx overrides the outdoor dry-bulb temperature read from the weather file or derived from design conditions.

Caution: izTEx is not checked for reasonableness.

One use of izTEx is in representation of leaks in surfaces adjacent to zones not being simulated.  "Pseudo-interior" surface leakage can be created as follows (where "Z1" is the name of the leak's zone and izALo and izHD are set to appropriate values) --

     IZXFER RLF izNVTYPE=AirNetExt izZN1="Z1" izALo=.1 izHD=10  izTEx=@zone["Z1"].tzls izWEx=@zone["Z1"].wzls

This will allow Z1's pressure to be realistic without inducing thermal loads that would occur if the leak source had outdoor conditions.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "", 
    "default": "Outdoor dry-bulb",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izWEx=*float***

Alternative exterior air humidity ratio seen by this vent. Allowed only with izNVTypes that use outdoor air (AIRNETEXT, AIRNETEXTFAN, AIRNETEXTFLOW, and AIRNETHERV).  If given, izWEx overrides the outdoor humidity ratio derived from weather file data or design conditions.

Caution: izWEx is not checked against saturation -- there is no verification that the value provided is physically possible.

See izTEx example just above.

{{
  member_table({
    "units": "",
    "legal_range": "$\\gt$ 0", 
    "default": "Outdoor humidity ratio",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izWindSpeed=*float***

Alternative windspeed seen by this vent.  Allowed only with izNVTypes that use outdoor air (AIRNETEXT, AIRNETEXTFAN, AIRNETEXTFLOW, and AIRNETHERV).  If given, izWindSpeed overrides the windspeed read from the weather file or derived from design conditions.

No adjustments such as TOP windF or ZONE znWindFLkg are applied to izWindSpeed when it is used in derivation of wind-driven air flow.

Note that izCpr must be non-0 for izWindSpeed to have any effect.

{{
  member_table({
    "units": "mph",
    "legal_range": "$\\ge$ 0", 
    "default": "Zone adjusted windspeed",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izL1=*float***

Length or width of AIRNETHORIZ opening.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $>$ 0", 
    "default": "*none*",
    "required": "if izNVType = AIRNETHORIZ",
    "variability": "constant" 
  })
}}

**izL2=*float***

Width or length of AIRNETHORIZ opening.

{{
  member_table({
    "units": "ft",
    "legal_range": "*x* $>$ 0", 
    "default": "*none*",
    "required": "if izNVType = AIRNETHORIZ",
    "variability": "constant" 
  })
}}

**izStairAngle=*float***

Stairway angle for AIRNETHORIZ opening. Use 90 for an open hole. Note that 0 prevents flow.

{{
  member_table({
    "units": "^o^ degrees",
    "legal_range": "*x* $>$ 0", 
    "default": "34",
    "required": "No",
    "variability": "constant" 
  })
}}

**izHD=*float***

Vent center-to-center height difference (for TWOWAY) or vent height above nominal 0 level (for AirNet types)

{{
  member_table({
    "units": "ft",
    "legal_range": "", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**izNVEff=*float***

Vent discharge coefficient.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "0.8",
    "required": "No",
    "variability": "constant" 
  })
}}

**izfanVfDs=*float***

Fan design or rated flow at rated pressure.  For AIRNETHERV, this is the net air flow into the zone, gross flow at the fan is derived using izEATR (see below).

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0 (no fan)",
    "required": "If fan present",
    "variability": "constant" 
  })
}}

**izCpr=*float***

Wind pressure coefficient (for AIRNETEXT).

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**izExp=*float***

Opening exponent (for AIRNETEXT).

{{
  member_table({
    "units": "",
    "legal_range": "*x* $>$ 0", 
    "default": "0.5",
    "required": "No",
    "variability": "constant" 
  })
}}

**izVfMin=*float***

Minimum volume flow rate (VentOff mode).

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "izfanVfDs",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izVfMax=*float***

Maximum volume flow rate (VentOn mode)

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "izVfMin",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izASEF=*float***

Apparent sensible effectiveness for AIRNETHERV ventilator.  ASEF is a commonly-reported HERV rating and is calculated as (supplyT - sourceT) / (returnT - sourceT).  This formulation includes fan heat (in supplyT), hence the term "apparent".  Ignored if izSRE is given.  CSE does not HRV exhaust-side condensation, so this model is approximate.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ x $\\le$ 1", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izSRE=*float***

Sensible recovery efficiency (SRE) for AIRNETHERV ventilator.  Used as the sensible effectiveness in calculation of the supply air temperature.  Note that values of SRE greater than approximately 0.6 imply exhaust-side condensation under HVI rating conditions.  CSE does not adjust for these effects.  High values of izSRE will produce unrealistic results under mild outdoor conditions and/or dry indoor conditions.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ x $\\le$ 1", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izASRE=*float***

Adjusted sensible recovery efficiency (ASRE) for AIRNETHERV ventilator.  The difference izASRE - izSRE is used to calculate fan heat added to the supply air stream.  See izSRE notes.  No effect when izSRE is 0.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ x $\\le$ izSRE", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izEATR=*float***

Exhaust air transfer ratio for AIRNETHERV ventilator.  NetFlow = (1 - EATR)*(grossFlow).

{{
  member_table({
    "units": "cfm",
    "legal_range": "0 $\\le$ x $\\le$ 1", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izLEF=*float***

Latent heat recovery effectiveness for AIRNETHERV ventilator.  The default value (0) results in sensible-only heat recovery.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ x $\\le$ 1", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izRVFanHeatF=*float***

Fraction of fan heat added to supply air stream for AIRNETHERV ventilator.  Used only when when izSRE is 0 (that is, when izASEF specifies the sensible effectiveness).

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ x $\\le$ 1", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**izVfExhRat=*float***

Exhaust volume flow ratio for AIRNETHERV ventilator = (exhaust flow) / (supply flow).  Any
value other than 1 indicates unbalanced flow that effects the zone pressure.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "1 (balanced)",
    "required": "No",
    "variability": "constant" 
  })
}}

**izfanPress=*float***

Design or rated fan pressure.

{{
  member_table({
    "units": "inches H~2~O",
    "legal_range": "*x* $>$ 0", 
    "default": "0.3",
    "required": "No",
    "variability": "constant" 
  })
}}

Only one of izfanElecPwr, izfanEff, and izfanShaftBhp may be given: together with izfanVfDs and izfanPress, any one is sufficient for CSE to determine the others and to compute the fan heat contribution to the air stream.

**izfanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "*x* $>$ 0", 
    "default": "derived from izfanEff and izfanShaftBhp",
    "required": "If izfanEff and izfanShaftBhp not present",
    "variability": "constant" 
  })
}}

**izfanEff=*float***

Fan efficiency at design flow and pressure, as a fraction.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "derived from *izfanShaftBhp* if given, else 0.08",
    "required": "No",
    "variability": "constant" 
  })
}}

**izfanShaftBhp=*float***

Fan shaft brake horsepower at design flow and pressure.

{{
  member_table({
    "units": "bhp",
    "legal_range": "*x* $>$ 0", 
    "default": "derived from *izfanEff*.",
    "required": "No",
    "variability": "constant" 
  })
}}

**izfanCurvePy=$k_0$, $k_1$, $k_2$, $k_3$, $x_0$**

$k_0$ through $k_3$ are the coefficients of a cubic polynomial for the curve relating fan relative energy consumption to relative air flow above the minimum flow $x_0$. Up to five *floats* may be given, separated by commas. 0 is used for any omitted trailing values. The values are used as follows:

$$z = k_0 + k_1 \cdot (x - x_0)|  +  k_2 \cdot (x - x_0)|^2 + k_3 \cdot (x - x_0)|^3$$

where:

-   $x$ is the relative fan air flow (as fraction of izfan*VfDs*; 0 $\le$ $x$ $\le$ 1);
-   $x_0$ is the minimum relative air flow (default 0);
-   $(x - x_0)|$ is the "positive difference", i.e. $(x - x_0)$ if $x > x_0$; else 0;
-   $z$ is the relative energy consumption.

If $z$ is not 1.0 for $x$ = 1.0, a warning message is displayed and the coefficients are normalized by dividing by the polynomial's value for $x$ = 1.0.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "*0, 1, 0, 0, 0 (linear)*",
    "required": "No",
    "variability": "constant" 
  })
}}

**izFanMtr=*mtrName***

Name of meter, if any, to record energy used by supply fan. End use category used is specified by izFanEndUse (next).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**izFanEndUse=*choice***

End use to which fan energy is recorded (in METER specified by izFanMtr).  See METER for available end use choices.

{{
  member_table({
    "units": "",
    "legal_range": "*end use choice*", 
    "default": "Fan",
    "required": "No",
    "variability": "constant" 
  })
}}

**endIZXFER**

Optionally indicates the end of the interzone transfer definition.

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

- @[izXfer][p_izxfer]
