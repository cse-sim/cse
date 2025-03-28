# EXPORT

Exports contain the same information as CSE reports, but in a "comma-quote" format intended for reading into a spreadsheet or other program for further processing, plotting, special print formatting, etc.

No exports are generated by default; each desired export must be specified with an EXPORT object.

Each row of an export contains several values, separated by commas, with quotes around string values. The row is terminated with a carriage return/line feed character pair. The first fields of the row identify the data. Multiple fields are used as necessary to identify the data. For example, the rows of an hourly ZEB export begin with the month, day of month, and hour of day. In contrast, reports, being subject to a width limitation, use only a single column of each row to identify the data; additional identification is put in the header. For example, an hourly ZEB Report shows the hour in a column and the day and month in the header; the header is repeated at the start of each day. The header of an export is never repeated.

Depending on your application, if you specify multiple exports, you may need to place each in a separate file. Generate these files with EXPORTFILE, preceding section. You may also need to suppress the export header and/or footer, with *exHeader* and/or *exFooter*, described in this section.

Input for EXPORTs is similar to input for REPORTs; refer to the REPORT description in Section 5.25 for further discussion of the members shown here.

**exName**

Name of export. Give after the word EXPORT.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**exExportfile=*fname***

Name of export file to which current export will be written. If omitted, if EXPORT is within an EXPORTFILE object, report will be written to that export file, or else to the automatically-supplied EXPORTFILE "Primary", which by default uses the name of the input file with the extension .csv.

<%= member_table(
  units: "",
  legal_range: "name of an  *EXPORTFILE*",
  default: "current *EXPORTFILE*, if any, else 'Primary'",
  required: "No",
  variability: "constant")
  %>

**exType=*choice***

Choice indicating export type. See descriptions in Section 5.22, REPORT. While not actually disallowed, use of *exType* = ERR, LOG, INP, or ZDD is unexpected.

<%= member_table(
  units: "",
  legal_range: "ZEB, ZST, MTR, DHWMTR, AH, UDT, or SUM",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**exFreq=*choice***

Export Frequency: specifies interval for generating rows of export data:

<%= member_table(
  units: "",
  legal_range: "YEAR, MONTH, DAY, HOUR, HOURANDSUB, SUBHOUR",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**exDayBeg=*date***

Initial day of export. Exports for which *exFreq* = YEAR do not allow specification of *exDayBeg* and *exDayEnd*; for MONTH exports, these members are optional and default to include the entire run; for DAY and shorter-interval exports, *exDayBeg* is required and *exDayEnd* defaults to *exDayBeg*.

<%= member_table(
  units: "",
  legal_range: "*date*",
  default: "first day of simulation if *exFreq* = MONTH",
  required: "Required for *exTypes* ZEB, ZST, MTR, AH, and UDT if *exFreq* is DAY, HOUR, HOURANDSUB, or SUBHOUR",
  variability: "constant")
  %>

**exDayEnd=*date***

Final day of export period, except for YEAR exports.

<%= member_table(
  units: "",
  legal_range: "*date*",
  default: "last day of simulation if *exFreq*= MONTH, else *exDayBeg*",
  required: "No",
  variability: "constant")
  %>

**exZone=*znName***

Name of ZONE for which a ZEB, ZST, or ZDD export is being requested; ALL and SUM are also allowed except with *exType* = ZST.

<%= member_table(
  units: "",
  legal_range: "name of a *ZONE*, ALL, SUM",
  default: "*none*",
  required: "Required for *exTypes* ZDD, ZEB, and ZST.",
  variability: "constant")
  %>

**exMeter=*mtrName***

Specifies meter(s) whose data is to be exported, for *exType*=MTR.

<%= member_table(
  units: "",
  legal_range: "name of a *METER*, ALL, SUM",
  default: "*none*",
  required: "for *exType*=MTR",
  variability: "constant")
  %>

**exTu=*tuName***

Specifies air handler(s) to be reported, for *rpType*=TUSIZE or TULOAD.

<%= member_table(
  units: "",
  legal_range: "name of a TERMINAL, ALL, SUM",
  default: "",
  required: "Required for *rpType*",
  variability: "constant") %>

**exDHWMeter=*dhwMtrName***

Specifies DHW meter(s) whose data is to be exported, for *exType*=DHWMTR.

<%= member_table(
  units: "",
  legal_range: "name of a *DHWMETER*, ALL, SUM",
  default: "*none*",
  required: "for *exType*=DHWMTR",
  variability: "constant")
  %>

**exAFMeter=*afMtrName***

Air flow meter report.

<%= member_table(
  units: "",
  legal_range: "*Name of AFMETER*",
  default: "0",
  required: "No",
  variability: "runly") %>

**exAh=ah*Name***

Specifies air handler(s) to be exported, for *exType*=AH.

<%= member_table(
  units: "",
  legal_range: "name of an *AIRHANDLER*, ALL, SUM",
  default: "*none*",
  required: "for *exType*=AH",
  variability: "constant")
  %>

**exBtuSf=*float***

Scale factor used for exported energy values.

<%= member_table(
  units: "",
  legal_range: "*any multiple of ten*",
  default: "1,000,000: energy exported in MBtu.",
  required: "No",
  variability: "constant")
  %>

**exCond=*expression***

Conditional exporting flag. If given, export rows are generated only when value of expression is non-0. Allowed with *exTypes* ZEB, ZST, MTR, AH, and UDT.

<%= member_table(
  units: "",
  legal_range: "*any numeric expression*",
  default: "1 (exporting enabled)",
  required: "No",
  variability: "subhour /end of interval")
  %>

**exTitle=*string***

Title for use in export header of User-Defined export. Disallowed if *exType* is not UDT.

<%= member_table(
  units: "",
  legal_range: "",
  default: "User-defined Export",
  required: "No",
  variability: "constant")
  %>

**exHeader=*choice***

Use NO to suppress the export header which gives the export type, zone, meter, or air handler being exported, time interval, column headings, etc. You might do this if the export is to be subsequently imported to a program that is confused by the header information. Alternatively, one may use COLUMNSONLY to print only the column headings. This can be useful when plotting CSV data in a spreadsheet tool or [DView](https://beopt.nrel.gov/downloadDView).

The choices YESIFNEW and COLUMNSONLYIFNEW cause header generation when the associated EXPORTFILE is being created but suppress headers when appending to an existing file.  This is useful for accumulating results from a set of runs where typically column headings are desired only once.

If not suppressed, the export header shows, in four lines:

*runTitle* and *runSerial* (see Section 5.1);the run date and time the export type ("Energy Balance", "Statistics", etc., or *exTitle* if given)and frequency ("year", "day", etc.)a list of field names in the order they will be shown in the data rows("Mon", "Day", "Tair", etc.)

The *specific* month, day, etc. is NOT shown in the export header (as it is shown in the report header), because it is shown in each export row.

The field names may be used by a program reading the export to identify the data in the rows which follow; if the program does this, it will not require modification when fields are added to or rearranged in the export in a future version of CSE.

<%= member_table(
  units: "",
  legal_range: "YES, YESIFNEW, NO, COLUMNSONLY, COLUMNSONLYIFNEW",
  default: "YES",
  required: "No",
  variability: "constant") %>

**exFooter=*choice***

Use NO to suppress the blank line otherwise output as an export "footer". (Exports do not receive the total lines that most reports receive as footers.)

<%= member_table(
  units: "",
  legal_range: "YES, NO",
  default: "YES",
  required: "No",
  variability: "constant")
  %>

**endExport**

Optionally indicates the end of the export definition. Alternatively, the end of the export definition can be indicated by END or by beginning another object.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**Related Probes:**

- @[export](#p_export)
