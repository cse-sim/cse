# IMPORTFILE

IMPORTFILE allows specification of a file from which external data can be accessed using the [import()](#import) and [importStr()](#importstr) functions. This allows external values to be referenced in expressions.  Any number of IMPORTFILEs can be defined and any number of import()/importStr() references can be made to a give IMPORTFILE.

Import files are text files containing an optional header and comma-separated data fields.  With
the header present, the structure of an import file matches that of an [EXPORT](#export) file.  This makes it convenient to import unmodified files EXPORTed from prior runs.  The file structure is as follows (noting that the header in lines 1-4 should not be present when imHeader=NO) --

  Line      Contents                     Notes
  --------- -----------------------      --------------------------------------
  1         *runTitle*, *runNumber*      read but not checked
  2         *timestamp*                  in quotes, read but not checked
  3         *title*, *freq*              should match imTitle and imFreq (see below)
  4         *colName1*,*colName2*,...    comma separated column names optionally in quotes
  5 ..      *val1*,*val2*,...            comma separated values (string values optionally in quotes)


Example import file imp1.csv

        "Test run",001
        "Fri 04-Nov-16  10:54:37 am"
        "Daily Data","Day"
        Mon,Day,Tdb,Twb
        1,1,62.2263,53.2278
        1,2,61.3115,52.8527
        1,3,60.4496,52.4993
        1,4,60.2499,52.4174
        1,5,60.9919,52.7216
        1,6,61.295,52.8459
        1,7,62.3178,53.2654
        1,8,62.8282,53.4747
        (... continues for 365 data lines ...)

Example IMPORTFILE use (reading from imp1.csv)

        // ... various input statements ...

        IMPORTFILE Example imFileName="imp1.csv" imFreq=Day imTitle="Daily Data"
        ...
        // Compute internal gain based on temperature read from import file.
        // result is 3000 W per degree temperature is above 60.
        // Note gnPower can have hourly variability, but here varies daily.
        GAIN gnPower = 3000 * max( 0, import(Example,"Tdb") - 60) / 3.412
        ...

Notes

 * As usual, file order is not important -- IMPORTFILEs can be referenced before they are defined.
 * Columns are referenced by 1-based index or column names (assuming file header is present).
 In the example above, "Tdb" could be replaced by 3.
 * Column names should be case-insensitive unique.  CSE issues a warning for each non-unique name found. Reference to a non-unique name in import()/importStr() is treated as an error (no run).
 * Heading or data string values generally do not need to be quoted except for values that include comma(s).



**imName**

Name of IMPORTFILE object (for reference from Import()).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 No             constant

**imFileName=*string***

Gives path name of file to be read. If directory is specified, CSE first looks for the file the current directory and searches include paths specified by the -I command line parameter (if any).

  **Units**   **Legal Range**                            **Default**   **Required**   **Variability**
  ----------- ------------------------------------------ ------------- -------------- -----------------
              *file name, path optional*                                Yes             constant

**imTitle=*string***

Title expected to be found on line 3 of the import file.  A warning is issued if a non-blank imTitle does not match the import file title.

  **Units**   **Legal Range**          **Default**   **Required**   **Variability**
  ----------- ------------------------ ------------- -------------- -----------------
               Text string             (blank)       No             constant

**imFreq=*choice***

Specifies the interval at which CSE reads from the import file.  Data is read at the beginning of the indicated interval and buffered in memory for access in expressions via import() or importStr().

  **Units**   **Legal Range**           **Default**   **Required**   **Variability**
  ----------- ------------------------- ------------- -------------- -----------------
              YEAR, MONTH, DAY, or HOUR                    Yes             constant


**imHeader=*choice***

Indicates whether the import file include a 4 line header, as described above.  If NO, the import file
should contain only comma-separated data rows and data items can be referenced only by 1-based column number.

**Units**   **Legal Range**          **Default**   **Required**   **Variability**
----------- ------------------------ ------------- -------------- -----------------
            YES NO                   YES            No             constant

**imBinary=*choice***

Possible future binary file option.

<%= member_table(
  units: "",
  legal_range: "YES, NO",
  default: "No",
  required: "No",
  variability: "constant") %>

**endImportFile**

Optionally indicates the end of the import file definition. Alternatively, the end of the import file definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[importFile](#p_importfile)
- @[impFileFldNames](#p_impfilefldnames)
