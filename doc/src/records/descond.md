# DESCOND

Specifies conditions for a cooling design day.  When referenced in Top coolDsCond, these items are used to generate a 24 hour design day used during autosizing.  Any DESCONDs that are not referenced in coolDsCond have no effect.

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

**dcTauB=*float***

ASHRAE beam "pseudo optical depth".

<%= member_table(
  units: "",
  legal_range: "",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcTauD=*float***

ASHRAE diffuse "pseudo optical depth".

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcEbnSlrNoon=*float***

Solar noon beam normal.  Alternative to dcTauB

<%= member_table(
  units: "Btuh/ft^2^",
  legal_range: "x $\\geq$ 0",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcEdhSlrNoon=*float***

Solar noon diffuse horizon.

<%= member_table(
  units: "Btuh/ft^2^",
  legal_range: "x $\\geq$ 0",
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

**endDesCond**

Optionally indicates the end of the descond definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "**none**",
  required: "No",
  variability: "") %>