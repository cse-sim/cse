# SHADEX

SHADEX describes an object that shades other building surfaces.  As initially implemented, SHADEX calculations are provided only for PVARRAYs.

**sxName**

Name of photovoltaic array. Give after the word SHADEX.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**sxType=*choice***

Specifies type of shade.  sxType=Site indicates the SHADEX position is not altered by building rotation via bldgAzm, while SHADEXs with sxType=Building are assumed to rotate with the building.

**Units**   **Legal Range**        **Default**   **Required**   **Variability**
----------- ---------------------- ------------- -------------- -----------------
            Building or Site        Site           No             constant

**sxVertices=*list of up to 27 floats***

  Vertices of a polygon representing the shape of the shading object.

  The values that follow sxVertices are a series of X, Y, and Z values for the vertices of the polygon. The coordinate system is defined from a viewpoint facing north.  X and Y values convey east-west and north-south location respectively relative to an arbitrary origin (positive X value are to the east; positive Y values are to the north).  Z values convey height relative to the building 0 level and positive values are upward.

  The vertices are specified in counter-clockwise order when facing the shading object from the south.  The number of values provided must be a multiple of 3.  The defined polygon must be planar and have no crossing edges.  When sxType=Building, the effective position of the polygon reflects building rotation specified by bldgAzm.

  For example, to specify a rectangular shade "tree" that is 10 x 40 ft, facing south, and 100 ft to the south of the nominal building origin --

     sxVertices = 5, -100, 0,   15, -100, 0,  15, -100, 40,  5, -100, 40

  ------------------------------------------------------------------
  **Units** **Legal Range** **Default** **Required**     **Variability**
  --------- --------------- ----------- ---------------- ---------------
  ft         unrestricted     *none*      9, 12, 15, 18,      constant
                                         21, 24, or 27
                                         values
  ------------------------------------------------------------------

  **endSHADEX**

  Optionally indicates the end of the SHADEX definition. Alternatively, the end of the definition can be indicated by END or by beginning another object.

    **Units**   **Legal Range**   **Default**   **Required**   **Variability**
    ----------- ----------------- ------------- -------------- -----------------
                                  *N/A*         No             constant
