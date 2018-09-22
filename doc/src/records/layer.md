# LAYER

LAYER constructs a subobject of class LAYER belonging to the current CONSTRUCTION. LAYER is not recognized except immediately following CONSTRUCTION or another LAYER. The members represent one layer (that optionally includes framing) within the CONSTRUCTION.

The layers should be specified in inside to outside order. <!-- TODO: this order is unfortunate! --> A framed layer (lrFrmMat and lrFrmFrac given) is modeled by creating a homogenized material with weighted combined conductivity and volumetric heat capacity. Caution: it is generally preferable to model framed constructions using two separate surfaces (one with framing, one without). At most one framed layer (lrFrmMat and lrFrmFrac given) is allowed per construction.

The layer thickness may be given by lrThk, or matThk of the material, or matThk of the framing material if any. The thickness must be specified at least one of these three places; if specified in more than one place and not consistent, an error message occurs.

**lrName**

Name of layer (follows "LAYER"). Required only if the LAYER is later referenced in another object, for example with LIKE or ALTER; however, we suggest naming all objects for clearer error messages and future flexibility.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *None*        No             constant

**lrMat=*matName***

Name of primary MATERIAL in layer.

  **Units**   **Legal Range**        **Default**   **Required**   **Variability**
  ----------- ---------------------- ------------- -------------- -----------------
              name of a *MATERIAL*   *none*        Yes            constant

**lrThk=*float***

Thickness of layer.

  **Units**   **Legal Range**   **Default/Required**                                       **Variability**
  ----------- ----------------- ---------------------------------------------------------- -----------------
  ft          *x* $>$ 0         Required if *matThk* not specified in referenced *lrMat*   constant

**lrFrmMat=*matName***

Name of framing MATERIAL in layer, if any. At most one layer with lrFrmMat is allowed per CONSTRUCTION. See caution above regarding framed-layer model.

  **Units**   **Legal Range**      **Default**         **Required**   **Variability**
  ----------- -------------------- ------------------- -------------- -----------------
              name of a MATERIAL   *no framed layer*   No             constant

**lrFrmFrac=*float***

Fraction of layer that is framing. Must be specified if frmMat is specified. See caution above regarding framed-layer model.

  ---------------------------------------------------------------
  **Units** **Legal**    **Default** **Required** **Variability**
            **Range**
  --------- ------------ ----------- ------------ ---------------
            0 $\leq$ *x* *no framed  Required if  constant
            $\leq$ 1     layer*      *lrFrmMat*
                                     specified,
                                     else
                                     disallowed

  ---------------------------------------------------------------

**endLayer**

Optional end-of-LAYER indicator; LAYER definition may also be indicated by "END" or just starting the definition of another LAYER or other object.

**Related Probes:**

- @[layer](#p_layer)
