# EXPORTCOL

Each EXPORTCOL defines a single datum of a User Defined Table (UDT) export; EXPORTCOLs are not used with other export types.

Use as many EXPORTCOLs as there are values to be shown in each row of the user-defined export. The values will appear in the order defined in each data row output. Be sure to include values needed to identify the data, such as the month, day, and hour, as appropriate -- these are NOT automatically supplied in user-defined exports.

EXPORTCOL members are similar to the corresponding REPORTCOL members. See Section 5.265.1.5 for further discussion.

**colName**

Name of EXPORTCOL.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "No",
variability: "constant")
%>

**colExport=_exName_**

Name of export to which this column belongs. If the EXPORTCOL is given within an EXPORT object, then _colExport_ defaults to that export.

<%= member_table(
units: "",
legal_range: "name of an _EXPORT_",
default: "_current export, if any_",
required: "Unless in an _EXPORT_",
variability: "constant")
%>

**colVal=_expression_**

Value to show in this position in each row of export.

<%= member_table(
units: "",
legal_range: "_any numeric or string expression_",
default: "_none_",
required: "Yes",
variability: "subhour /end interval")
%>

**colHead=_string_**

Text used for field name in export header.

<%= member_table(
units: "",
legal_range: "",
default: "_colName_ or blank",
required: "No",
variability: "constant")
%>

**colWid=_int_**

Maximum width. Leading and trailing spaces and non-significant zeroes are removed from export data to save file space. Specifying a _colWid_ less than the default may reduce the maximum number of significant digits output.

<%= member_table(
units: "",
legal_range: "x $\\ge$ 0",
default: "13",
required: "No",
variability: "constant")
%>

**colDec=_int_**

Number of digits after decimal point.

<%= member_table(
units: "",
legal_range: "x $\\ge$ 0",
default: "_flexible format_",
required: "No",
variability: "constant")
%>

**colJust=_choice_**

Specifies positioning of data within column:

<%= csv_table(<<END, :row_header => false)
Left, Left justified
Right, Right justified
END
%>

**endExportCol**

Optionally indicates the end of the EXPORTCOL. Alternatively, the end of the definition can be indicated by END or by beginning another object.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant")
%>

**Related Probes:**

- @[exportCol][p_exportcol]
