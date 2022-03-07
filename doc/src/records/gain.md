# GAIN

A GAIN object adds sensible and/or latent heat to the ZONE, and/or adds arbitrary energy use to a METER. GAINs may be subobjects of ZONEs and are normally given within the input for their ZONE.  As many GAINs as desired (or none) may be given for each ZONE.  Alternatively, GAINs may be subobjects of TOP and specify gnZone to specify their associate zone.

Each gain has an amount of power (gnPower), which may optionally be accumulated to a METER (gnMeter). The power may be distributed to the zone, plenum, or return as sensible heat with an optional fraction radiant, or to the zone as latent heat (moisture addition), or not.

# Gain zones

The gain to the zone may be further divided into convective sensible, radiant sensible and latent heat via the gnFrRad and gnFrLat members; the plenum and return gains are assumed all convective sensible.


In the CNE zone mode, the radiant internal gain is distributed to the surfaces in the zone, rather than going directly to the zone "air" heat capacity (znCAir). A simple model is used -- all surfaces are assumed to be opaque and to have the same (infrared) absorptivity -- even windows. Along with the assumption that the zone is spherical (implicit in the current treatment of solar gains), this allows distribution of gains to surfaces in proportion to their area, without any absorptivity or transmissivity calculations. The gain for windows and quick-model surfaces is assigned to the znCAir, except for the portion which conducts through the surface to the other side rather than through the surface film to the adjacent zone air; the gain to massive (delayed-model) surfaces is assigned to the side of surface in the zone with the gain.

# Gain Modeling in <% if inactive_CNE_records %>CNE <% end %>zones

<% if inactive_CNE_records %>In the CNE zone mode, t<% else %>T<% end %>he radiant internal gain is distributed to the surfaces in the zone, rather than going directly to the zone "air" heat capacity (znCAir). A simple model is used -- all surfaces are assumed to be opaque and to have the same (infrared) absorptivity -- even windows. Along with the assumption that the zone is spherical (implicit in the current treatment of solar gains), this allows distribution of gains to surfaces in proportion to their area, without any absorptivity or transmissivity calculations. The gain for windows and quick-model surfaces is assigned to the znCAir, except for the portion which conducts through the surface to the other side rather than through the surface film to the adjacent zone air; the gain to massive (delayed-model) surfaces is assigned to the side of surface in the zone with the gain.

Radiant internal gains are included in the IgnS (Sensible Internal Gain) column in the zone energy balance reports. (They could easily be shown in a separate IgnR column if desired.) Any energy transfer shows two places in the ZEB report, with opposite signs, so that the result is zero -- otherwise it wouldn't be an energy balance. The rest of the reporting story for radiant internal gains turns out to be complex. The specified value of the radiant gain (gnPower \* gnFrZn \* gnFrRad) shows in the IgnS column. To the extent that the gain heats the zone, it also shows negatively in the Masses column, because the zone CAir is lumped with the other masses. To the extent that the gain heats massive surfaces, it also shows negatively in the masses column. To the extent that the gain conducts through windows and quick-model surfaces, it shows negatively in the Conduction column. If the gain conducts through a quick-model surface to another zone, it shows negatively in the Izone (Interzone) column, positively in the Izone column of the receiving zone, and negatively in the receiving zone's Masses or Cond column.


**gnName**

Name of gain; follows the word GAIN if given.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**gnZone=*znName***

Name of ZONE to which heat gains are added.  Omitted when GAIN is given as a ZONE subobject.  If a TOP subobject (i.e., not a ZONE subobject) and znZone is omitted, heat gains are discarded but energy use is still recorded to gnMeter.  This feature can be used to represent energy uses that our outside of conditioned zones (e.g. exterior lighting).

**Units**   **Legal Range**   **Default**             **Required**   **Variability**
----------- ----------------- ----------------------  -------------- -----------------
            *name of ZONE*     *parent zone if any*       No             constant


**gnPower=*float***

Rate of heat addition/energy use. Negative gnPower values may be used to represent heat removal/energy generation. Expressions containing functions are commonly used with this member to schedule the gain power on a daily and/or hourly basis. Refer to the functions section in Section 4 for details and examples.

All gains, including electrical, are specified in Btuh units unless associated with DHW use (see gnCtrlDHWSYS), in which case gnPower is specified in Btuh/gal.  Note that meter reporting of internal gain is in MBtu (millions of Btu) by default.  

  **Units**   **Legal Range**     **Default**   **Required**   **Variability**
  ----------- ------------------- ------------- -------------- -----------------
  Btuh        *no restrictions*   *none*        Yes            hourly

**gnMeter=*choice***

Name of meter by which this GAIN's gnPower is recorded. If omitted, gain is assigned to no meter and energy use is not accounted in CSE simulation reports; thus, gnMeter should only be omitted for "free" energy sources.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
            *name of METER*      *none*        No             constant

**gnEndUse=*choice***

Meter end use to which the GAIN's energy use should be accumulated.

<%= insert_file("doc/src/enduses.md") %>


  **Units**   **Legal Range**        **Default**   **Required**                   **Variability**
  ----------- ---------------------- ------------- ------------------------------ -----------------
              *Codes listed above*   *none*        Required if gnMeter is given   constant

The gnFrZn, gnFrPl, and gnFrRtn members allow you to allocate the gain among the zone, the zone's plenum, and the zone's return air flow. Values that total to more than 1.0 constitute an error. If they total less than 1, the unallocated portion of the gain is recorded by the meter (if specified) but not transferred into the building. By default, all of the gain not directed to the return or plenum goes to the zone.

**gnFrZn=*float***

Fraction of gain going to zone. gnFrLat (below) gives portion of this gain that is latent, if any; the remainder is sensible.

<% if not_yet_implemented %>
  **Units**   **Legal Range**                      **Default**                        **Required**   **Variability**
  ----------- ------------------------------------ ---------------------------------- -------------- -----------------
              gnFrZn + gnFrPl + gnFrRtn $\leq$ 1   *1 - gnFrPl - gnFrRtn*             No             hourly
<% else %>
  **Units**   **Legal Range**                      **Default**                        **Required**   **Variability**
  ----------- ------------------------------------ ---------------------------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1                1.                                 No             hourly
<% end %>

**gnFrPl=*float***

Fraction of gain going to plenum.

<%= member_table(
  units: "",
  legal_range: "gnFrZn + gnFrPl + gnFrRtn $\\leq$ 1",
  default: "0.",
  required: "No",
  variability: "hourly") %>


**gnFrRtn=*float***

Fraction of gain going to return.

<%= member_table(
  units: "",
  legal_range: "gnFrZn + gnFrPl + gnFrRtn $\\leq$ 1",
  default: "0.",
  required: "No",
  variability: "hourly") %>

**gnFrRad=*float***

Fraction of total gain going to zone (gnFrZn) that is radiant rather than convective or latent.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1   0.            No             hourly

**gnFrLat=*float***

Fraction of total gain going to zone (gnFrZn) that is latent heat (moisture addition).

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\le$ x $\le$ 1         0.            No             hourly


**gnDlFrPow=*float***

Hourly power reduction factor, typically used to modify lighting power to account for
day lighting.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- -----------------
              0 $\leq$ *x* $\leq$ 1    1.            No             hourly


**gnCtrlDHWSYS=*dhwsysName***

Name of a DHWSYS whose water use modulates gnPower.  For example, electricity use of water-using appliances (e.g. dishwasher or clothes washer) can be modeled based on water use, ensuring that the uses are synchronized.  When this feature is used, gnPower should be specified in Btuh/gal.

**Units**   **Legal Range**     **Default**            **Required**   **Variability**
----------- ------------------- ---------------------- -------------- -----------------
            *name of a DHWSYS*  no DHWSYS/GAIN linkage            No         constant

**gnCtrlDHWMETER=*integer***

Controls the DHWMETER. Allows gains to track water usage such as dishwashers, clothes washers, etc.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>

**gnCtrlDHWEndUse=*dhwEndUseName***

Name of the DHWSYS end use consumption that modulates gnPower.  See DHWMETER for DHW end use definitions.

**Units**   **Legal Range**     **Default**            **Required**   **Variability**
----------- ------------------- ---------------------- -------------- -----------------
             DHW end use         Total                     No          constant

**endGain**

Optional to indicate the end of the GAIN definition. Alternatively, the end of the gain definition can be indicated by END or by the declaration of another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[gain](#p_gain)
