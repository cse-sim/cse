# CHILLER

CHILLERs are subobjects of COOLPLANTs (Section 5.21). CHILLERs supply coldness, in the form of chilled water, via their COOLPLANT, to CHW (CHilled Water) cooling coils in AIRHANDLERs. CHILLERs exhaust heat through the cooling towers in their COOLPLANT's TOWERPLANT. Each COOLPLANT can contain multiple CHILLERs; chiller operation is controlled by the scheduling and staging logic of the COOLPLANT, as described in the previous section.

Each chiller has primary and secondary pumps that operate when the chiller is on. The pumps add heat to the primary and secondary loop water respectively; this heat is considered in the modeling of the loop's water temperature.

**chillerName**

Name of CHILLER object, given immediately after the word CHILLER. This name is used to refer to the chiller in *cpStage* commands.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

The next four inputs allow specification of the CHILLER's capacity (amount of heat it can remove from the primary loop water) and how this capacity varies with the supply (leaving) temperature of the primary loop water and the entering temperature of the condenser (secondary loop) water. The chiller capacity at any supply and condenser temperatures is *chCapDs* times the value of *chPyCapT* at those temperatures.

**chCapDs=*float***

Chiller design capacity, that is, the capacity at *chTsDs* and *chTcndDs* (next).

<%= member_table(
  units: "Btuh",
  legal_range: "*x* $\\neq$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant") %>

**chTsDs=*float***

Design supply temperature: temperature of primary water leaving chiller at which capacity is *chCapDs*.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $\\gt$ 0",
  default: "44",
  required: "No",
  variability: "constant") %>

**chTcndDs=*float***

Design condenser temperature: temperature of secondary water entering chiller condenser at which capacity is *chCapDs*.

<%= member_table(
  units: "^o^F",
  legal_range: "*x* $\\gt$ 0",
  default: "85",
  required: "No",
  variability: "constant") %>

**chPyCapT=*a, b, c, d, e, f***

Coefficients of bi-quadratic polynomial function of supply (ts) and condenser (tcnd) temperatures that specifies how capacity varies with these temperatures. This polynomial is of the form

$$a + b \cdot ts + c \cdot ts^2 + d \cdot tcnd + e \cdot tcnd^2 + f \cdot ts \cdot tcnd$$

Up to six *float* values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when ts is chTsDs and tcnd is chTcndDs, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

<%= member_table(
  units: "",
  legal_range: "",
  default: "-1.742040, .029292, .000067, .048054, .000291, -.000106",
  required: "No",
  variability: "constant") %>

The next three inputs allow specification of the CHILLER's full-load energy input and how it varies with supply and condenser temperature. Only one of *chCop* and *chEirDs* should be given. The full-load energy input at any supply and condenser temperatures is the chiller's capacity at these temperatures, times *chEirDs* (or 1/*chCop*), times the value of *chPyEirT* at these temperatures.

**chCop=*float***

Chiller full-load COP (Coefficient Of Performance) at *chTsDs*and *chTcndDs*. This is the output energy divided by the electrical input energy (in the same units) and reflects both motor and compressor efficiency.

<%= member_table(
  units: "",
  legal_range: "*x* $\\gt$ 0",
  default: "4.2",
  required: "No",
  variability: "constant") %>

**chEirDs=*float***

Alternate input for COP: Full-load Energy Input Ratio (electrical input energy divided by output energy) at design temperatures; the reciprocal of *chCop*.

<%= member_table(
  units: "",
  legal_range: "*x* $\\gt$ 0",
  default: "*chCop* is defaulted",
  required: "No",
  variability: "constant") %>

**chPyEirT=*a, b, c, d, e, f***

Coefficients of bi-quadratic polynomial function of supply (ts) and condenser (tcnd) temperatures that specifies how energy input varies with these temperatures. This polynomial is of the form

$$a + b \cdot ts + c \cdot ts^2 + d \cdot tcnd + e \cdot tcnd^2 + f \cdot ts \cdot tcnd$$

Up to six *float* values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when ts is chTsDs and tcnd is chTcndDs, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

<%= member_table(
  units: "",
  legal_range: "",
  default: "3.117600, -.109236, .001389, .003750, .000150, -.000375",
  required: "No",
  variability: "constant") %>

The next three inputs permit specification of the CHILLER's part load energy input. In the following the part load ratio (plr) is defined as the actual load divided by the capacity at the current supply and condenser temperatures. The energy input is defined as follows for four different plr ranges:

<%= csv_table(<<END, :row_header => false)
full, loadplr (part load ratio) = 1.0
 , Power input is full-load input&comma; as described above.
compressor unloading region, 1.0 &gt; plr $\\ge$ *chMinUnldPlr*
 , Power input is the full-load input times the value of the *chPyEirUl* polynomial for the current plr&comma; that is&comma; *chPyEirUl*(plr).
false loading region, *chMinUnldPlr* &gt; plr &gt; *chMinFsldPlr*
 , Power input in this region is constant at the value for the low end of the compressor unloading region&comma; i.e. *chPyEirUl*(*chMinUnldPlr*).
cycling region,  *chMinFsldPlr* &gt; plr $\\ge$ 0
 , In this region the chiller runs at the low end of the false loading region for the necessary fraction of the time&comma; and the power input is the false loading value correspondingly prorated&comma; i.e. *chPyEirUl*(*chMinUnldPlr*) plr / *chMinFsldPlr*.
END
%>

These plr regions are similar to those for a DX coil & compressor in an AIRHANDLER, Section 0.

**chPyEirUl=*a, b, c, d***

Coefficients of cubic polynomial function of part load ratio (plr) that specifies how energy input varies with plr in the compressor unloading region (see above). This polynomial is of the form

$$a + b \cdot plr + c \cdot plr^2 + d \cdot plr^3$$

Up to four *float* values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when plr is 1.0, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

<%= member_table(
  units: "",
  legal_range: "",
  default: ".222903, .313387, .463710, 0.",
  required: "No",
  variability: "constant") %>

**chMinUnldPlr=*float***

Minimum compressor unloading part load ratio (plr); maximum false loading plr. See description above.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ *x* $\\le$ 1",
  default: "0.1",
  required: "No",
  variability: "constant") %>

**chMinFsldPlr=*float***

Minimum compressor false loading part load ratio (plr); maximum cycling plr. See description above.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ *x* $\\le$ *chMinFsldPlr*",
  default: "0.1",
  required: "No",
  variability: "constant") %>

**chMotEff=*float***

Fraction of CHILLER compressor motor input power which goes to the condenser. For an open-frame motor and compressor, where the motor's waste heat goes to the air, enter the motor's efficiency: a fraction around .8 or .9. For a hermetic compressor, where the motor's waste heat goes to the refrigerant and thence to the condenser, use 1.0.

<%= member_table(
  units: "",
  legal_range: "0 &lt; *x* $\\le$ 1",
  default: "1.0",
  required: "No",
  variability: "constant") %>

**chMtr=*name***

Name of METER to which to accumulate CHILLER's electrical input energy. Category "Clg" is used. Note that two additional commands, *chppMtr* and *chcpMtr*, are used to specify meters for recording chiller pump input energy.

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*not recorded*",
  required: "No",
  variability: "constant") %>

The next six inputs specify this CHILLER's *PRIMARY PUMP*, which pumps chilled water from the chiller through the CHW coils connected to the chiller's COOLPLANT.

**chppGpm=*float***

Chiller primary pump flow in gallons per minute: amount of water pumped from this chiller through the primary loop supplying the COOLPLANT's loads (CHW coils) whenever chiller is operating. Any excess flow over that demanded by coils is assumed to go through a bypass valve. If coil flows exceed *chppGpm*, CSE assumes the pressure drops and the pump "overruns" to deliver the extra flow with the same energy input. The default is one gallon per minute for each 5000 Btuh of chiller design capacity.

<%= member_table(
  units: "gpm",
  legal_range: "*x* $\\gt$ 0",
  default: "*chCapDs* / 5000",
  required: "No",
  variability: "constant") %>

**chppHdloss=*float***

Chiller primary pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

<%= member_table(
  units: "ft H2O",
  legal_range: "*x* $\\ge$ 0",
  default: "57.22\*",
  required: "No",
  variability: "constant") %>

\* May be temporary default for 10-31-92 version; prior value (65) may be restored.

**chppMotEff=*float***

Chiller primary pump motor efficiency.

<%= member_table(
  units: "",
  legal_range: "0 &lt; *x* $\\le$ 1.0",
  default: ".88",
  required: "No",
  variability: "constant") %>

**chppHydEff=*float***

Chiller primary pump hydraulic efficiency

<%= member_table(
  units: "",
  legal_range: "0 &lt; *x* $\\le$ 1.0",
  default: "0.7",
  required: "No",
  variability: "constant") %>

**chppOvrunF=*float***

Chiller primary pump maximum overrun: factor by which flow demanded by coils can exceed *chppGpm*. The primary flow is not simulated in detail; *chppOvrun* is currently used only to issue an error message if the sum of the design flows of the coils connected to a COOLPLANT exceeds the sum of the products of *chppGpm* and *chppOvrun* for the chiller's in the plants most powerful stage.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 1.0",
  default: "1.3",
  required: "No",
  variability: "constant") %>

**chppMtr=*name of a METER***

Meter to which primary pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

The next five inputs specify this CHILLER's *CONDENSER PUMP*, also known as the *SECONDARY PUMP* or the *HEAT REJECTION PUMP*. This pump pumps water from the chiller's condenser through the cooling towers in the COOLPLANT's TOWERPLANT.

**chcpGpm=*float***

Chiller condenser pump flow in gallons per minute: amount of water pumped from this chiller through the cooling towers when chiller is operating.

<%= member_table(
  units: "gpm",
  legal_range: "*x* $\\gt$ 0",
  default: "*chCapDs* / 4000",
  required: "No",
  variability: "constant") %>

**chcpHdloss=*float***

Chiller condenser pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

<%= member_table(
  units: "ft H2O",
  legal_range: "*x* $\\ge$ 0",
  default: "45.78\*",
  required: "No",
  variability: "constant") %>

\* May be temporary default for 10-31-92 version; prior value (45) may be restored.

**chcpMotEff=*float***

Chiller condenser pump motor efficiency.

<%= member_table(
  units: "",
  legal_range: "0 &lt; *x* $\\le$ 1.0",
  default: ".88",
  required: "No",
  variability: "constant") %>

**chcpHydEff=*float***

Chiller condenser pump hydraulic efficiency

<%= member_table(
  units: "",
  legal_range: "0 &lt; *x* $\\le$ 1.0",
  default: "0.7",
  required: "No",
  variability: "constant") %>

**chcpMtr=*name of a METER***

Meter to which condenser pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

The following four members permit specification of auxiliary input power use associated with the chiller under the conditions indicated.

**chAuxOn=*float***

Auxiliary power used when chiller is running, in proportion to its subhour average part load ratio (plr).

**chAuxOff=*float***

Auxiliary power used when chiller is not running, in proportion to 1 - plr.

**chAuxFullOff=*float***

Auxiliary power used only when chiller is off for entire subhour; not used if the chiller is on at all during the subhour.

**chAuxOnAtAll=*float***

Auxiliary power used in full value if chiller is on for any fraction of subhour.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>

The following four allow specification of meters to record chiller auxiliary energy use through chAuxOn, chAuxOff, chFullOff, and chAuxOnAtAll, respectively. End use category "Aux" is used.

**chAuxOnMtr=*mtrName***

**chAuxOffMtr=*mtrName***

**chAuxFullOffMtr=*mtrName***

**chAuxOnAtAllMtr=*mtrName***

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**endChiller**

Optionally indicates the end of the CHILLER definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>

**Related Probes:**

- @[chiller](#p_chiller)
