# ZONE

ZONE constructs an object of class ZONE, which describes an area of the building to be modeled as having a uniform condition. ZONEs are large, complex objects and can have many subobjects that describe associated surfaces, shading devices, HVAC equipment, etc.

## ZONE General Members

**znName**

Name of zone. Enter after the word ZONE; no "=" is used.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**znModel=*choice***

Selects model for zone.

<%= csv_table(<<END, :row_header => false)
CNE, Older central difference model based on original CALPAS methods.  Not fully supported and not suitable for current compliance applications.
CZM, Conditioned zone model. Forward-difference&comma; short time step methods are used.
UZM, Unconditioned zone model. Identical to CZM except heating and cooling are not supported. Typically used for attics&comma; garages&comma; and other ancillary spaces.
END
%>

<%= member_table(
  units: "",
  legal_range: "*choices above*",
  default: "CNE",
  required: "No",
  variability: "constant")
  %>

**znArea=*float***

Nominal zone floor area.

<%= member_table(
  units: "ft^2^",
  legal_range: "*x* $>$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**znVol=*float***

Nominal zone volume.

<%= member_table(
  units: "ft^3^",
  legal_range: "*x* $>$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**znAzm=*float***

Zone azimuth with respect to bldgAzm. All surface azimuths are relative to znAzm, so that the zone can be rotated by changing this member only. Values outside the range 0^o^ to 360^o^ are normalized to that range.

<%= member_table(
  units: "degrees",
  legal_range: "unrestricted",
  default: "0",
  required: "No",
  variability: "constant")
  %>

**znFloorZ=*float***

Nominal zone floor height relative to arbitrary 0 level. Used re determination of vent heights <!-- TODO -->

<%= member_table(
  units: "ft",
  legal_range: "unrestricted",
  default: "0",
  required: "No",
  variability: "constant")
  %>

**znCeilingHt=*float***

Nominal zone ceiling height relative to zone floor (typically 8 – 10 ft).

<%= member_table(
  units: "ft",
  legal_range: "*x* $>$ 0",
  default: "*znVol* / *znArea*",
  required: "No",
  variability: "constant")
  %>

**znEaveZ=*float***

Nominal eave height above ground level. Used re calculation of local surface wind speed. This in turn influences outside convection coefficients in some surface models and wind-driven air leakage.

<%= member_table(
  units: "ft",
  legal_range: "x $>$ 0",
  default: "*znFloorZ + infStories\*8*",
  required: "No",
  variability: "constant")
  %>

**znCAir=*float***

Zone "air" heat capacity: represents heat capacity of air, furniture, "light" walls, and everything in zone except surfaces having heat capacity (that is, non-QUICK surfaces).

<%= member_table(
  units: "Btu/^o^F",
  legal_range: "x $\\geq$ 0",
  default: "3.5 \* *znArea*",
  required: "No",
  variability: "constant")
  %>

**znHcAirX=*float***

Zone air exchange rate used in determination of interior surface convective coefficients.  This item is generally used only for model testing.

<%= member_table(
  units: "ACH",
  legal_range: "x $>$ 0",
  default: "as modeled",
  required: "No",
  variability: "subhourly")
  %>

**znHcFrcF=*float***

Zone surface forced convection factor.  Interior surface convective transfer is modeled as a combination of forced and natural convection.  hcFrc = znHcFrcF * znHcAirX^.8.  See CSE Engineering Documentation.

<%= member_table(
  units: "Btuh/ft^2^-^o^F",
  legal_range: ".2",
  default: ".2",
  required: "No",
  variability: "hourly")
  %>

**znHIRatio=*float***

Zone hygric inertia ratio.  In zone moisture balance calculations, the effective dry-air mass = znHIRatio * (zone dry air mass).  This enhancement can be used to represente the moisture storage capacity of zone surfaces and contents.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1",
  required: "No",
  variability: "constant")
  %>

**znSC=*float***

Zone shade closure. Determines insolation through windows (see WINDOW members *wnSCSO* and *wnSCSC*) and solar gain distribution: see SGDIST members *sgFSO* and *sgFSC*. 0 represents shades open; 1 represents shades closed; intermediate values are allowed. An hourly variable CSE expression may be used to schedule shade closure as a function of weather, time of year, previous interval HVAC use or zone temperature, etc.

<%= member_table(
  units: "",
  legal_range: "0 $\\leq$ *x* $\\leq$ 1",
  default: "1 when cooling was used in *previous* hour, else 0",
  required: "No",
  variability: "hourly")
  %>

**znTH=*float***

Heating set point used (and required) when znModel=CZM and zone has no terminals.

<%= member_table(
  units: "^o^F",
  legal_range: "0 < znTH < znTC",
  default: "*none*",
  required: "Per above",
  variability: "subhourly")
  %>

**znTD=*float***

Desired set point (temperature maintained with ventilation if possible) for znModel=CZM.  Must be specified when zone ventilation is active.  

<%= member_table(
  units: "^o^F",
  legal_range: "x > 0; znTH < znTD < znTC",
  default: "*none*",
  required: "if venting",
  variability: "subhourly")
  %>

**znTC=*float***

Cooling set point used (and required) when znModel=CZM and zone has no terminals.

<%= member_table(
  units: "^o^F",
  legal_range: "0 < znTC > znTH",
  default: "*none*",
  required: "Per above",
  variability: "subhourly")
  %>

znModel = CZM zone heating and cooling is provided either via an RSYS HVAC system, by "magic" heat transfers specified by znQxxx items, or via TERMINAL (s).  One of these must be defined.

**znRSys=*rsysName***

Name of RSYS providing heating, cooling, and optional central fan integrated ventilation to this zone.

<%= member_table(
  units: "",
  legal_range: "*RSYS name*",
  default: "(no RSYS)",
  required: "No",
  variability: "constant")
  %>

**znQMxH=*float***

Heating capacity at current conditions

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "hourly")
  %>

**znQMxHRated=*float***

Rated heating capacity

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**znQMxC=*float***

Cooling capacity at current conditions

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\leq$ 0",
  default: "*none*",
  required: "No",
  variability: "hourly")
  %>

**znQMxCRated=*float***

Rated cooling capacity

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\leq$ 0",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

<% if comfort_model %>
The following provide parameters for comfort calculations

**znComfClo=*float***

Occupant clothing resistance, used only when a comfort model is enabled.

<%= member_table(
  units: "clo",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "subhourly")
  %>

**znComfMet=*float***

Occupant metabolic rate, used only when a comfort model is enabled.

<%= member_table(
  units: "met",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "hourly")
  %>

**znComfAirV=*float***

Nominal air velocity used for comfort model, used only when a comfort model is enabled.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "No",
  variability: "Hourly")
  %>

**znComfRh=*float***

Nominal zone relative humidity used for comfort model, used only when a comfort model is enabled.

<%= member_table(
  units: "",
  legal_range: "0 $\\leq$ *x* $\\leq$ 1",
  default: "*none*",
  required: "No",
  variability: "hourly")
  %>

<% end %>

## ZONE Infiltration

The following control a simplified air change plus leakage area model. The Sherman-Grimsrud model is used to derive air flow rate from leakage area and this rate is added to the air changes specified with infAC. Note that TOP.windF does *not* modify calculated infiltration rates, since the Sherman-Grimsrud model uses its own modifiers. See also AirNet models available via IZXFER.

**infAC=*float***

Zone infiltration air changes per hour.

<%= member_table(
  units: "1/hr",
  legal_range: "x $\\geq$ 0",
  default: "0.5",
  required: "No",
  variability: "hourly")
  %>

**infELA=*float***

Zone effective leakage area (ELA).

<%= member_table(
  units: "in^2^",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "hourly")
  %>

**infShld=*int***

Zone local shielding class, used in derivation of local wind speed for ELA infiltration model, wind-driven AirNet leakage, and exterior surface coefficients. infShld values are --

<%= csv_table(<<END, :row_header => false)
  1, no obstructions or local shielding
  2, light local shielding with few obstructions
  3, moderate local shielding&comma; some obstructions within two house heights
  4, heavy shielding&comma; obstructions around most of the perimeter
  5, very heavy shielding&comma; large obstructions surrounding the perimeter within two house heights
END
%>

<%= member_table(
  units: "",
  legal_range: "1 $\\leq$ *x* $\\leq$ 5",
  default: "3",
  required: "No",
  variability: "constant")
  %>

**infStories=*int***

Number of stories in zone, used in ELA model.

<%= member_table(
  units: "",
  legal_range: "1 $\\leq$ *x* $\\leq$ 3",
  default: "1",
  required: "No",
  variability: "constant")
  %>

**znWindFLkg=*float***

Wind speed modifier factor.  The weather file wind speed is multiplied by this factor to yield a local wind speed for use in infiltration and convection models.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "derived from znEaveZ and infShld",
  required: "No",
  variability: "constant")
  %>

**znAFMtr=*afMtrName***

Name of an AFMETER object, if any, to which zone AirNet air flows are recorded.  *ZnAFMtr* defines a pressure boundary for accounting purposes.  Multiple zones having the same AFMETER are treated as a single volume -- interzone flows within that volume are not recorded.  For example, to obtain "building total" flow data, a common AFMETER could be assigned to several conditioned zones but not to adjacent unconditioned zones such as attic spaces.

<%= member_table(
  units: "",
  legal_range: "*name of an AFMETER*",
  default: "*not recorded*",
  required: "No",
  variability: "constant")
  %>

**znLoadMtr=*ldMtrName***

Name of a LOADMETER object, if any, to which zone heating and cooling loads are recorded.

<%= member_table(
  units: "",
  legal_range: "*name of a LOADMETER*",
  default: "*not recorded*",
  required: "No",
  variability: "constant")
  %>

## ZONE Exhaust Fan

Presence of an exhaust fan in a zone is indicated by specifying a non-zero design flow value (xfanVfDs).

Zone exhaust fan model implementation is incomplete as of July, 2011. The current code calculates energy use but does not account for the effects of air transfer on room heat balance. IZXFER provides a more complete implementation.

**xfanFOn=*float***

Exhaust fan on fraction. On/off control assumed, so electricity requirement is proportional to run time.

<%= member_table(
  units: "fraction",
  legal_range: "0 $\\leq$ x $\\leq$ 1",
  default: "1",
  required: "No",
  variability: "hourly")
  %>

Example: The following would run an exhaust fan 70% of the time between 8 AM and 5 PM:

        xfanFOn = select( (\$hour >= 7 && \$hour < 5), .7,
                                              default, 0 );


**xfanVfDs=*float***

Exhaust fan design flow; 0 or not given indicates no fan.

<%= member_table(
  units: "cfm",
  legal_range: "x $\\geq$ 0",
  default: "0, no fan",
  required: "If fan present",
  variability: "constant")
  %>

**xfanPress=*float***

Exhaust fan external static pressure.

<%= member_table(
  units: "inches",
  legal_range: "0.05 $\\leq$ *x* $\\leq$ 1.0",
  default: "0.3",
  required: "No",
  variability: "constant")
  %>

Only one of xfanElecPwr, xfanEff, and xfanShaftBhp may be given: together with xfanVfDs and xfanPress, any one is sufficient for CSE to detemine the others and to compute the fan heat contribution to the air stream.

**xfanElecPwr=*float***

Fan input power per unit air flow (at design flow and pressure).

<%= member_table(
  units: "W/cfm",
  legal_range: "x $>$ 0",
  default: "derived from xfanEff",
  required: "If xfanEff and xfanShaftBhp not present",
  variability: "constant")
  %>

**xfanEff=*float***

Exhaust fan/motor/drive combined efficiency.

<%= member_table(
  units: "fraction",
  legal_range: "0 $\\leq$ x $\\leq$ 1",
  default: "0.08",
  required: "No",
  variability: "constant")
  %>

**xfanShaftBhp=*float***

Fan shaft power at design flow and pressure.

<%= member_table(
  units: "BHP",
  legal_range: "x $>$ 0",
  default: "derived from xfanElecPwr and xfanVfDs",
  required: "If xfanElecPwr not present",
  variability: "constant")
  %>

**xfanMtr=*mtrName***

Name of METER object, if any, by which fan's energy use is recorded (under end use category "fan").

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*not recorded*",
  required: "No",
  variability: "constant")
  %>

**endZone**

Indicates the end of the zone definition. Alternatively, the end of the zone definition can be indicated by the declaration of another object or by "END". If END or endZone is used, it should follow the definitions of the ZONE's subobjects such as GAINs, SURFACEs, TERMINALs, etc.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**Related Probes:**

- @[zone](#p_zone)
- @[znRes](#p_znres) (accumulated results)
