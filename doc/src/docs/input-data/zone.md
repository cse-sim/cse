# ZONE

ZONE constructs an object of class ZONE, which describes an area of the building to be modeled as having a uniform condition. ZONEs are large, complex objects and can have many subobjects that describe associated surfaces, shading devices, HVAC equipment, etc.

## ZONE General Members

**znName**

Name of zone. Enter after the word ZONE; no "=" is used.

<%= member*table(
units: "",
legal_range: "\_63 characters*",
default: "_none_",
required: "Yes",
variability: "constant")
%>

**znModel=_choice_**

Selects model for zone.

<%= csv_table(<<END, :row_header => false)
CNE, Older central difference model based on original CALPAS methods. Not fully supported and not suitable for current compliance applications.
CZM, Conditioned zone model. Forward-difference&comma; short time step methods are used.
UZM, Unconditioned zone model. Identical to CZM except heating and cooling are not supported. Typically used for attics&comma; garages&comma; and other ancillary spaces.
END
%>

<%= member*table(
units: "",
legal_range: "\_choices above*",
default: "CNE",
required: "No",
variability: "constant")
%>

**znArea=_float_**

Nominal zone floor area.

<%= member*table(
units: "ft^2^",
legal_range: "\_x* $>$ 0",
default: "_none_",
required: "Yes",
variability: "constant")
%>

**znVol=_float_**

Nominal zone volume.

<%= member*table(
units: "ft^3^",
legal_range: "\_x* $>$ 0",
default: "_none_",
required: "Yes",
variability: "constant")
%>

**znAzm=_float_**

Zone azimuth with respect to bldgAzm. All surface azimuths are relative to znAzm, so that the zone can be rotated by changing this member only. Values outside the range 0^o^ to 360^o^ are normalized to that range.

<%= member_table(
units: "degrees",
legal_range: "unrestricted",
default: "0",
required: "No",
variability: "constant")
%>

**znFloorZ=_float_**

Nominal zone floor height relative to arbitrary 0 level. Used re determination of vent heights <!-- TODO -->

<%= member_table(
units: "ft",
legal_range: "unrestricted",
default: "0",
required: "No",
variability: "constant")
%>

**znCeilingHt=_float_**

Nominal zone ceiling height relative to zone floor (typically 8 – 10 ft).

<%= member*table(
units: "ft",
legal_range: "\_x* $>$ 0",
default: "_znVol_ / _znArea_",
required: "No",
variability: "constant")
%>

**znEaveZ=_float_**

Nominal eave height above ground level. Used re calculation of local surface wind speed. This in turn influences outside convection coefficients in some surface models and wind-driven air leakage.

<%= member*table(
units: "ft",
legal_range: "x $>$ 0",
default: "\_znFloorZ + infStories\*8*",
required: "No",
variability: "constant")
%>

**znCAir=_float_**

Zone "air" heat capacity: represents heat capacity of air, furniture, "light" walls, and everything in zone except surfaces having heat capacity (that is, non-QUICK surfaces).

<%= member*table(
units: "Btu/^o^F",
legal_range: "x $\\geq$ 0",
default: "3.5 \* \_znArea*",
required: "No",
variability: "constant")
%>

**znHcAirX=_float_**

Zone air exchange rate used in determination of interior surface convective coefficients. This item is generally used only for model testing.

<%= member_table(
units: "ACH",
legal_range: "x $>$ 0",
default: "as modeled",
required: "No",
variability: "subhourly")
%>

**znHcFrcF=_float_**

Zone surface forced convection factor. Interior surface convective transfer is modeled as a combination of forced and natural convection. hcFrc = znHcFrcF \* znHcAirX^.8. See CSE Engineering Documentation.

<%= member_table(
units: "Btuh/ft^2^-^o^F",
legal_range: ".2",
default: ".2",
required: "No",
variability: "hourly")
%>

**znHIRatio=_float_**

Zone hygric inertia ratio. In zone moisture balance calculations, the effective dry-air mass = znHIRatio \* (zone dry air mass). This enhancement can be used to represente the moisture storage capacity of zone surfaces and contents.

<%= member_table(
units: "",
legal_range: "x $>$ 0",
default: "1",
required: "No",
variability: "constant")
%>

**znSC=_float_**

Zone shade closure. Determines insolation through windows (see WINDOW members _wnSCSO_ and _wnSCSC_) and solar gain distribution: see SGDIST members _sgFSO_ and _sgFSC_. 0 represents shades open; 1 represents shades closed; intermediate values are allowed. An hourly variable CSE expression may be used to schedule shade closure as a function of weather, time of year, previous interval HVAC use or zone temperature, etc.

<%= member*table(
units: "",
legal_range: "0 $\\leq$ \_x* $\\leq$ 1",
default: "1 when cooling was used in _previous_ hour, else 0",
required: "No",
variability: "hourly")
%>

**znTH=_float_**

Heating set point used (and required) when znModel=CZM and zone has no terminals.

<%= member*table(
units: "^o^F",
legal_range: "0 < znTH < znTC",
default: "\_none*",
required: "Per above",
variability: "subhourly")
%>

**znTD=_float_**

Desired set point (temperature maintained with ventilation if possible) for znModel=CZM. Must be specified when zone ventilation is active.

<%= member*table(
units: "^o^F",
legal_range: "x > 0; znTH < znTD < znTC",
default: "\_none*",
required: "if venting",
variability: "subhourly")
%>

**znTC=_float_**

Cooling set point used (and required) when znModel=CZM and zone has no terminals.

<%= member*table(
units: "^o^F",
legal_range: "0 < znTC > znTH",
default: "\_none*",
required: "Per above",
variability: "subhourly")
%>

znModel = CZM zone heating and cooling is provided either via an RSYS HVAC system, by "magic" heat transfers specified by znQxxx items, or via TERMINAL (s). One of these must be defined.

**znRSys=_rsysName_**

Name of RSYS providing heating, cooling, and optional central fan integrated ventilation to this zone.

<%= member*table(
units: "",
legal_range: "\_RSYS name*",
default: "(no RSYS)",
required: "No",
variability: "constant")
%>

**znQMxH=_float_**

Heating capacity at current conditions

<%= member*table(
units: "Btuh",
legal_range: "x $\\geq$ 0",
default: "\_none*",
required: "No",
variability: "hourly")
%>

**znQMxHRated=_float_**

Rated heating capacity

<%= member*table(
units: "Btuh",
legal_range: "x $\\geq$ 0",
default: "\_none*",
required: "No",
variability: "constant")
%>

**znQMxC=_float_**

Cooling capacity at current conditions

<%= member*table(
units: "Btuh",
legal_range: "x $\\leq$ 0",
default: "\_none*",
required: "No",
variability: "hourly")
%>

**znQMxCRated=_float_**

Rated cooling capacity

<%= member*table(
units: "Btuh",
legal_range: "x $\\leq$ 0",
default: "\_none*",
required: "No",
variability: "constant")
%>

<% if comfort_model %>
The following provide parameters for comfort calculations

**znComfClo=_float_**

Occupant clothing resistance, used only when a comfort model is enabled.

<%= member*table(
units: "clo",
legal_range: "x $\\geq$ 0",
default: "\_none*",
required: "No",
variability: "subhourly")
%>

**znComfMet=_float_**

Occupant metabolic rate, used only when a comfort model is enabled.

<%= member*table(
units: "met",
legal_range: "x $\\geq$ 0",
default: "\_none*",
required: "No",
variability: "hourly")
%>

**znComfAirV=_float_**

Nominal air velocity used for comfort model, used only when a comfort model is enabled.

<%= member*table(
units: "",
legal_range: "x $\\geq$ 0",
default: "\_none*",
required: "No",
variability: "Hourly")
%>

**znComfRh=_float_**

Nominal zone relative humidity used for comfort model, used only when a comfort model is enabled.

<%= member*table(
units: "",
legal_range: "0 $\\leq$ \_x* $\\leq$ 1",
default: "_none_",
required: "No",
variability: "hourly")
%>

<% end %>

## ZONE Infiltration

The following control a simplified air change plus leakage area model. The Sherman-Grimsrud model is used to derive air flow rate from leakage area and this rate is added to the air changes specified with infAC. Note that TOP.windF does _not_ modify calculated infiltration rates, since the Sherman-Grimsrud model uses its own modifiers. See also AirNet models available via IZXFER.

**infAC=_float_**

Zone infiltration air changes per hour.

<%= member_table(
units: "1/hr",
legal_range: "x $\\geq$ 0",
default: "0.5",
required: "No",
variability: "hourly")
%>

**infELA=_float_**

Zone effective leakage area (ELA).

<%= member_table(
units: "in^2^",
legal_range: "x $\\geq$ 0",
default: "0.0",
required: "No",
variability: "hourly")
%>

**infShld=_int_**

Zone local shielding class, used in derivation of local wind speed for ELA infiltration model, wind-driven AirNet leakage, and exterior surface coefficients. infShld values are --

<%= csv_table(<<END, :row_header => false)
1, no obstructions or local shielding
2, light local shielding with few obstructions
3, moderate local shielding&comma; some obstructions within two house heights
4, heavy shielding&comma; obstructions around most of the perimeter
5, very heavy shielding&comma; large obstructions surrounding the perimeter within two house heights
END
%>

<%= member*table(
units: "",
legal_range: "1 $\\leq$ \_x* $\\leq$ 5",
default: "3",
required: "No",
variability: "constant")
%>

**infStories=_int_**

Number of stories in zone, used in ELA model.

<%= member*table(
units: "",
legal_range: "1 $\\leq$ \_x* $\\leq$ 3",
default: "1",
required: "No",
variability: "constant")
%>

**znWindFLkg=_float_**

Wind speed modifier factor. The weather file wind speed is multiplied by this factor to yield a local wind speed for use in infiltration and convection models.

<%= member_table(
units: "",
legal_range: "x $\\geq$ 0",
default: "derived from znEaveZ and infShld",
required: "No",
variability: "constant")
%>

**znAFMtr=_afMtrName_**

Name of an AFMETER object, if any, to which zone AirNet air flows are recorded. _ZnAFMtr_ defines a pressure boundary for accounting purposes. Multiple zones having the same AFMETER are treated as a single volume -- interzone flows within that volume are not recorded. For example, to obtain "building total" flow data, a common AFMETER could be assigned to several conditioned zones but not to adjacent unconditioned zones such as attic spaces.

<%= member*table(
units: "",
legal_range: "\_name of an AFMETER*",
default: "_not recorded_",
required: "No",
variability: "constant")
%>

**znLoadMtr=_ldMtrName_**

Name of a LOADMETER object, if any, to which zone heating and cooling loads are recorded.

<%= member*table(
units: "",
legal_range: "\_name of a LOADMETER*",
default: "_not recorded_",
required: "No",
variability: "constant")
%>

## ZONE Exhaust Fan

Presence of an exhaust fan in a zone is indicated by specifying a non-zero design flow value (xfanVfDs).

Zone exhaust fan model implementation is incomplete as of July, 2011. The current code calculates energy use but does not account for the effects of air transfer on room heat balance. IZXFER provides a more complete implementation.

**xfanFOn=_float_**

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

**xfanVfDs=_float_**

Exhaust fan design flow; 0 or not given indicates no fan.

<%= member_table(
units: "cfm",
legal_range: "x $\\geq$ 0",
default: "0, no fan",
required: "If fan present",
variability: "constant")
%>

**xfanPress=_float_**

Exhaust fan external static pressure.

<%= member*table(
units: "inches",
legal_range: "0.05 $\\leq$ \_x* $\\leq$ 1.0",
default: "0.3",
required: "No",
variability: "constant")
%>

Only one of xfanElecPwr, xfanEff, and xfanShaftBhp may be given: together with xfanVfDs and xfanPress, any one is sufficient for CSE to detemine the others and to compute the fan heat contribution to the air stream.

**xfanElecPwr=_float_**

Fan input power per unit air flow (at design flow and pressure).

<%= member_table(
units: "W/cfm",
legal_range: "x $>$ 0",
default: "derived from xfanEff",
required: "If xfanEff and xfanShaftBhp not present",
variability: "constant")
%>

**xfanEff=_float_**

Exhaust fan/motor/drive combined efficiency.

<%= member_table(
units: "fraction",
legal_range: "0 $\\leq$ x $\\leq$ 1",
default: "0.08",
required: "No",
variability: "constant")
%>

**xfanShaftBhp=_float_**

Fan shaft power at design flow and pressure.

<%= member_table(
units: "BHP",
legal_range: "x $>$ 0",
default: "derived from xfanElecPwr and xfanVfDs",
required: "If xfanElecPwr not present",
variability: "constant")
%>

**xfanMtr=_mtrName_**

Name of METER object, if any, by which fan's energy use is recorded (under end use category "fan").

<%= member*table(
units: "",
legal_range: "\_name of a METER*",
default: "_not recorded_",
required: "No",
variability: "constant")
%>

**endZone**

Indicates the end of the zone definition. Alternatively, the end of the zone definition can be indicated by the declaration of another object or by "END". If END or endZone is used, it should follow the definitions of the ZONE's subobjects such as GAINs, SURFACEs, TERMINALs, etc.

<%= member*table(
units: "",
legal_range: "",
default: "\_none*",
required: "No",
variability: "constant")
%>

**Related Probes:**

- @[zone][p_zone]
- @[znRes][p_znres] (accumulated results)
