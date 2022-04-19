# PVARRAY

PVARRAY describes a photovoltaic panel system. The algorithms are based on the [PVWatts calculator](http://www.bwilcox.com/BEES/docs/Dobos%20-%20PVWatts%20v5.pdf).

**pvName**

Name of photovoltaic array. Give after the word PVARRAY.

<%= member_table(
  units: "",
  legal_range: "*63 characters*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**pvElecMtr=*choice***

Name of meter by which this PVARRAY's AC power out is recorded. Generated power is expressed as a negative value.

<%= member_table(
  units: "",
  legal_range: "*name of a METER*",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**pvEndUse=*choice***

Meter end use to which the PVARRAY's generated energy should be accumulated.

<%= insert_file('doc/src/enduses.md') %>

<%= member_table(
  units: "",
  legal_range: "*Codes listed above*",
  default: "PV",
  required: "No",
  variability: "constant")
  %>

**pvDCSysSize=*float***

The rated photovoltaic system DC capacity/size as indicated by the nameplate.

<%= member_table(
  units: "kW",
  legal_range: "x $\\geq$ 0",
  default: "*none*",
  required: "Yes",
  variability: "constant")
  %>

**pvModuleType=*choice***

Type of module to model. The module type determines the refraction index and temperature coefficient used in the simulation. Alternatively, the "Custom" module type may be used in conjunction with user-defined input for *pvCoverRefrInd* and *pvTempCoeff*.

<%= csv_table(<<END, :row_header => true)
  **Module Type**, **pvCoverRefrInd**, **pvTempCoeff**
  Standard,        1.3,                -0.00206
  Premium,         1.3,                -0.00194
  ThinFilm,        1.3,                -0.00178
  Custom,          User-defined,       User-defined
END
%>

<%= member_table(
  units: "",
  legal_range: "Standard Premium ThinFilm Custom",
  default: "Standard",
  required: "No",
  variability: "constant")
  %>

**pvCoverRefrInd=*float***

The refraction index for the coating applied to the module cover. A value of 1.0 represents refraction through air. Coatings have higher refraction indexes that capture more solar at lower angles of incidence.

<%= member_table(
  units: "",
  legal_range: "x $\\geq$ 0",
  default: "1.3",
  required: "No",
  variability: "constant")
  %>

**pvTempCoeff=*float***

The temperature coefficient how the efficiency of the module varies with the cell temperature. Values are typically negative.

<%= member_table(
  units: "1/^o^F",
  legal_range: "*no restrictions*",
  default: "-0.00206",
  required: "No",
  variability: "constant")
  %>

**pvArrayType=*choice***

The type of array describes mounting and tracking options. Roof mounted arrays have a higher installed nominal operating cell temperature (INOCT) of 120 ^o^F compared to the default of 113 ^o^F. Array self-shading is not currently calculated for adjacent rows of modules within an array.

<%= member_table(
  units: "",
  legal_range: " FixedOpenRack, FixedRoofMount, OneAxisTracking, TwoAxisTracking",
  default: "FixedOpenRack",
  required: "No",
  variability: "constant")
  %>

**pvTilt=*float***

The tilt of the photovoltaic array from horizontal.  Values outside the range 0 to 360 are first normalized to that range. For one-axis tracking, defines the tilt of the rotation axis. Not used for two-axis tracking arrays.  Should be omitted if pvVertices is given.

<%= member_table(
  units: "degrees",
  legal_range: "unrestricted",
  default: "from pvVertices (if given) else 0",
  required: "No",
  variability: "hourly")
  %>

The following figures illustrate the use of both pvTilt and pvAzm for various configurations:

![Fixed, south facing, tilted at 40^o^](media/pv_fixed.png)

![One-axis tracker, south facing, tilted at 20^o^](media/pv_tilted_tracker_south.png)

![One-axis tracker, horizontal aligned North/South (more common)](media/pv_horiz_tracker_south.png)

![One-axis tracker, horizontal aligned East/West (less common)](media/pv_horiz_tracker_east.png)

**pvAzm=*float***

Photovoltaic array azimuth (0 = north, 90 = east, etc.). If a value outside the range 0^o^ $\leq$ *x* $<$ 360^o^ is given, it is normalized to that range. For one-axis tracking, defines the azimuth of the rotation axis. Not used for two-axis tracking arrays. Should be omitted if pvVertices is given.

<%= member_table(
  units: "degrees",
  legal_range: "unrestricted",
  default: "from pvVertices (if given) else 0",
  required: "No",
  variability: "hourly")
  %>

**pvVertices=*list of up to 36 floats***

  Vertices of an optional polygon representing the position and shape of the photovoltaic array.  The polygon is used to calculate the shaded fraction using an advanced shading model.  Only PVARRAYs and [SHADEXs](#shadex) are considered in the advanced shading model -- PVARRAYs can be shaded by SHADEXs or other PVARRAYs.  If pvVertices is omitted, the PVARRAY is assumed to be unshaded at all times.  Advanced shading must be enabled via [TOP exShadeModel](#top-model-control-items).  Note that the polygon is used only for evaluating shading; array capacity is specified by pvDCSysSize (above).

  The values that follow pvVertices are a series of X, Y, and Z values for the vertices of the polygon using a coordinate system defined from a viewpoint facing north.  X and Y values convey east-west and north-south location respectively relative to an arbitrary origin (positive X value are to the east; positive Y values are to the north).  Z values convey height relative to the building 0 level and positive values are upward.

  The vertices are specified in counter-clockwise order when facing the receiving surface of the PVARRAY.  The number of values provided must be a multiple of 3.  The defined polygon must be planar and have no crossing edges.  When pvMounting=Building, the effective position of the polygon is modified in response to building rotation specified by [TOP bldgAzm](#top-general-data-items).

  For example, to specify a rectangular photovoltaic array that is 10 x 20 ft, tilted 45 degrees, and facing south --

     pvVertices = 0, 0, 15,   20, 0, 15,  20, 7.07, 22.07,  0, 7.07, 22.07

<%= member_table(
  units: "ft",
  legal_range: "unrestricted",
  default: "no polygon",
  required: "9, 12, 15, 18, 21, 24, 27, 30, 33, or 36 values",
  variability: "constant")
  %>

**pvSIF=*float***

  Shading Impact Factor (SIF) of the array used to represent the disproportionate impact on array output of partially shaded modules at the sub-array level. This impact is applied to the effective beam irradiance on the array:

  $$I_{poa,beam,eff} = \max\left(I_{poa,beam}\cdot\left(1-SIF\cdot f_{sh}\right),0\right)$$

  where $f_{sh}$ is the fraction of the array that is shaded.

  Default value is 1.2, which is representative of PV systems with sub-array microinverters or DC power optimizers. For systems without sub-array power electronics, values are closer to 2.0.

<%= member_table(
  legal_range: "*x* $\\geq$ 1.0",
  default: "1.2",
  required: "No",
  variability: "constant") %>


**pvMounting=*choice***

  Specified mounting location of this PVARRAY.  pvMounting=Site indicates the array position is not altered by building rotation via [TOP bldgAzm](#top-general-data-items), while PVARRAYs with pvMounting=Building are assumed to rotate with the building.

<%= member_table(
  units: "",
  legal_range: "Building or Site",
  default: "Building",
  required: "No",
  variability: "constant")
  %>

**pvGrndRefl=*float***

Ground reflectance used for calculating reflected solar incidence on the array.

<%= member_table(
  units: "",
  legal_range: "0 $<$ *x* $<$ 1.0",
  default: "0.2",
  required: "No",
  variability: "hourly")
  %>

<!-- Hide
**pvGCR=*float***

Ground coverage ratio is. This is currently unused as array self-shading is not calculated.

  ----------------------------------------------------------------------------------
  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ---------------------   ------------- -------------- -----------------
              0 &lt; *x* $\le$ 1.0    0.4           No             constant
  ----------------------------------------------------------------------------------

-->

**pvDCtoACRatio=*float***

DC-to-AC ratio used to intentionally undersize the AC inverter. This is used to increase energy production in the beginning and end of the day despite the possibility of clipping peak sun hours.

<%= member_table(
  units: "",
  legal_range: "*x* &gt; 0.0",
  default: "1.2",
  required: "No",
  variability: "constant")
  %>

**pvInverterEff=*float***

AC inverter efficiency at rated DC power.

<%= member_table(
  units: "",
  legal_range: "0 $<$ *x* $<$ 1.0",
  default: "0.96",
  required: "No",
  variability: "constant")
  %>

**pvSysLosses=*float***

Fraction of total DC energy lost. The total loss from a system is aggregated from several possible causes as illustrated below:

<%= csv_table(<<END, :row_header => true)
  **Loss Type**,             **Default Assumption**
  Soiling,                   0.02
  *Shading*,                 *0 (handled explicitly)*
  Snow,                      0
  *Mismatch*,                *0 (shading mismatch handled explicitly [see pvSIF])*
  Wiring,                    0.02
  Connections,               0.005
  Light-induced degradation, 0.015
  Nameplate rating,          0.01
  *Age*,                     *0.05 (estimated 0.5% degradation over 20 years)*
  Availability,              0.03
  **Total**,                 **0.14**
END
%>

*Italic* lines indicate differences from PVWatts assumptions.

<%= member_table(
  units: "",
  legal_range: "0 $<$ *x* $<$ 1.0",
  default: "0.14",
  required: "No",
  variability: "hourly")
  %>

**endPVARRAY**

Optionally indicates the end of the PVARRAY definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

<%= member_table(
  units: "",
  legal_range: "",
  default: "*none*",
  required: "No",
  variability: "constant")
  %>

**Related Probes:**

- @[PVArray](#p_pvarray)
