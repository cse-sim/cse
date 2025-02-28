# DHWUSE

Defines a single hot water draw as part of a DHWDAYUSE.  See discussion and examples under DHWDAYUSE. As noted there, most DHWUSE values have hourly variability, allowing flexible representation.

**wuName**

Optional name; give after the word “DHWUSE” if desired.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**wuStart=*float***

The starting time of the hot water draw.

<%= member_table(
  units: "hr",
  legal_range: "0 $\\le$ x $\\le$ 24",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**wuDuration=*float***

Draw duration.  wuDuration = 0 is equivalent to omitting the DHWUSE.
Durations that extend beyond midnight are included in the current day.

<%= member_table(
  units: "min",
  legal_range: "0 $\\le$ x $\\le$ 1440",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**wuFlow=*float***

Draw flow rate at the point of use (in other words, the mixed-water flow rate).  wuFlow = 0 is equivalent to omitting the DHWUSE.  There is no enforced upper limit on wuFlow, however, unrealistically large values can cause runtime errors.

<%= member_table(
  units: "gpm",
  legal_range: "x $\\le$ 0",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**wuHotF=*float***

Fraction of draw that is hot water.  Cannot be specified with wuTemp or wuHeatRecEF.

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 1",
  default: "1",
  required: "No",
  variability: "hourly")
  %>

**wuTemp=*float***

Mixed-water use temperature at the fixture. Cannot be specified when wuHotF is given.   

<%= member_table(
  units: "^o^F",
  legal_range: "x $\\le$ 0",
  default: "0",
  required: "when wuHeatRecEF is given or parent DHWSYS includes DHWHEATREC(s)",
  variability: "hourly")
  %>

**wuHeatRecEF=*float***

Heat recovery effectiveness, allows simple modeling of heat recovery devices such as drain water heat exchangers.

If non-0 (evaluated hourly), hot water use is reduced based on wuTemp, DHWSYS wsTUse, and DHWSYS wsTInlet.  DHWHEATREC(s), if any, are ignored for this use.  wuTemp must be specified.

If 0, detailed heat recovery modeling *may* apply, see [DHWHEATREC][dhwheatrec].

<%= member_table(
  units: "",
  legal_range: "0 $\\le$ x $\\le$ 0.9",
  default: "0",
  required: "No",
  variability: "hourly")
  %>

**wuHWEndUse=*choice***

Hot-water end use: one of Shower, Bath, CWashr, DWashr, or Faucet.  wuHWEndUse has the following functions --

 * Allocation of hot water use among multiple DHWSYSs (if more than one DHWSYS references a given DHWDAYUSE).
 * DHWMETER end-use accounting (via DHWSYS).
 * Activation of the detailed heat recovery model (available for end use Shower when wuHeatRecEF=0 and the parent DHWSYS includes DHWHEATREC(s)).

<%= member_table(
  units: "",
  legal_range: "One of above choices",
  default: "(use allocated to Unknown)",
  required: "No",
  variability: "constant")
  %>

**wuEventID=*integer***

User-defined identifier that associates multiple DHWUSEs with a single event or activity.  For example, a dishwasher uses water at several discrete times during a 90 minute cycle and all DHWUSEs would be assigned the same wuEventID.  All DHWUSEs having the same wuEventID should have the same wuHWEndUse.

<%= member_table(
  units: "",
  legal_range: "*x* $\\ge$ 0",
  default: "0",
  required: "No",
  variability: "constant")
  %>

**endDHWUSE**

Optionally indicates the end of the DHWUSE definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "")
  %>

**Related Probes:**

- @[DHWUse][p_dhwuse]
