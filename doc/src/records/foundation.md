# FOUNDATION

Foundation describes the two-dimensional relationship between ground-contact SURFACEs (i.e., **sfExCnd** = GROUND) and the surrounding ground. A FOUNDATION is referenced by SURFACEs (see **sfFnd**). FOUNDATIONs are used to describe the two-dimensional features of foundation designs that cannot be captured by the typical one-dimensional constructions.

<!--TODO: Add 2-D context image -->

Any wall SURFACEs in contact with the ground must refer to a FOUNDATION object that is also referenced by a Floor SURFACE. A common reference to a FOUNDATION between a Floor and any number of Wall SURFACEs establishes that all SURFACEs share the same ground domain as a boundary condition. Exactly one Floor SURFACE must reference each FOUNDATION. However, multiple floors may exist in the same thermal zone so long as they reference separate FOUNDATIONs.

FOUNDATION objects are used to instantiate instances of heat transfer within Kiva.

MATERIALs used in a FOUNDATION cannot have variable properties at this time.

<!-- TODO: Mention all other relevant inputs in SURFACE and TOP -->

Most of the relevant dimensions in the two-dimensional context are defined in the FOUNDATION object with a few exceptions specified by specific SURFACEs:

- sfHeight
- sfExpPerim
- sfCon

Some properties applying to all FOUNDATIONs are defined at the TOP level:

- soilCond
- soilSpHt
- soilDens
- grndEmiss
- grndRefl
- grndRf
- farFieldWidth
- deepGrndCnd
- deepGrndDepth
- deepGrndT
- grndMinDim
- grndMaxGrthCoeff
- grndTimeStep


**fdName**

Name of foundation; give after the word FOUNDATION. Required for reference from SURFACE objects.

<%= member_table(
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**fdWlHtAbvGrd=*float***

Wall height above grade.

<%= member_table(
  units: "ft",
  legal_range: "x $\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**fdWlDpBlwSlb=*float***

<!-- TODO: Optionally below grade? -->
Wall depth below slab.

<%= member_table(
  units: "ft",
  legal_range: "x $\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**fdFtCon=*conName***

Name of CONSTRUCTION of the footing wall. Only required **IF** it is a slab foundation (i.e., no wall surfaces reference this FOUNDATION object).

<%= member_table(
  legal_range: "Name of a *Construction*",
  default: "*none*",
  required: "if a slab foundation",
  variability: "constant") %>

s
