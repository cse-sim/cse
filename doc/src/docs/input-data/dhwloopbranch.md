# DHWLOOPBRANCH

DHWLOOPBRANCH constructs one or more objects representing a branch pipe from the preceding DHWLOOPSEG. A DHWLOOPSEG can have any number of DHWLOOPBRANCHs to represent pipe runs with differing sizes, insulation, or surrounding conditions.

**wbName**

Optional name of segment; give after the word “DHWLOOPBRANCH” if desired.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant")
%>

**wbMult=_float_**

Specifies the number of identical DHWLOOPBRANCHs. Note may be non-integer.

<%= member_table(
units: "",
legal_range: "_x_ $>$ 0",
default: "1",
required: "No",
variability: "constant")
%>

**wbLength=_float_**

Length of branch.

<%= member_table(
units: "ft",
legal_range: "_x_ $\\ge$ 0",
default: "0",
required: "No",
variability: "constant")
%>

**wbSize=_float_**

Nominal size of pipe. CSE assumes the pipe outside diameter = size + 0.125 in.

<%= member_table(
units: "in",
legal_range: "_x_ $>$ 0",
default: "_none_",
required: "Yes",
variability: "constant")
%>

**wbInsulK=_float_**

Pipe insulation conductivity

<%= member_table(
units: "Btuh-ft/ft^2^-^o^F",
legal_range: "_x_ $>$ 0",
default: "0.02167",
required: "No",
variability: "constant")
%>

**wbInsulThk=_float_**

Pipe insulation thickness

<%= member_table(
units: "in",
legal_range: "_x_ $\\ge$ 0",
default: "1",
required: "No",
variability: "constant")
%>

**wbExH=_float_**

Combined radiant/convective exterior surface conductance between insulation (or pipe if no insulation) and surround.

<%= member_table(
units: "Btuh/ft^2^-^o^F",
legal_range: "_x_ $>$ 0",
default: "1.5",
required: "No",
variability: "hourly")
%>

**wbExCnd=_choice_**

Specify exterior conditions.

<%= csv_table(<<END, :row_header => true)
Choice, Description
ADIABATIC, Adiabatic on other side
AMBIENT, Ambient exterior
SPECT, Specify temperature
ADJZN, Adjacent zone
GROUND, Ground conditions
END
%>

<%= member_table(
units: "",
legal_range: "See table above",
default: "SPECT",
required: "No",
variability: "constant") %>

**wbAdjZn=_float_**

Boundary conditions for adjacent zones.

<%= member_table(
units: "",
legal_range: "x $>$ 0",
default: "0.0",
required: "No",
variability: "runly") %>

**wbExTX=_float_**

External boundary conditions.

<%= member_table(
units: "",
legal_range: "x $>$ 0",
default: "70.0",
required: "No",
variability: "runly") %>

**wbFUA=_float_**

Adjustment factor applied to branch UA. UA is derived (from wbSize, wbLength, wbInsulK, wbInsulThk, and wbExH) and then multiplied by wbFUA. Used to represent e.g. imperfect insulation. Note that parent DHWLOOP wlFUA does not apply to DHWLOOPBRANCH (only DHWLOOPSEG)

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "1",
required: "No",
variability: "constant")
%>

**wbExT=_float_**

Surrounding equivalent temperature.

<%= member_table(
units: "^o^F",
legal_range: "_x_ $>$ 0",
default: "_70_",
required: "No",
variability: "hourly")
%>

**wbFlow=_float_**

Branch flow rate assumed during draw.

<%= member_table(
units: "gpm",
legal_range: "_x_ $\\ge$ 0",
default: "2",
required: "No",
variability: "hourly")
%>

**wbFWaste=_float_**

Number of times during the hour when the branch volume is discarded.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "0",
required: "No",
variability: "hourly")
%>

**endDHWLOOPBRANCH**

Optionally indicates the end of the DHWLOOPBRANCH definition.

<%= member_table(
units: "",
legal_range: "_none_",
default: "_none_",
required: "No",
variability: "")
%>

**Related Probes:**

- @[DHWLoopBranch][p_dhwloopbranch]
