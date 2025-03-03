# Input Data

This section describes the input for each CSE class (object type). For each object you wish to define, the usual input consists of the class name, your name for the particular object (usually), and zero or more member value statements of the form _name=expression_. The name of each subsection of this section is a class name (HOLIDAY, MATERIAL, CONSTRUCTION, etc.). The object name, if given, follows the class name; it is the first thing in each description (hdName, matName, conName, etc.). Exception: no statement is used to create or begin the predefined top-level object "Top" (of class TOP); its members are given without introduction.

After the object name, each member's description is introduced with a line of the form _name=type_. _Type_ indicates the appropriate expression type for the value:

- _float_

- _int_

- _string_

- \_\_\_\__name_ (object name for specified type of object)

- _choice_

- _date_

These types discussed in the section on [expression types][expression-types].

Each member's description continues with a table of the form:

{{
  member_table({
    "units": "ft^2^",
    "legal_range": "x &gt; 0", 
    "default": "wnHeight \\\* wnWidth",
    "required": "No",
    "variability": "constant" 
  })
}}

where the column headers have the following meaning:

{{ csv_table("**Units**,units of measure (lb.&comma; ft&comma; Btu&comma; etc.) where applicable
**Legal Range**,limits of valid range for numeric inputs; valid choices for *choice* members&comma; etc.
**Default**,value assumed if member not given; applicable only if not required
**Required**,YES if you must give this member
**Variability**,how often the given expression can change: hourly&comma; daily&comma; etc. See sections on [expressions][expressions-overview]&comma; [statements][member-statements]&comma; and [variation frequencies][variation-frequencies-revisited]")
}}
