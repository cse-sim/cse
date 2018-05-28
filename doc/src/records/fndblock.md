# FNDBLOCK

Foundation blocks are materials within the two-dimensional domain beyond those defined by the slab and wall SURFACEs. Each block is represented as a rectangle in the domain by specifying the X (lateral) and Z (vertical) coordinates of two opposite corners. The coordinate system for each point is relative to the X and Z references defined by the user.

<!-- TODO: Insert diagram of references -->

The default reference is WALLINT, WALLTOP.

**fcMat=*matName***

Name of MATERIAL of the foundation block.

<%= member_table(
  legal_range: "Name of a *Material*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**fcX1Ref=*choice***

Relative X origin for *fcX1* point. Options are:

- SYMMETRY
- WALLINT
- WALLCENTER
- WALLEXT
- FARFIELD

<%= member_table(
  units: "",
  legal_range: "*choices above*",
  default: "WALLINT",
  required: "No",
  variability: "constant") %>

**fcZ1Ref=*choice***

Relative Z origin for *fcZ1* point. Options are:

- WALLTOP
- GRADE
- SLABTOP
- SLABBOTTOM
- WALLBOTTOM
- DEEPGROUND

<%= member_table(
  units: "",
  legal_range: "*choices above*",
  default: "WALLTOP",
  required: "No",
  variability: "constant") %>

**fcX1=*float***

The X position of the first corner of the block relative to *fcX1Ref*.

<%= member_table(
  units: "ft",
  legal_range: "",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**fcZ1=*float***

The Z position of the first corner of the block relative to *fcZ1Ref*.

<%= member_table(
  units: "ft",
  legal_range: "",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**fcX2Ref=*choice***

Relative X origin for *fcX2* point. Options are:

- SYMMETRY
- WALLINT
- WALLCENTER
- WALLEXT
- FARFIELD

<%= member_table(
  units: "",
  legal_range: "*choices above*",
  default: "WALLINT",
  required: "No",
  variability: "constant") %>

**fcZ2Ref=*choice***

Relative Z origin for *fcZ2* point. Options are:

- WALLTOP
- GRADE
- SLABTOP
- SLABBOTTOM
- WALLBOTTOM
- DEEPGROUND

<%= member_table(
  units: "",
  legal_range: "*choices above*",
  default: "WALLTOP",
  required: "No",
  variability: "constant") %>

**fcX2=*float***

The X position of the second corner of the block relative to *fcX2Ref*.

<%= member_table(
  units: "ft",
  legal_range: "",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**fcZ2=*float***

The Z position of the second corner of the block relative to *fcZ2Ref*.

<%= member_table(
  units: "ft",
  legal_range: "",
  default: "0.0",
  required: "No",
  variability: "constant") %>
