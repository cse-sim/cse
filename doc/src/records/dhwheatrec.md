# DHWHEATREC

DHWHEATREC constructs an object representing one or more heat recovery devices in a DHWSYS. Heat recovered by the device increases parent DHWSYS wsInlet and thus reduces water heating load.

**wrName**

Optional name of device; give after the word “DHWHEATREC” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wrMult=*integer***

Number of identical heat recovery devices of this type. Any value >1 is equivalent to repeated entry of the same DHWHEATREC.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0                1             No             constant

**wrHWEndUse=*choice***

Hot water end use to which this DHWHEATREC is applied: one of Shower, Bath, CWashr, DWashr, or Faucet.  Selects DHWUSE draws for heat recovery calculations.  Currently, only Shower is supported.


**Units**   **Legal Range**       **Default**                 **Required**  **Variability**
----------- --------------------  --------------------------- ------------- -------------------------
  --        Shower                  Shower                         No          constant


**wrFxServed=*integer***

  Number of fixtures (of type wrHWEndUse) whose drain lines pass through this heat recovery device.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                $>$ 0             1             No             constant


**wrType=*choice***

Specifies the type of heat recovery device: Vertical, Horizontal, or SetEF.  Horizontal and Vertical derive effectiveness from wrCSARatedEF, flow rates, and water temperatures.  As of Feb. 2019, the same correlation is used for both Horizontal and Vertical, so these choices have effect on results.  Choice SetEF uses wrCSARatedEF without modification as the effectiveness (note hourly variability).

  **Units**   **Legal Range**      **Default**       **Required**    **Variability**
  ----------- -------------------- ---------------- -------------- -----------------
  --          one of above choices  Vertical            No           constant

  **wrCSARatedEF=*float***

Specifies the heat recovery effectiveness, generally determined using CSA B55.2 rating conditions.  This value is modified during simulation based on water flow rates and temperatures.  If wrType=SetEF, wsCSARatedEF is used without modification. 

  **Units**         **Legal Range**       **Default**   **Required**   **Variability**
  ----------------- -------------------- ------------- -------------- -----------------
  --                   0 $\le$ x $\le$ 1         --          Yes             hourly

**wrConfig=*choice***

Specifies the plumbing configuration:

* Equal: Potable-side heat exchanger output feeds both the fixture and the water heater inlet.  Potable and drain flow rates are equal assuming no other simultaneous hot water draws.
* UnequalWH: Potable-side heat exchanger output feeds the inlet(s) of water heater(s) that are part of the parent DHWSYS.  That is, wsInlet is adjusted to reflect recovered heat.
* UnequalFX: Potable-side heat exchanger output feeds *only* the associated fixture.


  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
                one of above choices    Equal              No             constant


**wrTDInDiff=*float***

Temperature drop between the fixture drain and DHWHEATREC drain-side inlet.  The drain-side inlet temperature is thus DHWUSE wuTemp - wrTDInDiff.

  **Units**          **Legal Range**      **Default**   **Required**   **Variability**
  ----------------- -------------------- ------------- -------------- -----------------
    ^o^F                 $\ge$ 0            4.6 ^o^F            N             hourly

**wrTDInWarmup=*float***

Drain-side inlet water temperature during warmup.  During the warmup portion of a draw (if any), the drain-side inlet temperature will initially be lower than that based on DHWUSE wuTemp.  wrTDInWarmup allows input of user estimates for this temperature.  Note wrTDInWarmup is *not* adjusted by wrTDInDiff.

  **Units**          **Legal Range**      **Default**   **Required**   **Variability**
  ----------------- -------------------- ------------- -------------- -----------------
    ^o^F                 $\gt$ 0            65 ^o^F            N             hourly

**endDHWHEATREC**

Optionally indicates the end of the DHWHEATREC definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**
