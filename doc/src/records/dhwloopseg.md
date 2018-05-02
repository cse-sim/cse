# DHWLOOPSEG

DHWLOOPSEG constructs one or more objects representing a segment of the preceeding DHWLOOP. A DHWLOOP can have any number of DHWLOOPSEGs to represent the segments of the loop with possibly differing sizes, insulation, or surrounding conditions.

**wgName**

Optional name of segment; give after the word “DHWLOOPSEG” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wgTy=*choice***

Specifies the type of segment

  --------- ---------------------------------------
  SUPPLY    Indicates a supply segment (flow is sum
            of circulation and draw flow, child
            DHWLOOPBRANCHs permitted).

  RETURN    Indicates a return segment (flow is
            only due to circulation, child
            DHWLOOPBRANCHs not allowed)
  --------- ---------------------------------------

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  --                            --            Yes            constant

**wgLength=*float***

Length of segment.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          $\ge$ 0           0             No             constant

**wgSize=*float***

Nominal size of pipe. CSE assumes the pipe outside diameter = size + 0.125 in.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  in          $>$ 0             1             Yes            constant

**wgInsulK=*float***

Pipe insulation conductivity

  **Units**          **Legal Range**   **Default**   **Required**   **Variability**
  ------------------ ----------------- ------------- -------------- -----------------
  Btuh-ft/ft^2^-^o^F   $>$ 0             0.02167       No             constant

**wgInsulThk=*float***

Pipe insulation thickness

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  in          $\ge$ 0           1             No             constant

**wgExH=*float***

Combined radiant/convective exterior surface conductance between insulation (or pipe if no insulation) and surround.

  **Units**       **Legal Range**   **Default**   **Required**   **Variability**
  --------------- ----------------- ------------- -------------- -----------------
  Btuh/ft^2^-^o^F   $>$ 0             1.5           No             hourly

**wgExT=*float***

Surrounding equivalent temperature.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F          $>$ 0             70            No             hourly

**wgFNoDraw=*float***

Fraction of hour when no draw occurs.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F          $>$ 0             70            No             hourly

**endDHWLoopSeg**

Optionally indicates the end of the DHWLOOPSEG definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWLoopSeg](#p_dhwloopseg)
