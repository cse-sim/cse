# DHWSOLARSYS

Solar water heating system.

- DHWSOLARSYS
    - DHWSOLARCOLLECTOR
    - DHWSOLARTANK

May have any number of solar collectors, but only one tank.

May have no tank for direct system? What if system has multiple primary tanks?

**swElecMtr=*mtrName***

Name of METER object, if any, to which DHWSOLARSYS electrical energy use is recorded (under end use ???).

<%= member_table(
  units: "F",
  legal_range: "*name of a METER*",
  default: "*not recorded*",
  required: "No",
  variability: "constant") %>

**swEndUse**

End use of pump energy; defaults to "DHW".
  
**swParElec=*float***

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
			 x $\ge$ 0         0             No             hourly

**swFluidVolSpHt=*float***
 
Default specific heat for Ethylene Glycol.

<%= member_table(
  units: "Btu/gal-^o^F",
  legal_range: "",
  default: "5.31",
  required: "No",
  variability: "constant") %>

**swTankHXEff=*float***

Tank heat exchanger effectiveness.

**Units**   **Legal Range**         **Default**   **Required**   **Variability**
----------- ---------------------   ------------- -------------- -----------------
			 0 $\le$ x $\le$ 0.99    0             No             hourly

**swTankUA=*float***

Heat transfer coefficient for the tank multiplied by area.
  
<%= member_table(
  units: "Btuh/^o^F",
  legal_range: "",
  default: "",
  required: "No",
  variability: "constant") %>

**swTankVol=*float***

<%= member_table(
  units: "gal",
  legal_range: "",
  default: "",
  required: "No",
  variability: "constant") %>

**swTankInsulR=*float***

Total tank insulation resistance, built-in plus exterior wrap.
  
<%= member_table(
  units: "ft^2^-^o^F/Btuh",
  legal_range: "",
  default: "",
  required: "No",
  variability: "constant") %>

**swTankZn**

Pointer to tank zone location, use sw_tankTEx if NULL

<%= member_table(
  units: "",
  legal_range: "",
  default: "",
  required: "No",
  variability: "constant") %>

**swTankTEx=*float***

Surrounding temperature.

<%= member_table(
  units: "^o^F",
  legal_range: "",
  default: "",
  required: "No",
  variability: "hourly") %>

**endDHWSOLARSYS**

Optionally indicates the end of the DHWSOLARSYS definition.

<%= member_table(
  units: "*n/a*",
  legal_range: "*n/a*",
  default: "*n/a*",
  required: "No",
  variability: "*n/a*") %>

