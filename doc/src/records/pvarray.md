# PVARRAY

PVARRAY describes a photovoltaic panel system. The algorithms are based on the [PVWatts calculator](http://www.bwilcox.com/BEES/docs/Dobos%20-%20PVWatts%20v5.pdf).

**pvName**

Name of photovoltaic array. Give after the word PVARRAY.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**pvElecMeter=*choice***

Name of meter by which this PVARRAY's AC power out is recorded. Generated power is expressed as a negative value.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              meter name        *none*        No             constant

**pvEndUse=*choice***

Meter end use to which the PVARRAY's generated energy should be accumulated.

  ------- -------------------------------------------------------------
  Clg     Cooling
  Htg     Heating (includes heat pump compressor)
  HPHTG   Heat pump backup heat
  DHW     Domestic (service) hot water
  DHWBU   Domestic (service) hot water heating backup (HPWH resistance)
  FANC    Fans, AC and cooling ventilation
  FANH    Fans, heating
  FANV    Fans, IAQ venting
  FAN     Fans, other purposes
  AUX     HVAC auxiliaries such as pumps
  PROC    Process
  LIT     Lighting
  RCP     Receptacles
  EXT     Exterior lighting
  REFR    Refrigeration
  DISH    Dishwashing
  DRY     Clothes drying
  WASH    Clothes washing
  COOK    Cooking
  USER1   User-defined category 1
  USER2   User-defined category 2
  PV      Photovoltaic power generation
  ------- -------------------------------------------------------------

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              *Codes listed above*   PV            No             constant

**pvDCSysSize=*float***

The rated photovoltaic system DC capacity/size as indicated by the nameplate.

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
  kW          *x* &ge; 0          *none*        Yes            constant

**pvModuleType=*choice***

Type of module to model. The module type determines the refraction index and temperature coefficient used in the simulation. Alternatively, the "Custom" module type may be used in conjunction with user-defined input for *pvCoverRefrInd* and *pvTempCoeff*.

  **Module Type** **pvCoverRefrInd** **pvTempCoeff**
  --------------- ------------------ ---------------
  Standard        1.0                -0.0026
  Premium         1.3                -0.0019
  ThinFilm        1.0                -0.0011
  Custom          User-defined       User-defined

  ------------------------------------------------------------------------------
  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
              Standard \          Standard      No             constant
              Premium \
              ThinFilm \
              Custom
  ------------------------------------------------------------------------------

**pvCoverRefrInd=*float***

The refraction index for the coating applied to the module cover. A value of 1.0 represents refraction through air. Coatings have higher refraction indexes that capture more solar at lower angles of incidence.

  ------------------------------------------------------------------------------
  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
              *x* &ge; 1.0        1.0           No             constant
  ------------------------------------------------------------------------------

**pvTempCoeff=*float***

The temperature coefficient how the efficiency of the module varies with the cell temperature. Values are typically negative.

  ------------------------------------------------------------------------------
  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
  1/^o^F      *no restrictions*   -0.0026       No             constant
  ------------------------------------------------------------------------------

**pvArrayType=*choice***

The type of array describes mounting and tracking options. Roof mounted arrays have a higher installed nominal operating cell temperature (INOCT) of 120 ^o^F compared to the default of 113 ^o^F. Array self-shading is not currently calculated for adjacent rows of modules within an array.

  --------------------------------------------------------------------------------
  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- -------------------   ------------- -------------- -----------------
              FixedOpenRack \       FixedOpenRack No             constant
              FixedRoofMount \
              OneAxisTracking \
              TwoAxisTracking
  --------------------------------------------------------------------------------
  <!-- Hide              OneAxisBacktracking \ -->

**pvTilt=*float***

The tilt of the photovoltaic array from horizontal.  Values outside the range 0 to 360 are first normalized to that range. For one-axis tracking, defines the tilt of the rotation axis. Not used for two-axis tracking arrays.

  --------------------------------------------------------------------------------
  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- -------------------   ------------- -------------- -----------------
  degrees     unrestricted          0.0           No             hourly
  --------------------------------------------------------------------------------

**pvAzm=*float***

Photovoltaic array azimuth (0 = north, 90 = east, etc.). If a value outside the range 0° $\leq$ *x* $<$ 360° is given, it is normalized to that range. For one-axis tracking, defines the azimuth of the rotation axis. Not used for two-axis tracking arrays.

  --------------------------------------------------------------------------------
  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- -------------------   ------------- -------------- -----------------
  degrees     unrestricted          0.0           No             hourly
  --------------------------------------------------------------------------------

**pvGrndRefl=*float***

Ground reflectance used for calculating reflected solar incidence on the array.

  ----------------------------------------------------------------------------------
  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ---------------------   ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0    0.2           No             hourly
  ----------------------------------------------------------------------------------

<!-- Hide
**pvGCR=*float***

Ground coverage ratio is. This is currently unused as array self-shading is not calculated.

  ----------------------------------------------------------------------------------
  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ---------------------   ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0    0.4           No             constant
  ----------------------------------------------------------------------------------

-->

**pvDCtoACRatio=*float***

DC-to-AC ratio used to intentionally undersize the AC inverter. This is used to increase energy production in the beginning and end of the day despite the possibility of clipping peak sun hours.

  ----------------------------------------------------------------------------------
  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ---------------------   ------------- -------------- -----------------
              *x* > 0.0               1.1           No             constant
  ----------------------------------------------------------------------------------

**pvInverterEff=*float***

AC inverter efficiency at rated DC power.

  ----------------------------------------------------------------------------------
  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ---------------------   ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0    0.96          No             constant
  ----------------------------------------------------------------------------------

**pvSysLosses=*float***

Fraction of total DC energy lost. The total loss from a system is aggregated from several possible causes as illustrated below:

  **Loss Type**             **Default Assumption**
  ------------------------- ----------------------
  Soiling                   0.02
  Shading                   0.03
  Snow                      0
  Mismatch                  0.02
  Wiring                    0.02
  Connections               0.005
  Light-induced degradation 0.015
  Nameplate rating          0.01
  Age                       0
  Availability              0.03
  **Total**                 **0.14**

  ----------------------------------------------------------------------------------
  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ---------------------   ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0    0.14          No             hourly
  ----------------------------------------------------------------------------------

**endPVARRAY**

Optionally indicates the end of the PVARRAY definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
