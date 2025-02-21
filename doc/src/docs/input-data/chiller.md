# CHILLER

CHILLERs are subobjects of COOLPLANTs (Section 5.21). CHILLERs supply coldness, in the form of chilled water, via their COOLPLANT, to CHW (CHilled Water) cooling coils in AIRHANDLERs. CHILLERs exhaust heat through the cooling towers in their COOLPLANT's TOWERPLANT. Each COOLPLANT can contain multiple CHILLERs; chiller operation is controlled by the scheduling and staging logic of the COOLPLANT, as described in the previous section.

Each chiller has primary and secondary pumps that operate when the chiller is on. The pumps add heat to the primary and secondary loop water respectively; this heat is considered in the modeling of the loop's water temperature.

**chillerName**

Name of CHILLER object, given immediately after the word CHILLER. This name is used to refer to the chiller in _cpStage_ commands.

<%= member_table(
units: "",
legal_range: "_63 characters_",
default: "_none_",
required: "Yes",
variability: "constant") %>

The next four inputs allow specification of the CHILLER's capacity (amount of heat it can remove from the primary loop water) and how this capacity varies with the supply (leaving) temperature of the primary loop water and the entering temperature of the condenser (secondary loop) water. The chiller capacity at any supply and condenser temperatures is _chCapDs_ times the value of _chPyCapT_ at those temperatures.

**chCapDs=_float_**

Chiller design capacity, that is, the capacity at _chTsDs_ and _chTcndDs_ (next).

<%= member_table(
units: "Btuh",
legal_range: "_x_ $\\neq$ 0",
default: "_none_",
required: "Yes",
variability: "constant") %>

**chTsDs=_float_**

Design supply temperature: temperature of primary water leaving chiller at which capacity is _chCapDs_.

<%= member_table(
units: "^o^F",
legal_range: "_x_ $\\gt$ 0",
default: "44",
required: "No",
variability: "constant") %>

**chTcndDs=_float_**

Design condenser temperature: temperature of secondary water entering chiller condenser at which capacity is _chCapDs_.

<%= member_table(
units: "^o^F",
legal_range: "_x_ $\\gt$ 0",
default: "85",
required: "No",
variability: "constant") %>

**chPyCapT=_a, b, c, d, e, f_**

Coefficients of bi-quadratic polynomial function of supply (ts) and condenser (tcnd) temperatures that specifies how capacity varies with these temperatures. This polynomial is of the form

$$a + b \cdot ts + c \cdot ts^2 + d \cdot tcnd + e \cdot tcnd^2 + f \cdot ts \cdot tcnd$$

Up to six _float_ values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when ts is chTsDs and tcnd is chTcndDs, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

<%= member_table(
units: "",
legal_range: "",
default: "-1.742040, .029292, .000067, .048054, .000291, -.000106",
required: "No",
variability: "constant") %>

The next three inputs allow specification of the CHILLER's full-load energy input and how it varies with supply and condenser temperature. Only one of _chCop_ and _chEirDs_ should be given. The full-load energy input at any supply and condenser temperatures is the chiller's capacity at these temperatures, times _chEirDs_ (or 1/_chCop_), times the value of _chPyEirT_ at these temperatures.

**chCop=_float_**

Chiller full-load COP (Coefficient Of Performance) at *chTsDs*and _chTcndDs_. This is the output energy divided by the electrical input energy (in the same units) and reflects both motor and compressor efficiency.

<%= member_table(
units: "",
legal_range: "_x_ $\\gt$ 0",
default: "4.2",
required: "No",
variability: "constant") %>

**chEirDs=_float_**

Alternate input for COP: Full-load Energy Input Ratio (electrical input energy divided by output energy) at design temperatures; the reciprocal of _chCop_.

<%= member_table(
units: "",
legal_range: "_x_ $\\gt$ 0",
default: "_chCop_ is defaulted",
required: "No",
variability: "constant") %>

**chPyEirT=_a, b, c, d, e, f_**

Coefficients of bi-quadratic polynomial function of supply (ts) and condenser (tcnd) temperatures that specifies how energy input varies with these temperatures. This polynomial is of the form

$$a + b \cdot ts + c \cdot ts^2 + d \cdot tcnd + e \cdot tcnd^2 + f \cdot ts \cdot tcnd$$

Up to six _float_ values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when ts is chTsDs and tcnd is chTcndDs, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

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
compressor unloading region, 1.0 &gt; plr $\\ge$ _chMinUnldPlr_
, Power input is the full-load input times the value of the _chPyEirUl_ polynomial for the current plr&comma; that is&comma; _chPyEirUl_(plr).
false loading region, _chMinUnldPlr_ &gt; plr &gt; _chMinFsldPlr_
, Power input in this region is constant at the value for the low end of the compressor unloading region&comma; i.e. _chPyEirUl_(_chMinUnldPlr_).
cycling region, _chMinFsldPlr_ &gt; plr $\\ge$ 0
, In this region the chiller runs at the low end of the false loading region for the necessary fraction of the time&comma; and the power input is the false loading value correspondingly prorated&comma; i.e. _chPyEirUl_(_chMinUnldPlr_) plr / _chMinFsldPlr_.
END
%>

These plr regions are similar to those for a DX coil & compressor in an AIRHANDLER, Section 0.

**chPyEirUl=_a, b, c, d_**

Coefficients of cubic polynomial function of part load ratio (plr) that specifies how energy input varies with plr in the compressor unloading region (see above). This polynomial is of the form

$$a + b \cdot plr + c \cdot plr^2 + d \cdot plr^3$$

Up to four _float_ values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when plr is 1.0, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

<%= member_table(
units: "",
legal_range: "",
default: ".222903, .313387, .463710, 0.",
required: "No",
variability: "constant") %>

**chMinUnldPlr=_float_**

Minimum compressor unloading part load ratio (plr); maximum false loading plr. See description above.

<%= member_table(
units: "",
legal_range: "0 $\\le$ _x_ $\\le$ 1",
default: "0.1",
required: "No",
variability: "constant") %>

**chMinFsldPlr=_float_**

Minimum compressor false loading part load ratio (plr); maximum cycling plr. See description above.

<%= member_table(
units: "",
legal_range: "0 $\\le$ _x_ $\\le$ _chMinFsldPlr_",
default: "0.1",
required: "No",
variability: "constant") %>

**chMotEff=_float_**

Fraction of CHILLER compressor motor input power which goes to the condenser. For an open-frame motor and compressor, where the motor's waste heat goes to the air, enter the motor's efficiency: a fraction around .8 or .9. For a hermetic compressor, where the motor's waste heat goes to the refrigerant and thence to the condenser, use 1.0.

<%= member_table(
units: "",
legal_range: "0 &lt; _x_ $\\le$ 1",
default: "1.0",
required: "No",
variability: "constant") %>

**chMtr=_name_**

Name of METER to which to accumulate CHILLER's electrical input energy. Category "Clg" is used. Note that two additional commands, _chppMtr_ and _chcpMtr_, are used to specify meters for recording chiller pump input energy.

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_not recorded_",
required: "No",
variability: "constant") %>

The next six inputs specify this CHILLER's _PRIMARY PUMP_, which pumps chilled water from the chiller through the CHW coils connected to the chiller's COOLPLANT.

**chppGpm=_float_**

Chiller primary pump flow in gallons per minute: amount of water pumped from this chiller through the primary loop supplying the COOLPLANT's loads (CHW coils) whenever chiller is operating. Any excess flow over that demanded by coils is assumed to go through a bypass valve. If coil flows exceed _chppGpm_, CSE assumes the pressure drops and the pump "overruns" to deliver the extra flow with the same energy input. The default is one gallon per minute for each 5000 Btuh of chiller design capacity.

<%= member_table(
units: "gpm",
legal_range: "_x_ $\\gt$ 0",
default: "_chCapDs_ / 5000",
required: "No",
variability: "constant") %>

**chppHdloss=_float_**

Chiller primary pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

<%= member_table(
units: "ft H2O",
legal_range: "_x_ $\\ge$ 0",
default: "57.22\*",
required: "No",
variability: "constant") %>

\* May be temporary default for 10-31-92 version; prior value (65) may be restored.

**chppMotEff=_float_**

Chiller primary pump motor efficiency.

<%= member_table(
units: "",
legal_range: "0 &lt; _x_ $\\le$ 1.0",
default: ".88",
required: "No",
variability: "constant") %>

**chppHydEff=_float_**

Chiller primary pump hydraulic efficiency

<%= member_table(
units: "",
legal_range: "0 &lt; _x_ $\\le$ 1.0",
default: "0.7",
required: "No",
variability: "constant") %>

**chppOvrunF=_float_**

Chiller primary pump maximum overrun: factor by which flow demanded by coils can exceed _chppGpm_. The primary flow is not simulated in detail; _chppOvrun_ is currently used only to issue an error message if the sum of the design flows of the coils connected to a COOLPLANT exceeds the sum of the products of _chppGpm_ and _chppOvrun_ for the chiller's in the plants most powerful stage.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 1.0",
default: "1.3",
required: "No",
variability: "constant") %>

**chppMtr=_name of a METER_**

Meter to which primary pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_none_",
required: "No",
variability: "constant") %>

The next five inputs specify this CHILLER's _CONDENSER PUMP_, also known as the _SECONDARY PUMP_ or the _HEAT REJECTION PUMP_. This pump pumps water from the chiller's condenser through the cooling towers in the COOLPLANT's TOWERPLANT.

**chcpGpm=_float_**

Chiller condenser pump flow in gallons per minute: amount of water pumped from this chiller through the cooling towers when chiller is operating.

<%= member_table(
units: "gpm",
legal_range: "_x_ $\\gt$ 0",
default: "_chCapDs_ / 4000",
required: "No",
variability: "constant") %>

**chcpHdloss=_float_**

Chiller condenser pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

<%= member_table(
units: "ft H2O",
legal_range: "_x_ $\\ge$ 0",
default: "45.78\*",
required: "No",
variability: "constant") %>

\* May be temporary default for 10-31-92 version; prior value (45) may be restored.

**chcpMotEff=_float_**

Chiller condenser pump motor efficiency.

<%= member_table(
units: "",
legal_range: "0 &lt; _x_ $\\le$ 1.0",
default: ".88",
required: "No",
variability: "constant") %>

**chcpHydEff=_float_**

Chiller condenser pump hydraulic efficiency

<%= member_table(
units: "",
legal_range: "0 &lt; _x_ $\\le$ 1.0",
default: "0.7",
required: "No",
variability: "constant") %>

**chcpMtr=_name of a METER_**

Meter to which condenser pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_none_",
required: "No",
variability: "constant") %>

The following four members permit specification of auxiliary input power use associated with the chiller under the conditions indicated.

**chAuxOn=_float_**

Auxiliary power used when chiller is running, in proportion to its subhour average part load ratio (plr).

**chAuxOff=_float_**

Auxiliary power used when chiller is not running, in proportion to 1 - plr.

**chAuxFullOff=_float_**

Auxiliary power used only when chiller is off for entire subhour; not used if the chiller is on at all during the subhour.

**chAuxOnAtAll=_float_**

Auxiliary power used in full value if chiller is on for any fraction of subhour.

<%= member_table(
units: "",
legal_range: "_x_ $\\ge$ 0",
default: "0",
required: "No",
variability: "constant") %>

The following four allow specification of meters to record chiller auxiliary energy use through chAuxOn, chAuxOff, chFullOff, and chAuxOnAtAll, respectively. End use category "Aux" is used.

**chAuxOnMtr=_mtrName_**

**chAuxOffMtr=_mtrName_**

**chAuxFullOffMtr=_mtrName_**

**chAuxOnAtAllMtr=_mtrName_**

<%= member_table(
units: "",
legal_range: "_name of a METER_",
default: "_none_",
required: "No",
variability: "constant") %>

**endChiller**

Optionally indicates the end of the CHILLER definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

<%= member_table(
units: "",
legal_range: "",
default: "_none_",
required: "No",
variability: "constant") %>

**Related Probes:**

- @[chiller][p_chiller]
