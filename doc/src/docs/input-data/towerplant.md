# TOWERPLANT

A TOWERPLANT object simulates a group of cooling towers which operate together to cool water for one or more CHILLERs and/or HPLOOP heat exchangers. There can be more than one TOWERPLANT in a simulation. Each CHILLER or hploop heat exchanger contains a pump (the "heat rejection pump") to circulate water through its associated TOWERPLANT. The circulating water is cooled by evaporation and conduction to the air; cooling is increased by operating fans in the cooling towers as necessary. These fans are the only energy consuming devices simulated in the TOWERPLANT.

The TOWERPLANT models the leaving water temperature as a function of the entering water temperature, flow, outdoor air temperature, and humidity. The fans are operated as necessary to achieve a specified leaving water temperature setpoint, or as close to it as achievable.

Two methods of staging the cooling tower fans in a TOWERPLANT are supported: "TOGETHER", under which all the tower fans operate together, at the same speed or cycling on and off at once, and "LEAD", in which a single "lead" tower's fan modulates for fine control of leaving water temperature, and as many additional towers fans as necessary operate at fixed speed. The water flows through all towers even when their fans are off; sometimes this will cool the water below setpoint with no fans operating.

All the towers in a TOWERPLANT are identical, except that under LEAD staging, the towers other than the lead tower have one-speed fans. The group of towers can thus be described by giving the description of one tower, the number of towers, and the type of staging to be used. All of this information is given by TOWERPLANT members, so there is no need for individual TOWER objects.

There is no provision for scheduling a TOWERPLANT: it operates whenever the heat rejection pump in one or more of its associated CHILLERs or HPLOOP heat exchangers operates. However, the setpoint for the water leaving the TOWERPLANT is hourly schedulable.

### towerplantName

Name of TOWERPLANT object, given immediately after the word TOWERPLANT to begin the object's input. The name is used to refer to the TOWERPLANT in COOLPLANTs and HPLOOPs.

{{
  member_table({
    "units": "",
    "legal_range": "*63 characters*", 
    "default": "",
    "required": "Yes",
    "variability": "constant" 
  })
}}


### tpTsSp

Type: float

Setpoint temperature for water leaving towers.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "85",
    "required": "No",
    "variability": "hourly" 
  })
}}

### tpMtr

Type: *name of a METER*

METER object by which TOWERPLANT's fan input energy is to be recorded, in category "Aux". If omitted, energy use is not recorded, and thus cannot be reported. Towerplants have no modeled input energy other than for their fans (the heat rejection pumps are part of the CHILLER and HPLOOP objects).

{{
  member_table({
    "units": "",
    "legal_range": "*name of a METER*", 
    "default": "*none*",
    "required": "No",
    "variability": "constant" 
  })
}}

### tpStg

Type: choice

How tower fans are staged to meet the load:

{{
  csv_table("TOGETHER, All fans operate at the same speed or cycle on and off together.
LEAD, A single \"Lead\" tower's fan is modulated as required and as many additional fans as necessary run at their (single) full speed.")
}}

Whenever the heat rejection pump in a CHILLER or HPLOOP heat exchanger is on, the water flows through all towers in the TOWERPLANT, regardless of the number of fans operating.

{{
  member_table({
    "units": "",
    "legal_range": "TOGETHER, LEAD", 
    "default": "TOGETHER",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctN

Type: integer

Number of towers in the TOWERPLANT.

{{
  member_table({
    "units": "",
    "legal_range": "*x* &gt; 0", 
    "default": "1",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctType

Type: choice

Cooling tower fan control type: ONESPEED, TWOSPEED, or VARIABLE. This applies to all towers under TOGETHER staging. For LEAD staging, *ctType* applies only to the lead tower; additional towers have ONESPEED fans.

{{
  member_table({
    "units": "",
    "legal_range": "ONESPEED, TWOSPEED, VARIABLE", 
    "default": "ONESPEED",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctLoSpd

Type: float

Low speed for TWOSPEED fan, as a fraction of full speed cfm.

{{
  member_table({
    "units": "",
    "legal_range": "0 &lt; *x* ≤ 1", 
    "default": "0.5",
    "required": "No",
    "variability": "constant" 
  })
}}

Note: full speed fan cfm is given by *ctVfDs*, below.

The rest of the input variables apply to each tower in the group; the towers are identical except for the single-speed fan on non-lead towers when *tpStg* is LEAD.

The following two inputs permit computation of the tower fan electrical energy consumption:

### ctShaftBhp

Type: float

Shaft brake horsepower of each tower fan motor.

The default value is the sum of the rejected (condenser) heats (including pump heat) at design conditions of the most powerful stage of each connected COOLPLANT, plus the design capacity of each connected HPLOOP heat exchanger, all divided by 290,000 and by the number of cooling towers in the TOWERPLANT.

{{
  member_table({
    "units": "Bhp",
    "legal_range": "*x* &gt; 0", 
    "default": "(sum of loads)/290000/*cTn*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctMotEff

Type: float

Motor (and drive, if any) efficiency for tower fans.

{{
  member_table({
    "units": "",
    "legal_range": "*x* &gt; 0", 
    "default": ".88",
    "required": "No",
    "variability": "constant" 
  })
}}

The next four items specify the coefficients of polynomial curves relating fan power consumption to average speed (cfm) for the various fan types. For the non-variable speed cases CSE uses linear polynomials of the form

$$
p = a + b \cdot \text{spd}
$$

where *p* is the power consumption as a fraction of full speed power consumption, and *spd* is the average speed as a fraction of full speed. The linear relationship reflects the fact that the fans cycle to match partial loads. A non-0 value may be given for the constant part *a* to reflect start-stop losses. For the two speed fan, separate polynomials are used for low and high speed operation; the default coefficients assume power input varies with the cube of speed, that is, at low speed (*ctLoSpd*) the relative power input is *ctLoSpd3*. For the variable speed case a cubic polynomial is used.

For each linear polynomial, two *float* expressions are given, separated by a comma. The first expression is the constant, *a*. The second expression is the coefficient of the average speed, *b*. Except for *ctFcLo*, *a* and *b* should add up to 1, to make the relative power consumption 1 when *spd* is 1; otherwise, CSE will issue a warning message and normalize them.

### ctFcOne

Type: *a, b*

Coefficients of linear fan power consumption polynomial $p = a + b \cdot \text{spd}$ for ONESPEED fan. For the one-speed case, the relative average speed *spd* is the fraction of the time the fan is on.

{{
  member_table({
    "units": "",
    "legal_range": "*a + b = 1.0*", 
    "default": "0, 1",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctFcLo

Type: *a, b*

Coefficients of linear fan power consumption polynomial $p = a + b \cdot \text{spd}$ for low speed of TWOSPEED fan, when *spd* ≤ *ctLoSpd*.

{{
  member_table({
    "units": "",
    "legal_range": "*a + b = 1.0*", 
    "default": "0, *ctLoSpd^2^*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctFcHi

Type: *a, b*

Coefficients of linear fan power consumption polynomial $p = a + b \cdot \text{spd}$ for high speed of TWOSPEED fan, when *spd* &gt; *ctLoSpd*.

{{
  member_table({
    "units": "",
    "legal_range": "*a + b = 1.0*", 
    "default": "*-ctLoSpd^2^ - ctLoSpd, ctLoSpd^2^ + ctLoSpd + 1*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctFcVar

Type: *a, b, c, d*

For VARIABLE speed fan, four *float* values for coefficients of cubic fan power consumption polynomial of the form $p = a + b \cdot \text{spd} + c \cdot \text{spd}^2 + d \cdot \text{spd}^3$.

{{
  member_table({
    "units": "",
    "legal_range": "*a + b + c + d = 1.0*", 
    "default": "0, 0, 0, 1",
    "required": "No",
    "variability": "constant" 
  })
}}

The next six items specify the tower performance under one set of conditions, the "design conditions". The conditions should be chosen to be representative of full load operating conditions.

### ctCapDs

Type: float

Design capacity: amount of heat extracted from water under design conditions by one tower.

The default value is the sum of the rejected (condenser) heats (including pump heat) at design conditions of the most powerful stage of each connected COOLPLANT, plus the design capacity of each connected HPLOOP heat exchanger, all divided by the number of towers.

{{
  member_table({
    "units": "Btuh",
    "legal_range": "*x* ≠ 0", 
    "default": "(sum of loads)/*ctN*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctVfDs

Type: float

Design air flow, per tower; also the fan full-speed cfm specification.

The default value is the sum of the loads (computed as for *ctCapDs*, just above) divided by 51, divided by the number of cooling towers.

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* &gt; 0", 
    "default": "(sum of loads)/51/*ctN*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctGpmDs

Type: float

Design water flow, per tower.

The default is the sum of the flows of the connected heat rejection pumps, using the largest stage for COOLPLANTs, divided by the number of towers.

{{
  member_table({
    "units": "gpm",
    "legal_range": "*x* &gt; 0", 
    "default": "(sum of pumps)/*ctN*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTDbODs

Type: float

Design outdoor drybulb temperature (needed to convert *ctVfDs* from cfm to lb/hr).

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "93.5",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTWbODs

Type: float

Design outdoor wetbulb temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "78",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTwoDs

Type: float

Design leaving water temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "85",
    "required": "No",
    "variability": "constant" 
  })
}}

The following six items allow optional specification of tower performance under another set of conditions, the "off design" conditions. If given, they allow CSE to compute the tower's relation between flows and heat transfer; in this case, *ctK* (below) may not be given.

### ctCapOd

Type: float

Off-design capacity, per tower.

{{
  member_table({
    "units": "Btuh",
    "legal_range": "*x* ≠ 0", 
    "default": "(sum of loads)/*ctN*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctVfOd

Type: float

Off-design air flow, per tower. Must differ from design air flow; thus *ctVfDs* and *ctVfOd* cannot both be defaulted if off-design conditions are being given. The off-design air and water flows must be chosen so that maOd/mwOd are not equal to maDs/mwDs.

{{
  member_table({
    "units": "cfm",
    "legal_range": "*x* &gt; 0; *x* ≠ *ctVfDs*", 
    "default": "(sum of loads)/51/*ctN*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctGpmOd

Type: float

Off-design water flow, per tower. Must differ from design water flow; thus, both cannot be defaulted if off-design conditions are being given. Value must be chosen so that maOd/mwOd are not equal to maDs/mwDs.

{{
  member_table({
    "units": "gpm",
    "legal_range": "*x* &gt; 0; *x* ≠ *ctGpmDs*", 
    "default": "(sum of pumps)/*ctN*",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTDbOOd

Type: float

Off-design outdoor drybulb temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "93.5",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTWbOOd

Type: float

Off-design outdoor wetbulb temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "78",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTwoOd

Type: float

Off-design leaving water temperature.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "85",
    "required": "No",
    "variability": "constant" 
  })
}}

The following item allows explicit specification of the relationship between flows and heat transfer, when the preceding "off design" inputs are not given. If omitted, it will be computed from the "off design" inputs if given, else the default value of 0.4 will be used.

### ctK

Type: float

Optional. Exponent in the formula

$$
\text{ntuA} = k \cdot (mwi/ma)^{ctK}
$$

where ntuA is the number of transfer units on the air side, mwi and ma are the water and air flows respectively, and k is a constant.

{{
  member_table({
    "units": "",
    "legal_range": "0 &lt; *x* &lt; 1", 
    "default": "*from \"Od\" members if given, else* 0.4",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctStkFlFr

Type: float

Fraction of air flow which occurs when tower fan is off, due to stack effect (convection). Cooling due to this air flow occurs in all towers whenever the water flow is on, and may, by itself, cool the water below the setpoint *tpTsSp*. Additional flow, when fan is on, is proportional to fan speed.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": ".18",
    "required": "No",
    "variability": "constant" 
  })
}}

The following items allow CSE to compute the effect of makeup water on the leaving water temperature.

### ctBldn

Type: float

Blowdown rate: fraction of inflowing water that is bled from the sump down the drain, to reduce the buildup of impurities that don't evaporate.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": ".01",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctDrft

Type: float

Drift rate: fraction of inflowing water that is blown out of tower as droplets without evaporating.

{{
  member_table({
    "units": "",
    "legal_range": "0 ≤ *x* ≤ 1", 
    "default": "0",
    "required": "No",
    "variability": "constant" 
  })
}}

### ctTWm

Type: float

Temperature of makeup water from mains, used to replace water lost by blowdown, drift, and evaporation. Blowdown and drift are given by the preceding two inputs; evaporation is computed.

{{
  member_table({
    "units": "^o^F",
    "legal_range": "*x* &gt; 0", 
    "default": "60",
    "required": "No",
    "variability": "constant" 
  })
}}

### endTowerplant

Optionally indicates the end of the TOWERPLANT definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

*Related Probes:**

- @[towerPlant][p_towerplant]
