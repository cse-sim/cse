# PERIMETER

PERIMETER defines a subobject belonging to the current zone that represents a length of exposed edge of a (slab on grade) floor.

**prName**

Optional name of perimeter.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**prLen=*float***

Length of exposed perimeter.

<%= member_table(
  units: "ft",
  legal_range: "*x* $>$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**prF2=*float***

Perimeter conduction per unit length.

<%= member_table(
  units: "Btuh/ft-^o^F",
  legal_range: "*x* $>$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**endPerimeter**

Optionally indicates the end of the perimeter definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[perimeter](#p_perimeter)
- @[xsurf](#p_xsurf)
