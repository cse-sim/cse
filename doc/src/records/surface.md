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

  ---------------------------------------------------------------
  **Units** **Legal**    **Default** **Required** **Variability**
            **Range**
  --------- ------------ ----------- ------------ ---------------
  degrees   unrestricted *none*      Required if       constant
                                     *sfTilt*
                                     $\neq$ 0
                                     and *sfTilt*
                                     $\neq$ 180               

  ---------------------------------------------------------------

**sfModel=*choice***

Provides user control over how CSE models conduction for this surface.

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
                                      and the CZM/UZM zone model).

  2D_FND                              Uses a two-dimensional finite
                                      difference model to simulate heat
                                      flow through foundation surfaces.
  ----------------------------------- -----------------------------------

<%= member_table(
  legal_range: "QUICK, DELAYED, DELAYED\_HOUR, DELAYED\_SUBOUR, AUTO, 2D_FND",
  default: "AUTO",
  required: "No",
  variability: "constant") %>

<!--
TODO: better sfModel desciptions
-->
Either sfU or sfCon must be specified, but not both.

**sfU=*float***

Surface U-value (NOT including surface (air film) conductances). For surfaces for which no heat capacity is to be modeled, allows direct entry of U-value without defining a CONSTRUCTION.

  **Units**         **Legal Range**   **Default**               **Required**           **Variability**
  ----------------- ----------------- ------------------------- ---------------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         Determined from *sfCon*   if *sfCon* not given   constant

**sfCon=*conName***

Name of CONSTRUCTION of the surface.

  **Units**   **Legal Range**            **Default**   **Required**         **Variability**
  ----------- -------------------------- ------------- -------------------- -----------------
              Name of a *CONSTRUCTION*   *none*        unless *sfU* given   constant

**sfLThkF=*float***

Sublayer thickness adjustment factor for FORWARD\_DIFFERENCE conduction model used with sfCon surfaces.  Material layers in the construction are divided into sublayers as needed for numerical stability.  sfLThkF allows adjustment of the thickness criterion used for subdivision.  A value of 0 prevents subdivision; the default value (0.5) uses layers with conservative thickness equal to half of an estimated safe value.  Fewer (thicker) sublayers improves runtime at the expense of accurate representation of rapid changes.

**Units**   **Legal Range**            **Default**   **Required**         **Variability**
----------- -------------------------- ------------- -------------------- -----------------
             x $\geq$ 0                    .5             No                  constant

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

  GROUND         The surface is in contact with the
                 ground. Details of the two-dimensional
                 foundation design are defined by
                 sfFnd. Only floor and wall surfaces
                 may use this option.

  ADIABATIC      Exterior surface heat flow is 0.
                 Thermal storage effects of delayed
                 surfaces are modeled.
  -------------- ---------------------------------------

**sfExAbs=*float***

Surface exterior absorptivity.

-------------------------------------------------------------------------
**Units** **Legal**   **Default** **Required**            **Variability**
          **Range**
--------- ----------- ----------- ----------------------- ---------------
(none)    0 $\le$ *x*     0.5     Required if *sfExCnd* = monthly-
          $\le$ 1                 AMBIENT or *sfExCnd* =  hourly
                                  SPECIFIEDT
-------------------------------------------------------------------------

**sfInAbs=*float***

Surface interior solar absorptivity.

----------------------------------------------------------------------------
**Units** **Legal**    **Default**              **Required** **Variability**
          **Range**
--------- ------------ ------------------------ ------------ ---------------
(none)    0 $\le$ *x*  sfType = CEILING, 0.2;\  No           monthly-
          $\le$ 1      sfType = WALL, 0.6;\                  hourly
                       sfType = FLOOR, 0.8
----------------------------------------------------------------------------

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
^o^F        *unrestricted*    *none*        Required if *sfExCnd* = SPECIFIEDT   hourly

**sfAdjZn=*znName***

Name of adjacent zone; used only when sfExCnd is ADJZN. Can be the same as the current zone.

  **Units**   **Legal Range**    **Default**   **Required**                          **Variability**
  ----------- ------------------ ------------- ------------------------------------- -----------------
              name of a *ZONE*   *none*        Required when<br/>*sfExCnd* = ADJZN   constant

**sfGrndRefl=*float***

Ground reflectivity for this surface.

**Units**   **Legal Range**         **Default**   **Required**   **Variability**
----------- ----------------------- ------------- -------------- ------------------
fraction    0 $\leq$ *x* $\leq$ 1   grndRefl      No             Monthly - Hourly


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

Outside combined surface (air film) conductance. Ignored for sfModel = Forward\_Difference. The default value is dependent upon the exterior conditions:

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


When sfModel = Forward\_Difference, several models are available for calculating inside and outside surface convective coefficients.  Inside surface faces can be exposed only to zone conditions. Outside faces may be exposed either to ambient conditions or zone conditions, based on sfExCnd.  Only UNIFIED and INPUT are typically used.  The other models were used during CSE development for comparison.  For details, see CSE Engineering Documentation.

Model            Exposed to ambient              Exposed to zone
---------------- ------------------------------- ----------------------------
UNIFIED          default CSE model               default CSE model
INPUT            hc = sfExHcMult                 hc = sfxxHcMult
AKBARI           Akbari model                    n/a
WALTON           Walton model                    n/a
WINKELMANN       Winkelmann model                n/a
MILLS            n/a                             Mills model
ASHRAE           n/a                             ASHRAE handbook values
--------------- ------------------------------- ----------------------------

**sfExHcModel=*choice***

<!-- TODO: How does this impact Kiva? -->

Selects the model used for exterior surface convection when sfModel = Forward\_Difference.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
---------- ----------------- ------------- -------------- -----------------
            *choices above*  UNIFIED         No             constant

**sfExHcLChar=*float***

Characteristic length of surface, used in derivation of forced exterior convection coefficients in some models when outside surface is exposed to ambient.  See sfExHcModel.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
ft            x > 0              10            No            constant


**sfExHcMult=*float***

Exterior convection coefficient adjustment factor.  When sfExHcModel=INPUT, hc=sfExHcMult.  For other sfExHcModel choices, the model-derived hc is multiplied by sfExHcMult.

**Units**         **Legal Range**   **Default**   **Required**   **Variability**
----------------- ----------------- ------------- -------------- -----------------
                                       1               No            subhourly

**sfExRf=*float***

Exterior surface roughness factor.  Used only when surface is exposed to ambient (i.e. with wind exposure).  Typical values:

Roughness Index	   sfExRf	 Example
-----------------  ------  ---------
1 (very rough)		 2.17	   Stucco
2 (rough)          1.67 	 Brick
3 (medium rough)	 1.52 	 Concrete
4 (Medium smooth)	 1.13	   Clear pine
5 (Smooth)         1.11    Smooth plaster
6 (Very Smooth)		 1		   Glass

----------------------------------------------------------------------------------------------
**Units** **Legal Range**  **Default**                       **Required**   **Variability**
--------  ---------------- --------------------------------- -------------- -----------------
                           sfExHcModel = WINKELMANN: 1.66\        No           constant
                           else 2.17               
--------  ---------------- --------------------------------- -------------- -----------------


**sfInHcModel=*choice***

<!-- TODO How does this change for Kiva? -->

  Selects the model used for the inside (zone) surface convection when sfModel = Forward\_Difference.

  **Units**   **Legal Range**                    **Default**   **Required**   **Variability**
  ---------- ----------------------------------- ------------- -------------- -----------------
              *choices above (see sfExHcModel)*  UNIFIED         No             constant

  **sfInHcMult=*float***

  Interior convection coefficient adjustment factor.  When sfInHcModel=INPUT, hc=sfInHcMult.  For other sfInHcModel choices, the model-derived hc is multiplied by sfInHcMult.

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
                                         1               No            subhourly

The items below give values associated with CSE's model for below grade surfaces (sfExCnd=GROUND).  See CSE Engineering Documentation for technical details.

**sfFnd=*fdName***

Name of FOUNDATION applied to ground-contact surfaces; used only for Floor and Wall SURFACEs when sfExCnd is GROUND.

<%= member_table(
  legal_range: "Name of a *Foundation*",
  default: "*none*",
  required: "when<br/>*sfExCnd* = GROUND",
  variability: "constant") %>


**sfHeight=*float***

Needed for foundation wall height, otherwise ignored. Maybe combine with sfDepthBG?

<%= member_table(
  units: "ft",
  legal_range: "x $>$ 0",
  default: "*none*",
  required: "when *sfType* is WALL and *sfExtCnd* is GROUND",
  variability: "constant") %>

**sfExpPerim=*float***

Exposed perimeter of foundation floors.

<%= member_table(
  units: "ft",
  legal_range: "x $\geq$ 0",
  default: "*none*",
  required: "when *sfType* is FLOOR and *sfExtCnd* is GROUND",
  variability: "constant") %>


**sfDepthBG=*float***

<!-- TODO How does this change for Kiva? -->

Depth below grade of surface.  For walls, sfDepthBG is measured to the lower edge.  For floors, sfDepthBG is measured to the bottom face.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- --------------   ------------- -------------- ------------------
ft            x $\geq$ 0                      No                constant


**sfExCTGrnd=*float***

**sfExCTaDbAvg07=*float***

**sfExCTaDbAvg14=*float***

**sfExCTaDbAvg31=*float***

**sfExCTaDbAvgYr=*float***

Conductances from outside face of surface to the weather file ground temperature and the moving average outdoor dry-bulb temperatures for 7, 14, 31, and 365 days.

**Units**         **Legal Range**   **Default**   **Required**   **Variability**
----------------- ----------------- ------------- -------------- -----------------
Btuh/ft^2^-^o^F    x $\geq$ 0         see above     No             constant

**sfExRConGrnd=*float***

Resistance overall construction resistance.  TODO: full documentation.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- --------------   ------------- -------------- ------------------
             x $\geq$ 0                      No                constant


**endSURFACE**

Optional to indicates the end of the surface definition. Alternatively, the end of the surface definition can be indicated by END, or by beginning another SURFACE or other object definition. If used, should follow the definitions of the SURFACE's subobjects -- DOORs, WINDOWs, SHADEs, SGDISTs, etc.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

<!--
The following tables summarize the defaults and legal ranges of surface members for each sfType. "n.a." indicates "not applicable" and also "not allowed".

  -------------------------------------------------------------------------
  **Member** **WALL**                  **FLOOR**    **CEILING**
  ---------- ------------------------- ------------ -----------------------
  sfTilt     optional, default=90, 60  n.a. (fixed  optional, default=0, 0
             $<$ *sfTilt* $<$ 180      at 180)      $\le$ *sfTilt* $\le$ 60

  sfAzm      **required**              n.a.         **required if sfTilt
                                                    $>$ 0**
  -------------------------------------------------------------------------

  -----------------------------------------------------------------------
  **Member**  **WALL**/**FLOOR**/**CEILING**
  ----------- ------------------------------------------------------------
  sfArea      **required**

  sfCon       **required unless sfU given**

  sfU         **required unless sfCon given**

  sfInH       optional, default = 1.5

  sfExH       optional, default per *sfExCnd*

  sfExCnd     optional, default = AMBIENT

  sfExAbs     optional if *sfExCnd* = AMBIENT or *sfExCnd* = SPECIFIEDT
              (default = .5), else n.a.

  sfExT       **required if sfExCnd = SPECIFIEDT; else n.a.**

  sfAdjZn     **required if sfExCnd = ADJZN; else n.a.**

  sfGrndRefl  optional, default to grndRefl
  -----------------------------------------------------------------------
  -->
