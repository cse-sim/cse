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

{{ read_csv('../assets/tables/input-data--member-table.csv') }}

where the column headers have the following meaning:

{{ read_csv('../assets/tables/input-data--member-table-definition.csv') }}
