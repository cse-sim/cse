# SHADEX

SHADEX describes an object that shades other building surfaces using an advanced shading model.  Advanced shading calculations are provided only for [PVARRAYs](#pvarray). Advanced shading must be enabled via [Top exShadeModel](#top-model-control-items).

**sxName**

Name of photovoltaic array. Give after the word SHADEX.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**sxMounting=*choice***

Specifies the mounting location of the shade.  sxMounting=Site indicates the SHADEX position is fixed and is not modified if the building is rotated.  The position of SHADEXs with sxMounting=Building are modified to include the effect of building rotation specified via [Top bldgAz](#bldgAzm)

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              Building or Site        Site           No             constant

**sxVertices=*list of up to 36 floats***

Vertices of a polygon representing the shape of the shading object.

The values that follow sxVertices are a series of X, Y, and Z values for the vertices of the polygon. The coordinate system is defined from a viewpoint facing north.  X and Y values convey east-west and north-south location respectively relative to an arbitrary origin (positive X value are to the east; positive Y values are to the north).  Z values convey height relative to the building 0 level and positive values are upward.

The vertices are specified in counter-clockwise order when facing the shading object from the south.  The number of values provided must be a multiple of 3.  The defined polygon must be planar and have no crossing edges.  When sxType=Building, the effective position of the polygon reflects building rotation specified by [TOP bldgAzm](#top-general-data-items).

For example, to specify a rectangular shade "tree" that is 10 x 40 ft, facing south, and 100 ft to the south of the nominal building origin --

    sxVertices = 5, -100, 0,   15, -100, 0,  15, -100, 40,  5, -100, 40

  ----------------------------------------------------------------------
  **Units** **Legal Range** **Default** **Required**     **Variability**
  --------- --------------- ----------- ---------------- ---------------
  ft         unrestricted     *none*     9, 12, 15, 18,      constant
                                         21, 24, 27, 30,
                                         33 or 36
                                         values
  ----------------------------------------------------------------------

**endSHADEX**

Optionally indicates the end of the SHADEX definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- @[SHADEX](#p_shadex)
