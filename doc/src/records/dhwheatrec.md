# DHWHEATREC

DHWHEATREC constructs an object representing one or more heat recovery devices in a DHWSYS. Heat recovered by the device increases parent DHWSYS wsInlet and thus reduces water heating load.

**wrName**

Optional name of device; give after the word “DHWHEATREC” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**wrMult=*integer***

Number of identical heat recovery devices of this type. Any value $>1$ is equivalent to repeated entry of the same DHWHEATREC.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              $>$ 0             1             No             constant

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

Specifies the type of heat recovery device: Vertical or Horizontal.  Currently, wrType has no effect on model results.

  **Units**   **Legal Range**      **Default**       *Required**    **Variability**
  ----------- -------------------- ---------------- -------------- -----------------
  --          one of above choices  Vertical            No           constant

**wrConfig=*choice***

Specifies the plumbing configuration:

* Equal: Potable-side heat exchanger output feeds both the fixture and the water heater inlet.  Potable and drain flow rates are equal assuming no other simultaneous hot water draws.
* UnequalWH: Potable-side heat exchanger output feeds the inlet(s) of water heater(s) that are part of the parent DHWSYS.  That is, wsInlet is adjusted to reflect recovered heat.
* UnequalFX: Potable-side heat exchanger output feeds *only* the associated fixture.


  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
                one of above choices    Equal              No             constant

**wrCSARatedEF=*float***

Specifies the heat recovery effectiveness determined using CSA xxx rating conditions.

  **Units**         **Legal Range**       **Default**   **Required**   **Variability**
  ----------------- -------------------- ------------- -------------- -----------------
  --                   0 $\le$ x $\le$ 1         --          Yes             constant


**endDHWHEATREC**

Optionally indicates the end of the DHWHEATREC definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[DHWHeatRec](#p_dhwtank)
