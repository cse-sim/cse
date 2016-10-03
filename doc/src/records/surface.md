# SURFACE

Surface constructs a ZONE subobject of class SURFACE that represents a surrounding or interior surface of the zone. Internally, SURFACE generates a QUICK surface (U-value only), a DELAYED (massive) surface (using the finite-difference mass model), interzone QUICK surface, or interzone DELAYED surface, as appropriate for the specified construction and exterior conditions.

**sfName**

Name of surface; give after the word SURFACE.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**sfType=*choice***

Type of surface:

  ------------- ---------------------------------------------------------
  FLOOR         Surface defines part or all of the "bottom" of the zone;
                it is horizontal with inside facing up. The outside of
                the surface is not adjacent to the current zone.

  WALL          Surface defines a "side" of the zone; its outside is not
                adjacent to the current zone.

  CEILING       Surface defines part or all of the "top" of the zone with
                the inside facing down. The outside of the surface is not
                adjacent to the current zone.
  ------------- ---------------------------------------------------------

sfType is used extensively for default determination and input checking, but does not have any further internal effect. The Floor, Wall, and Ceiling choices identify surfaces that form boundaries between the zone and some other condition.

  **Units**   **Legal Range**      **Default**   **Required**   **Variability**
  ----------- -------------------- ------------- -------------- -----------------
              FLOOR WALL CEILING   *none*        Yes            constant

**sfArea=*float***

Gross area of surface. (CSE computes the net area for simulation by subtracting the areas of any windows and doors in the surface.).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^2^       *x* $>$ 0         *none*        Yes            constant

**sfTilt=*float***

Surface tilt from horizontal. Values outside the range 0 to 360 are first normalized to that range. The default and allowed range depend on sfType, as follows:

  ------------------ -------------------------------------------
  sfType = FLOOR     *sfTilt*=180, default = 180 (fixed value)
  sfType = WALL      60 $<$ *sfTilt* $<$ 180, default = 90
  sfType = CEILING   0 $\leq$ *sfTilt* $\leq$ 60, default = 0
  ------------------ -------------------------------------------

  **Units**   **Legal Range / Default**            **Required**   **Variability**
  ----------- ------------------------------------ -------------- -----------------
  degrees     Dependent upon *sfType. See above*   No             constant

**sfAzm=*float***

Azimuth of surface with respect to znAzm. The azimuth used in simulating a surface is bldgAzm + znAzm + sfAzm; the surface is rotated if any of those are changed. Values outside the range 0 to 360 are normalized to that range. Required for non-horizontal surfaces.

  ----------------------------------------------------------------------
  **Units **Legal    **Defaul **Required**                      **Variab
  **      Range**    t**                                        i
                                                                l ity**
  ------- ---------- -------- --------------------------------- --------
  degrees unrestrict *none*   Required if *sfTilt* $\neq$ 0 and constant
          ed                  *sfTilt* $\neq$ 180               
  ----------------------------------------------------------------------

**sfModel=*choice***

Provides user control over how program models this surface:

  ----------------------------------- -----------------------------------
  QUICK                               Surface is modeled using a simple
                                      conductance. Heat capacity effects
                                      are ignored. Either sfCon or sfU
                                      (next) can be specified.

  DELAYED, DELAYED\_HOUR,             Surface is modeled using a
  DELAYED\_SUBHOUR                    multi-layer finite difference
                                      technique that represents heat
                                      capacity effects. If the time
                                      constant of the surface is too
                                      short to accurately simulate, a
                                      warning message is issued and the
                                      Quick model is used. The program
                                      **cannot** use the finite
                                      difference model if sfU rather than
                                      sfCon is specified.

  AUTO                                Program selects Quick or the
                                      appropriate Delayed automatically
                                      according to the time constant of
                                      the surface (if sfU is specified,
                                      Quick is selected).

  FD (or FORWARD\_DIFFERENCE)         Selects the forward difference
                                      model (used with short time steps
                                      and the CZM zone model)
  ----------------------------------- -----------------------------------

  **Units**   **Legal Range**                                            **Default**   **Required**   **Variability**
  ----------- ---------------------------------------------------------- ------------- -------------- -----------------
              QUICK, DELAYED, DELAYED\_HOUR, DELAYED\_SUBOUR, AUTO, FD   AUTO          No             constant

<!--
TODO: better sfModel desciptions
-->
One of the following two parameters must be specified:

**sfCon=*conName***

Name of CONSTRUCTION of the surface.

  **Units**   **Legal Range**            **Default**   **Required**         **Variability**
  ----------- -------------------------- ------------- -------------------- -----------------
              Name of a *CONSTRUCTION*   *none*        unless *sfU* given   constant

**sfU=*float***

Surface U-value (NOT including surface (air film) conductances). For surfaces for which no heat capacity is to be modeled, allows direct entry of U-value without defining a CONSTRUCTION.

  **Units**         **Legal Range**   **Default**               **Required**           **Variability**
  ----------------- ----------------- ------------------------- ---------------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         Determined from *sfCon*   if *sfCon* not given   constant

**sfExCnd=*choice***

Specifies the thermal conditions assumed at surface exterior, and at exterior of any subobjects (windows or doors) belonging to current surface. The conditions accounted for are dry bulb temperature and incident solar radiation.

  -------------- ---------------------------------------
  AMBIENT        Exterior surface is exposed to the
                 "weather" as read from the weather
                 file. Solar gain is calculated using
                 solar geometry, sfAzm, sfTilt, and
                 sfExAbs.

  SPECIFIEDT     Exterior surface is exposed to solar
                 radiation as in AMBIENT, but the dry
                 bulb temperature is calculated with a
                 user specified function (sfExT).
                 sfExAbs can be set to 0 to eliminate
                 solar effects.

  ADJZN          Exterior surface is exposed to another
                 zone, whose name is specified by
                 sfAdjZn. Solar gain is 0 unless gain is
                 targeted to the surface with SGDIST
                 below.

  ADIABATIC      Exterior surface heat flow is 0.
                 Thermal storage effects of delayed
                 surfaces are modeled.
  -------------- ---------------------------------------

**sfInH=*float***

Inside surface (air film) conductance. Ignored for sfModel = Forward\_Difference. Default depends on the surface type.

  --------------------------- ------
  sfType = FLOOR or CEILING   1.32
  other                       1.5
  --------------------------- ------

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         *see above*   No             constant

**sfExH=*float***

Outside surface (air film) conductance. Ignored for sfModel = Forward\_Difference. The default value is dependent upon the exterior conditions:

  ------------------------ ---------------------------------------
  sfExCnd = AMBIENT        dflExH (Top-level member, described
                           above)

  sfExCnd = SPECIFIEDT     dflExH (described above)

  sfExCnd = ADJZN          1.5

  sfExCnd = ADIABATIC      not applicable
  ------------------------ ---------------------------------------

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         see above     No             constant

**sfExAbs=*float***

Surface exterior absorptivity.

  -----------------------------------------------------------------------
  **Unit **Legal      **Defaul **Required**                      **Variab
  s**    Range**      t**                                        i
                                                                 l ity**
  ------ ------------ -------- --------------------------------- --------
  (none) 0 $\le$ *x*  0.5      Required if *sfExCon* = AMBIENT   monthly-
         $\le$ 1               or *sfExCon* = SPECIFIEDT         h
                                                                 o urly
  -----------------------------------------------------------------------

**sfInAbs=*float***

Surface interior solar absorptivity.

  -----------------------------------------------------------------------
  **Unit **Legal      **Default**                       **Requir **Variab
  s**    Range**                                        ed**     i lity**
  ------ ------------ --------------------------------- -------- --------
  (none) 0 $\le$ *x*  sfType = CEILING, 0.2; sfType =   No       monthly-
         $\le$ 1      WALL, 0.6; sfType = FLOOR, 0.8             h ourly
  -----------------------------------------------------------------------

**sfExEpsLW=*float***

Surface exterior long wave (thermal) emittance.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
  (none)      0 $\le$ *x* $\le$ 1   0.9           No             constant

**sfInEpsLW=*float***

Surface interior long wave (thermal) emittance.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
  (none)      0 $\le$ *x* $\le$ 1   0.9           No             constant

**sfExT=*float***

Exterior air temperature.

  **Units**   **Legal Range**   **Default**   **Required**                         **Variability**
  ----------- ----------------- ------------- ------------------------------------ -----------------
  ^o^F        *unrestricted*    *none*        Required if *sfExCon* = SPECIFIEDT   hourly

**sfAdjZn=*znName***

Name of adjacent zone; used only when sfExCon is ADJZN. Can be the same as the current zone.

  **Units**   **Legal Range**    **Default**   **Required**                          **Variability**
  ----------- ------------------ ------------- ------------------------------------- -----------------
              name of a *ZONE*   *none*        Required when<br/>*sfExCon* = ADJZN   constant

**sfGrndRefl=*float***

Ground reflectivity for this surface.

  **Units**   **Legal Range**         **Default**   **Required**   **Variability**
  ----------- ----------------------- ------------- -------------- ------------------
  fraction    0 $\leq$ *x* $\leq$ 1   grndRefl      No             Monthly - Hourly

**endSurface**

Optional to indicates the end of the surface definition. Alternatively, the end of the surface definition can be indicated by END, or by beginning another SURFACE or other object definition. If used, should follow the definitions of the SURFACE's subobjects -- DOORs, WINDOWs, SHADEs, SGDISTs, etc.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

The following tables summarize the defaults and legal ranges of surface members for each sfType. "n.a." indicates "not applicable" and also "not allowed".

  ----------------------------------------------------------------------
  **Membe **WALL**                  **FLOOR**    **CEILING**
  r**                                            
  ------- ------------------------- ------------ -----------------------
  sfTilt  optional, default=90, 60  n.a. (fixed  optional, default=0, 0
          $<$ *sfTilt* $<$ 180      at 180)      $\le$ *sfTilt* $\le$ 60

  sfAzm   **required**              n.a.         **required if sfTilt
                                                 $>$ 0**
  ----------------------------------------------------------------------

  -----------------------------------------------------------------------
  \*\*Membe  **WALL**/**FLOOR**/**CEILING**
  r* *       
  ---------- ------------------------------------------------------------
  sfArea     **required**

  sfCon      **required unless sfU given**

  sfU        **required unless sfCon given**

  sfInH      optional, default = 1.5

  sfExH      optional, default per *sfExCon*

  sfExCnd    optional, default = AMBIENT

  sfExAbs    optional if *sfExCon* = AMBIENT or *sfExCon* = SPECIFIEDT
             (default = .5), else n.a.

  sfExT      **required if sfExCon = SPECIFIEDT; else n.a.**

  sfAdjZn    **required if sfExCon = ADJZN; else n.a.**

  sfGrndRef  optional, default to grndRefl
  l          
  -----------------------------------------------------------------------


