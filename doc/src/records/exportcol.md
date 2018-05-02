# EXPORTCOL

Each EXPORTCOL defines a single datum of a User Defined Table (UDT) export; EXPORTCOLs are not used with other export types.

Use as many EXPORTCOLs as there are values to be shown in each row of the user-defined export. The values will appear in the order defined in each data row output. Be sure to include values needed to identify the data, such as the month, day, and hour, as appropriate -- these are NOT automatically supplied in user-defined exports.

EXPORTCOL members are similar to the corresponding REPORTCOL members. See Section 5.265.1.5 for further discussion.

**colName**

Name of EXPORTCOL.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**colExport=*exName***

Name of export to which this column belongs. If the EXPORTCOL is given within an EXPORT object, then *colExport* defaults to that export.

  **Units**   **Legal Range**       **Default**                **Required**            **Variability**
  ----------- --------------------- -------------------------- ----------------------- -----------------
              name of an *EXPORT*   *current export, if any*   Unless in an *EXPORT*   constant

**colVal=*expression***

Value to show in this position in each row of export.

  **Units**   **Legal Range**                      **Default**   **Required**   **Variability**
  ----------- ------------------------------------ ------------- -------------- -----------------------
              *any numeric or string expression*                 Yes            subhour /end interval

**colHead=*string***

Text used for field name in export header.

  **Units**   **Legal Range**   **Default**          **Required**   **Variability**
  ----------- ----------------- -------------------- -------------- -----------------
                                *colName* or blank   No             constant

**colWid=*int***

Maximum width. Leading and trailing spaces and non-significant zeroes are removed from export data to save file space. Specifying a *colWid* less than the default may reduce the maximum number of significant digits output.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 0       13            No             constant

**colDec=*int***

Number of digits after decimal point.

  **Units**   **Legal Range**   **Default**         **Required**   **Variability**
  ----------- ----------------- ------------------- -------------- -----------------
              *x* $\ge$ 0       *flexible format*   No             constant

**colJust=*choice***

Specifies positioning of data within column:

  ------- -----------------
  Left    Left justified
  Right   Right justified
  ------- -----------------

**endExportCol**

Optionally indicates the end of the EXPORTCOL. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- [@exportCol](#p_exportcol)
