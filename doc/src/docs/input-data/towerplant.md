# TOWERPLANT

A TOWERPLANT object simulates a group of cooling towers which operate together to cool water for one or more CHILLERs and/or HPLOOP heat exchangers. There can be more than one TOWERPLANT in a simulation. Each CHILLER or hploop heat exchanger contains a pump (the "heat rejection pump") to circulate water through its associated TOWERPLANT. The circulating water is cooled by evaporation and conduction to the air; cooling is increased by operating fans in the cooling towers as necessary. These fans are the only energy consuming devices simulated in the TOWERPLANT.

The TOWERPLANT models the leaving water temperature as a function of the entering water temperature, flow, outdoor air temperature, and humidity. The fans are operated as necessary to achieve a specified leaving water temperature setpoint, or as close to it as achievable.

Two methods of staging the cooling tower fans in a TOWERPLANT are supported: "TOGETHER", under which all the tower fans operate together, at the same speed or cycling on and off at once, and "LEAD", in which a single "lead" tower's fan modulates for fine control of leaving water temperature, and as many additional towers fans as necessary operate at fixed speed. The water flows through all towers even when their fans are off; sometimes this will cool the water below setpoint with no fans operating.

All the towers in a TOWERPLANT are identical, except that under LEAD staging, the towers other than the lead tower have one-speed fans. The group of towers can thus be described by giving the description of one tower, the number of towers, and the type of staging to be used. All of this information is given by TOWERPLANT members, so there is no need for individual TOWER objects.

There is no provision for scheduling a TOWERPLANT: it operates whenever the heat rejection pump in one or more of its associated CHILLERs or HPLOOP heat exchangers operates. However, the setpoint for the water leaving the TOWERPLANT is hourly schedulable.

**towerplantName**

Name of TOWERPLANT object, given immediately after the word TOWERPLANT to begin the object's input. The name is used to refer to the TOWERPLANT in COOLPLANTs and HPLOOPs.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *63 characters*                 Yes            constant

**tpTsSp=_float_**

Setpoint temperature for water leaving towers.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 85 No hourly

**tpMtr=_name of a METER_**

METER object by which TOWERPLANT's fan input energy is to be recorded, in category "Aux". If omitted, energy use is not recorded, and thus cannot be reported. Towerplants have no modeled input energy other than for their fans (the heat rejection pumps are part of the CHILLER and HPLOOP objects).

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *name of a METER*   *none*        No             constant

**tpStg=_choice_**

How tower fans are staged to meet the load:

---

TOGETHER All fans operate at the same speed or cycle on and off
together.

LEAD A single "Lead" tower's fan is modulated as required and
as many additional fans as necessary run at their (single)
full speed.

---

Whenever the heat rejection pump in a CHILLER or HPLOOP heat exchanger is on, the water flows through all towers in the TOWERPLANT, regardless of the number of fans operating.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              TOGETHER, LEAD    TOGETHER      No             constant

**ctN=_integer_**

Number of towers in the TOWERPLANT.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *x* &gt; 0        1             No             constant

**ctType=_choice_**

Cooling tower fan control type: ONESPEED, TWOSPEED, or VARIABLE. This applies to all towers under TOGETHER staging. For LEAD staging, _ctType_ applies only to the lead tower; additional towers have ONESPEED fans.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              ONESPEED, TWOSPEED, VARIABLE   ONESPEED      No             constant

**ctLoSpd=_float_**

Low speed for TWOSPEED fan, as a fraction of full speed cfm.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              0 &lt; *x* $\le$ 1   0.5           No             constant

Note: full speed fan cfm is given by _ctVfDs_, below.

The rest of the input variables apply to each tower in the group; the towers are identical except for the single-speed fan on non-lead towers when _tpStg_ is LEAD.

The following two inputs permit computation of the tower fan electrical energy consumption:

**ctShaftBhp=_float_**

Shaft brake horsepower of each tower fan motor.

The default value is the sum of the rejected (condenser) heats (including pump heat) at design conditions of the most powerful stage of each connected COOLPLANT, plus the design capacity of each connected HPLOOP heat exchanger, all divided by 290,000 and by the number of cooling towers in the TOWERPLANT.

**Units** **Lgl Range** **Default** **Req'd** **Variability**

---

Bhp _x_ &gt; 0 (sum of loads)/290000/_cTn_ No constant

**ctMotEff=_float_**

Motor (and drive, if any) efficiency for tower fans.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *x* &gt; 0        .88           No             constant

The next four items specify the coefficients of polynomial curves relating fan power consumption to average speed (cfm) for the various fan types. For the non-variable speed cases CSE uses linear polynomials of the form

$$p = a + b \cdot \text{spd}$$

where _p_ is the power consumption as a fraction of full speed power consumption, and _spd_ is the average speed as a fraction of full speed. The linear relationship reflects the fact that the fans cycle to match partial loads. A non-0 value may be given for the constant part _a_ to reflect start-stop losses. For the two speed fan, separate polynomials are used for low and high speed operation; the default coefficients assume power input varies with the cube of speed, that is, at low speed (_ctLoSpd_) the relative power input is _ctLoSpd3_. For the variable speed case a cubic polynomial is used.

For each linear polynomial, two _float_ expressions are given, separated by a comma. The first expression is the constant, _a_. The second expression is the coefficient of the average speed, _b_. Except for _ctFcLo_, _a_ and _b_ should add up to 1, to make the relative power consumption 1 when _spd_ is 1; otherwise, CSE will issue a warning message and normalize them.

**ctFcOne=_a, b_**

Coefficients of linear fan power consumption polynomial $p = a + b \cdot \text{spd}$ for ONESPEED fan. For the one-speed case, the relative average speed _spd_ is the fraction of the time the fan is on.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *a + b = 1.0*     0, 1          No             constant

**ctFcLo=_a, b_**

Coefficients of linear fan power consumption polynomial $p = a + b \cdot \text{spd}$ for low speed of TWOSPEED fan, when _spd_ $\le$ _ctLoSpd_.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *a + b = 1.0*     0, *ctLoSpd^2^*   No             constant

**ctFcHi=_a, b_**

Coefficients of linear fan power consumption polynomial $p = a + b \cdot \text{spd}$ for high speed of TWOSPEED fan, when _spd_ &gt; _ctLoSpd_.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *a + b = 1.0*     *-ctLoSpd^2^ - ctLoSpd, ctLoSpd^2^ + ctLoSpd + 1*   No             constant

**ctFcVar=_a, b, c, d_**

For VARIABLE speed fan, four _float_ values for coefficients of cubic fan power consumption polynomial of the form $p = a + b \cdot \text{spd} + c \cdot \text{spd}^2 + d \cdot \text{spd}^3$.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              *a + b + c + d = 1.0*   0, 0, 0, 1    No             constant

The next six items specify the tower performance under one set of conditions, the "design conditions". The conditions should be chosen to be representative of full load operating conditions.

**ctCapDs=_float_**

Design capacity: amount of heat extracted from water under design conditions by one tower.

The default value is the sum of the rejected (condenser) heats (including pump heat) at design conditions of the most powerful stage of each connected COOLPLANT, plus the design capacity of each connected HPLOOP heat exchanger, all divided by the number of towers.

**Units** **Legal Range** **Default** **Required** **Variability**

---

Btuh _x_ $\neq$ 0 (sum of loads)/_ctN_ No constant

**ctVfDs=_float_**

Design air flow, per tower; also the fan full-speed cfm specification.

The default value is the sum of the loads (computed as for _ctCapDs_, just above) divided by 51, divided by the number of cooling towers.

**Units** **Legal Range** **Default** **Required** **Variability**

---

cfm _x_ &gt; 0 (sum of loads)/51/_ctN_ No constant

**ctGpmDs=_float_**

Design water flow, per tower.

The default is the sum of the flows of the connected heat rejection pumps, using the largest stage for COOLPLANTs, divided by the number of towers.

**Units** **Legal Range** **Default** **Required** **Variability**

---

gpm _x_ &gt; 0 (sum of pumps)/_ctN_ No constant

**ctTDbODs=_float_**

Design outdoor drybulb temperature (needed to convert _ctVfDs_ from cfm to lb/hr).

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 93.5 No constant

**ctTWbODs=_float_**

Design outdoor wetbulb temperature.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 78 No constant

**ctTwoDs=_float_**

Design leaving water temperature.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 85 No constant

The following six items allow optional specification of tower performance under another set of conditions, the "off design" conditions. If given, they allow CSE to compute the tower's relation between flows and heat transfer; in this case, _ctK_ (below) may not be given.

**ctCapOd=_float_**

Off-design capacity, per tower.

**Units** **Legal Range** **Default** **Required** **Variability**

---

Btuh _x_ $\neq$ 0 (sum of loads)/_ctN_ No constant

**ctVfOd=_float_**

Off-design air flow, per tower. Must differ from design air flow; thus _ctVfDs_ and _ctVfOd_ cannot both be defaulted if off-design conditions are being given. The off-design air and water flows must be chosen so that maOd/mwOd $\neq$ maDs/mwDs.

**Units** **Legal Range** **Default** **Required** **Variability**

---

cfm _x_ &gt; 0; _x_ $\neq$ _ctVfDs_ (sum of loads)/51/_ctN_ No constant

**ctGpmOd=_float_**

Off-design water flow, per tower. Must differ from design water flow; thus, both cannot be defaulted if off-design conditions are being given. Value must be chosen so that maOd/mwOd $\neq$ maDs/mwDs.

**Units** **Legal Range** **Default** **Required** **Variability**

---

gpm _x_ &gt; 0; _x_ $\neq$ _ctGpmDs_ (sum of pumps)/_ctN_ No constant

**ctTDbOOd=_float_**

Off-design outdoor drybulb temperature.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 93.5 No constant

**ctTWbOOd=_float_**

Off-design outdoor wetbulb temperature.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 78 No constant

**ctTwoOd=_float_**

Off-design leaving water temperature.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 85 No constant

The following item allows explicit specification of the relationship between flows and heat transfer, when the preceding "off design" inputs are not given. If omitted, it will be computed from the "off design" inputs if given, else the default value of 0.4 will be used.

**ctK=_float_**

Optional. Exponent in the formula

$$\text{ntuA} = k \cdot (mwi/ma)^{ctK}$$

where ntuA is the number of transfer units on the air side, mwi and ma are the water and air flows respectively, and k is a constant.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              0 &lt; *x* &lt; 1   *from "Od" members if given, else* 0.4   No             constant

**ctStkFlFr=_float_**

Fraction of air flow which occurs when tower fan is off, due to stack effect (convection). Cooling due to this air flow occurs in all towers whenever the water flow is on, and may, by itself, cool the water below the setpoint _tpTsSp_. Additional flow, when fan is on, is proportional to fan speed.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              0 $\le$ *x* $\le$ 1   .18           No             constant

The following items allow CSE to compute the effect of makeup water on the leaving water temperature.

**ctBldn=_float_**

Blowdown rate: fraction of inflowing water that is bled from the sump down the drain, to reduce the buildup of impurities that don't evaporate.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              0 $\le$ *x* $\le$ 1   .01           No             constant

**ctDrft=_float_**

Drift rate: fraction of inflowing water that is blown out of tower as droplets without evaporating.

**Units** **Legal Range** **Default** **Required** **Variability**

---

              0 $\le$ *x* $\le$ 1   0             No             constant

**ctTWm=_float_**

Temperature of makeup water from mains, used to replace water lost by blowdown, drift, and evaporation. Blowdown and drift are given by the preceding two inputs; evaporation is computed.

**Units** **Legal Range** **Default** **Required** **Variability**

---

^o^F _x_ &gt; 0 60 No constant

**endTowerplant**

Optionally indicates the end of the TOWERPLANT definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

**Units** **Legal Range** **Default** **Required** **Variability**

---

                                *N/A*         No             constant

**Related Probes:**

- @[towerPlant][p_towerplant]
