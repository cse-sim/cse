# CSE Input Language Statements

This section describes the general form of CSE input language statements that define objects, assign values to the data members of objects, and initiate actions. The concepts of objects and the class hierarchy were introduced in the section on [form of CSE data][form-of-the-cse-data]. Information on statements for specific CSE input language classes and their members is the subject of the [input data][input-data] section.

## Object Statements

As we described in a [previous section][statements-overview], the description of an object is introduced by a statement containing at least the class name, and usually your chosen name for the particular object. In addition, this section will describe several optional qualifiers and modifying clauses that permit defining similar objects without repeating all of the member details, and reopening a previously given object description to change or add to it.

Examples of the basic object-beginning statement:

        ZONE "North";

        METER "Electric - Cooling";

        LAYER;

As described in [the section on nested objects][nested-objects], such a statement is followed by statements giving the object's member values or describing subobjects of the object. The object description ends when you begin another object that is not of a subclass of the object, or when a member of an embedding (higher level) object previously begun is given, or when END is given.

### Object Names

An object name consists of up to 63 characters. If you always enclose the name in quotation marks, punctuation and spaces may be used freely; if the name starts with a letter or dollar sign and consists only of letters, digits, underscore, and dollar sign, and is different from all of the words already defined in CSE input language (as listed below in this section), you may omit the quotes. Capitalization, and Leading and trailing spaces and tabs, are always disregarded by input language processor. Names of 0 length, and names containing control characters (ASCII codes 0-31) are not allowed.

Examples of valid names that do not require quotes:

        North
        gas_meter
        slab140E

The following object names are acceptable if always enclosed in quotes:

        "Front Door"
        "M L King Day"
        "123"
        "3.5-inch wall"

We suggest always quoting object names so you won't have to worry about disallowed words and characters.

Duplicate names result in error messages. Object names must be distinct between objects of the same class which are subobjects of the same object. For example, all ZONE names must be distinct, since all ZONEs are subobjects of Top. It is permissible to have SURFACEs with the same name in different ZONEs -- but it is a good idea to keep all of your object names distinct to minimize the chance of an accidental mismatch or a confusing message regarding some other error.

For some classes, such as ZONE, a name is required for each object. This is because several other statements refer to specific ZONEs, and because a name is needed to identify ZONEs in reports. For other classes, the name is optional. The specific statement descriptions in the [Input Data][input-data] Section 5 say which names are required. We suggest always using object names even where not required; one reason is because they allow CSE to issue clearer error messages.

The following *reserved words will not work as object names unless enclosed in quotes*:

*(this list needs to be assembled and typed in)*

### ALTER

ALTER is used to reopen a previously defined object when it is not possible or desired to give the entire description contiguously.

ALTER could be used if you wish to order the input in a special way. For example, SURFACE objects are subobjects of ZONE and are normally described with the ZONE they are part of. However, if you wanted to put all roofs together, you could use input of the general form:

        ZONE "1";  . . .  (zone 1 description)
        ZONE "2";  . . .
        . . .
        ALTER ZONE "1";               // revert to specifying zone 1
            SURFACE "Roof1";  . . .   (describe roof of zone 1)
        ALTER ZONE "2";
            SURFACE "Roof2";  . . .

ALTER can be used to facilitate making similar runs. For example, to evaluate the effect of a change in the size of a window, you might use:

        ZONE "South";
            SURFACE "SouthWall";
            ...
                WINDOW "BigWindow";
                    wnHeight = 6;  wnWidth = 20;
        . . .
        RUN;          // perform simulation and generate reports
        // data from simulation is still present unless CLEAR given
        ALTER ZONE "South";
            ALTER SURFACE "SouthWall";
                ALTER WINDOW "BigWindow";
                    wnHeight = 4;  wnWidth = 12;  // make window smaller
        RUN;          // perform simulation and print reports again

ALTER also lets you access the predefined "Primary" REPORTFILE and EXPORTFILE objects which will be described in the [Input Data][input-data] Section:

        ALTER REPORTFILE "Primary";    /* open description of object automatically
                                          supplied by CSE -- no other way to access */
            rfPageFmt = NO;            /* Turn off page headers and footers --
                                          not desired when reports are to be
                                          reviewed on screen. */

### DELETE

DELETE followed by a class name and an object name removes the specified object, and any subobjects it has. You might do this after RUN when changing the data for a similar run (but to remove all data, CLEAR is handier), or you might use DELETE after COPYing (below) an object if the intent is to copy all but certain subobjects.

### LIKE clause

LIKE lets you specify that an object being defined starts with the same member values as another object already defined. You then need give only those members that are different. For Example:

        MATERIAL "SheetRock";         // half inch gypsum board
            matCond = .0925;          // conductivity per foot
            matSpHt = .26;            // specific heat
            matDens = 50;             // density
            matThk = 0'0.5;           // thickness 1/2 inch
        MATERIAL "5/8 SheetRock" LIKE "SheetRock"; // 5/8" gypsum board
            matThk = 0'0.625;         // thickness 5/8 inch
            // other members same as "SheetRock", need not be repeated

The object named after LIKE must be already defined and must be of the same class as the new object.

LIKE copies only the member values; it does not copy any subobjects of the prototype object. For example, LIKEing a ZONE to a previously defined ZONE does not cause the new zone to contain the surfaces of the prototype ZONE. If you want to duplicate the surfaces, use COPY instead of LIKE.

### COPY clause

COPY lets you specify that the object being defined is the same as a previously defined object including all of the subobjects of that object. For example,

        . . .
        ZONE "West" COPY "North";
            DELETE WALL "East";
            ALTER WALL "South";
                sfExCnd = ambient;

Specifies a ZONE named "West" which is the same as ZONE North except that it does not contain a copy of West's East wall, and the South wall has ambient exposure.

### USETYPE clause

USETYPE followed by the type name is used in creating an object of a type previously defined with DEFTYPE (next section). Example:

        SURFACE "EastWall" USETYPE "IntWall";     // use interior wall TYPE (below)
            sfAzm = 90;                           // this wall faces to the East
            sfArea = 8 * 30;                      // area of each wall is different
            sfAdjZn = "East";                     // zone on other side of wall

Any differences from the type, and any required information not given in the type, must then be specified. Any member specified in the type may be respecified in the object unless FROZEN (see [this section][freeze]) in the type (normally, a duplicate specification for a member results in an error message).

### DEFTYPE

DEFTYPE is used to begin defining a TYPE for a class. When a TYPE is created, no object is created; rather, a partial or complete object description is stored for later use with DEFTYPE. TYPES facilitate creating multiple similar objects, as well as storing commonly used descriptions in a file to be #included in several different files, or to be altered for multiple runs in comparative studies without changing the including files. Example (boldface for emphasis only):

<!-- this works: tested 7-92 -->
        DEFTYPE SURFACE "BaseWall"                // common characteristics of all walls
            sfType = WALL;                        // walls are walls, so say it once
            sfTilt = 90;                          // all our walls are vertical;
                                                  //  but sfAzm varies, so it is not in TYPE.
            sfU = .83;                            // surf conductance; override if different
            sfModel = QUICK;

        DEFTYPE SURFACE "ExtWall" USETYPE "BaseWall";
            sfExCnd = AMBIENT;                    // other side of wall is outdoors
            sfExAbs = 0.5;                        // member only needed for exterior walls

        DEFTYPE SURFACE "IntWall" USETYPE "BaseWall";   // interior wall
            sfExCnd = ADJZN;                      // user must give sfAdjZn.

In a TYPE as much or as little of the description as desired may be given. Omitting normally-required members does not result in an error message in the type definition, though of course an error will occur at use if the member is not given there.

At use, member values specified in the TYPE can normally be re specified freely; to prevent this, "freeze" the desired member(s) in the type definition with

        FREEZE *memberName*;

Alternately, if you wish to be sure the user of the TYPE enters a particular member even if it is normally optional, use

        REQUIRE *memberName*

Sometimes in the TYPE definition, member(s) that you do not want defined are defined -- for example, if the TYPE definition were itself initiated with a statement containing LIKE, COPY, or USETYPE. In such cases the member specification can be removed with

        UNSET *memberName*;

<!--
Check on LIKE TYPE <name>, COPY TYPE <name>.... no such animals, 7-92 rob
-->
### END and ENDxxxx

END, optionally followed by an object name, can be used to unequivocally terminate an object. Further, as of July 1992 there is still available a specific word to terminate each type of object, such as ENDZONE to terminate a ZONE object. If the object name is given after END or ENDxxxx, an additional check is performed: if the name is not that of an object which has been begun and not terminated, an error message occurs. Generally, we have found it is not important to use END or ENDxxxx, especially since the member names in different classes are distinct.

## Member Statements

As introduced in the section on [statements][statements-overview], statements which assign values to members are of the general form:

        *memberName* = *expression*;

The specific member names for each class of objects are given in Section 5; many have already been shown in examples.

Depending on the member, the appropriate type for the expression giving the member value may be numeric (integer or floating point), string, object name, or multiple-choice. Expressions of all types will be described in detail in the section on [expressions][expressions].

Each member also has its *variability* (also given in the [input data][input-data] section), or maximum acceptable *variation*. This is how often the expression for the value can change during the simulation -- hourly, daily, monthly, no change (constant), etc. The "variations" were introduced in the [expressions overview][expressions-overview] section and will be further detailed in a [section on variation frequencies][variation-frequencies-revisited].

Four special statements, AUTOSIZE, UNSET, REQUIRE, and FREEZE, add flexibility in working with members.

### AUTOSIZE

AUTOSIZE followed by a member name, sets the member to be sized by CSE. The option to AUTOSIZE a member will be shown under its legal range where it is described in the [input data][input-data] section. AUTOSIZE is only applicable to members describing HVAC system airflows and heating/cooling capacities. If AUTOSIZE is used for any member in the input, input describing design day conditions must also be specified (see [TOP Autosizing][top-autosizing]).

### UNSET

UNSET followed by a member name is used when it is desired to delete a member value previously given. UNSETing a member resets the object to the same internal state it was in before the member was originally given. This makes it legal to specify a new value for the member (normally, a duplicate specification results in an error message); if the member is required (as specified in the [input data][input-data] section), then an error message will occur if RUN is given without re specifying the member.

Situations where you really might want to specify a member, then later remove it, include:

-   After a RUN command has completed one simulation run, if you wish to specify another simulation run without CLEARing and giving all the data again, you may need to UNSET some members of some objects in order to re specify them or because they need to be omitted from the new run. In this case, use ALTER(s) to reopen the object(s) before UNSETing.

-   In defining a TYPE (see [this section][deftype]), you may wish to make sure certain members are not specified so that the user must give them or omit them if desired. If the origin of the type (possibly a sequence of DEFTYPEs, LIKEs, and/or COPYs) has defined unwanted members, get rid of them with UNSET.

Note that UNSET is only for deleting *members* (names that would be followed with an = and a a value when being defined). To delete an entire *object*, use DELETE (see [this section][delete]).

### REQUIRE

REQUIRE followed by a member name makes entry of that member mandatory if it was otherwise optional; it is useful in defining a TYPE (see [this section][deftype]) when you desire to make sure the user enters a particular member, for example to be sure the TYPE is applied in the intended manner. REQUIRE by itself does not delete any previously entered value, so if the member already has a value, you will need to UNSET it. ?? *verify*

### FREEZE

FREEZE followed by a member name makes it illegal to UNSET or redefine that member of the object. Note that FREEZE is unnecessary most of the time since CSE issues an error message for duplicate definitions without an intervening UNSET, unless the original definition came from a TYPE (see [this section][deftype]). Situations where you might want to FREEZE one or more members include:

-   When defining a TYPE (see [this section][deftype]). Normally, the member values in a type are like defaults; they can be freely overridden by member specifications at each use. If you wish to insure a TYPE is used as intended, you may wish to FREEZE members to prevent accidental misuse.

-   When your are defining objects for later use or for somebody else to use (perhaps in a file to be included) and you wish to guard against misuse, you may wish to FREEZE members. Of course, this is not foolproof, since there is at present no way to allow use of predefined objects or types without allowing access to the statements defining them.

## Action Commands

CSE has two action commands, RUN and CLEAR.

### RUN

RUN tells `CSE` to do an hourly simulation with the data now in memory, that is, the data given in the preceding part of the input file.

Note that CSE does NOT automatically run the simulator; an input file containing no RUN results in no simulation (you might nevertheless wish to submit an incomplete file to CSE to check for errors in the data already entered). The explicit RUN command also makes it possible to do multiple simulation runs in one session using a single input file.

When RUN is encountered in the input file, CSE checks the data. Many error messages involving inconsistencies between member values or missing required members occur at this time. If the data is good, CSE starts the simulation. When the simulation is complete and the reports have been output, CSE continues reading the input file. Statements after the first run can add to or change the data in preparation for another RUN. Note that the data for the first run is NOT automatically removed; if you wish to start over with complete specifications, use CLEAR after RUN.

### CLEAR

CLEAR removes all input data (objects and all their members) from CSE memory. CLEAR is normally used after RUN, when you wish to perform another simulation run and wish to start clean. If CLEAR is not used, the objects from the prior run's input remain in memory and may be changed or added to produce the input data for the next simulation run.
