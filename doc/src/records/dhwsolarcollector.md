# DHWSOLARCOLLECTOR

Solar Collector Array. May be multiple collectors on the same DHWSOLARSYS system. All inlets come from the DHWSOLARTANK.

Uses SRCC Ratings.

**scArea=*float***

Collector area.

<%= member_table(
  units: "ft^2",
  legal_range: "$>$ 0",
  default: "0",
  required: "Yes",
  variability: "constant") %>

**scMult**

Number of identical collectors, default 1

<%= member_table(
  units: "",
  legal_range: "$>$ 0",
  default: "1",
  required: "No",
  variability: "constant") %>

**scTilt=*float***

Array tilt.

<%= member_table(
  units: "deg",
  legal_range: "",
  default: "0",
  required: "Yes",
  variability: "constant") %>

**scAzm=*float***

Array azimuth.

<%= member_table(
  units: "deg",
  legal_range: "",
  default: "0",
  required: "Yes",
  variability: "constant") %>

**scFRUL=*float***

Fit slope

<%= member_table(
  units: "Btuh/ft^2-^o^F",
  legal_range: "",
  default: "-0.727",
  required: "No",
  variability: "constant") %>

**scFRTA=*float***

Fit y-intercept

<%= member_table(
  units: "*none*",
  legal_range: "$>$ 0",
  default: "0.758",
  required: "No",
  variability: "constant") %>

**scPumpFlow=*float***

**Units**   **Legal Range**         **Default**              **Required**   **Variability**
----------- ---------------------   -------------            -------------- -----------------
 gpm		 x $\ge$ 0               from *scArea*, *scMult*  No             constant

**scPumpPwr=*float***

**Units**   **Legal Range**         **Default**              **Required**   **Variability**
----------- ---------------------   -------------            -------------- -----------------
 Btu/h		 x $\ge$ 0               from *scPumpflow*        No             constant

**scPumpOnDeltaT=*float***

Temperature difference between the tank and collector outlet where pump turns on
  
<%= member_table(
  units: "^o^F",
  legal_range: "",
  default: "10.0",
  required: "No",
  variability: "constant") %>

**scPumpOffDeltaT=*float*** 

Temperature difference between the tank and collector outlet where pump turns off

<%= member_table(
  units: "^o^F",
  legal_range: "",
  default: "5.0",
  required: "No",
  variability: "constant") %>

**endDHWSOLARCOLLECTOR**

Optionally indicates the end of the DHWSOLARCOLLECTOR definition.

<%= member_table(
  units: "*n/a*",
  legal_range: "*n/a*",
  default: "*n/a*",
  required: "No",
  variability: "*n/a*") %>

