# EXPORTCOL

Each EXPORTCOL defines a single datum of a User Defined Table (UDT) export; EXPORTCOLs are not used with other export types.

Use as many EXPORTCOLs as there are values to be shown in each row of the user-defined export. The values will appear in the order defined in each data row output. Be sure to include values needed to identify the data, such as the month, day, and hour, as appropriate -- these are NOT automatically supplied in user-defined exports.

EXPORTCOL members are similar to the corresponding REPORTCOL members. See Section 5.265.1.5 for further discussion.

**colName**

Name of EXPORTCOL.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**colExport=*exName***

Name of export to which this column belongs. If the EXPORTCOL is given within an EXPORT object, then *colExport* defaults to that export.

{{
  member_table({
    "units": "",
    "legal_range": "name of an *EXPORT*", 
    "default": "*current export, if any*",
    "required": "Unless in an *EXPORT*",
    "variability": "constant" 
  })
}}

**colVal=*expression***

Value to show in this position in each row of export.

{{
  member_table({
    "units": "",
    "legal_range": "*any numeric or string expression*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "subhour /end interval" 
  })
}}

**colHead=*string***

Text used for field name in export header.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*colName* or blank",
    "required": "No",
    "variability": "constant" 
  })
}}

**colWid=*int***

Maximum width. Leading and trailing spaces and non-significant zeroes are removed from export data to save file space. Specifying a *colWid* less than the default may reduce the maximum number of significant digits output.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "13",
    "required": "No",
    "variability": "constant" 
  })
}}

**colDec=*int***

Number of digits after decimal point.

{{
  member_table({
    "units": "",
    "legal_range": "x $\\ge$ 0", 
    "default": "*flexible format*",
    "required": "No",
    "variability": "constant" 
  })
}}

**colJust=*choice***

Specifies positioning of data within column:

{{ csv_table("Left,    Left justified
  Right,   Right justified")
}}

**endExportCol**

Optionally indicates the end of the EXPORTCOL. Alternatively, the end of the definition can be indicated by END or by beginning another object.

{{
  member_table({
    "units": "",
    "legal_range": "", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**Related Probes:**

- @[exportCol][p_exportcol]
