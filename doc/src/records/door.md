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

Provides user control over how program models this door:

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

  FORWARD\_DIFFERENCE                 Selects the forward difference
                                      model (used with short time steps
                                      and the CZM zone model)
  ----------------------------------- -----------------------------------

  ----------------------------------------------------------------------
  **Unit **Legal Range**                       **Defau **Requir **Variab
  s**                                          lt**    ed**     i lity**
  ------ ------------------------------------- ------- -------- --------
         QUICK, DELAYED DELAYED\_HOUR,         AUTO    No       constant
         DELAYED\_SUBOUR, AUTO,                                 
         FORWARD\_DIFFERENCE                                    
  ----------------------------------------------------------------------

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              QUICK, DELAYED, AUTO   Auto          No             constant

Either drCon or drU must be specified, but not both.

**drCon=*conName***

Name of construction for door.

  **Units**   **Legal Range**            **Default**   **Required**         **Variability**
  ----------- -------------------------- ------------- -------------------- -----------------
              name of a *CONSTRUCTION*   *None*        unless *drU* given   constant

**drU=*float***

Door U-value, NOT including surface (air film) conductances. Allows direct entry of U-value, without defining a CONSTRUCTION, when no heat capacity effects are to be modeled.

  **Units**         **Legal Range**   **Default**               **Required**           **Variability**
  ----------------- ----------------- ------------------------- ---------------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         Determined from *drCon*   if *drCon* not given   constant

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

**drExAbs=*float***

Door exterior solar absorptivity. Applicable only if sfExCon of owning surface is AMBIENT or SPECIFIEDT.

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

**endDoor**

Indicates the end of the door definition. Alternatively, the end of the door definition can be indicated by the declaration of another object or by END.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


