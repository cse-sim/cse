# Input Data

This section describes the input for each CSE class (object type). For each object you wish to define, the usual input consists of the class name, your name for the particular object (usually), and zero or more member value statements of the form *name=expression*. The name of each subsection of this section is a class name (HOLIDAY, MATERIAL, CONSTRUCTION, etc.). The object name, if given, follows the class name; it is the first thing in each description (hdName, matName, conName, etc.). Exception: no statement is used to create or begin the predefined top-level object "Top" (of class TOP); its members are given without introduction.

After the object name, each member's description is introduced with a line of the form *name=type*. *Type* indicates the appropriate expression type for the value:

-   *float*

-   *int*

-   *string*

-   \_\_\_\_*name* (object name for specified type of object)

-   *choice*

-   *date*

These types discussed in the section on [expression types](#expression-types).

Each member's description continues with a table of the form

  -------------------------------------------------------------------------
  **Units**  **Legal Range**  **Default**  **Required**   **Variability**
  ---------- ---------------- ------------ ------------- ------------------
    ft^2^        x &gt; 0     wnHeight \*       No            constant
                                wnWidth                  
  -------------------------------------------------------------------------

  -------------- ---------------------------------------------------------
  *Units*        units of measure (lb., ft, Btu, etc.) where applicable

  *Legal*        limits of valid range for numeric inputs; valid choices
  *Range*        for *choice* members, etc.

  *Default*      value assumed if member not given; applicable only if not
                 required

  *Required*     YES if you must give this member

  *Variability*  how often the given expression can change: hourly, daily,
                 etc. See sections on
                 [expressions](#expressions-overview),
                 [statements](#member-statements), and
                 [variation frequencies](#variation-frequencies-revisited)
  -------------- ---------------------------------------------------------

