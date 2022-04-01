# DHWTANK

DHWTANK constructs an object representing one or more unfired water storage tanks in a DHWSYS. DHWTANK heat losses contribute to the water heating load.

**wtName**

Optional name of tank; give after the word “DHWTANK” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wtMult=*integer***

Number of identical tanks of this type. Any value $>1$ is equivalent to repeated entry of the same DHWTANK.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

Tank heat loss is calculated hourly (note that default heat loss is 0) --

$$\text{qLoss} = \text{wtMult} \cdot (\text{wtUA} \cdot (\text{wtTTank} - \text{wtTEx}) + \text{wtXLoss})$$

**wtUA=*float***

Tank heat loss coefficient.

  **Units**   **Legal Range**   **Default**                       **Required**   **Variability**
  ----------- ----------------- --------------------------------- -------------- -----------------
  Btuh/^o^F   $\ge$ 0           Derived from wtVol and wtInsulR   No             constant

**wtVol=*float***

Specifies tank volume.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  gal         $\ge$ 0           0             No             constant

**wtInsulR=*float***

Specifies total tank insulation resistance. The input value should represent the total resistance from the water to the surroundings, including both built-in insulation and additional exterior wrap insulation.

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
  ft^2^-^o^F/Btuh   $\ge$ .01         0             No             constant

**wtZone=*znName***

Zone location of DHWTANK regarding tank loss. The value of zero only valid if wtTEx is being used. Half of the heat losses go to zone air and the other goes to half radiant.

<%= member_table(
  units: "",
  legal_range: "*Name of ZONE*",
  default: "0",
  required: "No",
  variability: "constant") %>

**wtTEx=*float***

Tank surround temperature.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        $\ge$ 0           70            No             hourly

**wtTTank=*float***

Tank average water temperature.

  **Units**   **Legal Range**   **Default**               **Required**   **Variability**
  ----------- ----------------- ------------------------- -------------- -----------------
  ^o^F        $>$ 32 ^o^F       Parent DHWSYSTEM wsTUse   No             hourly

**wtXLoss=*float***

Additional tank heat loss. To duplicate CEC 2016 procedures, this value should be used to specify the fitting loss of 61.4 Btuh.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        (any)             0             No             hourly

**endDHWTank**

Optionally indicates the end of the DHWTANK definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWTank](#p_dhwtank)
