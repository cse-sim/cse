# DHWLOOPPUMP

DHWLOOPPUMP constructs an object representing a pump serving part a DHWLOOP. The model is identical to DHWPUMP *except* that that the electricity use calculation reflects wlRunF of the parent DHWLOOP.

**wlpName**

Optional name of pump; give after the word “DHWLOOPPUMP” if desired.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**wlpMult=*integer***

Number of identical pumps of this type. Any value $>1$ is equivalent to repeated entry of the same DHWPUMP.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0 ",
  default: "1",
  required: "No",
  variability: "constant")
  %>

**wlpPwr=*float***

Pump power.

<%= member_table(
  units: "W",
  legal_range: "x $>$ 0",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**wlpLiqHeatF=*float***

Fraction of pump power that heats circulating liquid.  The remainder is discarded.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 1",
  default: "1",
  required: "No",
  variability: "hourly")
  %>

**wlpElecMtr=*mtrName***

Name of METER object, if any, to which DHWLOOPPUMP electrical energy use is recorded (under end use dhwMFL).

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*Parent DHWLOOP wlElecMtr*",
  required: "No",
  variability: "constant")
  %>

**endDHWLOOPPUMP**

Optionally indicates the end of the DHWPUMP definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**Related Probes:**

- @[DHWLoopPump](#p_dhwlooppump)
