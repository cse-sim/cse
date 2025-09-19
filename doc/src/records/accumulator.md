# ACCUMULATOR

An ACCUMULATOR is driven by arbitrary subhourly expression value computed during the CSE simulation and calculates minimum, maximum, mean, and sum values for hour, day, month, and year intervals.  In addition, timestamps are retained for the minimum and maximum values. ACCUMULATORs are useful for summarizing and reporting values for which there is not built-in accounting (for example, ZNRES or RSYSRES).  One common use case is for reporting hour average values of internal variables that vary subhourly.

ACCUMULATORs are "observing" devices -- they have no effect on the CSE building model or calculations.  ACCUMULATOR values must be reported using user-defined REPORTs or EXPORTs.

As a simple example, a report of monthly outdoor drybulb temperatures can be generated as follows --

    ACCUMULATOR "ACTDB" acmValue=$tdbosh

    REPORT "TODB" rpType=UDT rpFreq=Month rpTitle="Outdoor drybulb temp (F)"
      REPORTCOL colHead="Mon" colVal=$Month colWid=3
      REPORTCOL colHead="Min" colVal=@Accumulator[ "ACTDB"].M.acmMin colDec=2 colWid=6
      REPORTCOL colHead="Mean" colVal=@Accumulator[ "ACTDB"].M.acmMean colDec=2 colWid=6
      REPORTCOL colHead="Max" colVal=@Accumulator[ "ACTDB"].M.acmMax colDec=2 colWid=6
    REPORT rpType=UDT rpFreq=Year rpHeader=No
      REPORTCOL colVal="Yr" colWid=3
      REPORTCOL colVal=@Accumulator[ "ACTDB"].Y.acmMin colDec=2 colWid=6
      REPORTCOL colVal=@Accumulator[ "ACTDB"].Y.acmMean colDec=2 colWid=6
      REPORTCOL colVal=@Accumulator[ "ACTDB"].Y.acmMax colDec=2 colWid=6

Resulting output --

      Outdoor drybulb temp (F)

      Mon    Min   Mean    Max
      --- ------ ------ ------
        1  28.22  46.87  60.08
        2  32.00  49.26  69.98
        3  42.08  60.79  91.40
        4  34.70  58.13  79.16
        5  44.42  67.07  97.88
        6  50.36  73.43 107.96
        7  55.04  74.02 101.48
        8  53.60  74.99 104.18
        9  48.92  69.24  97.52
       10  47.84  64.00  98.42
       11  32.54  54.47  77.00
       12  28.04  46.58  64.04

       Yr  28.04  61.65 107.96

Generalizing what is illustrated, probing @accumulator[ ].H yields statistics for the current hour, .D for the current day, .M the current month, and .Y for the year (or, more precisely, the full run, which may or may not be a full year).

A complete list of the available statistics for each interval is found in the ACCUMULATOR probe documentation.

Note: The initial version of ACCUMULATOR contains unresolved bugs related to the timing of the determination of acmValue.  In some cases, acmValue is set to the expression value from the prior substep.  This is being investigated.


**acmName**

Name of ACCUMULATOR: required for referencing in reports.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**acmValue=*float***

The value being accumulated.  Generally expression with subhourly variability.

<%= member_table(
  units: "any",
  legal_range: "",
  default: "",
  required: "Yes",
  variability: "subhourly") %>


**endACCUMULATOR**

Indicates the end of the ACCUMULATOR definition. Alternatively, the end of the definition can be indicated by the declaration of another object or by END.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[accumulator](#p_accumulator)