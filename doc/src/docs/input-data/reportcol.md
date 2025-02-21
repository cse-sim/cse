# REPORTCOL

Each REPORTCOL defines a single column of a User Defined Table (UDT) report. REPORTCOLs are not used with report types other than UDT.

Use as many REPORTCOLs as there are values to be shown in each row of the user-defined report. The values will appear in columns, ordered from left to right in the order defined. Be sure to include any necessary values to identify the row, such as the day of month, hour of day, etc. CSE supplies _NO_ columns automatically.

**colName**

Name of REPORTCOL.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant") %>

**colReport=_rpName_**

Name of report to which current report column belongs. If REPORTCOL is given within a REPORT object, then _colReport_ defaults to that report.

<%= member_table(
units: "",
legal_range: "name of a _REPORT_",
default: "_current report, if any_",
required: "Unless in a _REPORT_",
variability: "constant") %>

**colVal=_expression_**

Value to show in this column of report.

<%= member_table(
units: "",
legal_range: "_any numeric or string expression_",
default: "_none_",
required: "Yes",
variability: "subhour /end interval") %>

**colHead=_string_**

Text used for column head.

<%= member_table(
units: "",
legal_range: "",
default: "_colName_ or blank",
required: "No",
variability: "constant") %>

**colGap=_int_**

Space between (to left of) column, in character positions. Allows you to space columns unequally, to emphasize relations among columns or to improve readability. If the total of the _colGaps_ and _colWids_ in the report's REPORTCOLs is substantially less than the REPORT's _rpCPL_ (characters per line, see REPORT), CSE will insert additional spaces between columns. To suppress these spaces, use a smaller _rpCPL_ or use _rpCPL_ = -1.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "1",
required: "No",
variability: "constant") %>

**colWid=_int_**

Column width.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "10",
required: "No",
variability: "constant") %>

**colDec=_int_**

Number of digits after decimal point.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "_flexible format_",
required: "No",
variability: "constant") %>

**colJust=_choice_**

Specifies positioning of data within column:

<%= csv_table(<<END, :row_header => false)
Left, Left justified
Right, Right justified
END
%>

**endReportCol**

Optionally indicates the end of the report column definition. Alternatively, the end of the report column definition can be indicated by END or by beginning another REPORTCOL or other object.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

**Related Probes:**

- @[reportCol][p_reportcol]
