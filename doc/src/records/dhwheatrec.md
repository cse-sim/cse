# DHWHEATREC

DHWHEATREC constructs an object representing one or more heat recovery devices in a DHWSYS. Drain water heat recovered by the device increases parent DHWSYS wsInlet temperature and/or fixture cold water feed temperature.  This reduces water heating energy consumption.

**wrName**

Optional name of device; give after the word “DHWHEATREC” if desired.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**wrMult=*integer***

Number of identical heat recovery devices of this type. Any value >1 is equivalent to repeated entry of the same DHWHEATREC.

<%= member_table(
  units: "",
  legal_range: "*x* $>$ 0",
  default: "1",
  required: "No",
  variability: "constant") %>

**wrHWEndUse=*choice***

Hot water end use to which this DHWHEATREC is applied: one of Shower, Bath, CWashr, DWashr, or Faucet.  Selects DHWUSE draws for heat recovery calculations.  Currently, only Shower is supported.


<%= member_table(
  units: "",
  legal_range: "Shower",
  default: "Shower",
  required: "No",
  variability: "constant") %>

**wrCountFXDrain=*integer***

  Number of fixtures (of type wrHWEndUse) whose drain lines pass through this heat recovery device.  wrCountFXDrain=0 causes this DHWHEATREC to have no effect (that is, equivalent to omitting the DHWHEATREC command).

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "1",
  required: "No",
  variability: "constant") %>

**wrCountFXCold=*integer***

  Number of fixtures (of type wrHWEndUse) with cold water supply connected to the DHWHEATREC potable-side outlet and thus use tempered water to mix with hot water.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "1",
  required: "No",
  variability: "constant") %>

**wrFeedsWH=*choice***

Specifies whether the potable-side outlet of the DHWHEATREC is connected to the DHWHEATER(s) inlet.

<%= member_table(
  units: "",
  legal_range: "Yes, No",
  default: "No",
  required: "No",
  variability: "constant") %>

**wrType=*choice***

Specifies the type of heat recovery device: Vertical, Horizontal, or SetEF.  Horizontal and Vertical derive effectiveness from wrCSARatedEF, flow rates, and water temperatures.  As of Feb. 2019, the same correlation is used for both Horizontal and Vertical, so these choices have no effect on results.  Choice SetEF uses wrCSARatedEF without modification as the effectiveness (note hourly variability).

<%= member_table(
  units: "",
  legal_range: "Vertical, Horizontal, SetE0",
  default: "Vertical",
  required: "No",
  variability: "constant") %>

  **wrCSARatedEF=*float***

Specifies the heat recovery effectiveness, generally determined using CSA B55.2 rating conditions.  This value is modified during simulation based on water flow rates and temperatures.  If wrType=SetEF, wsCSARatedEF is used without modification.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ *x* $\\le$ 1",
  default: "*none*",
  required: "Yes",
  variability: "hourly") %>

**wrTDInDiff=*float***

Temperature drop between the fixture drain and DHWHEATREC drain-side inlet.  The drain-side inlet temperature is thus DHWUSE wuTemp - wrTDInDiff.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $\\ge$ 0",
  default: "4.6 ^o^F",
  required: "No",
  variability: "hourly") %>

**wrTDInWarmup=*float***

Drain-side inlet water temperature during warmup.  During the warmup portion of a draw (if any), the drain-side inlet temperature will initially be lower than that based on DHWUSE wuTemp.  wrTDInWarmup allows input of user estimates for this temperature.  Note wrTDInWarmup is *not* adjusted by wrTDInDiff.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $>$ 0",
  default: "65 ^o^F",
  required: "No",
  variability: "hourly") %>

**endDHWHEATREC**

Optionally indicates the end of the DHWHEATREC definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "") %>

**Related Probes:**
