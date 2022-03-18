# REPORTCOL

Each REPORTCOL defines a single column of a User Defined Table (UDT) report. REPORTCOLs are not used with report types other than UDT.

Use as many REPORTCOLs as there are values to be shown in each row of the user-defined report. The values will appear in columns, ordered from left to right in the order defined. Be sure to include any necessary values to identify the row, such as the day of month, hour of day, etc. CSE supplies *NO* columns automatically.

**colName**

Name of REPORTCOL.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**colReport=*rpName***

Name of report to which current report column belongs. If REPORTCOL is given within a REPORT object, then *colReport* defaults to that report.

<%= member_table(
  units: "",
  legal_range: "name of a *REPORT*",
  default: "*current report, if any*",
  required: "Unless in a *REPORT*",
  variability: "constant") %>

**colVal=*expression***

Value to show in this column of report.

<%= member_table(
  units: "",
  legal_range: "*any numeric or string expression*",
  default: "*none*",
  required: "Yes",
  variability: "subhour /end interval") %>

**colHead=*string***

Text used for column head.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*colName* or blank",
  required: "No",
  variability: "constant") %>

**colGap=*int***

Space between (to left of) column, in character positions. Allows you to space columns unequally, to emphasize relations among columns or to improve readability. If the total of the *colGaps* and *colWids* in the report's REPORTCOLs is substantially less than the REPORT's *rpCPL* (characters per line, see REPORT), CSE will insert additional spaces between columns. To suppress these spaces, use a smaller *rpCPL* or use *rpCPL* = -1.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "1",
  required: "No",
  variability: "constant") %>

**colWid=*int***

Column width.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "10",
  required: "No",
  variability: "constant") %>

**colDec=*int***

Number of digits after decimal point.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "*flexible format*",
  required: "No",
  variability: "constant") %>

**colJust=*choice***

Specifies positioning of data within column:

<%= csv_table(<<END, :row_header => false)
  Left,    Left justified
  Right,   Right justified
END
%>

**endReportCol**

Optionally indicates the end of the report column definition. Alternatively, the end of the report column definition can be indicated by END or by beginning another REPORTCOL or other object.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[reportCol](#p_reportcol)
