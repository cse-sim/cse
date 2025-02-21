# DHWLOOPSEG

DHWLOOPSEG constructs one or more objects representing a segment of the preceeding DHWLOOP. A DHWLOOP can have any number of DHWLOOPSEGs to represent the segments of the loop with possibly differing sizes, insulation, or surrounding conditions.

**wgName**

Optional name of segment; give after the word “DHWLOOPSEG” if desired.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant")
%>

**wgTy=_choice_**

Specifies the type of segment. RETURN segments, if any, must follow SUPPLY segments.

<%= csv_table(<<END, :row_header => false)
SUPPLY, Indicates a supply segment (flow is sum of circulation and draw flow&comma; child DHWLOOPBRANCHs permitted).
RETURN, Indicates a return segment (flow is only due to circulation&comma; child DHWLOOPBRANCHs not allowed)
END
%>

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "Yes",
variability: "constant")
%>

**wgLength=_float_**

Length of segment.

<%= member_table(
units: "ft",
legal_range: "x $\\ge$ 0",
default: "0",
required: "No",
variability: "constant")
%>

**wgSize=_float_**

Nominal size of pipe. CSE assumes the pipe outside diameter = size + 0.125 in.

<%= member_table(
units: "in",
legal_range: "x $>$ 0",
default: "1",
required: "Yes",
variability: "constant")
%>

**wgInsulK=_float_**

Pipe insulation conductivity

<%= member_table(
units: "Btuh-ft/ft^2^-^o^F",
legal_range: "x $>$ 0",
default: "0.02167",
required: "No",
variability: "constant")
%>

**wgInsulThk=_float_**

Pipe insulation thickness

<%= member_table(
units: "in",
legal_range: "x $\\ge$ 0",
default: "1",
required: "No",
variability: "constant")
%>

**wgExH=_float_**

Combined radiant/convective exterior surface conductance between insulation (or pipe if no insulation) and surround.

<%= member_table(
units: "Btuh/ft^2^-^o^F",
legal_range: "x $>$ 0",
default: "1.5",
required: "No",
variability: "hourly")
%>

**wgExT=_float_**

Surrounding equivalent temperature.

<%= member_table(
units: "^o^F",
legal_range: "x $>$ 0",
default: "70",
required: "No",
variability: "hourly")
%>

**wgFNoDraw=_float_**

Fraction of hour when no draw occurs.

<%= member_table(
units: "^o^F",
legal_range: "x $>$ 0",
default: "70",
required: "No",
variability: "hourly")
%>

**endDHWLoopSeg**

Optionally indicates the end of the DHWLOOPSEG definition.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "")
%>

**Related Probes:**

- @[DHWLoopSeg][p_dhwloopseg]
