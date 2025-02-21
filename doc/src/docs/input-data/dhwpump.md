# DHWPUMP

DHWPUMP constructs an object representing a domestic hot water circulation pump (or more than one if identical).

**wpName**

Optional name of pump; give after the word “DHWPUMP” if desired.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant")
%>

**wpMult=_integer_**

Number of identical pumps of this type. Any value $>1$ is equivalent to repeated entry of the same DHWPUMP.

<%= member_table(
units: "",
legal_range: "x $>$ 0",
default: "1",
required: "No",
variability: "constant")
%>

**wpPwr=_float_**

Pump power.

<%= member_table(
units: "W",
legal_range: "x $>$ 0",
default: "0",
required: "No",
variability: "hourly")
%>

**wpElecMtr=_mtrName_**

Name of METER object, if any, to which DHWPUMP electrical energy use is recorded (under end use DHW).

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_Parent DHWSYS wsElecMtr_",
required: "No",
variability: "constant")
%>

**endDHWPump**

Optionally indicates the end of the DHWPUMP definition.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "")
%>

**Related Probes:**

- @[DHWPump][p_dhwpump]
