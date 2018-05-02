# CONSTRUCTION

CONSTRUCTION constructs an object of class CONSTRUCTION that represents a light weight or massive ceiling, wall, floor, or mass assembly (mass assemblies cannot, obviously, be lightweight). Once defined, CONSTRUCTIONs can be referenced from SURFACEs (below). A defined CONSTRUCTION need not be referenced. Each CONSTRUCTION is optionally followed by LAYERs, which define the constituent LAYERs of the construction.

**conName**

Name of construction. Required for reference from SURFACE and DOOR objects, below.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
              *63 characters*   *none*        Yes            constant

**conU=*float***

U-value for the construction (NOT including surface (air film) conductances; see SURFACE statements). If omitted, one or more LAYERs must immediately follow to specify the LAYERs that make up the construction. If specified, no LAYERs can follow.

  --------------------------------------------------------------
  **Units**   **Legal** **Default** **Required** **Variability**
              **Range**
  ----------- --------- ----------- ------------ ---------------
  Btuh/ft^2^- *x* $>$ 0 calculated  if omitted,  constant
  ^o^F                  from LAYERs LAYERs must
                                    follow         

  --------------------------------------------------------------

**endConstruction**

Optional to indicates the end of the CONSTRUCTION. Alternatively, the end of the CONSTRUCTION definition can be indicated by "END" or by beginning another object If END or endConstruction is used, it should follow the construction's LAYER subobjects, if any.

  **Units**   **Legal Range**   **Default**   **Required**   **Variability**
  ----------- ----------------- ------------- -------------- -----------------
                                *N/A*         No             constant

**Related Probes:**

- [@construction](#p_construction)
