# SHADE

SHADE constructs a subobject associated with the current WINDOW that represents fixed shading devices (overhangs and/or fins). A window may have at most one SHADE and only windows in vertical surfaces may have SHADEs. A SHADE can describe an overhang, a left fin, and/or a right fin; absence of any of these is specified by omitting or giving 0 for its depth. SHADE geometry can vary on a monthly basis, allowing modeling of awnings or other seasonal shading strategies.

<!--
  ??Add figure showing shading geometry; describe overhangs and fins.
-->
**shName**

Name of shade; follows the word "SHADE" if given.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**ohDepth=*float***

Depth of overhang (from plane of window to outside edge of overhang). A zero value indicates no overhang.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**ohDistUp=*float***

Distance from top of window to bottom of overhang.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**ohExL=*float***

Distance from left edge of window (as viewed from the outside) to the left end of the overhang.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**ohExR=*float***

Distance from right edge of window (as viewed from the outside) to the right end of the overhang.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**ohFlap=*float***

Height of flap hanging down from outer edge of overhang.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**lfDepth=*float***

Depth of left fin from plane of window. A zero value indicates no fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**lfTopUp=*float***

Vertical distance from top of window to top of left fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**lfDistL=*float***

Distance from left edge of window to left fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**lfBotUp=*float***

Vertical distance from bottom of window to bottom of left fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**rfDepth=*float***

Depth of right fin from plane of window. A 0 value indicates no fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**rfTopUp=*float***

Vertical distance from top of window to top of right fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**rfDistR=*float***

Distance from right edge of window to right fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**rfBotUp=*float***

Vertical distance from bottom of window to bottom of right fin.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $\ge$ 0       0             No             monthly-hourly

**endShade**

Optional to indicate the end of the SHADE definition. Alternatively, the end of the shade definition can be indicated by END or the declaration of another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


