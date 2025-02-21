# DHWLOOPPUMP

DHWLOOPPUMP constructs an object representing a pump serving part a DHWLOOP. The model is identical to DHWPUMP _except_ that that the electricity use calculation reflects wlRunF of the parent DHWLOOP.

**wlpName**

Optional name of pump; give after the word “DHWLOOPPUMP” if desired.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant")
%>

**wlpMult=_integer_**

Number of identical pumps of this type. Any value $>1$ is equivalent to repeated entry of the same DHWPUMP.

<%= member_table(
units: "",
legal_range: "x $>$ 0 ",
default: "1",
required: "No",
variability: "constant")
%>

**wlpPwr=_float_**

Pump power.

<%= member_table(
units: "W",
legal_range: "x $>$ 0",
default: "0",
required: "No",
variability: "hourly")
%>

**wlpLiqHeatF=_float_**

Fraction of pump power that heats circulating liquid. The remainder is discarded.

<%= member_table(
units: "",
legal_range: "0 $\\le$ x $\\le$ 1",
default: "1",
required: "No",
variability: "hourly")
%>

**wlpElecMtr=_mtrName_**

Name of METER object, if any, to which DHWLOOPPUMP electrical energy use is recorded (under end use dhwMFL).

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_Parent DHWLOOP wlElecMtr_",
required: "No",
variability: "constant")
%>

**endDHWLOOPPUMP**

Optionally indicates the end of the DHWPUMP definition.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant")
%>

**Related Probes:**

- @[DHWLoopPump][p_dhwlooppump]
