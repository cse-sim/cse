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
  units: "^o^F",
  legal_range: "*name of a METER*",
  default: "*not recorded*",
  required: "No",
  variability: "constant") %>

**swSCFluidSpHt=*float***

Specify the specific heat for the collector fluid.

<%= member_table(
  units: "Btu/lbm-^o^F",
  legal_range: "x $>$ 0",
  default: "0.9",
  required: "No",
  variability: "constant") %>

**swSCFluidDens=*float***

Specify the density for the collector fluid.

<%= member_table(
  units: "lb/ft^3^",
  legal_range: "x $>$ 0",
  default: "64.0",
  required: "No",
  variability: "constant") %>

**swEndUse**

End use of pump energy; defaults to "DHW".
  
**swParElec=*float***

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**swTankHXEff=*float***

Tank heat exchanger effectiveness.

<%= member_table(
  units: "",
  legal_range: "0 $<$ x $<$ 0.99",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**swTankTHxLimit=*float***

Temperature limit for the tank collector.

<%= member_table(
  units: "^o^F",
  legal_range: "x $\\geq$ 0",
  default: "180.0",
  required: "No",
  variability: "constant") %>

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

**swTankZone=*integer***

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
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "") %>

