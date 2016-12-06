# DOOR

DOOR constructs a subobject belonging to the current SURFACE. The azimuth, tilt, ground reflectivity and exterior conditions associated with the door are the same as those of the owning surface, although the exterior surface conductance and the exterior absorptivity can be altered.

**drName**

Name of door.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**drArea=*float***

Overall area of door.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^2^       *x* $>$ 0         *none*        Yes            constant

**drModel=*choice***

Provides user control over how CSE models conduction for this door:

  ----------------------------------- -----------------------------------
  QUICK                               Surface is modeled using a simple
                                      conductance. Heat capacity effects
                                      are ignored. Either drCon or drU
                                      (next) can be specified.

  DELAYED, DELAYED\_HOUR,             Surface is modeled using a
  DELAYED\_SUBOUR                     multi-layer finite difference
                                      technique which represents heat
                                      capacity effects. If the time
                                      constant of the door is too short
                                      to accurately simulate, a warning
                                      message is issued and the Quick
                                      model is used. drCon (next) must be
                                      specified -- the program cannot use
                                      the finite difference model if drU
                                      rather than drCon is specified.

  AUTO                                Program selects Quick or
                                      appropriate Delayed automatically
                                      according to the time constant of
                                      the surface (if drU is specified,
                                      Quick is selected).

  FD or                               Selects the forward difference
  FORWARD\_DIFFERENCE                 model (used with short time steps
                                      and the CZM/UZM zone models)
  ----------------------------------- -----------------------------------

  ---------------------------------------------------------------
  **Units** **Legal**        **Default** **Required** **Variability**
            **Range**
  ------    --------------- ----------- ------------ ---------------
            *choices above*      AUTO        No           constant

  ---------------------------------------------------------------


Either drU or drCon must be specified, but not both.

**drU=*float***

Door U-value, NOT including surface (air film) conductances. Allows direct entry of U-value, without defining a CONSTRUCTION, when no heat capacity effects are to be modeled.

  **Units**         **Legal Range**   **Default**               **Required**           **Variability**
  ----------------- ----------------- ------------------------- ---------------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         Determined from *drCon*   if *drCon* not given   constant

**drCon=*conName***

Name of construction for door.

  **Units**   **Legal Range**            **Default**   **Required**         **Variability**
  ----------- -------------------------- ------------- -------------------- -----------------
              name of a *CONSTRUCTION*   *None*        unless *drU* given   constant

**drLThkF=*float***

Sublayer thickness adjustment factor for FORWARD\_DIFFERENCE conduction model used with drCon surfaces.  Material layers in the construction are divided into sublayers as needed for numerical stability.  drLThkF allows adjustment of the thickness criterion used for subdivision.  A value of 0 prevents subdivision; the default value (0.5) uses layers with conservative thickness equal to half of an estimated safe value.  Fewer (thicker) sublayers improves runtime at the expense of accurate representation of rapid changes.

**Units**   **Legal Range**            **Default**   **Required**         **Variability**
----------- -------------------------- ------------- -------------------- -----------------
             x $\ge$ 0                    .5             No                  constant


**drExAbs=*float***

Door exterior solar absorptivity. Applicable only if sfExCnd of owning surface is AMBIENT or SPECIFIEDT.

**Units**         **Legal Range**   **Default**              **Required**   **Variability**
----------------- ----------------- ------------------------ -------------- -----------------
Btuh/ft^2^-^o^F   *x* $>$ 0         same as owning surface   No             monthly-hourly

**drInAbs=*float***

Door interior solar absorptivity.

**Units**   **Legal Range**       **Default**   **Required**   **Variability**
----------- --------------------- ------------- -------------- -----------------
(none)      0 $\le$ *x* $\le$ 1   0.5           No             monthly-hourly

**drExEpsLW=*float***

Door exterior long wave (thermal) emittance.

**Units**   **Legal Range**       **Default**   **Required**   **Variability**
----------- --------------------- ------------- -------------- -----------------
(none)      0 $\le$ *x* $\le$ 1   0.9           No             constant

**drInEpsLW=*float***

Door interior long wave (thermal) emittance.

**Units**   **Legal Range**       **Default**   **Required**   **Variability**
----------- --------------------- ------------- -------------- -----------------
(none)      0 $\le$ *x* $\le$ 1   0.9           No             constant


**drInH=*float***

Door interior surface (air film) conductance. Ignored if drModel = Forward\_Difference

**Units**         **Legal Range**   **Default**              **Required**   **Variability**
----------------- ----------------- ------------------------ -------------- -----------------
Btuh/ft^2^-^o^F   *x* $>$ 0         same as owning surface   No             constant

**drExH=*float***

Door exterior surface (air film) conductance. Ignored if drModel = Forward\_Difference

**Units**         **Legal Range**   **Default**              **Required**   **Variability**
----------------- ----------------- ------------------------ -------------- -----------------
Btuh/ft^2^-^o^F   *x* $>$ 0         same as owning surface   No             constant



  When drModel = Forward\_Difference, several models are available for calculating inside and outside surface convective coefficients.  Inside surface faces can be exposed only to zone conditions. Outside faces may be exposed either to ambient conditions or zone conditions, based on drExCnd.  Only UNIFIED and INPUT are typically used.  The other models were used during CSE development for comparison.  For details, see CSE Engineering Documentation.

  Model            Exposed to ambient              Exposed to zone
  ---------------- ------------------------------- ----------------------------
  UNIFIED          default CSE model               default CSE model
  INPUT            hc = drExHcMult                 hc = drxxHcMult
  AKBARI           Akbari model                    n/a
  WALTON           Walton model                    n/a
  WINKELMANN       Winkelmann model                n/a
  MILLS            n/a                             Mills model
  ASHRAE           n/a                             ASHRAE handbook values
  --------------- ------------------------------- ----------------------------

**drExHcModel=*choice***

Selects the model used for exterior surface convection when drModel = Forward\_Difference.

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
---------- ----------------- ------------- -------------- -----------------
            *choices above*  UNIFIED         No             constant

**drExHcLChar=*float***

Characteristic length of surface, used in derivation of forced exterior convection coefficients in some models when outside face is exposed to ambient (i.e. to wind).

**Units**   **Legal Range**   **Default**   **Required**   **Variability**
----------- ----------------- ------------- -------------- -----------------
      ft            x > 0              10            No            constant


**drExHcMult=*float***

Exterior convection coefficient adjustment factor.  When drExHcModel=INPUT, hc=drExHcMult.  For other drExHcModel choices, the model-derived hc is multiplied by drExHcMult.

**Units**         **Legal Range**   **Default**   **Required**   **Variability**
----------------- ----------------- ------------- -------------- -----------------
                                       1               No            subhourly

**drInHcModel=*choice***

Selects the model used for the inside (zone) surface convection when drModel = Forward\_Difference.

**Units**   **Legal Range**                    **Default**   **Required**   **Variability**
---------- ----------------------------------- ------------- -------------- -----------------
            *choices above (see drExHcModel)*  UNIFIED         No             constant

**drInHcMult=*float***

Interior convection coefficient adjustment factor.  When drInHcModel=INPUT, hc=drInHcMult.  For other drInHcModel choices, the model-derived hc is multiplied by drInHcMult.

**Units**         **Legal Range**   **Default**   **Required**   **Variability**
----------------- ----------------- ------------- -------------- -----------------
                                       1               No            subhourly

**endDoor**

Indicates the end of the door definition. Alternatively, the end of the door definition can be indicated by the declaration of another object or by END.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant
