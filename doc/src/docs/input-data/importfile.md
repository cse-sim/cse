# IMPORTFILE

IMPORTFILE allows specification of a file from which external data can be accessed using the [import()](../input-structure.md#import) and [importStr()](../input-structure.md#importstr) functions. This allows external values to be referenced in expressions. Any number of IMPORTFILEs can be defined and any number of import()/importStr() references can be made to a give IMPORTFILE.

Import files are text files containing an optional header and comma-separated data fields. With
the header present, the structure of an import file matches that of an [EXPORT][export] file. This makes it convenient to import unmodified files EXPORTed from prior runs. The file structure is as follows (noting that the header in lines 1-4 should not be present when imHeader=NO) --

<%= csv*table(<<END, :row_header => true)
Line, Contents, Notes
1, \_runTitle*&comma; _runNumber_, read but not checked
2, _timestamp_, in quotes&comma; read but not checked
3, _title_&comma; _freq_, should match imTitle and imFreq (see below)
4, _colName1_&comma; _colName2_&comma; ..., comma separated column names optionally in quotes
5 .., _val1_&comma; _val2_&comma; ..., comma separated values (string values optionally in quotes)
END
%>

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

- As usual, IMPORTFILEs can be referenced before they are defined. HOWEVER, _imFreq_ is not known for forward-references to IMPORTFILEs and will be assumed to be subhour. Errors (and no run) will result if the referencing import()s expect values at another _imFreq_. **Recommendation: locate IMPORTFILEs prior to associated imports() in the input file.**
- Columns are referenced by 1-based index or column names (assuming file header is present).
  In the example above, "Tdb" could be replaced by 3.
- Column names should be case-insensitive unique. CSE issues a warning for each non-unique name found. Reference to a non-unique name in import()/importStr() is treated as an error (no run).
- Heading or data string values generally do not need to be quoted except for values that include comma(s).

**imName**

Name of IMPORTFILE object (for reference from Import()).

<%= member*table(
units: "",
legal_range: "\_63 characters*",
default: "_none_",
required: "No",
variability: "constant") %>

**imFileName=_string_**

Gives path name of file to be read. If directory is specified, CSE first looks for the file the current directory and searches include paths specified by the -I command line parameter (if any).

<%= member*table(
units: "",
legal_range: "\_file name, path optional*",
default: "_none_",
required: "Yes",
variability: "constant") %>

**imTitle=_string_**

Title expected to be found on line 3 of the import file. A warning is issued if a non-blank imTitle does not match the import file title.

<%= member*table(
units: "",
legal_range: "Text string",
default: "\_none*",
required: "No",
variability: "constant") %>

**imFreq=_choice_**

Specifies the interval at which CSE reads from the import file. Data is read at the beginning of the indicated interval and buffered in memory for access in expressions via import() or importStr().

<%= member*table(
units: "",
legal_range: "YEAR, MONTH, DAY, HOUR, or SUBHOUR",
default: "\_none*",
required: "Yes",
variability: "constant") %>

**imHeader=_choice_**

Indicates whether the import file include a 4 line header, as described above. If NO, the import file
should contain only comma-separated data rows and data items can be referenced only by 1-based column number.

<%= member_table(
units: "",
legal_range: "YES NO",
default: "YES",
required: "No",
variability: "constant") %>

**imBinary=_choice_**

Adds the possibility to output the file as a binary option.

<%= member_table(
units: "",
legal_range: "YES NO",
default: "No",
required: "No",
variability: "constant") %>

**endImportFile**

Optionally indicates the end of the import file definition. Alternatively, the end of the import file definition can be indicated by END or by beginning another object.

<%= member*table(
units: "",
legal_range: "",
default: "\_none*",
required: "No",
variability: "constant") %>

**Related Probes:**

- @[importFile][p_importfile]
- @[impFileFldNames][p_impfilefldnames]
