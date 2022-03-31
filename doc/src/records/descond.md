# Descond

Cooling design conditions

**dcDB=*float***

Design dry-bulb temp.

<%= member_table(
  units: "^o^F",
  legal_range: "no-limitations?",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**dcDay=*DOY***

Calendar date for this design cooling conditions.

<%= member_table(
  units: "",
  legal_range: "1-365",
  default: "200",
  required: "No",
  variability: "constant") %>

**dcEbnSlrNoon=*float***

Solar noon beam normal.

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

**dcMCDBR=*float***

Coincident daily dry-bulb range.

<%= member_table(
  units: "^o^F",
  legal_range: "no-limitations?",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcMCWB=*float***

Coincident wet-bulb temp.

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

Beam tau.

<%= member_table(
  units: "",
  legal_range: "",
  default: "**none**",
  required: "No",
  variability: "constant") %>

**dcTauD=*float***

Diffuse tau.

<%= member_table(
  units: "",
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