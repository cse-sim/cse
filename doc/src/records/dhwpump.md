# DHWPUMP

DHWPUMP constructs an object representing a domestic hot water circulation pump (or more than one if identical).

**wpName**

Optional name of pump; give after the word “DHWPUMP” if desired.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**wpMult=*integer***

Number of identical pumps of this type. Any value $>1$ is equivalent to repeated entry of the same DHWPUMP.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1",
  required: "No",
  variability: "constant")
  %>

**wpPwr=*float***

Pump power.

<%= member_table(
  units: "W",
  legal_range: "x $>$ 0",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**wpElecMtr=*mtrName***

Name of METER object, if any, to which DHWPUMP electrical energy use is recorded (under end use DHW).

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*Parent DHWSYS wsElecMtr*",
  required: "No",
  variability: "constant")
  %>

**endDHWPump**

Optionally indicates the end of the DHWPUMP definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**Related Probes:**

- @[DHWPump](#p_dhwpump)
