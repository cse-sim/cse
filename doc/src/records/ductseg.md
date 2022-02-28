# DUCTSEG

DUCTSEG defines a duct segment. Each RSYS has at most one return duct segment and at most one supply duct segment. That is, DUCTSEG input may be completely omitted to eliminate duct losses.

**dsName**

Optional name of duct segment; give after the word “DUCTSEG” if desired.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        No             constant


**dsTy=*choice***

Duct segment type.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              SUPPLY, RETURN                  Yes            constant

The surface area of a DUCTSEG depends on its shape. 0 surface area is legal (leakage only). DUCTSEG shape is modeled either as flat or round --

-   dsExArea specified: Flat. Interior and exterior areas are assumed to be equal (duct surfaces are flat and corner effects are neglected).
-   dsExArea *not* specified: Round. Any two of dsInArea, dsDiameter, and dsLength must be given. Insulation thickness is derived from dsInsulR and dsInsulMat and this thickness is used to calculate the exterior surface area. Overall inside-to-outside conductance is also calculated including suitable adjustment for curvature.

**dsBranchLen=*float***

Average branch length.

<%= member_table(
  units: "ft",
  legal_range: "x $>$ 0",
  default: "-1.0",
  required: "No",
  variability: "constant") %>

**dsBranchCount=*integer***

Number of branches.

<%= member_table(
  units: "",
  legal_range: "x $>$ 0",
  default: "-1",
  required: "No",
  variability: "constant") %>

**dsBranchCFA=*float***

Floor area served per branch

<%= member_table(
  units: "ft^2^",
  legal_range: "x $>$ 0",
  default: "-1.0",
  required: "No",
  variability: "constant") %>

**dsAirVelDs=*float***

Specified air velocity design.

<%= member_table(
  units: "fpm",
  legal_range: "x $>$ 0",
  default: "-1.0",
  required: "No",
  variability: "constant") %>

**dsExArea=*float***

Duct segment surface area at outside face of insulation for flat duct shape, see above.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft^2^       *x* $\ge$ 0                     No             constant

**dsInArea=*float***

Duct segment inside surface area (at duct wall, duct wall thickness assumed negligible) for round shaped duct.

  -----------------------------------------------------------------
  **Units** **Legal**   **Default**   **Required**  **Variability**
            **Range**
  --------- ----------- ------------- ------------- ---------------
  ft^2^     *x* $\ge$ 0 Derived from  (see above re constant
                         dsDiameter   duct shape)       
                         and dsLength

  -----------------------------------------------------------------

**dsDiameter=*float***

Duct segment round duct diameter (duct wall thickness assumed negligible)

  ----------------------------------------------------------------
  **Units** **Legal**   **Default**  **Required**  **Variability**
            **Range**
  --------- ----------- ------------ ------------- ---------------
  ft        *x* $\ge$ 0 Derived from (see above re constant
                        dsInArea and          duct shape)       
                        dsLength

  ----------------------------------------------------------------

**dsLength=*float***

Duct segment length.

  ----------------------------------------------------------------
  **Units** **Legal**   **Default**  **Required**  **Variability**
            **Range**
  --------- ----------- ------------ ------------- ---------------
  ft        *x* $\ge$ 0 Derived from (see above re constant
                        dsInArea and duct shape)       
                        dsDiameter

  ----------------------------------------------------------------

**dsExCnd=*choice***

Conditions surrounding duct segment.

  ------------------------------------------------------------------
  **Units** **Legal**   **Default**    **Required**  **Variability**
            **Range**
  --------- ----------- -------------  ------------- ---------------
            ADIABATIC,       ADJZN         No           constant
            AMBIENT,                                      
            SPECIFIEDT,                                   
            ADJZN                                         

  ------------------------------------------------------------------

**dsAdjZn=*znName***

Name of zone surrounding duct segment; used only when dsExCon is ADJZN. Can be the same as a zone served by the RSYS owning the duct segment.

  **Units**   **Legal Range**    **Default**   **Required**                      **Variability**
  ----------- ------------------ ------------- --------------------------------- -----------------
              name of a *ZONE*   *none*        Required when *dsExCon* = ADJZN   constant

**dsEpsLW=*float***

Exposed (i.e. insulation) outside surface exterior long wave (thermal) emittance.

  **Units**   **Legal Range**       **Default**   **Required**   **Variability**
  ----------- --------------------- ------------- -------------- -----------------
  (none)      0 $\le$ *x* $\le$ 1   0.9           No             constant

**dsExT=*float***

Air dry-bulb temperature surrounding duct segment. <!-- TODO: what is humidity? -->

  **Units**   **Legal Range**   **Default**   **Required**                         **Variability**
  ----------- ----------------- ------------- ------------------------------------ -----------------
  ^o^F        *unrestricted*    *none*        Required if *sfExCnd* = SPECIFIEDT   hourly


**dsInsulR=*float***

Insulation thermal resistance *not including* surface conductances. dsInsulR and dsInsulMat are used to calculate insulation thickness (see below).  Duct insulation is modeled as a pure conductance (no mass).

  **Units**          **Legal Range**   **Default**   **Required**   **Variability**
  ------------------ ----------------- ------------- -------------- -----------------
  ft^2^-F-hr / Btu   *x* $\ge$ 0       0             No             constant

**dsInsulMat=*matName***

Name of insulation MATERIAL. The conductivity of this material at 70 ^o^F is combined with dsInsulR to derive the duct insulation thickness. If omitted, a typical fiberglass material is assumed having conductivity of 0.025 Btu/hr-ft^2^-F at 70 ^o^F and a conductivity coefficient of .00418 1/F (see MATERIAL). In addition, insulation conductivity is adjusted during the simulation in response its average temperature.  As noted with dsInsulR, duct insulation is modeled as pure conductance -- MATERIAL matDens and matSpHt are ignored.

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              name of a *MATERIAL*   fiberglass    No             constant

**dsLeakF=*float***

Duct leakage. Return duct leakage is modeled as if it all occurs at the segment inlet. Supply duct leakage is modeled as if it all occurs at the outlet.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              0 $<$ x $\le$ 1                 No             constant

**dsExH=*float***

Outside (exposed) surface convection coefficient.

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
  Btuh/ft^2^-^o^F   *x* $>$ 0         .54           No             subhourly

**endDuctSeg**

Optionally indicates the end of the DUCTSEG definition.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             

**Related Probes:**

- @[ductSeg](#p_ductseg)
- @[izXfer](#p_izxfer) (generated as "\<Zone Name\>-DLkI" for supply or "\<Zone Name\>-DLkO" for return)
