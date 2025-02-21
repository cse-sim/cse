# DHWLOOP

DHWLOOP constructs one or more objects representing a domestic hot water circulation loop. The actual pipe runs in the DHWLOOP are specified by any number of DHWLOOPSEGs (see below). Circulation pumps are specified by DHWLOOPPUMPs (also below).

**wlName**

Optional name of loop; give after the word “DHWLOOP” if desired.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant") %>

**wlMult=_integer_**

Number of identical loops of this type. Any value $>1$ is equivalent to repeated entry of the same DHWLOOP (and all child objects).

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "1",
required: "No",
variability: "constant") %>

**wlFlow=_float_**

Loop flow rate (when operating).

<%= member_table(
units: "gpm",
legal_range: "_x_ $\\ge$ 0",
default: "6",
required: "No",
variability: "hourly") %>

**wlTIn1=_float_**

Inlet temperature of first DHWLOOPSEG.

<%= member_table(
units: "^o^F",
legal_range: "_x_ $>$ 0",
default: "DHWSYS wsTUse",
required: "No",
variability: "hourly") %>

**wlRunF=_float_**

Fraction of hour that loop circulation operates.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "1",
required: "No",
variability: "hourly") %>

**wlFUA=_float_**

DHWLOOPSEG pipe heat loss adjustment factor. DHWLOOPSEG UA is derived (from wgSize, wgLength, wgInsulK, wgInsulThk, and wgExH) and multiplied by wlFUA. Note: does not apply to child DHWLOOPBRANCHs (see wbFUA).

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "1",
required: "No",
variability: "constant") %>

**wlLossMakeupPwr=_float_**

Specifies electrical power available to make up losses from DHWLOOPSEGs (loss from DHWLOOPBRANCHs is not included). Separate loss makeup is typically used in multi-unit HPWH systems to avoid inefficiencies associated with high condenser temperatures. Loss-makeup energy is calculated hourly and is the smaller of loop losses and wlLossMakeupPwr. The resulting electricity use (including the effect of wlLossMakeupEff) is accumulated to the METER specified by wlElecMtr (end use dhwMFL). No other effect, such as heat gain to surroundings, is modeled.

<%= member_table(
units: "W",
legal_range: "_x_ $\\ge$ 0",
default: "0",
required: "No",
variability: "hourly") %>

**wlLossMakeupEff=_float_**

Specifies the efficiency of loss makeup heating if any. No effect when wlLossMakeupPwr is 0.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "1",
required: "No",
variability: "hourly") %>

**wlElecMtr=_mtrName_**

Name of METER object, if any, to which DHWLOOP electrical energy use is recorded (under end use dhwMFL).

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_Parent DHWSYS wsElecMtr_",
required: "No",
variability: "constant") %>

**endDHWLoop**

Optionally indicates the end of the DHWLOOP definition.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "") %>

**Related Probes:**

- @[DHWLoop][p_dhwloop]
