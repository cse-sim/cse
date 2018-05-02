# BOILER

BOILERs are subObjects of HEATPLANTs (preceding Section 5.20). BOILERs supply heat, through their associated HEATPLANT, to HW coils and heat exchangers.

Each boiler has a pump. The pump operates whenever the boiler is in use; the pump generates heat in the water, which is added to the boiler's output. The pump heat is independent of load -- the model assumes a bypass valve keeps the water flow constant when the loads are using less than full flow -- except that the heat is assumed never to exceed the load.

**boilerName**

Name of BOILER object, given immediately after the word BOILER. The name is used to refer to the boiler in heat plant stage commands.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*                 Yes            constant

**blrCap=*float***

Heat output capacity of this BOILER.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        *x* &gt; 0                      Yes            constant

**blrEffR=*float***

Boiler efficiency at steady-state full load, as a fraction. 1.0 may be specified to model a 100% efficient boiler.

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .80           No             constant

**blrEirR=*float***

Boiler Energy Input Ratio: alternate method of specifying efficiency.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              x $\ge$ 1.0       1/*blrEffR*   No             constant

**blrPyEi=*a, b, c, d***

Coefficients of cubic polynomial function of part load ratio (load/capacity) to adjust full-load energy input for part load operation. Up to four floats may be given, separated by commas, lowest order (i.e. constant term) coefficient first. If the given coefficients result in a polynomial whose value is not 1.0 when the input variable, part load ratio, is 1.0, a warning message will be printed and the coefficients will be normalized to produce value 1.0 at input 1.0.

  **Units**   **Legal Range**   **Default**                     **Required**   **Variability**
  ----------- ----------------- ------------------------------- -------------- -----------------
                                .082597, .996764, 0.79361, 0.   No             constant

**blrMtr=*name of a METER***

Meter to which Boiler's input energy is accumulated; if omitted, input energy is not recorded.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              name of a METER   *none*        No             constant

**blrpGpm=*float***

Boiler pump flow in gallons per minute: amount of water pumped from this boiler through the hot water loop supplying the HEATPLANT's loads (HW coils and heat exchangers) whenever boiler is operating.

  **Units**   **Legal Range**   **Default**    **Required**   **Variability**
  ----------- ----------------- -------------- -------------- -----------------
  gpm         *x* &gt; 0        blrCap/10000   No             constant

**blrpHdloss=*float***

Boiler pump head loss (pressure). 0 may be specified to eliminate pump heat and pump energy input.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft H2O      *x* $\ge$ 0       114.45\*      No             constant

\* may be temporary value for 10-31-92 version; prior value of 35 may be restored.

**blrpMotEff=*float***

Boiler pump motor efficiency.

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .88           No             constant

**blrpHydEff=*float***

Boiler pump hydraulic efficiency

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0   .70           No             constant

**blrpMtr=*name of a METER***

Meter to which pump electrical input energy is accumulated. If omitted, pump input energy use is not recorded.

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
              *name of a METER*   *none*        No             constant

The following four members permit specification of auxiliary input power use associated with the boiler under the conditions indicated.

  --------------------------- -------------------------------------------
  blrAuxOn=*float*            Auxiliary power used when boiler is
                              running, in proportion to its subhour
                              average part load ratio (plr).

  blrAuxOff=*float*           Auxiliary power used when boiler is not
                              running, in proportion to 1 - plr.

  blrAuxFullOff=*float*       Auxiliary power used only when boiler is
                              off for entire subhour; not used if the
                              boiler is on at all during the subhour.

  blrAuxOnAtAll=*float*       Auxiliary power used in full value if
                              boiler is on for any fraction of subhour.
  --------------------------- -------------------------------------------

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *x* $\ge$ 0       0             No             hourly

The following four allow specification of meters to record boiler auxiliary energy use through blrAuxOn, blrAuxOff, blrFullOff, and blrAuxOnAtAll, respectively. End use category "Aux" is used.

**blrAuxOnMtr=*mtrName***

**blrAuxOffMtr=*mtrName***

**blrAuxFullOffMtr=*mtrName***

**blrAuxOnAtAllMtr=*mtrName***

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**endBoiler**

Optionally indicates the end of the boiler definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


**Related Probes:**

- @[boiler](#p_boiler)
