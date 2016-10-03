# CHILLER

CHILLERs are subobjects of COOLPLANTs (Section 5.21). CHILLERs supply coldness, in the form of chilled water, via their COOLPLANT, to CHW (CHilled Water) cooling coils in AIRHANDLERs. CHILLERs exhaust heat through the cooling towers in their COOLPLANT's TOWERPLANT. Each COOLPLANT can contain multiple CHILLERs; chiller operation is controlled by the scheduling and staging logic of the COOLPLANT, as described in the previous section.

Each chiller has primary and secondary pumps that operate when the chiller is on. The pumps add heat to the primary and secondary loop water respectively; this heat is considered in the modeling of the loop's water temperature.

**chillerName**

Name of CHILLER object, given immediately after the word CHILLER. This name is used to refer to the chiller in *cpStage* commands.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 Yes            constant

The next four inputs allow specification of the CHILLER's capacity (amount of heat it can remove from the primary loop water) and how this capacity varies with the supply (leaving) temperature of the primary loop water and the entering temperature of the condenser (secondary loop) water. The chiller capacity at any supply and condenser temperatures is *chCapDs* times the value of *chPyCapT* at those temperatures.

**chCapDs=*float***

Chiller design capacity, that is, the capacity at *chTsDs* and *chTcndDs* (next).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        *x* != 0                        Yes            constant

**chTsDs=*float***

Design supply temperature: temperature of primary water leaving chiller at which capacity is *chCapDs*.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* &gt; 0        44            No             constant

**chTcndDs=*float***

Design condenser temperature: temperature of secondary water entering chiller condenser at which capacity is *chCapDs*.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* &gt; 0        85            No             constant

**chPyCapT=*a, b, c, d, e, f***

Coefficients of bi-quadratic polynomial function of supply (ts) and condenser (tcnd) temperatures that specifies how capacity varies with these temperatures. This polynomial is of the form

$$a + b \cdot \bm{ts} + c \cdot \bm{ts}^2 + d \cdot \bm{tcnd} + e \cdot \bm{tcnd}^2 + f \cdot \bm{ts} \cdot \bm{tcnd}$$

Up to six *float* values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when ts is chTsDs and tcnd is chTcndDs, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

  **Units**   **Legal Range**   **Default**                                               **Required**   **Variability**
  ----------- ----------------- --------------------------------------------------------- -------------- -----------------
                                -1.742040, .029292, .000067, .048054, .000291, -.000106   No             constant

The next three inputs allow specification of the CHILLER's full-load energy input and how it varies with supply and condenser temperature. Only one of *chCop* and *chEirDs* should be given. The full-load energy input at any supply and condenser temperatures is the chiller's capacity at these temperatures, times *chEirDs* (or 1/*chCop*), times the value of *chPyEirT* at these temperatures.

**chCop=*float***

Chiller full-load COP (Coefficient Of Performance) at *chTsDs*and *chTcndDs*. This is the output energy divided by the electrical input energy (in the same units) and reflects both motor and compressor efficiency.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* &gt; 0        4.2           No             constant

**chEirDs=*float***

Alternate input for COP: Full-load Energy Input Ratio (electrical input energy divided by output energy) at design temperatures; the reciprocal of *chCop*.

  **Units**   **Legal Range**   **Default**            **Required**   **Variability**
  ----------- ----------------- ---------------------- -------------- -----------------
              *x* &gt; 0        *chCop* is defaulted   No             constant

**chPyEirT=*a, b, c, d, e, f***

Coefficients of bi-quadratic polynomial function of supply (ts) and condenser (tcnd) temperatures that specifies how energy input varies with these temperatures. This polynomial is of the form

$$a + b \cdot \bm{ts} + c \cdot \bm{ts}^2 + d \cdot \bm{tcnd} + e \cdot \bm{tcnd}^2 + f \cdot \bm{ts} \cdot \bm{tcnd}$$

Up to six *float* values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when ts is chTsDs and tcnd is chTcndDs, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

  **Units**   **Legal Range**   **Default**                                               **Required**   **Variability**
  ----------- ----------------- --------------------------------------------------------- -------------- -----------------
                                3.117600, -.109236, .001389, .003750, .000150, -.000375   No             constant

The next three inputs permit specification of the CHILLER's part load energy input. In the following the part load ratio (plr) is defined as the actual load divided by the capacity at the current supply and condenser temperatures. The energy input is defined as follows for four different plr ranges:

  --------------- -------------------------------------------------------
  full            loadplr (part load ratio) = 1.0

                  Power input is full-load input, as described above.

  compressor      1.0 &gt; plr $\ge$ *chMinUnldPlr*
  unloading       
  region          

                  Power input is the full-load input times the value of
                  the *chPyEirUl* polynomial for the current plr, that
                  is, *chPyEirUl*(plr).

  false loading   *chMinUnldPlr* &gt; plr &gt; *chMinFsldPlr*
  region          

                  Power input in this region is constant at the value for
                  the low end of the compressor unloading region, i.e.
                  *chPyEirUl*(*chMinUnldPlr*).

  cycling region  *chMinFsldPlr* &gt; plr $\ge$ 0

                  In this region the chiller runs at the low end of the
                  false loading region for the necessary fraction of the
                  time, and the power input is the false loading value
                  correspondingly prorated, i.e.
                  *chPyEirUl*(*chMinUnldPlr*) plr / *chMinFsldPlr*.
  --------------- -------------------------------------------------------

These plr regions are similar to those for a DX coil & compressor in an AIRHANDLER, Section 0.

**chPyEirUl=*a, b, c, d***

Coefficients of cubic polynomial function of part load ratio (plr) that specifies how energy input varies with plr in the compressor unloading region (see above). This polynomial is of the form

$$a + b \cdot plr + c \cdot plr^2 + d \cdot plr^3$$

Up to four *float* values may be entered, separated by commas; CSE will use zero for omitted trailing values. If the polynomial does not evaluate to 1.0 when plr is 1.0, a warning message will be issued and the coefficients will be adjusted (normalized) to make the value 1.0.

  **Units**   **Legal Range**   **Default**                     **Required**   **Variability**
  ----------- ----------------- ------------------------------- -------------- -----------------
                                .222903, .313387, .463710, 0.   No             constant

**chMinUnldPlr=*float***

Minimum compressor unloading part load ratio (plr); maximum false loading plr. See description above.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1   0.1           No             constant

**chMinFsldPlr=*float***

Minimum compressor false loading part load ratio (plr); maximum cycling plr. See description above.

  **Units**   **Legal Range**                    **Default**   **Required**   **Variability**
  ----------- ---------------------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ *chMinFsldPlr*   0.1           No             constant

**chMotEff=*float***

Fraction of CHILLER compressor motor input power which goes to the condenser. For an open-frame motor and compressor, where the motor's waste heat goes to the air, enter the motor's efficiency: a fraction around .8 or .9. For a hermetic compressor, where the motor's waste heat goes to the refrigerant and thence to the condenser, use 1.0.

  **Units**   **Legal Range**      **Default**   **Required**   **Variability**
  ----------- -------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1   1.0           No             constant

**chMeter=*name***

Name of METER to which to accumulate CHILLER's electrical input energy. Category "Clg" is used. Note that two additional commands, *chppMtr* and *chcpMtr*, are used to specify meters for recording chiller pump input energy.

  **Units**   **Legal Range**     **Default**    **Required**   **Variability**
  ----------- ------------------- -------------- -------------- -----------------
              *name of a METER*   not recorded   No             constant

The next six inputs specify this CHILLER's *PRIMARY PUMP*, which pumps chilled water from the chiller through the CHW coils connected to the chiller's COOLPLANT.

**chppGpm=*float***

Chiller primary pump flow in gallons per minute: amount of water pumped from this chiller through the primary loop supplying the COOLPLANT's loads (CHW coils) whenever chiller is operating. Any excess flow over that demanded by coils is assumed to go through a bypass valve. If coil flows exceed *chppGpm*, CSE assumes the pressure drops and the pump "overruns" to deliver the extra flow with the same energy input. The default is one gallon per minute for each 5000 Btuh of chiller design capacity.

  **Units**   **Legal Range**   **Default**        **Required**   **Variability**
  ----------- ----------------- ------------------ -------------- -----------------
  gpm         *x* &gt; 0        *chCapDs* / 5000   No             constant

**chppHdloss=*float***

Chiller primary pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft H2O      *x* $\ge$ 0       57.22\*       No             constant

\* May be temporary default for 10-31-92 version; prior value (65) may be restored.

**chppMotEff=*float***

Chiller primary pump motor efficiency.

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .88           No             constant

**chppHydEff=*float***

Chiller primary pump hydraulic efficiency

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .70           No             constant

**chppOvrun=*float***

Chiller primary pump maximum overrun: factor by which flow demanded by coils can exceed *chppGpm*. The primary flow is not simulated in detail; *chppOvrun* is currently used only to issue an error message if the sum of the design flows of the coils connected to a COOLPLANT exceeds the sum of the products of *chppGpm* and *chppOvrun* for the chiller's in the plants most powerful stage.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 1.0     1.3           No             constant

**chppMtr=*name of a METER***

Meter to which primary pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
              *name of a METER*   *none*        No             constant

The next five inputs specify this CHILLER's *CONDENSER PUMP*, also known as the *SECONDARY PUMP* or the *HEAT REJECTION PUMP*. This pump pumps water from the chiller's condenser through the cooling towers in the COOLPLANT's TOWERPLANT.

**chcpGpm=*float***

Chiller condenser pump flow in gallons per minute: amount of water pumped from this chiller through the cooling towers when chiller is operating.

  **Units**   **Legal Range**   **Default**        **Required**   **Variability**
  ----------- ----------------- ------------------ -------------- -----------------
  gpm         *x* &gt; 0        *chCapDs* / 4000   No             constant

**chcpHdloss=*float***

Chiller condenser pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft H2O      *x* $\ge$ 0       45.78\*       No             constant

\* May be temporary default for 10-31-92 version; prior value (45) may be restored.

**chcpMotEff=*float***

Chiller condenser pump motor efficiency.

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .88           No             constant

**chcpHydEff=*float***

Chiller condenser pump hydraulic efficiency

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .70           No             constant

**chcpMtr=*name of a METER***

Meter to which condenser pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
              *name of a METER*   *none*        No             constant

The following four members permit specification of auxiliary input power use associated with the chiller under the conditions indicated.

**chAuxOn=*float***

Auxiliary power used when chiller is running, in proportion to its subhour average part load ratio (plr).

**chAuxOff=*float***

Auxiliary power used when chiller is not running, in proportion to 1 - plr.

**chAuxFullOff=*float***

Auxiliary power used only when chiller is off for entire subhour; not used if the chiller is on at all during the subhour.

**chAuxOnAtAll=*float***

Auxiliary power used in full value if chiller is on for any fraction of subhour.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 0       0             No             hourly

The following four allow specification of meters to record chiller auxiliary energy use through chAuxOn, chAuxOff, chFullOff, and chAuxOnAtAll, respectively. End use category "Aux" is used.

**chAuxOnMtr=*mtrName***

**chAuxOffMtr=*mtrName***

**chAuxFullOffMtr=*mtrName***

**chAuxOnAtAllMtr=*mtrName***

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**endChiller**

Optionally indicates the end of the CHILLER definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


