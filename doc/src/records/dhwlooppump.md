# DHWLOOPPUMP

DHWLOOPPUMP constructs an object representing a pump serving part a DHWLOOP. The model is identical to DHWPUMP *except* that that the electricity use calculation reflects wlRunF of the parent DHWLOOP.

**wlpName**

Optional name of pump; give after the word “DHWLOOPPUMP” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wlpMult=*integer***

Number of identical pumps of this type. Any value $>1$ is equivalent to repeated entry of the same DHWPUMP.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**wlpPwr=*float***

Pump power.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  W           $>$ 0             0             No             hourly

**wlpElecMtr=*mtrName***

Name of METER object, if any, to which DHWLOOPPUMP electrical energy use is recorded (under end use dhwMFL).

  **Units**   **Legal Range**     **Default**                 **Required**   **Variability**
  ----------- ------------------- --------------------------- -------------- -----------------
              *name of a METER*   *Parent DHWLOOP wlElecMtr*   No             constant

**endDHWLOOPPUMP**

Optionally indicates the end of the DHWPUMP definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWLoopPump](#p_dhwlooppump)
