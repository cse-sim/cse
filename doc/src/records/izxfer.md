# IZXFER

IZXFER constructs an object that represents an interzone or zone/ambient heat transfer due to conduction and/or air transfer. The air transfer modeled by IZXFER transfers heat only; humidity transfer is not modeled as of July 2011. Note that SURFACE is the preferred way represent conduction between ZONEs.

The AIRNET types are used in a multi-cell pressure balancing model that finds zone pressures that produce net 0 mass flow into each zone. The model operates in concert with the znType=CZM to represent ventilation strategies. During each time step, the pressure balance is found for two modes that can be thought of as “VentOff” (or infiltration-only) and “VentOn” (or infiltration+ventilation). The zone model then determines the ventilation fraction required to hold the desired zone temperature (if possible).

**izName**

Optional name of interzone transfer; give after the word "IZXFER" if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**izNVType=*choice***

Choice determining interzone ventilation

  ---------------- ---------------------------------------
  NONE             No interzone ventilation

  TWOWAY           Uncontrolled flow in either direction
                   (using ASHRAE high/low vent model)

  AIRNETIZ         Single opening to another zone (using
                   pressure balance AirNet model)

  AIRNETEXT        Single opening to ambient (using
                   pressure balance AirNet model)

  AIRNETHORIZ      Horizontal (large) opening between two
                   zones, used to represent e.g.
                   stairwells

  AIRNETEXTFAN     Fan from exterior to zone (flow either
                   direction).

  AIRNETIZFAN      Fan between two zones (flow either
                   direction).

  AIRNETEXTFLOW    Specified flow from exterior to zone
                   (either direction). Behaves identically
                   to AIRNETEXTFAN except no electricity
                   is consumed and no fan heat is added to
                   the air stream.

  AIRNETIZFLOW     Specicified flow between two zones
                   (either direction). Behaves identically
                   to AIRNETIZFAN except no electricity is
                   consumed and no fan heat is added to
                   the air stream
  ---------------- ---------------------------------------

  -----------------------------------------------------------------------
  **Uni **Legal Range**                             **Defa **Requ **Varia
  ts**                                              ult**  ired** b
                                                                  ility**
  ----- ------------------------------------------- ------ ------ -------
        NONE, TWOWAY, AIRNET, AIRNETEXT,            None   No     constan
        AIRNETHORIZ, AIRNETEXTFAN, AIRNETIZFAN,                   t
        AIRNETEXTFLOW, AIRNETIZFLOW                               
  -----------------------------------------------------------------------

**izZn1=*znName***

Name of primary zone. Flow rates $>$ 0 are into the primary zone.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              name of a ZONE                  Yes            constant

**izZn2=*znName***

Name of secondary zone.

  ----------------------------------------------------------------------
  **Unit **Legal   **Defau **Required**                         **Variab
  s**    Range**   lt**                                         i
                                                                lity**
  ------ --------- ------- ------------------------------------ --------
         name of a         required unless izNVType =           constant
         ZONE              AIRNETEXT,AIRNETEXTFAN,              
                           orAIRNETEXTFLOW                      
  ----------------------------------------------------------------------

Give izHConst for a conductive transfer between zones. Give izNVType other than NONE and the following variables for a convective (air) transfer between the zones or between a zone and outdoors. Both may be given if desired. ?? Not known to work properly as of July 2011

**izHConst=*float***

Conductance between zones.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btu/^o^F    *x* $\ge$ 0       0             No             hourly

**izALo=*float***

Area of low or only vent (typically VentOff)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^2^       *x* $\ge$ 0       0             No             hourly

**izAHi=*float***

Additional vent area (high vent or VentOn). If used in AIRNET, izAHi &gt; izALo typically but this is not required.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^2^       *x* $\ge$ 0       izALo         No             hourly

**izL1=*float***

Length or width of AIRNETHORIZ opening

  **Units**   **Legal Range**   **Default**   **Required**                **Variability**
  ----------- ----------------- ------------- --------------------------- -----------------
  ft          *x* $>$ 0                       if izNVType = AIRNETHORIZ   constant

**izL2=*float***

width or length of AIRNETHORIZ opening

  **Units**   **Legal Range**   **Default**   **Required**                **Variability**
  ----------- ----------------- ------------- --------------------------- -----------------
  ft          *x* $>$ 0                       if izNVType = AIRNETHORIZ   constant

**izStairAngle=*float***

stairway angle for AIRNETHORIZ opening. Use 90 for open hole. Note that 0 prevents flow.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  degrees     *x* $>$ 0         34            No             constant

**izHD=*float***

Vent center-to-center height difference (for TWOWAY) or vent height above nominal 0 level (for AirNet types)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft                            0             No             constant

**izNVEff=*float***

Vent discharge coefficient coefficient.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1   0.8           No             constant

**izfanVfDs=*float***

Fan design or rated flow at rated pressure

  **Units**   **Legal Range**   **Default**   **Required**     **Variability**
  ----------- ----------------- ------------- ---------------- -----------------
  cfm         *x* $\ge$ 0       0 (no fan)    If fan present   constant

**izCpr=*float***

Wind pressure coefficient (for AIRNETEXT)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ??          *x* $\ge$ 0       0.            No             constant

**izExp=*float***

Opening exponent

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  none        *x* $>$ 0         0.5           No             constant

**izfanVfMin=*float***

Minimum volume flow rate (VentOff mode)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  cfm         *x* $\ge$ 0       0             No             subhourly

**izfanVfMax=*float***

Maximum volume flow rate (VentOn mode)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  cfm         *x* $\ge$ 0       0             No             subhourly

**izfanPress=*float***

Design or rated pressure.

  **Units**      **Legal Range**   **Default**   **Required**   **Variability**
  -------------- ----------------- ------------- -------------- -----------------
  inches H~2~O   *x* $>$ 0         .3            No             constant

Only one of izfanElecPwr, izfanEff, and izfanShaftBhp may be given: together with izfanVfDs and izfanPress, any one is sufficient for CSE to detemine the others and to compute the fan heat contribution to the air stream.

**izfanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

  ------------------------------------------------------------------------
  **Unit **Legal   **Default**            **Required**            **Variab
  s**    Range**                                                  i
                                                                  lity**
  ------ --------- ---------------------- ----------------------- --------
  W/cfm  *x* $>$ 0 derived from izfanEff  If izfanEff and         constant
                   and izfanShaftBhp      izfanShaftBhp not       
                                          present                 
  ------------------------------------------------------------------------

**izfanEff=*float***

Fan efficiency at design flow and pressure, as a fraction.

  **Units**   **Legal Range**       **Default**                                        **Required**   **Variability**
  ----------- --------------------- -------------------------------------------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1   derived from *izfanShaftBhp* if given, else 0.08   No             constant

**izfanShaftBhp=*float***

Fan shaft brake horsepower at design flow and pressure.

  **Units**   **Legal Range**   **Default**                **Required**   **Variability**
  ----------- ----------------- -------------------------- -------------- -----------------
  bhp         *x* $>$ 0         derived from *izfanEff*.   No             constant

**izfanCurvePy=$k_0$, $k_1$, $k_2$, $k_3$, $x_0$**

$k_0$ through $k_3$ are the coefficients of a cubic polynomial for the curve relating fan relative energy consumption to relative air flow above the minimum flow $x_0$. Up to five *floats* may be given, separated by commas. 0 is used for any omitted trailing values. The values are used as follows:

$$z = k_0 + k_1 \cdot (x - x_0)|  +  k_2 \cdot (x - x_0)|^2 + k_3 \cdot (x - x_0)|^3$$

where:

-   $x$ is the relative fan air flow (as fraction of izfan*VfDs*; 0 $\le$ $x$ $\le$ 1);
-   $x_0$ is the minimum relative air flow (default 0);
-   $(x - x_0)|$ is the "positive difference", i.e. $(x - x_0)$ if $x > x_0$; else 0;
-   $z$ is the relative energy consumption.

If $z$ is not 1.0 for $x$ = 1.0, a warning message is displayed and the coefficients are normalized by dividing by the polynomial's value for $x$ = 1.0.

  **Units**   **Legal Range**   **Default**                **Required**   **Variability**
  ----------- ----------------- -------------------------- -------------- -----------------
                                *0, 1, 0, 0, 0 (linear)*   No             constant

**izfanMtr=*mtrName***

Name of meter, if any, to record energy used by supply fan. End use category used is "Fan".

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**endIzxfer**

Optionally indicates the end of the interzone transfer definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


