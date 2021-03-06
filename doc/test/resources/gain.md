# GAIN

A GAIN object adds sensible and/or latent heat to the ZONE, and/or adds arbitrary energy use to a METER. GAINs are subobjects of ZONEs and are normally given within the input for their ZONE (also see ALTER). As many GAINs as desired (or none) may be given for each ZONE.

Each gain has an amount of power (gnPower), which may optionally be accumulated to a METER (gnMeter). The power may be distributed to the zone, plenum, or return as sensible heat with an optionl fraction radiant, or to the zone as latent heat (moisture addition), or not.

**gnName**

Name of gain; follows the word GAIN if given.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**gnPower=*float***

Rate of heat addition to zone. Negative gnPower values may be used to represent heat removal. Expressions containing functions are commonly used with this member to schedule the gain power on a daily and/or hourly basis. Refer to the functions section in Section 4 for details and examples.

All gains, including electrical, are specified in Btuh units. But note that internal gain and meter reporting is in MBtu (millions of Btu) by default even though input is in Btuh.

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
  Btuh        *no restrictions*   *none*        Yes            hourly

**gnMeter=*choice***

Name of meter by which this GAIN's gnPower is recorded. If omitted, gain is assigned to no meter and energy use is not accounted in CSE simulation reports; thus, gnMeter should only be omitted for "free" energy sources.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              meter name        *none*        No             constant

**gnEndUse=*choice***

Meter end use to which the GAIN's energy use should be accumulated.

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

  **Units**   **Legal Range**        **Default**   **Required**                   **Variability**
  ----------- ---------------------- ------------- ------------------------------ -----------------
              *Codes listed above*   *none*        Required if gnMeter is given   constant

The gnFrZn, gnFrPl, and gnFrRtn members allow you to allocate the gain among the zone, the zone's plenum, and the zone's return air flow. Values that total to more than 1.0 constitute an error. If they total less than 1, the unallocated portion of the gain is recorded by the meter (if specified) but not transferred into the building. By default, all of the gain not directed to the return or plenum goes to the zone.

**gnFrZn=*float***

Fraction of gain going to zone. gnFrLat (below) gives portion of this gain that is latent, if any; the remainder is sensible.

  **Units**   **Legal Range**                      **Default**                        **Required**   **Variability**
  ----------- ------------------------------------ ---------------------------------- -------------- -----------------
              gnFrZn + gnFrPl + gnFrRtn $\leq$ 1   *1 - gnFrLat - gnFrPl - gnFrRtn*   No             hourly

**gnFrPl=*float***

Fraction of gain going to plenum. Plenums are not implementented as of August, 2012. Any gain directed to the plenum is discarded.

**gnFrRtn=*float***

Fraction of gain going to return. The return fraction model is not implemented as of August, 2012. Any gain directed to the return is discarded.

  **Units**   **Legal Range**                      **Default**   **Required**   **Variability**
  ----------- ------------------------------------ ------------- -------------- -----------------
              gnFrZn + gnFrPl + gnFrRtn $\leq$ 1   0.            No             hourly

The gain to the zone may be further divided into convective sensible, radiant sensible and latent heat via the gnFrRad and gnFrLat members; the plenum and return gains are assumed all convective sensible.

<!-- TODO: verify / update when implemented -->
**Gain Modeling in CR zone**

In the CNE zone mode, the radiant internal gain is distributed to the surfaces in the zone, rather than going directly to the zone "air" heat capacity (znCAir). A simple model is used -- all surfaces are assumed to be opaque and to have the same (infrared) absorptivity -- even windows. Along with the assumption that the zone is spherical (implicit in the current treatment of solar gains), this allows distribution of gains to surfaces in proportion to their area, without any absorptivity or transmissivity calculations. The gain for windows and quick-model surfaces is assigned to the znCAir, except for the portion which conducts through the surface to the other side rather than through the surface film to the adjacent zone air; the gain to massive (delayed-model) surfaces is assigned to the side of surface in the zone with the gain.

**Gain Modeling in CNE zones**

In the CNE zone mode, the radiant internal gain is distributed to the surfaces in the zone, rather than going directly to the zone "air" heat capacity (znCAir). A simple model is used -- all surfaces are assumed to be opaque and to have the same (infrared) absorptivity -- even windows. Along with the assumption that the zone is spherical (implicit in the current treatment of solar gains), this allows distribution of gains to surfaces in proportion to their area, without any absorptivity or transmissivity calculations. The gain for windows and quick-model surfaces is assigned to the znCAir, except for the portion which conducts through the surface to the other side rather than through the surface film to the adjacent zone air; the gain to massive (delayed-model) surfaces is assigned to the side of surface in the zone with the gain.

Radiant internal gains are included in the IgnS (Sensible Internal Gain) column in the zone energy balance reports. (They could easily be shown in a separate IgnR column if desired.) Any energy transfer shows two places in the ZEB report, with opposite signs, so that the result is zero -- otherwise it wouldn't be an energy balance. The rest of the reporting story for radiant internal gains turns out to be complex. The specified value of the radiant gain (gnPower \* gnFrZn \* gnFrRad) shows in the IgnS column. To the extent that the gain heats the zone, it also shows negatively in the Masses column, because the zone CAir is lumped with the other masses. To the extent that the gain heats massive surfaces, it also shows negatively in the masses column. To the extent that the gain conducts through windows and quick-model surfaces, it shows negatively in the Conduction column. If the gain conducts through a quick-model surface to another zone, it shows negatively in the Izone (Interzone) column, positively in the Izone column of the receiving zone, and negatively in the receiving zone's Masses or Cond column.

**gnFrRad=*float***

Fraction of total gain going to zone (gnFrZn) that is radiant rather than convective or latent.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1   0.            No             hourly

**gnFrLat=*float***

Fraction of total gain going to zone (gnFrZn) that is latent heat (moisture addition).

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1   0.            No             hourly

**endGain**

Optional to indicate the end of the GAIN definition. Alternatively, the end of the gain definition can be indicated by END or by the declaration of another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
