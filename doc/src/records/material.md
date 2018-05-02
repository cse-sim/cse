# MATERIAL

MATERIAL constructs an object of class MATERIAL that represents a building material or component for later reference a from LAYER (see below). A MATERIAL so defined need not be referenced. MATERIAL properties are defined in a consistent set of units (all lengths in feet), which in some cases differs from units used in tabulated data. Note that the convective and air film resistances for the inside wall surface is defined within the SURFACE statements related to conductances.

**matName**

Name of material being defined; follows the word "MATERIAL".

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        Yes            constant

**matThk=*float***

Thickness of material. If specified, matThk indicates the discreet thickness of a component as used in construction assemblies. If omitted, matThk indicates that the material can be used in any thickness; the thickness is then specified in each LAYER using the material (see below).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ft          *x* $>$ 0         (omitted)     No             constant

**matCond=*float***

Conductivity of material. Note that conductivity is *always* stated for a 1 foot thickness, even when matThk is specified; if the conductance is known for a specific thickness, an expression can be used to derive matCond.

  **Units**            **Legal Range**   **Default**   **Required**   **Variability**
  -------------------- ----------------- ------------- -------------- -----------------
  Btuh-ft/ft^2^-^o^F   *x* $>$ 0         *none*        Yes            constant

**matCondT=*float***

Temperature at which matCond is rated. See matCondCT (next).

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F        *x* $>$ 0         70 ^o^F       No             constant

**matCondCT=*float***

Coefficient for temperature adjustment of matCond in the forward difference surface conduction model. Each hour (not subhour), the conductivity of layers using this material are adjusted as followslrCond = matCond \* (1 + matCondCT\*(T~layer~ – matCondT))

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
  ^o^F^-1^                      0             No             constant

Note: A typical value of matCondCT for fiberglass batt insulation is 0.00418 F^-1^

**matSpHt=*float***

Specific heat of material.

  **Units**     **Legal Range**   **Default**              **Required**   **Variability**
  ------------- ----------------- ------------------------ -------------- -----------------
  Btu/lb-^o^F   *x* $\ge$ 0       0 (thermally massless)   No             constant

**matDens=*float***

Density of material.

  **Units**   **Legal Range**   **Default**    **Required**   **Variability**
  ----------- ----------------- -------------- -------------- -----------------
  lb/ft^3^    *x* $\ge$ 0       0 (massless)   No             constant

**matRNom=*float***

Nominal R-value per foot of material. Appropriate for insulation materials only and *used for documentation only*. If specified, the current material is taken to have a nominal R-value that contributes to the reported nominal R-value for a construction. As with matCond, matRNom is *always* stated for a 1 foot thickness, even when matThk is specified; if the nominal R-value is known for a specific thickness, an expression can be used to derive matRNom.

  **Units**         **Legal Range**   **Default**   **Required**   **Variability**
  ----------------- ----------------- ------------- -------------- -----------------
  ft^2^-^o^F/Btuh   *x* $>$ 0         (omitted)     No             constant

**endMaterial**

Optional to indicate the end of the material. Alternatively, the end of the material definition can be indicated by "END" or simply by beginning another object.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- [@material](#p_material)
