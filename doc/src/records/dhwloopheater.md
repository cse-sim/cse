# DHWLOOPHEATER

DHWHEATERLOOP constructs an object representing a hot water heater dedicated to heating DHWLOOP return water (or several if identical).

Refer to DHWHEATER.

**whmult=*float***

Count of identical water heaters. Non-integral values are allowed for resizeing methods.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "1.0",
  required: "No",
  variability: "constant") %>

**whType=*choice***

Heater type.

<%= member_table(
  units: "",
  legal_range: " C_WHTYPECH_STRGSML\nC_WHTYPECH_STRGLRG\nC_WHTYPECH_INSTSML\nC_WHTYPECH_INSTLRG\nC_WHTYPECH_INSTUEF\nC_WHTYPECH_BUILTUP",
  default: "C_WHTYPECH_STRGSML",
  required: "No",
  variability: "constant") %>

**whHeatSrc=*choice***

Heat source.

<%= member_table(
  legal_range: "C_WHHEATSRCCH_ELRES\nC_WHHEATSRCCH_FUEL\nC_WHHEATSRCCH_ASHP\nC_WHHEATSRCCH_ASHPX\nC_WHHEATSRCCH_ELRESX",
  default: "Electric resistance\nFuel-fired burner\nAir source heat pump (T24DHW.DLL model)\nAir source heat pump (Ecotope HPWH)\nElectric resistance (Ecotope HPWH)",
  ) %>

**whZone=*integer***

DHWHEATER location zone of re-tank loss. Zero only valid if whTEx is being used. The heat losses are split half way into the zone air and half to radiant.

<%= member_table(
  units: "",
  legal_range: "-32,768 $\\leq$ x $\\leq$ 32,767",
  default: "0",
  required: "No",
  variability: "constant") %>

**whTEx=*float***

Surrounding temperature around the tank. Creates a temperature loss for the tank. When whTEx is set whZone is ignored.

<%= member_table(
  units: "^o^F",
  legal_range: "Temperature range",
  default: "70.0",
  required: "No",
  variability: "subhourly") %>

**whResType=*choice***

Resistance heater type

<%= member_table(
  units: "",
  legal_range: "C_WHRESTYCH_TYPICAL\nC_WHRESTYCH_SWINGTANK",
  default: "C_WHRESTYCH_TYPICAL",
  required: "No",
  variability: "constant") %>

**whASHPType=*choice***

Air source heat pump type, valid only if whHeatSrc=ASHPX, otherwise is ignored.

<%= member_table(
  units: "",
  legal_range: "",
  default: "-1",
  required: "No",
  variability: "constant") %>

**whASHPSrcZn=*integer***

ASHP source zone, input heat removed from zone air. whASHPSrcZn is only 0 if whASHPSrcT is being used.

<%= member_table(
  units: "",
  legal_range: "-32,768 $\\leq$ x $\\leq$ 32,767",
  default: "0",
  required: "No",
  variability: "constant") %>

**whASHPSrcT=*float***

ASHP source temperature. When whASHPSrcT is set heat transfer and whASHPSrcZn are ignored.

<%= member_table(
  units: "^o^F",
  legal_range: " $\\geq$ x $\\leq$ 0",
  default: "70",
  required: "No",
  variability: "subhourly") %>

**whASHPResUse=*float***

Resistance heat parameter for ECOTOPE HPWH model. Valid only if wh_ashpTy = C_WHASHPTYCH_GENERIC.

<%= member_table(
  units: "",
  legal_range: " x $>$ 0",
  default: "7.22",
  required: "No",
  variability: "constant") %>

**whtankCount=*float***

Number of storage tanks per DHWHEATER. It does not reflect whMult (whMult=2, whtankCount=3 -> 6 tanks).

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "1.0",
  required: "No",
  variability: "constant") %>

**whHeatingCap=*float***

Nominal heating capacity.

<%= member_table(
  units: "Btuh",
  legal_range: "x $>$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whVol=*float***

Total actual storage volume. Not per tank.

<%= member_table(
  units: "gal",
  legal_range: "x $\\geq$ 0",
  default: "50.0",
  required: "No",
  variability: "constant") %>

**whVolRunning=*float***

Running storage volume is equal to volume above aquastat.

<%= member_table(
  units: "gal",
  legal_range: "x $>$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whUA=*float***

HPWH-type total UA. Not per tank.

<%= member_table(
  units: "Btuh/F",
  legal_range: "x $\\geq$ 0",
  default: "HPWH preset",
  required: "No",
  variability: "constant") %>

**whInsulR=*float***

Insulation resistant for the HPWH-type tank.

<%= member_table(
  units: "hr-F/Btuh",
  legal_range: "x $>$ 0",
  default: "12.0 or from whUA and whVol",
  required: "No",
  variability: "constant") %>

**whInHtSupply=*fraction***

Fractional tank height of supply inlet. Zero implies the bottom of the tank while one implies the top. This is only use for HPWH models only.

<%= member_table(
  units: "",
  legal_range: "0 $\\leq$ x $\\leq$ 1",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whInHtLoopRet=*fraction***

Fractional tank height of loop return inlet(s). Zero implies the bottom of the tank while one implies the top. This is only use for HPWH models only.

<%= member_table(
  units: "",
  legal_range: "0 $\\leq$ x $\\leq$ 1",
  default: "0",
  required: "No",
  variability: "constant") %>

**whEF=*float***

Rated energy factor.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "0.82",
  required: "No",
  variability: "constant") %>

**whUEF=*float***

Rated uniform energy factor.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whStbyElec=*float***

electrical power during standby

<%= member_table(
  units: "W",
  legal_range: "x $\\geq$ 0",
  default: "4.0",
  required: "No",
  variability: "constant") %>

**whRatedFlow=*float***

Maximum rated flow as per UEF test.

<%= member_table(
  units: "gpm",
  legal_range: "x $>$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whLoadCFwdF=*float***

Load carry-foward fraction will equal the number of hours DHWHEATER is allowed to meet. Load that is unmet is on a 1 minute basis.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "1.0",
  required: "No",
  variability: "constant") %>

**whAnnualFuel=*float***

Annual fuel use per UEF method.

<%= member_table(
  units: "therms/yr",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whAnnualElec=*float***

Annual electricity use per UEF method.

<%= member_table(
  units: "kWh/yr",
  legal_range: "x $\\geq$ 0",
  default: "0",
  required: "No",
  variability: "constant") %>

**whResHtPwr=*float***

Upper range for power resistance. Used for some models only.

<%= member_table(
  units: "W",
  legal_range: "x $\\geq$ 0",
  default: "4500",
  required: "No",
  variability: "constant") %>

**whResHtPwr2=*float***

Lower range for power resistance.

<%= member_table(
  units: "W",
  legal_range: "x $\\geq$ 0",
  default: "whResHtPwr",
  required: "No",
  variability: "constant") %>

**whLDEF=*float***

Load-dependent energy factor

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "constant") %>

**whEff=*float***

Energy recovery efficiency.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "1.0",
  required: "No",
  variability: "constant") %>

**whSBL=*float***

Standby loss

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whPilotPwr=*float***

Pilot light power. Included in whInFuel

<%= member_table(
  units: "Btuh",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "hourly") %>

**whParElec=*float***

Parasitic electric use.

<%= member_table(
  units: "W",
  legal_range: "x $\\geq$ 0",
  default: "0.0",
  required: "No",
  variability: "constant") %>

**whElecMtr=*integer***

Meter for system electricity use.

<%= member_table(
  units: "",
  legal_range: "-32,768 $\\leq$ x $\\leq$ 32,767",
  default: "wsElecMtr",
  required: "No",
  variability: "constant") %>

**whFuelMtr=*integer***

Meter for system fuel use.

<%= member_table(
  units: "",
  legal_range: "-32,768 $\\leq$ x $\\leq$ 32,767",
  default: "wsFuelMtr",
  required: "No",
  variability: "constant") %>

**whxBUEndUse=*datatype***

whElecMtr end use for separate accounting of wh_HPWHxBU
<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "N/A",
  required: "No",
  variability: "constant") %>

**endDHWHEATER**

Indicates the end of the DHWHEATER. Alternatively, the end of the DHWHEATER definition can be indicated by the declaration of another object or by END

<%= member_table(
  units: "",
  legal_range: "",
  default: "N/A",
  required: "No",
  variability: "constant") %>