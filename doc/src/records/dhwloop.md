# DHWLOOP

DHWLOOP constructs one or more objects representing a domestic hot water circulation loop. The actual pipe runs in the DHWLOOP are specified by any number of DHWLOOPSEGs (see below). Circulation pumps are specified by DHWLOOPPUMPs (also below).

**wlName**

Optional name of loop; give after the word “DHWLOOP” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wlMult=*integer***

Number of identical loops of this type. Any value $>1$ is equivalent to repeated entry of the same DHWLOOP (and all child objects).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**wlFlow=*float***

Loop flow rate (when operating).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  gpm         $\ge$ 0           6             No             hourly

**wlTIn1=*float***

Inlet temperature of first DHWLOOPSEG.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F          $>$ 0             130           No             hourly

**wlRunF=*float***

Fraction of hour that loop circulation operates.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  --          $\ge$ 0           1             No             hourly

**wlFUA=*float***

DHWLOOPSEG pipe heat loss adjustment factor.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  --          $>$ 0             1             No             constant

**endDHWLoop**

Optionally indicates the end of the DHWLOOP definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             
