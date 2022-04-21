# REPORT

REPORT generates a report object to specify output of specific textual information about the results of the run, the input data, the error messages, etc. The various report types available are enumerated in the description of *rpType* in this section, and may be described at greater length in Section 6.

REPORTs are output by CSE to files, via the REPORTFILE object (previous section). After CSE has completed, you may print the report file(s), examine them with a text editor or by TYPEing, process them with another program, etc., as desired.

REPORTs that you do not direct to a different file are written to the automatically-supplied "Primary" report file, whose file name is (by default) the input file name with the extension changed to .REP.

Each report consists of a report header, one or more data rows, and a report footer. The header gives the report type (as specified with *rpType*, described below), the frequency (as specified with *rpFreq)*, the month or date where appropriate, and includes headings for the report's columns where appropriate.

Usually a report has one data row for each interval being reported. For example, a daily report has a row for each day, with the day of the month shown in the first column.

The report footer usually contains a line showing totals for the rows in the report.

The header-data-footer sequence is repeated as necessary. For example, a daily report extending over more than one month has a header-data-footer sequence for each month. The header shows the month name; the data rows show the day of the month; the footer contains totals for the month.

In addition to the headers and footers of individual reports, the report file has (by default) *page headers* and *footers*, described in the preceding section.

**Default Reports:** CSE generates the following reports by default for each run, in the order shown. They are output by default to the "Primary" report file. They may be ALTERed or DELETEd as desired, using the object names shown.

<%= csv_table(<<END, :row_header => true)
  rpName,        rpType,        Additional members
  Err,           ERR
  eb,            ZEB,           rpFreq=MONTH; rpZone=SUM;
  Log,           LOG
  Inp,           INP
END
%>

<!-- from second row of table: SumSUMNot implemented?? -->
Any reports specified by the user and not assigned to another file appear in the Primary report file between the default reports "eb" and "Log", in the order in which the REPORT objects are given in the input file.

Because of the many types of reports supported, the members required for each REPORT depend on the report type and frequency in a complex manner. When in doubt, testing is helpful: try your proposed REPORT specification; if it is incomplete or overspecified, CSE will issue specific error messages telling you what additional members are required or what inappropriate members have been given and why.

**rpName**

Name of report. Give after the word REPORT.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**rpReportfile=*rfname***

Name of report file to which current report will be written. If omitted, if REPORT is within a REPORTFILE object, report will be written to that report file, or else to REPORTFILE "Primary", which (as described in previous section) is automatically supplied and by default uses the file name of the input file with the extension .REP.

<%= member_table(
  units: "",
  legal_range: "name of a *REPORTFILE*",
  default: "current *REPORTFILE*, if any, else Primary",
  required: "No",
  variability: "constant") %>

**rpType=*choice***

Choice indicating report type. Report types may be described at greater length, with examples, in Section 6.

<%= csv_table(<<END, :row_header => false)
  ERR,     Error and warning messages. If there are any such messages&comma; they are also displayed on the screen *AND* written to a file with the same name as the input file and extension .ERR. Furthermore&comma; \* \*many error messages are repeated in the INP report.
  LOG,     Run 'log'. As of July 1992&comma; contains only CSE version number; should be enhanced or deleted.??
  INP,     Input echo: shows the portion of the input file used to specify this run. Does not repeat descriptions of objects left from prior runs in the same session when CLEAR is not used. Error and warning messages relating to specific lines of the input are repeated after or near the line to which they relate&comma; prefixed with '?'. Lines not used due to a preprocessor \#if command (Section 4.4.4) with a false expression are prefixed with a '0' in the leftmost column; all preprocessor command lines are prefixed with a '\#' in that column.
  SUM,     Run summary. As of July 1992&comma; *NOT IMPLEMENTED*: generates no  output and no error message. Should be defined and implemented&comma; or else deleted??.
  ZDD,     Zone data dump. Detailed dump of internal simulation values&comma; useful for verifying that your input is as desired. Should be made less cryptic (July 1992)??. Requires *rpZone*.
  ZST,     Zone statistics. Requires *rpZone*.
  ZEB,     Zone energy balance. Requires *rpZone*.
  MTR,     Meter report. Requires *rpMeter*.
  DHWMTR,  DHW meter report.  Requires *rpDHWMeter*
  AFMTR,   Air flow meter report.  Requires *rpAFMeter*
  UDT,     User-defined table. Data items are specified with REPORTCOL commands (next section). Allows creating almost any desired report by using CSE expressions to specify numeric or string values to tabulate; 'Probes' may be used in the expressions to access CSE internal data.
END
%>

<%= member_table(
  units: "",
  legal_range: "*see above*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

The next three members specify how frequently values are reported and the start and end dates for the REPORT. They are not allowed with *rpTypes* ERR, LOG, INP, SUM, and ZDD, which involve no time-varying data.

**rpFreq=*choice***

Report Frequency: specifies interval for generating rows of report data:

<%= csv_table(<<END, :row_header => false)
  YEAR, at run completion
  MONTH, at end of each month (and at run completion if mid-month)
  DAY, at end of each day
  HOUR, at end of each hour
  HOURANDSUB, at end of each subhour and at end of hour
  SUBHOUR, at end of each subhour
END
%>



*rpFreq* values of HOURANDSUB and SUBHOUR are not supported in some combinations with data selection of ALL or SUM.

We recommend using HOURly and more frequent reports sparingly, to report on only a few typical or extreme days, or to explore a problem once it is known what day(s) it occurs on. Specifying such reports for a full-year run will generate a huge amount of output and cause extremely slow CSE execution.

<%= member_table(
  units: "",
  legal_range: "*choices above*",
  default: "*none*",
  required: "per rpType",
  variability: "constant") %>

**rpDayBeg=*date***

Initial day of period to be reported. Reports for which *rpFreq* = YEAR do not allow specification of *rpDayBeg* and *rpDayEnd*; for MONTH reports, these members default to include all months in the run; for DAY and shorter-interval reports, *rpDayBeg* is required and *rpDayEnd* defaults to *rpDayBeg*.

<%= member_table(
  units: "",
  legal_range: "*date*",
  default: "first day of simulation if *rpFreq* = MONTH",
  required: "Required for *rpTypes* ZEB, ZST, MTR, AH, and UDT if *rpFreq* is DAY, HOUR, HOURANDSUB, or SUBHOUR",
  variability: "constant") %>

**rpDayEnd=*date***

Final day of period to be reported, except for YEAR reports.

<%= member_table(
  units: "",
  legal_range: "*date*",
  default: "last day of simulation if *rpFreq*= MONTH, else *rpDayBeg*",
  required: "No",
  variability: "constant") %>

**rpZone=*znName***

Name of ZONE for which a ZEB, ZST, or ZDD report is being requested. For *rpType* ZEB or ZST, you may use *rpZone*=SUM to obtain a report showing only the sum of the data for all zones, or *rpZone*=ALL to obtain a report showing, for each time interval, a row of data for each zone plus a sum-of-zones row.

<%= member_table(
  units: "",
  legal_range: "name of a *ZONE*, ALL, SUM",
  default: "*none*",
  required: "Required for *rpTypes* ZDD, ZEB, and ZST.",
  variability: "constant") %>

**rpMeter=*mtrName***

Specifies meter(s) to be reported, for *rpType*=MTR.

<%= member_table(
  units: "",
  legal_range: "name of a *METER*, ALL, SUM",
  default: "*none*",
  required: "Required for *rpType*=MTR",
  variability: "constant") %>

**rpDHWMeter=*dhwMtrName***

Specifies DHW meter(s) to be reported, for *rpType*=DHWMTR.

<%= member_table(
  units: "",
  legal_range: "name of a *DHWMETER*, ALL, SUM",
  default: "*none*",
  required: "Required for *rpType*=DHWMTR",
  variability: "constant") %>

**rpAFMeter=*afMtrName***

Specifies air flow meter(s) to be reported, for *rpType*=AFMTR.

<%= member_table(
  units: "",
  legal_range: "name of a *DHWMETER*, ALL, SUM",
  default: "*none*",
  required: "Required for *rpType*=AFMTR",
  variability: "constant") %>

**rpAh=*ahName***

Specifies air handler(s) to be reported, for *rpType*=AH, AHSIZE, or AHLOAD.

<%= member_table(
  units: "",
  legal_range: "name of an *AIRHANDLER*, ALL, SUM",
  default: "*none*",
  required: "Required for *rpType*=AH, AHSIZE, or AHSIZE",
  variability: "constant") %>

**rpTu=*tuName***

Specifies air handler(s) to be reported, for *rpType*=TUSIZE or TULOAD.

<%= member_table(
  units: "",
  legal_range: "name of a TERMINAL, ALL, SUM",
  default: "*none*",
  required: "Required for *rpType*",
  variability: "constant") %>

**rpBtuSf=*float***

Scale factor to be used when reporting energy values. Internally, all energy values are represented in Btu. This member allows scaling to more convenient units for output. *rpBtuSf* is not shown in the output, so if you change it, be sure the readers of the report know the energy units being used. *rpBtuSf* is not applied in UDT reports, but column values can be scaled as needed with expressions.

<%= member_table(
  units: "",
  legal_range: "*any multiple of ten*",
  default: "1,000,000: energy reported in MBtu.",
  required: "No",
  variability: "constant") %>

**rpCond=*expression***

Conditional reporting flag. If given, report rows are printed only when value of expression is non-0. Permits selective reporting according to any condition that can be expressed as a CSE expression. Such conditional reporting can be used to shorten output and make it easy to find data of interest when you are only interested in the information under exceptional conditions, such as excessive zone temperature. Allowed with *rpTypes* ZEB, ZST, MTR, AH, and UDT.

<%= member_table(
  units: "",
  legal_range: "*any numeric expression*",
  default: "1 (reporting enabled)",
  required: "No",
  variability: "subhour end of interval") %>

**rpCPL=*int***

Characters per line for a UDT (user-defined report). If widths specified in REPORTCOLs add up to more than this, a message occurs; if they total substantially less, additional whitespace is inserted between columns to make the report more readable. If rpCPL = -1, the report width determined based on required space with a single between each column.  rpCPL=0 uses the Top level *repCPL*. rpCPL is Not allowed if *rpType* is not UDT.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ -1",
  default: "as wide as needed",
  required: "No",
  variability: "constant") %>

**rpTitle=*string***

Title for use in report header of User-Defined report. Disallowed if *rpType* is not UDT.

<%= member_table(
  units: "",
  legal_range: "",
  default: "User-defined Report",
  required: "No",
  variability: "constant") %>

**rpHeader=*choice***

Use NO to suppress the report header which gives the report type, zone, meter, or air handler being reported, time interval, column headings, etc. One reason to do this might be if you are putting only a single report in a report file and intend to later embed the report in a document or process it with some other program (but for the latter, see also EXPORT, below).

Use with caution, as the header contains much of the identification of the data. For example, in an hourly report, only the hour of the day is shown in each data row; the day and month are shown in the header, which is repeated for each 24 data rows.

See REPORTFILE member *rfPageFmt*, above, to control report *FILE* page headers and footers, as opposed to *REPORT* headers and footers.

<%= member_table(
  units: "",
  legal_range: "YES, NO",
  default: "YES",
  required: "No",
  variability: "constant") %>

**rpFooter=*choice***

Use NO to suppress the report footers. The report footer is usually a row which sums hourly data for the day, daily data for the month, or monthly data for the year. For a report with *rpZone, rpMeter,*or *rpAh* = ALL, the footer row shows sums for all zones, meters, or air handlers. Sometimes the footer is merely a blank line.

<%= member_table(
  units: "",
  legal_range: "YES, NO",
  default: "YES",
  required: "No",
  variability: "constant") %>

**endReport**

Optionally indicates the end of the report definition. Alternatively, the end of the report definition can be indicated by END or by beginning another object.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[report](#p_report)
