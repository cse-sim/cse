# FOUNDATION

Foundation describes the two-dimensional relationship between ground-contact SURFACEs (i.e., **sfExCnd** = GROUND) and the surrounding ground. A FOUNDATION is referenced by Floor SURFACEs (see **sfFnd**). FOUNDATIONs are used to describe the two-dimensional features of foundation designs that cannot be captured by the typical one-dimensional constructions. Dimensions from the one-dimensional CONSTRUCTIONs associated with ground-contact floors and walls will automatically be interpreted into the two-dimensional context.

![Two-dimensional context](media/fd_context.png)

Any wall SURFACEs in contact with the ground must refer to a Floor SURFACE object (see **sfFndFloor**) to indicate which floor shares the same ground domain as a boundary condition (and establish the two-dimensional context for the basis of the ground calculations).

FOUNDATION objects are used to instantiate instances of heat transfer within Kiva.

MATERIALs used in a FOUNDATION cannot have variable properties at this time.

Most of the relevant dimensions and properties in the two-dimensional context are defined in the FOUNDATION object (and FNDBLOCK subobjects) with a few exceptions specified by specific SURFACEs:

- sfFnd
- sfFndFloor
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

The following data members describe the dimensions and properties of the foundation wall. For below-grade walls, the CONSTRUCTION (and corresponding width) of the foundation wall is defined by the Wall SURFACEs referencing the FOUNDATION object. For on-grade floors, the CONSTRUCTION of the foundation wall must be defined using **fdFtCon**. The actual height of the foundation wall (from the top of the wall to the top of the slab) is defined by the corresponding SURFACE objects.

![Foundation wall dimensions](media/fd_dims.png)

 Other components of the foundation design (e.g., interior/exterior insulation) as well as other variations in thermal properties within the ground are defined using FNDBLOCK (foundation block) objects. Any number of FNDBLOCKs can appear after the definition of a FOUNDATION to be properly associated.

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
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**fdWlDpBlwSlb=*float***

<!-- TODO: Optionally below grade? -->
Wall depth below slab.

<%= member_table(
  units: "ft",
  legal_range: "x $\\geq$ 0",
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

**endFoundation**

Indicates the end of the foundation definition. Alternatively, the end of the foundation definition can be indicated by the declaration of another object or by END.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>
