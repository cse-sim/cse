# DESCOND

Specifies conditions for a cooling design day.  When referenced in TOP coolDsCond (see [TOP Autosizing][top-autosizing]), DESCOND members are used to generate a 24 hour design day used during cooling autosizing. Note that coolDsCond can reference more than one DESCOND, allowing multiple design conditions to be used for autosizing.  For example, both summer and fall days could be specified to ensure a range of sun angles are considered.  Any DESCONDs that are not referenced in coolDsCond have no effect.

**desCondName**

Object name, given after “DESCOND”.  Required for referencing from Top coolDsCond.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**dcDay=*date***

Calendar date for this design cooling condition.

<%= member_table(
  units: "",
  legal_range: "1-365",
  default: "200",
  required: "No",
  variability: "constant") %>
  
**dcDB=*float***

Design dry-bulb temperature (maxiumum temperature on design day).

<%= member_table(
  units: "^o^F",
  legal_range: "no-limitations?",
  default: "0.0",
  required: "No",
  variability: "constant") %>


**dcMCDBR=*float***

Coincident daily dry-bulb range.

<%= member_table(
  units: "^o^F",
  legal_range: "no-limitations?",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcMCWB=*float***

Coincident wet-bulb design temperature.

<%= member_table(
  units: "^o^F",
  legal_range: "no-limitations?",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcMCWBR=*float***

Coincident daily wet-bulb range.

<%= member_table(
  units: "^o^F",
  legal_range: "no-limitations?",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcWindSpeed=*float***

Wind speed for design conditions.

<%= member_table(
  units: "mph",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

DESCOND provides two mutually-exclusive methods for specifying design day direct beam and diffuse horizontal irradiance values.  Both use the ASHRAE clear sky model.  Consult the ASHRAE *Handbook of Fundamentals* Climatic Data chapter for model documentation.

- Pseudo optical depth.  dcTauB and dcTauD define the taub and taud parameters in the clear sky model.
- Solar noon irradiance.  CSE uses dcEbnSlrNoon and dcEdhSlrNoon to back-calculate taub and taud.

At most one of these methods can be used within a given DESCOND.  If all solar-related values are omitted, the generated design day has 0 irradiance for all hours.


**dcTauB=*float*** \
**dcTauD=*float***

ASHRAE clear sky model beam and diffuse pseudo optical depths.  These values are available by month for many locations in ASHRAE design weather data.  Cannot be given if dcEbnSlrNoon and dcEdhSlrNoon are specified.

<%= member_table(
  units: "",
  legal_range: "",
  default: "0 irradiance",
  required: "No",
  variability: "constant") %>


**dcEbnSlrNoon=*float*** \
**dcEdhSlrNoon=*float***

Solar noon direct beam and diffuse horizontal irradiance. Cannot be given if dcTauB and dcTauD are specified.

<%= member_table(
  units: "Btuh/ft^2^",
  legal_range: "x $\\geq$ 0",
  default: "0 irradiance",
  required: "No",
  variability: "constant") %>

**endDesCond**

Optionally indicates the end of the descond definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "**none**",
  required: "No",
  variability: "") %>