# PERFORMANCEMAP

PERFORMANCEMAP defines a multiple-dimension table of values from which models can derive performance data via interpolation.  Subordinate PMGRIDAXIS and PMLOOKUPDATA allow input of performance maps of a range of dimensions and granularity.

Following ASHRAE Standard 205 terminology, sets of "grid" values are the independent variables and sets of "lookup" values are the dependent variables.

The following example defines a 2D map based on grid variables outdoor dry-bulb temperature and (arbitrary) compressor speed.  For each grid value combination, lookup values are provided for capacity ratio and COP.

    PERFORMANCEMAP "PMClg"

        PMGRIDAXIS "ClgOutdoorDBT" pmGXType="OutdoorDBT" pmGXValues=60,82,95,115 pmGXRefValue=95
        PMGRIDAXIS "ClgSpeed" pmGXType="Speed" pmGXValues=1,2,3 pmGXRefValue=2

        // Capacity ratio = net total capacity / net rated total capacity
        PMLOOKUPDATA LUClgCapRat pmLUType = "CapRat" pmLUValues =
          0.48, 1.13, 1.26,   // 60F at min, mid, max speed
          0.42, 1.05, 1.17,   // 82F
          0.39, 1.00, 1.12,   // 95F
          0.34, 0.92, 1.04    // 115F

        // COP = net total COP
        PMLOOKUPDATA LUClgCOP pmLUType = "COP" pmLUValues =
          14.22, 16.44, 15.00,  // 60F at min, mid, max speed
          7.93,  7.59,  6.71,   // 82F
          6.01,  5.58,  4.91,   // 95F
          4.12,  3.82,  3.34    // 115F

    endPERFORMANCEMAP

At OutdoorDBT=95 and Speed=2, this performance map would yield CapRat=1.00 and COP=5.58.  At other (OutdoorDBT,Speed) combinations, suitable 2D interpolation is performed on each lookup variable.  Lookup variables are extrapolated outside of PMGRIDAXIS ranges; adequate axis ranges must be provided to avoid unrealistic extrapolation.

**pmName**

Name of performance map; given after the word "PERFORMANCEMAP".   Necessary to allow reference from e.g. RSYS.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**endPERFORMANCEMAP**

Optionally indicates the end of PERFORMANCEMAP definition.  It is good practice to include *endPerformanceMap* after the associated PMGRIDAXIS and PMLOOKDATA.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>



# PMGRIDAXIS

Defines grid values for a single dimension of the parent (preceeding) PERFORMANCEMAP.

The order of PMGRIDAXIS commands fixes the order of PMLOOKUPDATA values -- later PMGRIDAXIS dimensions vary more quickly (see example above).

**pmGXName**

Name of grid axis; optionally given after the word "PMGRIDAXIS".  Used in error messages.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**pmGXType=*string***

Documents the dimension of the axis, for example "OutdoorDBT", "Speed", or "AirFlow".

<%= member_table(
  units: "",
  legal_range: "*at least 1 char*",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**pmGXValues=*float array***

1 to 10 comma-separated values specifying the data points of this axis.  Must be in strictly ascending order.

<%= member_table(
  units: "various",
  legal_range: "",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**pmGXRefValue=*float***

Defines the reference or nominal value of this PMGRIDAXIS.  For example, when defining temperature points for a typical air conditioner, pmGXRefValue=95 might be used, since AC capacity is rated at 95 F. 

<%= member_table(
  units: "same as pmGXValues",
  legal_range: "",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**endPMGRIDAXIS**

Optionally indicates the end of PMGRIDAXIS definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>



# PMLOOKUPDATA

*pmLUName**

Name of lookup data; optionally given after the word "PMLOOKUPDATA".  Used in error messages.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**pmLUType=*string***

Documents the current lookup value, e.g. "COP" or "CapacityRatio".

<%= member_table(
  units: "",
  legal_range: "*at least 1 char*",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**pmLUValues=*float array***

Comma-separated values specifying the lookup data.  The number of values required is the product of the size of all PMGRIDAXISs in the current PEFORMANCEMAP.  In the example above, there are 4 OutdoorDBTs and 3 speeds, so 12 values must be provided.

<%= member_table(
  units: "various",
  legal_range: "",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**endPMLOOKUPDATA**

Optionally indicates the end of PMLOOKUPDATA definition.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant") %>