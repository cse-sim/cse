# EXPORTFILE

EXPORTFILE allows optional specification of different or additional files to receive CSE EXPORTs.

EXPORTs contain the same information as reports, but formatted for reading by other programs rather than by people. By default, CSE generates no exports. Exports are specified via the EXPORT object, described in Section 5.28 (next). As for REPORTs, CSE automatically supplies a primary export file; it has the same name and path as the input file, and extension .csv.

Input for EXPORTFILEs and EXPORTs is similar to that for REPORTFILEs and REPORTs, except that there is no page formatting. Refer to their preceding descriptions (Sections 5.24 and 5.25) for more additional discussion.

### xfName

Name of EXPORTFILE object.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### xfFileName

Type: *string*

path name of file to be written. If no path is specified, the file is written in the current directory. If no extension is specified, .csv is used.

{{
  member_table({
    "units": "",
    "legal_range": "*file name, path and extension optional*", 
    "default": "*none*",
    "required": "Yes",
    "variability": "constant" 
  })
}}

### xfFileStat

Type: *choice*

What CSE should do if file *xfFileName* already exists:

{{ csv_table("OVERWRITE,         Overwrite pre-existing file.
  NEW,               Issue error message if file exists.
  APPEND,            Append new output to present contents of existing file.")
}}

If the specified file does not exist, it is created and *xfFileStat* has no effect.

{{
  member_table({
    "units": "",
    "legal_range": "OVERWRITE, NEW, APPEND", 
    "default": "OVERWRITE",
    "required": "No",
    "variability": "constant" 
  })
}}

### endExportFile

Optionally indicates the end of the export file definition. Alternatively, the end of the Export file definition can be indicated by END or by beginning another object.

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

- @[exportFile][p_exportfile]
