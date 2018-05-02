# HOLIDAY

HOLIDAY objects define holidays. Holidays have no inherent effect, but input expressions can test for holidays via the \$DOWH, \$isHoliday, \$isHoliTrue, \$isWeHol, and \$isBegWeek system variables (4.6.4).

Examples and the list of default holidays are given after the member descriptions.

<!--
hdName is required in the program. WHY? 7-92.
-->
**hdName**

Name of holiday: <!-- if given,--> must follow the word HOLIDAY. <!-- Necessary only if the HOLIDAY object is referenced later with another statement, for example in a LIKE clause or with ALTER; however, we suggest always naming all objects for clearer error messages and future flexibility. -->

  **Units**   **Legal Range**   **Default**   **Required**    **Variability**
  ----------- ----------------- ------------- --------------- -----------------
              *63 characters*   *none*        Yes <!--No-->   constant

A holiday may be specified by date or via a rule such as "Fourth Thursday in November". To specify by date, give hdDateTrue, and also hdDateObs or hdOnMonday if desired. To specify by rule, give all three of hdCase, hdMon, and hdDow.

**hdDateTrue*=date***

The true date of a holiday, even if not celebrated on that day.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *date*            *blank*       No             constant

**hdDateObs*=date***

The date that a holiday will be observed. Allowed only if hdDateTrue given and hdOnMonday not given.

  **Units**   **Legal Range**   **Default**    **Required**   **Variability**
  ----------- ----------------- -------------- -------------- -----------------
              *date*            *hdDateTrue*   No             constant

**hdOnMonday=*choice***

If YES, holiday is observed on the following Monday if the true date falls on a weekend. Allowed only if hdDateTrue given and hdDateObs not given.

Note: there is no provision to celebrate a holiday that falls on a Saturday on *Friday* (as July 4 was celebrated in 1992).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              YES NO            YES           No             constant

**hdCase=*choice***

Week of the month that the holiday is observed. hdCase, hdMon, and hdDow may be given only if hdDateTrue, hdDateObs, and hdOnMonday are not given.

  **Units**   **Legal Range**                  **Default**   **Required**   **Variability**
  ----------- -------------------------------- ------------- -------------- -----------------
              FIRST SECOND THIRD FOURTH LAST   FIRST         No             constant

**hdMon=*choice***

Month that the holiday is observed.

  -----------------------------------------------------------------
  **Units** **Legal**      **Default** **Required** **Variability**
            **Range**
  --------- -------------- ----------- ------------ ---------------
            JAN, FEB, MAR, *none*      required if  constant
            APR, MAY, JUN,             hdCase given   
            JUL, AUG, SEP,
            OCT, NOV, DEC

  -----------------------------------------------------------------

**hdDow*=choice***

Day of the week that the holiday is observed.

  -------------------------------------------------------------
  **Units** **Legal**  **Default** **Required** **Variability**
            **Range**
  --------- ---------- ----------- ------------ ---------------
            SUNDAY,    MONDAY      required if  constant
            MONDAY,                hdCase given   
            TUESDAY,
            WEDNESDAY,
            THURSDAY,
            FRIDAY,
            SATURDAY                                                 

  -------------------------------------------------------------

**endHoliday**

Indicates the end of the holiday definition. Alternatively, the end of the holiday definition can be indicated by "END" or simply by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

Examples of valid HOLIDAY object specifications:

-   Holiday on May first, observed date moved to following Monday if the first falls on a weekend (hdOnMonday defaults Yes).

<!-- -->
        HOLIDAY MAYDAY;
            hdDateTrue = May 1;

-   Holiday on May 1, observed on May 3.

<!-- -->
        HOLIDAY MAYDAY;
            hdDateTrue = May 1;
            hdDateObs  = May 3;

-   Holiday observed on May 1 even if on a weekend.

<!-- -->
        HOLIDAY MAYDAY;
            hdDateTrue = May 1;
            hdOnMonday = No;

-   Holiday observed on Wednesday of third week of March

<!-- -->
        HOLIDAY HYPOTHET;
            hdCase = third;
            hdDow  = Wed;
            hdMon  = MAR

As with reports, Holidays are automatically generated for a standard set of Holidays. The following are the default holidays automatically defined by CSE:

  ----------------- --------------------------
  New Year's Day    \*January 1
  M L King Day      \*January 15
  President's Day   3rd Monday in February
  Memorial Day      last Monday in May
  Fourth of July    \*July 4
  Labor Day         1st Monday in September
  Columbus Day      2nd Monday in October
  Veterans Day      \*November 11
  Thanksgiving      4th Thursday in November
  Christmas         \*December 25
  ----------------- --------------------------

\* *observed on the following Monday if falls on a weekend, except as otherwise noted:*

If a particular default holiday is not desired, it can be removed with a DELETE statement:

        DELETE HOLIDAY Thanksgiving

        DELETE HOLIDAY "Columbus Day"  // Quotes necessary (due to space)

        DELETE HOLIDAY "VETERANS DAY"  // No case-sensitivity

Note that the name must be spelled *exactly* as listed above.

**Related Probes:**

- [@holiday](#p_holiday)
