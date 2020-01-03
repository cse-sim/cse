# DHWSOLARSYS

Solar water heating system.

- DHWSOLARSYS
    - DHWSOLARCOLLECTOR
    - DHWSOLARTANK

May have any number of solar collectors, but only one tank.

May have no tank for direct system? What if system has multiple primary tanks?

**swName**

**swFluid**

Fluid used in the water heater system.

**swSSF=*float***

Specifies the solar savings fraction, allowing recognition of externally-calculated solar water heating energy contributions.  The contributions are modeled by deriving an increased water heater feed temperature of any DHWSYS that references this DHWSOLARSYS --

$$tWHFeed = tInletAdj + swSSF*(wsTUse-tInletAdj)$$

where tInletAdj is the source cold water temperature *including any DHWHEATREC tempering* (that is, wsTInlet + heat recovery temperature increase, if any).  This model approximates the diminishing returns associated with combined preheat strategies such as drain water heat recovery and solar.

  **Units**    **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
                0 $\le$ x $\le$ 0.99           0             No             hourly

**endDHWSOLARSYS**
