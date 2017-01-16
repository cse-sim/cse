# SGDIST

SGDIST creates a subobject of the current window that distributes a specified fraction of that window's solar gain to a specified delayed model (massive) surface. Any remaining solar gain (all of the window's solar gain if no SGDISTs are given) is added to the air of the zone containing the window. A window may have up to three SGDISTs; an error occurs if more than 100% of the window's gain is distributed.

Via members sgFSO and sgFSC, the fraction of the insolation distributed to the surface can be made dependent on whether the zone's shades are open or closed (see ZONE member znSC).

**sgName**

Name of solar gain distribution (follows "SGDIST" if given).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant

**sgSurf=*sfName***

Name of surface to which gain is targeted.

If there is more than surface with the specified name: if one of the surfaces is in the current zone, it is used; otherwise, an error message is issued.

<!--
??Qualified naming scheme for referencing surfaces in other zones.  
-->
The specified surface must be modeled with the Delayed model. If gain is targeted to a Quick model surface, a warning message is issued and the gain is redirected to the air of the associated zone.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
              name of a *SURFACE*   *none*        Yes            constant

**sgSide=*choice***

Designates the side of the surface to which the gain is to be targeted:

  ---------- -----------------------------------
  INTERIOR   Apply gain to interior of surface
  EXTERIOR   Apply gain to exterior of surface
  ---------- -----------------------------------

  ------------------------------------------------------------
  **Units** **Legal** **Default** **Required** **Variability**
            **Range**
  --------- --------- ----------- ------------ ---------------
            INTERIOR, Side of     Yes          constant
            EXTERIOR  surface
                      in zone
                      containing
                      window; or
                      INTERIOR
                      if both
                      sides are
                      in zone
                      containing
                      window.

  ------------------------------------------------------------

<!--
  ??This can produce some strange arrangements; verify that energy balance can be properly defined in all cases.
-->
**sgFSO=*float***

Fraction of solar gain directed to specified surface when the owning window's interior shading is in the open position (when the window's zone's shade closure (znSC) is 0).

  **Units**   **Legal Range**                                           **Default**   **Required**   **Variability**
  ----------- --------------------------------------------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1,and sum of window's sgFSO's $\le$ 1   *none*        Yes            monthly-hourly

**sgFSC=*float***

Fraction of solar gain directed to specified surface when the owning window's interior shading is in the closed position. If the zone's shades are partly closed (znSC between 0 and 1), a proportional fraction between sgFSO and sgFSC is used.

  **Units**   **Legal Range**                                            **Default**   **Required**   **Variability**
  ----------- ---------------------------------------------------------- ------------- -------------- -----------------
              0 $\le$ *x* $\le$ 1, and sum of window's sgFSC's $\le$ 1   *sgFSO*       No             monthly-hourly

**endSGDist**

Optionally indicates the end of the solar gain distribution definition. Alternatively, the end of the solar gain distribution definition can be indicated by END or by just beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant


