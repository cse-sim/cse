# ZONE

ZONE constructs an object of class ZONE, which describes an area of the building to be modeled as having a uniform condition. ZONEs are large, complex objects and can have many subobjects that describe associated surfaces, shading devices, HVAC equipment, etc.

## ZONE General Members

**znName**

Name of zone. Enter after the word ZONE; no "=" is used.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        Yes            constant

**znModel=*choice***

Selects model for zone.

----------- ------------------------------
CNE         Older, central difference model based on
            original CALPAS methods.  Not fully supported
            and not suitable for current compliance applications.

CZM         Conditioned zone model.  Forward-difference,
            short time step methods are used.

UZM         Unconditioned zone model.  Identical to CZM
            except heating and cooling are not supported.
            Typically used for attics, garages, and other
            ancillary spaces.

----------- ------------------------------


  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *choices above*      CNE           No             constant

**znArea=*float***

Nominal zone floor area.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^2^       *x* $>$ 0         *none*        Yes            constant

**znVol=*float***

Nominal zone volume.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^3^       *x* $>$ 0         *none*        Yes            constant

**znAzm=*float***

Zone azimuth with respect to bldgAzm. All surface azimuths are relative to znAzm, so that the zone can be rotated by changing this member only. Values outside the range 0^o^ to 360^o^ are normalized to that range.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
  degrees     unrestricted      0             No             constant

**znFloorZ=*float***

Nominal zone floor height relative to arbitrary 0 level. Used re determination of vent heights <!-- TODO -->

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          unrestricted      *0*           No             constant

**znCeilingHt=*float***

Nominal zone ceiling height relative to zone floor (typically 8 – 10 ft).

  **Units**   **Legal Range**   **Default**          **Required**   **Variability**
  ----------- ----------------- -------------------- -------------- -----------------
  ft          *x* $>$ 0         *znVol* / *znArea*   No             constant

**znEaveZ=*float***

Nominal eave height above ground level. Used re calculation of local surface wind speed. This in turn influences outside convection coefficients in some surface models and wind-driven air leakage.

  **Units**   **Legal Range**   **Default**                  **Required**   **Variability**
  ----------- ----------------- ---------------------------- -------------- -----------------
  ft          *x* $\ge$ 0       *znFloorZ + infStories\*8*   No             constant

**znCAir=*float***

Zone "air" heat capacity: represents heat capacity of air, furniture, "light" walls, and everything in zone except surfaces having heat capacity (that is, non-QUICK surfaces).

  **Units**   **Legal Range**   **Default**       **Required**   **Variability**
  ----------- ----------------- ----------------- -------------- -----------------
  Btu/^o^F    *x* $\geq$ 0      3.5 \* *znArea*   No             constant

**znHcAirX=*float***

Zone air exchange rate used in determination of interior surface convective coefficients.  This item is generally used only for model testing.

**Units**   **Legal Range**   **Default**                   **Required**   **Variability**
----------- ----------------- ---------------------------- -------------- -----------------
  ACH          x $\ge$ 0       as modeled                       No        subhourly

**znHcFrcF=*float***

Zone surface forced convection factor.  Interior surface convective transfer is modeled as a combination of forced and natural convection.  hcFrc = znHcFrcF * znHcAirX^.8.  See CSE Engineering Documentation.

**Units**           **Legal Range**   **Default**   **Required**   **Variability**
------------------ ----------------- ------------ -------------- -----------------
Btuh/ft^2^-^o^F                        .2                No         hourly


**znHIRatio=*float***

Zone hygric inertia ratio.  In zone moisture balance calculations, the effective dry-air mass = znHIRatio * (zone dry air mass).  This enhancement can be used to represente the moisture storage capacity of zone surfaces and contents.

**Units**   **Legal Range**   **Default**       **Required**   **Variability**
----------- ----------------- ----------------- -------------- -----------------
             *x* &gt; 0         1                    No             constant

**znSC=*float***

Zone shade closure. Determines insolation through windows (see WINDOW members *wnSCSO* and *wnSCSC*) and solar gain distribution: see SGDIST members *sgFSO* and *sgFSC*. 0 represents shades open; 1 represents shades closed; intermediate values are allowed. An hourly variable CSE expression may be used to schedule shade closure as a function of weather, time of year, previous interval HVAC use or zone temperature, etc.

  **Units**   **Legal Range**         **Default**                                          **Required**   **Variability**
  ----------- ----------------------- ---------------------------------------------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1   1 when cooling was used in *previous* hour, else 0   No             hourly


**znTH=*float***

Heating set point for znModel=CZM.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* $\geq$ 0                                   hourly

**znTD=*float***

Desired set point (temperature maintained with ventilation if possible) for znModel=CZM

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* $\geq$ 0                                   hourly

**znTC=*float***

Cooling set point for znModel=CZM.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* $\geq$ 0                                   Hourly

CZM zone heating and cooling is provided either via an RSYS HVAC system or by "magic" heat transfers specified by znQxxx items.

**znRSys=*rsysName***

Name of RSYS providing heating, cooling, and optional central fan integrated ventilation to this zone.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
              *RSYS name*      (no RSYS)      No              constant


**znQMxH=*float***

Heating capacity at current conditions

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        *x* $\geq$ 0                                   hourly

**znQMxHRated=*float***

Rated heating capacity

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        *x* $\geq$ 0                                   constant

**znQMxC=*float***

Cooling capacity at current conditions

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        *x* $\leq$ 0                                   hourly

**znQMxCRated=*float***

Rated cooling capacity

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  Btuh        *x* $\leq$ 0                                   constant

<% if comfort_model %>
The following provide parameters for comfort calculations

**znComfClo=*float***

Occupant clothing resistance

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  clo                                                        subhourly ??

**znComfMet=*float***

Occupant metabolic rate

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  met         *x* $\geq$ 0                                   hourly

**znComfAirV=*float***

Nominal air velocity used for comfort model

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ?           *x* $\geq$ 0                    No             Hourly

**znComfRh=*float***

Nominal zone relative humidity used for comfort model

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1                 No             hourly
<% end %>

## ZONE Infiltration

The following control a simplified air change plus leakage area model. The Sherman-Grimsrud model is used to derive air flow rate from leakage area and this rate is added to the air changes specified with infAC. Note that TOP.windF does *not* modify calculated infiltration rates, since the Sherman-Grimsrud model uses its own modifiers. See also AirNet models available via IZXFER.

**infAC=*float***

Zone infiltration air changes per hour.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  1/hr        *x* $\geq$ 0      0.5           No             hourly

**infELA=*float***

Zone effective leakage area (ELA).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  in^2^       *x* $\geq$ 0      0.0           No             hourly

**infShld=*int***

Zone local shielding class, used in derivation of local wind speed for ELA infiltration model, wind-driven AirNet leakage, and exterior surface coefficients. infShld values are --

  ---------- -----------------------------------------------------------
  1          no obstructions or local shielding

  2          light local shielding with few obstructions

  3          moderate local shielding, some obstructions within two
             house heights

  4          heavy shielding, obstructions around most of the perimeter

  5          very heavy shielding, large obstructions surrounding the
             perimeter within two house heights
  ---------- -----------------------------------------------------------

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              1 $\leq$ *x* $\leq$ 5   3             No             constant

**infStories=*int***

Number of stories in zone, used in ELA model.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              1 $\leq$ *x* $\leq$ 3   1             No             constant

**znWindFLkg=*floatTODO***

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                1             No             constant

## ZONE Exhaust Fan

Presence of an exhaust fan in a zone is indicated by specifying a non-zero design flow value (xfanVfDs).

Zone exhaust fan model implementation is incomplete as of July, 2011. The current code calculates energy use but does not account for the effects of air transfer on room heat balance. IZXFER provides a more complete implementation.

**xfanFOn=*float***

Exhaust fan on fraction. On/off control assumed, so electricity requirement is proportional to run time.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
  fraction    0 $\leq$ x $\leq$ 1   1             No             hourly

Example: The following would run an exhaust fan 70% of the time between 8 AM and 5 PM:

        xfanFOn = select( (\$hour >= 7 && \$hour < 5), .7,
                                              default, 0 );

**xfanVfDs=*float***

Exhaust fan design flow; 0 or not given indicates no fan.

  **Units**   **Legal Range**   **Default**   **Required**     **Variability**
  ----------- ----------------- ------------- ---------------- -----------------
  cfm         *x* 0             0 (no fan)    If fan present   constant

**xfanPress=*float***

Exhaust fan external static pressure.

  **Units**      **Legal Range**              **Default**   **Required**   **Variability**
  -------------- ---------------------------- ------------- -------------- -----------------
  inches H~2~O   0.05 $\leq$ *x* $\leq$ 1.0   0.3           No             constant

Only one of xfanElecPwr, xfanEff, and xfanShaftBhp may be given: together with xfanVfDs and xfanPress, any one is sufficient for CSE to detemine the others and to compute the fan heat contribution to the air stream.

**xfanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

  -------------------------------------------------------------------------
  **Units** **Legal** **Default**          **Required**     **Variability**
            **Range**
  --------- --------- -------------------- ---------------- ---------------
  W/cfm     *x* $>$ 0 derived from xfanEff If xfanEff and   constant
                      and xfanShaftBhp     xfanShaftBhp not
                                           present
  -------------------------------------------------------------------------

**xfanEff=*float***

Exhaust fan/motor/drive combined efficiency.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
  fraction    0 $\leq$ x $\leq$ 1   0.08          No             constant

**xfanShaftBhp=*float***

Fan shaft power at design flow and pressure.

  ------------------------------------------------------------------
  **Units** **Legal** **Default**     **Required**   **Variability**
            **Range**
  --------- --------- --------------- -------------- ---------------
  BHP       *x* $>$ 0 derived from    If xfanElecPwr constant
                      xfanElecPwr and not present
                      xfanVfDs
  ------------------------------------------------------------------

**xfanMtr=*mtrName***

Name of METER object, if any, by which fan's energy use is recorded (under end use category "fan").

  **Units**   **Legal Range**     **Default**      **Required**   **Variability**
  ----------- ------------------- ---------------- -------------- -----------------
              *name of a METER*   *not recorded*   No             constant

**endZone**

Indicates the end of the zone definition. Alternatively, the end of the zone definition can be indicated by the declaration of another object or by "END". If END or endZone is used, it should follow the definitions of the ZONE's subobjects such as GAINs, SURFACEs, TERMINALs, etc.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- [@zone](#p_zone)
- [@znRes](#p_znRes) (accumulated results)
