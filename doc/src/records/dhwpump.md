# DHWPUMP

DHWPUMP constructs an object representing a domestic hot water circulation pump (or more than one if identical).

**wpName**

Optional name of pump; give after the word “DHWPUMP” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wpMult=*integer***

Number of identical pumps of this type. Any value $>1$ is equivalent to repeated entry of the same DHWPUMP.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

**wpPwr=*float***

Pump power.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  W           $>$ 0             0             No             hourly

**wpElecMtr=*mtrName***

Name of METER object, if any, to which DHWPUMP electrical energy use is recorded (under end use DHW).

  **Units**   **Legal Range**     **Default**                 **Required**   **Variability**
  ----------- ------------------- --------------------------- -------------- -----------------
              *name of a METER*   *Parent DHWSYS wsElecMtr*   No             constant

**endDHWPump**

Optionally indicates the end of the DHWPUMP definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWPump](#p_dhwpump)
