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

**scTestMassFlow=*flaot***

Mass flow rate for collector loop SRCC rating.

<%= member_table(
  units: "lb/h-ft^2^",
  legal_range: "x $>$ 0",
  default: "14.79",
  required: "No",
  variability: "constant") %>

**scKta60=*float***

Incident angle modifier at 60 degree, from SRCC rating.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "0.72",
  required: "No",
  variability: "constant") %>

**scOprMassFlow=*float***

Collector loop operating mass flow rate

<%= member_table(
  units: "lb/h-ft^2^",
  legal_range: "x $>$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**scPumpFlow=*float***

**Units**   **Legal Range**         **Default**              **Required**   **Variability**
----------- ---------------------   -------------            -------------- -----------------
 gpm		 x $\ge$ 0               from *scArea*, *scMult*  No             constant

**scPipingLength=*float***

Information about the collector piping lenght.

<%= member_table(
  units: "ft",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "Hourly and at the end of interval") %>

**scPipingInsulK=*float***

Information about the collector piping insulation conductivity.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "0.02167",
  required: "No",
  variability: "Hourly and at the end of interval") %>

**scPipingInsulThk=*float***

Information about the collector piping insulation thickness.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "1.0",
  required: "No",
  variability: "Hourly and at the end of interval") %>

**scPipingExH=*float***

Information about the collector piping heat transfer coefficient.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.5",
  required: "No",
  variability: "Hourly and at the end of interval") %>

**scPipingExT=*float***

Collector piping suround temperature.

<%= member_table(
  units: "^o^F",
  legal_range: "x $\\geq$ 32",
  default: "70.0",
  required: "No",
  variability: "hourly") %>

**scPumpPwr=*float***

**Units**   **Legal Range**         **Default**              **Required**   **Variability**
----------- ---------------------   -------------            -------------- -----------------
 Btu/h		 x $\ge$ 0               from *scPumpflow*        No             constant

**scPumpLiqHeatF=*float***

Fraction of scPumpPwr added to liquid stream, the remainder is discarded.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "1.0",
  required: "No",
  variability: "Every run") %>

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

