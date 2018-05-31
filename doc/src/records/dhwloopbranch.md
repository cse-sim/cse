# DHWLOOPBRANCH

DHWLOOPBRANCH constructs one or more objects representing a branch pipe from the preceeding DHWLOOPSEG. A DHWLOOPSEG can have any number of DHWLOOPBRANCHs to represent pipe runs with differing sizes, insulation, or surrounding conditions.

wbNameOptional name of segment; give after the word “DHWLOOPBRANCH” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wbMult=*float***

Specifies the number of identical DHWLOOPBRANCHs. Note may be non-integer.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  --          $>$ 0             1             No             constant

**wbLength=*float***

Length of branch.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          $\ge$ 0           0             No             constant

**wbSize=*float***

Nominal size of pipe. CSE assumes the pipe outside diameter = size + 0.125 in.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  in          $>$ 0             --            Yes            constant

**wbInsulK=*float***

Pipe insulation conductivity

  **Units**          **Legal Range**   **Default**   **Required**   **Variability**
  ------------------ ----------------- ------------- -------------- -----------------
  Btuh-ft/ft^2^-^o^F   $>$ 0             0.02167       No             constant

**wbInsulThk=*float***

Pipe insulation thickness

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  in          $\ge$ 0           1             No             constant

**wbExH=*float***

Combined radiant/convective exterior surface conductance between insulation (or pipe if no insulation) and surround.

  **Units**       **Legal Range**   **Default**   **Required**   **Variability**
  --------------- ----------------- ------------- -------------- -----------------
  Btuh/ft^2^-^o^F   $>$ 0             1.5           No             hourly

**wbExT=*float***

Surrounding equivalent temperature.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F          $>$ 0             70            No             hourly

**wbFlow=*float***

Branch flow rate assumed during draw.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  gpm         $\ge$ 0           2             No             hourly

**wbFWaste=*float***

Number of times during the hour when the branch volume is discarded.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $\ge$ 0           0             No             hourly

**endDHWLOOPBRANCH**

Optionally indicates the end of the DHWLOOPBRANCH definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWLoopBranch](#p_dhwloopbranch)
