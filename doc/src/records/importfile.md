# IMPORTFILE

IMPORTFILE allows specification of files from which external data can be accessed using the import() and importStr() functions, allowing external values to be referenced in expressions.

Import files are text files containing a header plus comma-separated data fields.  The structure of an import file matches that of an exported file (see EXPORTFILE and EXPORT), making it possible to directly import files exported from prior runs.

  Line     Contents                     Notes
  -------- -----------------------      --------------------------------------
  1        *runTitle*, *runNumber*      read but not checked
  2        *timestamp*                  read but not checked
  3        *title*, *freq*              must match imTitle and imFreq
  4        column headings
  5 ..     comma separated data




Line 2:
[Line 3]
Line 4:
data1, data2, data3, ...
...



EXPORT, then IMPORT



**imName**

Name of IMPORTFILE object (for reference from Import()).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 No             constant

**imFileName=*string***

path name of file to be read. If no path is specified, the file sought via include paths specified using the -I command line parameter (if any).

  **Units**   **Legal Range**                            **Default**   **Required**   **Variability**
  ----------- ------------------------------------------ ------------- -------------- -----------------
              *file name, path optional*                                Yes             constant

**imTitle=*string***

Expected title found on line 3 of file.  If title does not match, what?

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
               Text string             (blank)       No             constant

**imFreq=*choice***

Specifies the interval at which CSE reads from the import file.  Data is read at the beginning of the indicated interval and buffered in memory for access in expressions via import() or importStr().

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
              YEAR, MONTH, DAY, or HOUR                    Yes             constant


**imHeader=*choice***

**Units**   **Legal Range**          **Default**   **Required**   **Variability**
----------- ------------------------ ------------- -------------- -----------------
            YES NO                   YES            No             constant



**endImportFile**

Optionally indicates the end of the import file definition. Alternatively, the end of the import file definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
