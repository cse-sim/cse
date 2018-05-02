# REPORTCOL

Each REPORTCOL defines a single column of a User Defined Table (UDT) report. REPORTCOLs are not used with report types other than UDT.

Use as many REPORTCOLs as there are values to be shown in each row of the user-defined report. The values will appear in columns, ordered from left to right in the order defined. Be sure to include any necessary values to identify the row, such as the day of month, hour of day, etc. CSE supplies *NO* columns automatically.

**colName**

Name of REPORTCOL.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**colReport=*rpName***

Name of report to which current report column belongs. If REPORTCOL is given within a REPORT object, then *colReport* defaults to that report.

  **Units**   **Legal Range**      **Default**                **Required**           **Variability**
  ----------- -------------------- -------------------------- ---------------------- -----------------
              name of a *REPORT*   *current report, if any*   Unless in a *REPORT*   constant

**colVal=*expression***

Value to show in this column of report.

  **Units**   **Legal Range**                      **Default**   **Required**   **Variability**
  ----------- ------------------------------------ ------------- -------------- -----------------------
              *any numeric or string expression*                 Yes            subhour /end interval

**colHead=*string***

Text used for column head.

  **Units**   **Legal Range**   **Default**          **Required**   **Variability**
  ----------- ----------------- -------------------- -------------- -----------------
                                *colName* or blank   No             constant

**colGap=*int***

Space between (to left of) column, in character positions. Allows you to space columns unequally, to emphasize relations among columns or to improve readability. If the total of the *colGaps* and *colWids* in the report's REPORTCOLs is substantially less than the REPORT's *rpCPL* (characters per line, see REPORT), CSE will insert additional spaces between columns. To suppress these spaces, use a smaller *rpCPL* or use *rpCPL* = -1.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 0       1             No             constant

**colWid=*int***

Column width.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 0       10            No             constant

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

**endReportCol**

Optionally indicates the end of the report column definition. Alternatively, the end of the report column definition can be indicated by END or by beginning another REPORTCOL or other object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- [@reportCol](#p_reportcol)
