# IZXFER

IZXFER constructs an object that represents an interzone or zone/ambient heat transfer due to conduction and/or air transfer. The air transfer modeled by IZXFER transfers heat only; humidity transfer is not modeled as of July 2011. Note that SURFACE is the preferred way represent conduction between ZONEs.

The AIRNET types are used in a multi-cell pressure balancing model that finds zone pressures that produce net 0 mass flow into each zone. The model operates in concert with the znType=CZM or znType=UZM to represent ventilation strategies. During each time step, the pressure balance is found for two modes that can be thought of as “VentOff” (or infiltration-only) and “VentOn” (or infiltration+ventilation). The zone model then determines the ventilation fraction required to hold the desired zone temperature (if possible). AIRNET modeling methods are documented in the CSE Engineering Documentation.

Note that fan-driven types assume pressure-independent flow.  That is, the specified flow is included in the zone pressure balance but the modeled fan flow does not change with zone pressure.
The assumption is that in realistic configurations, zone pressure will generally be close to ambient pressure.  Unbalanced fan ventilation in a zone without relief area will result in runtime termination due to excessively high or low pressure.

**izName**

Optional name of interzone transfer; give after the word "IZXFER" if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**izNVType=*choice***

Choice determining interzone ventilation

  ---------------- ---------------------------------------
  NONE             No interzone ventilation

  ONEWAY           Uncontrolled flow from izZn1 to izZn2
                   when izZn1 air temperature exceeds
                   izZn2 air temperature
                   (using ASHRAE high/low vent model).

  TWOWAY           Uncontrolled flow in either direction
                   (using ASHRAE high/low vent model).

  AIRNETIZ         Single opening to another zone (using
                   pressure balance AirNet model).  Flow
                   is driven by buoyancy.

  AIRNETEXT        Single opening to ambient (using
                   pressure balance AirNet model).  Flow
                   is driven by buoyancy and wind pressure.

  AIRNETHORIZ      Horizontal (large) opening between two
                   zones, used to represent e.g.
                   stairwells.  Flow is driven by buoyancy;
                   simultaneous up and down flow is modeled.

  AIRNETEXTFAN     Fan from exterior to zone (flow either
                   direction).

  AIRNETIZFAN      Fan between two zones (flow either
                   direction).

  AIRNETEXTFLOW    Specified flow from exterior to zone
                   (either direction). Behaves identically
                   to AIRNETEXTFAN except no electricity
                   is consumed and no fan heat is added to
                   the air stream.

  AIRNETIZFLOW     Specified flow between two zones
                   (either direction). Behaves identically
                   to AIRNETIZFAN except no electricity is
                   consumed and no fan heat is added to
                   the air stream.

  AIRNETHERV       Heat or energy recovery ventilator.
                   Supply and exhaust air are exchanged
                   with the exterior with heat and/or
                   moisture exchange between the air streams.
                   Flow may or may not be balanced.
  ---------------- ---------------------------------------

  -----------------------------------------------------------------
  **Units** **Legal**      **Default** **Required** **Variability**
            **Range**
  --------- -------------- ----------- ------------ ---------------
            *choices above*   NONE        No           constant

  -----------------------------------------------------------------

**izZn1=*znName***

Name of primary zone. Flow rates $>$ 0 are into the primary zone.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              name of a ZONE                  Yes            constant

**izZn2=*znName***

Name of secondary zone.

  ---------------------------------------------------------------
  **Units** **Legal** **Default** **Required**    **Variability**
            **Range**
  --------- --------- ----------- --------------- ---------------
            name of a             required unless constant
            ZONE                  izNVType =
                                  AIRNETEXT,
                                  AIRNETEXTFAN,
                                  AIRNETEXTFLOW,
                                  or AIRNETHERV
  ---------------------------------------------------------------

Give izHConst for a conductive transfer between zones. Give izNVType other than NONE and the following variables for a convective (air) transfer between the zones or between a zone and outdoors. Both may be given if desired. Not known to work properly as of July 2011

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

Length or width of AIRNETHORIZ opening.

  **Units**   **Legal Range**   **Default**   **Required**                **Variability**
  ----------- ----------------- ------------- --------------------------- -----------------
  ft          *x* $>$ 0                       if izNVType = AIRNETHORIZ   constant

**izL2=*float***

Width or length of AIRNETHORIZ opening.

  **Units**   **Legal Range**   **Default**   **Required**                **Variability**
  ----------- ----------------- ------------- --------------------------- -----------------
  ft          *x* $>$ 0                       if izNVType = AIRNETHORIZ   constant

**izStairAngle=*float***

Stairway angle for AIRNETHORIZ opening. Use 90 for an open hole. Note that 0 prevents flow.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  degrees     *x* $>$ 0         34            No             constant

**izHD=*float***

Vent center-to-center height difference (for TWOWAY) or vent height above nominal 0 level (for AirNet types)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft                            0             No             constant

**izNVEff=*float***

Vent discharge coefficient.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1   0.8           No             constant

**izfanVfDs=*float***

Fan design or rated flow at rated pressure.  For AIRNETHERV, this is the net air flow into the zone, gross flow at the fan is derived using izEATR (see below).

  **Units**   **Legal Range**   **Default**   **Required**     **Variability**
  ----------- ----------------- ------------- ---------------- -----------------
  cfm         *x* $\ge$ 0       0 (no fan)    If fan present   constant

**izCpr=*float***

Wind pressure coefficient (for AIRNETEXT).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
               *x* $\ge$ 0       0.            No             constant

**izExp=*float***

Opening exponent (for AIRNETEXT).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  none        *x* $>$ 0         0.5           No             constant

**izVfMin=*float***

Minimum volume flow rate (VentOff mode).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  cfm         *x* $\ge$ 0        izfanVfDs             No             subhourly

**izVfMax=*float***

Maximum volume flow rate (VentOn mode)

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  cfm         *x* $\ge$ 0       izVfMin             No             subhourly


**izASEF=*float***

Apparent sensible effectiveness for AIRNETHERV ventilator.  ASEF is a commonly-reported HERV rating and is calculated as (supplyT - sourceT) / (returnT - sourceT).  This formulation includes fan heat (in supplyT), hence the term "apparent".  Ignored if izSRE is given.  CSE does not HRV exhaust-side condensation, so this model is approximate.

**Units**   **Legal Range**    **Default**   **Required**   **Variability**
----------- ------------------ ------------- -------------- -----------------
             0 $\le$ x $\le$ 1  0             No             subhourly

**izSRE=*float***

Sensible recovery efficiency (SRE) for AIRNETHERV ventilator.  Used as the sensible effectiveness in calculation of the supply air temperature.  Note that values of SRE greater than approximately 0.6 imply exhaust-side condensation under HVI rating conditions.  CSE does not adjust for these effects.  High values of izSRE will produce unrealistic results under mild outdoor conditions and/or dry indoor conditions.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
            0 $\le$ x $\le$ 1  0                No             subhourly

**izASRE=*float***

Adjusted sensible recovery efficiency (ASRE) for AIRNETHERV ventilator.  The difference izASRE - izSRE is used to calculate fan heat added to the supply air stream.  See izSRE notes.  No effect when izSRE is 0.

**Units**   **Legal Range**       **Default**   **Required**   **Variability**
----------- --------------------- ------------- -------------- -----------------
            0 $\le$ x $\le$ izSRE   0             No             subhourly

**izEATR=*float***

Exhaust air transfer ratio for AIRNETHERV ventilator.  NetFlow = (1 - EATR)*(grossFlow).

 **Units** **Legal Range**   **Default** **Required** **Variability**
 --------- ----------------- ----------- ------------ ---------------
 cfm       0 $\le$ x $\le$ 1 0           No           subhourly

**izLEF=*float***

Latent heat recovery effectiveness for AIRNETHERV ventilator.  The default value (0) results in sensible-only heat recovery.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              0 $\le$ x $\le$ 1       0             No             subhourly

**izRVFanHeatF=*float***

Fraction of fan heat added to supply air stream for AIRNETHERV ventilator.  Used only when when izSRE is 0 (that is, when izASEF specifies the sensible effectiveness).

**Units**   **Legal Range**       **Default**   **Required**   **Variability**
----------- --------------------- ------------- -------------- -----------------
            0 $\le$ x $\le$ 1      0             No             subhourly

**izVfExhRat=*float***

Exhaust volume flow ratio for AIRNETHERV ventilator = (exhaust flow) / (supply flow).  Any
value other than 1 indicates unbalanced flow that effects the zone pressure.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                 1 (balanced)      No             subhourly

**izfanPress=*float***

Design or rated fan pressure.

  **Units**      **Legal Range**   **Default**   **Required**   **Variability**
  -------------- ----------------- ------------- -------------- -----------------
  inches H~2~O   *x* $>$ 0         .3            No             constant

Only one of izfanElecPwr, izfanEff, and izfanShaftBhp may be given: together with izfanVfDs and izfanPress, any one is sufficient for CSE to determine the others and to compute the fan heat contribution to the air stream.

**izfanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

  ------------------------------------------------------------------
  **Units** **Legal** **Default**   **Required**     **Variability**
            **Range**
  --------- --------- ------------- ---------------- ---------------
  W/cfm     *x* $>$ 0 derived from  If izfanEff and  constant
                      izfanEff and  izfanShaftBhp
                      izfanShaftBhp not present
  ------------------------------------------------------------------

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

**izFanMtr=*mtrName***

Name of meter, if any, to record energy used by supply fan. End use category used is specified by izFanEndUse (next).

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**izFanEndUse=*choice***

End use to which fan energy is recorded (in METER specified by izFanMtr).  See METER for available end use choices.

**Units**   **Legal Range**     **Default**      **Required**   **Variability**
----------- ------------------- ---------------- -------------- -----------------
            *end use choice*       Fan             No             constant

**endIZXFER**

Optionally indicates the end of the interzone transfer definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[izXfer](#p_izxfer)
