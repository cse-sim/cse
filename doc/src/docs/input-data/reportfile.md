# REPORTFILE

REPORTFILE allows optional specification of different or additional files to receive CSE reports.

By default, CSE generates several "reports" on each run showing the simulated HVAC energy use, the input statements specifying the run, any error or warning messages, etc. Different or additional reports can be specified using the REPORT object, described in Section 5.25, next.

All CSE reports are written to text files as plain ASCII text. The files may be printed (on most printers other than postscript printers) by copying them to your printer with the COPY command. Since many built-in reports are over 80 characters wide; you may want to set your printer for "compressed" characters or a small font first. You may wish to examine the report file with a text editor or LIST program before printing it. (?? Improve printing discussion)

By default, the reports are output to a file with the same name as the input file and extension .REP, in the same directory as the input file. By default, this file is formatted into pages, and overwrites any existing file of the same name without warning. CSE automatically generates a REPORTFILE object called "Primary" for this report file, as though the following input had been given:

        REPORTFILE "Primary"
            rfFileName = <inputFile>.REP;
            // other members defaulted: rfFileStat=OVERWRITE; rfPageFmt=YES.

Using REPORTFILE, you can specify additional report files. REPORTs specified within a REPORTFILE object definition are output by default to that file; REPORTs specified elsewhere may be directed to a specific report file with the REPORT member *rpReportFile*. Any number of REPORTFILEs and REPORTs may be used in a run or session. Any number of REPORTs can be directed to each REPORTFILE.

Using ALTER (Section 4.5.1.2) with REPORTFILE, you can change the characteristics of the Primary report output file. For example:

        ALTER REPORTFILE Primary
            rfPageFmt = NO;     // do not format into pages
            rfFileStat = NEW;   // error if file exists

### rfName

Name of REPORTFILE object, given immediately after the word REPORTFILE. Note that this name, not the fileName of the report file, is used to refer to the REPORTFILE in REPORTs.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

**rfFileName=*path***

path name of file to be written. If no path is specified, the file is written in the current directory. The default extension is .REP.

{{
  member_table({
    "units": "",
    "legal_range": "file name, path and extension optional", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

**rfFileStat=*choice***

Choice indicating what CSE should do if the file specified by *rfFileName*already exists:

{{ csv_table("OVERWRITE,    Overwrite pre-existing file.
  NEW,          Issue error message if file exists at beginning of session. If there are several runs in session using same file&comma; output from runs after the first will append.
  APPEND,       Append new output to present contents of existing file.")
}}

If the specified file does not exist, it is created and *rfFileStat* has no effect.

{{
  member_table({
    "units": "",
    "legal_range": "OVERWRITE, NEW, APPEND", 
    "default": "OVERWRITE",
    "required": "No",
    "variability": "constant" 
  })
}}

**rfPageFmt=*Choice***

Choice controlling page formatting. Page formatting consists of dividing the output into pages (with form feed characters), starting a new page before each report too long to fit on the current page, and putting headers and footers on each page. Page formatting makes attractive printed output but is a distraction when examining the output on the screen and may inappropriate if you are going to further process the output with another program.

{{ csv_table("Yes,   Do page formatting in this report file.
  No,    Suppress page formatting. Output is continuous&comma; uninterrupted by page headers and footers or large blank spaces.")
}}

{{
  member_table({
    "units": "",
    "legal_range": "YES, NO", 
    "default": "No",
    "required": "No",
    "variability": "constant" 
  })
}}

Unless page formatting is suppressed, the page formats for all report files are controlled by the TOP members *repHdrL, repHdrR, repLPP, repTopM, repBotM,*and *repCPL*, described in Section 5.1.

Each page header shows the *repHdrL* and *repHdrR* text, if given.

Each page footer shows the input file name, run serial number within session (see *runSerial* in Section 5.1), user-input *runTitle* (see Section 5.1), date and time of run, and page number in file.

Vertical page layout is controlled by *repLPP, repTopM,* and *repBotM* (Section 5.1). The width of each header and footer is controlled by *repCPL*. Since many built-in reports are now over 80 columns wide, you may want to use repCPL=120 or repCPL=132 to make the headers and footers match the text better.

In addition to report file *page* headers and footers, individual REPORTs have *REPORT* headers and footers related to the report content. These are described under REPORT, Section 5.25.

### endReportFile

Optionally indicates the end of the report file definition. Alternatively, the end of the report file definition can be indicated by END or by beginning another object.

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

- @[reportFile][p_reportfile]
