# ACCUMULATOR

An ACCUMULATOR object is a user-defined "device" that records arbitrary values computed during the CSE simulation.

TODO: expand description and example

ACCUMULATOR results must be reported using user-defined REPORTs or EXPORTs.  For example --

    REPORT rpType=UDT rpFreq=Month rpDayBeg=Jan 1 rpDayEnd=Dec 31
        REPORTCOL colHead="mon" colVal=$Month colWid=3
        REPORTCOL colHead="Total" colVal=@Accumulator[ 1].M.sum colDec=0 colWid=10
        REPORTCOL colHead="Average" colVal=@Accumulator[ 1].M.avg colDec=0 colWid=10


**acName**

Name of ACCUMULATOR: required for referencing in reports.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**acValue=*float***

The value being accumulated.  Generally expression.

<%= member_table(
  units: "any",
  legal_range: "",
  default: "",
  required: "Yes",
  variability: "subhourly") %>


**endACCUMULATOR**

Indicates the end of the ACCUMULATOR definition. Alternatively, the end of the definition can be indicated by the declaration of another object or by END.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[accumulator](#p_accumulator)