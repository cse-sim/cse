# EXPORTFILE

EXPORTFILE allows optional specification of different or additional files to receive CSE EXPORTs.

EXPORTs contain the same information as reports, but formatted for reading by other programs rather than by people. By default, CSE generates no exports. Exports are specified via the EXPORT object, described in Section 5.28 (next). As for REPORTs, CSE automatically supplies a primary export file; it has the same name and path as the input file, and extension .csv.

Input for EXPORTFILEs and EXPORTs is similar to that for REPORTFILEs and REPORTs, except that there is no page formatting. Refer to their preceding descriptions (Sections 5.24 and 5.25) for more additional discussion.

**xfName**

Name of EXPORTFILE object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 No             constant

**xfFileName=*string***

path name of file to be written. If no path is specified, the file is written in the current directory. If no extension is specified, .csv is used.

  **Units**   **Legal Range**                            **Default**   **Required**   **Variability**
  ----------- ------------------------------------------ ------------- -------------- -----------------
              *file name, path and extension optional*                 No             constant

**xfFileStat=*choice***

What CSE should do if file *xfFileName* already exists:

  ----------------- ----------------------------------------------------
  OVERWRITE         Overwrite pre-existing file.

  NEW               Issue error message if file exists.

  APPEND            Append new output to present contents of existing
                    file.
  ----------------- ----------------------------------------------------

If the specified file does not exist, it is created and *xfFileStat* has no effect.

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
              OVERWRITE, NEW, APPEND   OVERWRITE     No             constant

**endExportFile**

Optionally indicates the end of the export file definition. Alternatively, the end of the Export file definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


