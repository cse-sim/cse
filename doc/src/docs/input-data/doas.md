# DOAS

DOAS (Dedicated Outdoor Air System) provides centralized supply and/or exhuast ventilation air to IZXFER objects with the **izNVType** = AIRNETDOAS. The supply air may be preconditioned using heat recovery and/or tempering coils.


### oaName

Name of DOAS.

## DOAS Supply Fan Data Members

**oaSupFanVfDs=*float***

Supply fan design or rated flow at rated pressure.

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "Sum of referencing IZXFER supply flows",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaSupFanPress=*float***

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

Only one of oaSupFanElecPwr, oaSupFanEff, and oaSupFanShaftBhp may be given.

**oaSupFanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "*x* $>$ 0", 
    "default": "derived from oaSupFanEff and oaSupFanShaftBhp",
    "required": "If oaSupFanEff and oaSupFanShaftBhp not present",
    "variability": "constant" 
  })
}}

**oaSupFanEff=*float***

Fan efficiency at design flow and pressure, as a fraction.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "derived from *oaSupFanShaftBhp* if given, else 0.08",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaSupFanShaftBhp=*float***

Fan shaft brake horsepower at design flow and pressure.

{{
  member_table({
    "units": "bhp",
    "legal_range": "*x* $>$ 0", 
    "default": "derived from *oaSupFanEff*.",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaSupFanCurvePy=$k_0$, $k_1$, $k_2$, $k_3$, $x_0$**

$k_0$ through $k_3$ are the coefficients of a cubic polynomial for the curve relating fan relative energy consumption to relative air flow above the minimum flow $x_0$. Up to five *floats* may be given, separated by commas. 0 is used for any omitted trailing values. The values are used as follows:

$$z = k_0 + k_1 \cdot (x - x_0)|  +  k_2 \cdot (x - x_0)|^2 + k_3 \cdot (x - x_0)|^3$$

where:

-   $x$ is the relative fan air flow (as fraction of oaSupFanVfDs; 0 $\le$ $x$ $\le$ 1);
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

**oaSupFanMtr=*mtrName***

Name of meter, if any, to record energy used by supply fan. End use category used is specified by oaSupFanEndUse (next).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaSupFanEndUse=*choice***

End use to which fan energy is recorded (in METER specified by oaSupFanMtr).  See METER for available end use choices.

{{
  member_table({
    "units": "",
    "legal_range": "*end use choice*", 
    "default": "Fan",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaTEx=*float***

Alternative supply fan source air dry bulb temperature. If given, oaTEx overrides the outdoor dry-bulb temperature read from the weather file or derived from design conditions.

Caution: oaTEx is not checked for reasonableness.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "", 
    "default": "Outdoor dry-bulb",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**oaWEx=*float***

Alternative supply fan source air air humidity ratio. If given, oaWEx overrides the outdoor humidity ratio derived from weather file data or design conditions.

Caution: oaWEx is not checked against saturation -- there is no verification that the value provided is physically possible.

{{
  member_table({
    "units": "",
    "legal_range": "$\\gt$ 0", 
    "default": "Outdoor humidity ratio",
    "required": "No",
    "variability": "subhourly" 
  })
}}

## DOAS Exhaust Fan Data Members

**oaExhFanVfDs=*float***

Exhaust fan design or rated flow at rated pressure.

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "Sum of referencing IZXFER exhaust flows",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaExhFanPress=*float***

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

Only one of oaExhFanElecPwr, oaExhFanEff, and oaExhFanShaftBhp may be given.

**oaExhFanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

{{
  member_table({
    "units": "W/cfm",
    "legal_range": "*x* $>$ 0", 
    "default": "derived from oaExhFanEff and oaExhFanShaftBhp",
    "required": "If oaExhFanEff and oaExhFanShaftBhp not present",
    "variability": "constant" 
  })
}}

**oaExhFanEff=*float***

Fan efficiency at design flow and pressure, as a fraction.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1", 
    "default": "derived from *oaExhFanShaftBhp* if given, else 0.08",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaExhFanShaftBhp=*float***

Fan shaft brake horsepower at design flow and pressure.

{{
  member_table({
    "units": "bhp",
    "legal_range": "*x* $>$ 0", 
    "default": "derived from *oaExhFanEff*.",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaExhFanCurvePy=$k_0$, $k_1$, $k_2$, $k_3$, $x_0$**

$k_0$ through $k_3$ are the coefficients of a cubic polynomial for the curve relating fan relative energy consumption to relative air flow above the minimum flow $x_0$. Up to five *floats* may be given, separated by commas. 0 is used for any omitted trailing values. The values are used as follows:

$$z = k_0 + k_1 \cdot (x - x_0)|  +  k_2 \cdot (x - x_0)|^2 + k_3 \cdot (x - x_0)|^3$$

where:

-   $x$ is the relative fan air flow (as fraction of oaExhFanVfDs; 0 $\le$ $x$ $\le$ 1);
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

**oaExhFanMtr=*mtrName***

Name of meter, if any, to record energy used by exhaust fan. End use category used is specified by oaExhFanEndUse (next).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaExhFanEndUse=*choice***

End use to which fan energy is recorded (in METER specified by oaExhFanMtr).  See METER for available end use choices.

{{
  member_table({
    "units": "",
    "legal_range": "*end use choice*", 
    "default": "Fan",
    "required": "No",
    "variability": "constant" 
  })
}}

## DOAS Tempering Coils Data Members

**oaSupTH=*float***

Heating setpoint for tempering and/or heat exchanger bypass.

{{
  member_table({
    "units": "^o^F",
    "default": "68", 
    "required": "No",
    "variability": "subhourly" 
  })
}}

**oaEIRH=*float***

Energy Input Ratio of the heating coil. This is the inverse of the coil efficiency or COP. A value of zero indicates that the coil does not use energy (e.g., hot water coils). Specifying input triggers the modeling of a heating coil.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "when modeling heating coil",
    "variability": "subhourly" 
  })
}}

**oaCoilHMtr=*mtrName***

Name of meter, if any, to record energy used by the heating coil.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaSupTC=*float***

Cooling setpoint for tempering and/or heat exchanger bypass.

{{
  member_table({
    "units": "^o^F",
    "default": "68", 
    "required": "No",
    "variability": "subhourly" 
  })
}}

**oaEIRC=*float***

Energy Input Ratio of the cooling coil. This is the inverse of the coil efficiency or COP. A value of zero indicates that the coil does not use energy (e.g., chilled water coils). Specifying input triggers the modeling of a cooling coil.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "when modeling cooling coil",
    "variability": "subhourly" 
  })
}}

**oaSHRtarget=*float***

Sensible Heat Ratio of the cooling coil. If the required sensible capacity of the coil and the entered SHR do not produce a valid psychrometric state, the SHR is adjusted and reported through the SHR probe.

{{
  member_table({
    "units": "",
    "legal_range": "*x* $\\gt$ 0", 
    "default": "1.0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**oaCoilCMtr=*mtrName***

Name of meter, if any, to record energy used by the cooling coil.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaLoadMtr=*ldMtrName***

Name of load meter, if any, to record load met by the heating coil or cooling coil.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a LOADMETER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}


## DOAS Heat Recovery Data Members

**oaHXVfDs=*float***

Heat exchanger design or rated flow.

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* $\\gt$ 0", 
    "default": "Average of supply and exhaust fan design flows",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXf2=*float***

Heat exchanger flow fraction (of design flow) used for second set of effectivenesses.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\lt$ *x* $\\lt$ 1.0", 
    "default": "0.75",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXSenEffHDs=*float***

Heat exchanger sensible effectiveness in heating mode at the design flow rate. Specifying input triggers modeling of heat recovery.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "required": "when modeling heat recovery",
    "variability": "constant" 
  })
}}

**oaHXSenEffHf2=*float***

Heat exchanger sensible effectiveness in heating mode at the second flow rate (**oaHXf2**).

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXLatEffHDs=*float***

Heat exchanger latent effectiveness in heating mode at the design flow rate.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXLatEffHf2=*float***

Heat exchanger latent effectiveness in heating mode at the second flow rate (**oaHXf2**).

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXSenEffCDs=*float***

Heat exchanger sensible effectiveness in cooling mode at the design flow rate.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXSenEffCf2=*float***

Heat exchanger sensible effectiveness in cooling mode at the second flow rate (**oaHXf2**).

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXLatEffCDs=*float***

Heat exchanger latent effectiveness in cooling mode at the design flow rate.

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXLatEffCf2=*float***

Heat exchanger latent effectiveness in cooling mode at the second flow rate (**oaHXf2**).

{{
  member_table({
    "units": "",
    "legal_range": "0 $\\le$ *x* $\\le$ 1.0", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXBypass=*choice***

Yes/No choice for enabling heat exchanger bypass. If selected, the outdoor air will bypass the heat exchanger when otherwise the heat exchanger would require more heating or cooling energy to meet the respective setpoints.

{{
  member_table({
    "units": "",
    "legal_range": "NO, YES", 
    "default": "NO",
    "required": "No",
    "variability": "constant" 
  })
}}

**oaHXAuxPwr=*float***

Auxiliary power required to operate the heat recovery device (e.g., wheel motor, contorls).

{{
  member_table({
    "units": "W",
    "legal_range": "*x* $\\ge$ 0", 
    "default": "0",
    "required": "No",
    "variability": "subhourly" 
  })
}}

**oaHXAuxMtr=*mtrName***

Name of meter, if any, to record energy used by auxiliary components of the heat recovery system.

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*not recorded*",
    "required": "No",
    "variability": "constant" 
  })
}}

### endDOAS

Indicates the end of the DOAS definition. Alternatively, the end of the DOAS definition can be indicated by the declaration of another object or by "END".

{{
  member_table({
    "default": "*N/A*", 
    "required": "No",
    "variability": "constant" 
  })
}}

**Related Probes:**

- @[doas][p_doas]