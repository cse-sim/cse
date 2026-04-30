# Probes

*Probes* provide a universal means of referencing data within the simulator. Probes permit using the inputtable members of each object, as described in the [Input Data][input-data] Section, as operands in expressions. In addition, most internal members can be probed; we will describe how to find their names shortly.

## Ways To Use Probes

The three general ways of using probes are:

1.  During input, to implement things like "make this window's width equal to 10% of the zone floor area" by using the zone's floor area in an expression:

        wnWidth = @zone[1].znArea * 0.1;

        Here "`@zone[1].znArea`" is the probe.

2.  During simulation. Probing during simulation, to make inputs be functions of conditions in the building or HVAC systems, is limited because most of the members of interest are updated *after* CSE has evaluated the user's expressions for the subhour or other time interval -- this is logically necessary since the expressions are inputs. (An exception is the weather data, but this is also available through system variables such as $tDbO.)

    However, a number of *prior subhour* values are available for probing, making it possible to implement relationships like "the local heat output of this terminal is 1000 Btuh if the zone temperature last subhour was below 65, else 500":

        tuMnLh = @znres["North"].S.prior.tAir < 65 ? 1000 : 500;

3.  For output reports, allowing arbitrary data to be reported at subhourly, hourly, daily, monthly, or annual intervals. The REPORT class description describes the user-defined report type (UDT), for which you write the expression for the value to be reported. With probes, you can thus report almost any datum within CSE -- not just those values chosen for reporting when the program was designed. Even values calculated during the current subhour simulation can be probed and reported, because expressions for reports are evaluated after the subhour's calculations are performed.

!!! example
        colVal = @airHandler["Hot"].ts;     // report air handler supply temp
        colVal = @terminal[NorthHot].cz;    // terminal air flow to zone (Btuh/F)

## General form

The general form of a probe is

    @ *className* [ *objName* ] . *member*


`className` 
:   is the CLASS being probed

`objName`
:   is the name of the specific object of the class; alternately, a numeric subscript is allowed. Generally, the numbers correspond to the objects in the order created. \[ *objName* \] can be omitted for the TOP class, which has only one member, Top.

`member`
:   is the name of the particular member being probed. This must be exactly correct. For some inputtable members, the probe name is not the same as the input name given in the [Input Data][input-data] Section, and there are many probe-able members not described in the [Input Data][input-data] section.

!!! tip
    The initial `@` is always necessary. And don't miss the period after the `]`.

## Ambiguous names

Note that CSE often allows multiple objects of the same type with the same name.  For example:

    GAIN G1 gnPower=100 ...

    ZONE Z1
       ...
       GAIN G1 gnPower=200 ...
       ...

No problem so far.  However, suppose some reporting is added:

    REPORT RP1 rpType=UDT ...
       REPORTCOL colHead="Gain" colVal=@GAIN["G1"].gnPower ...

The REPORTCOL colVal probe is ambiguous -- CSE cannot determine which "GAIN G1" is being referenced.  An "ambiguous name" error message is issued and the simulation does not run.  There is no provision in the probe syntax to provide context information resolve such ambiguity.

__However__, there is a simple work-around --

!!! tip
    Do not use ambiguous names, especially for objects that are likely to be probed.

An unambiguous version of the above example might look like this:

    GAIN TOPG1 gnPower=100 ...

    ZONE Z1
       ...
       GAIN Z1G1 gnPower=200 ...
       ...

    REPORT RP1 rpType=UDT ...
       REPORTCOL colHead="Gain" colVal=@GAIN["Z1G1"].gnPower ...

## probes.txt

How do you find out what the probe-able member names are? CSE will display the a list of the latest class and member names if invoked with the -p switch. Use the command line

        CSE -p > probes.txt

to put the displayed information into the file PROBES.TXT, then print the file or examine it with a text editor.

A portion of the `-p` output looks like:

        @exportCol[1..].        I   R                   owner: export
                         name   I   R   string            constant
                      colHead   I   R   string            input time
                       colGap   I   R   integer number    input time
                       colWid   I   R   integer number    input time
                       colDec   I   R   integer number    input time
                      colJust   I   R   integer number    constant
                       colVal   I   R   un-probe-able     end of each subhour
                       nxColi   I   R   integer number    constant

        @holiday[1..].          I
                         name   I       string            constant
                   hdDateTrue   I       integer number    constant
                    hdDateObs   I       integer number    constant
                   hdOnMonday   I       integer number    constant

In the above "exportCol" and "holiday" are class names, and "name", "colHead", "colGap", . . . are member names for class exportCol. Some members have multiple names separated by .'s, or they may contain an additional subscript. To probe one of these, type all of the names and punctuation exactly as shown (except capitalization may differ); if an additional subscript is shown, give a number in the specified range. An "I" designates an "input" parameter, an R means "runtime" parameter. The "owner" is the class of which this class is a subclass.

The data type and variation of each member is also shown. Note that *variation*, or how often the member changes, is shown here. (*Variability*, or how often an expression assigned to the member may change, is given for the input table members in the [Input Data][input-data] Section). Members for which an "end of" variation is shown can be probed only for use in reports. A name described as "un-probe-able" is a structure or something not convertible to an integer, float, or string.

<!--

hidden by rob 7-26-92: believe this is useless without member names; full detail is too long to put here; write an appendix B.

The following is a list of all of the top-level internal members (some of these members, like ahU,  are obsolete):

```
@top.                 I   R @zone[1..].           I   R @izXfer[1..].         I   R @gain[1..].           I   R                   owner: zone@meter[1..].          I   R @ahu[1..].            I   R @terminalx[1..].      I   R                   owner: zone@terminal[1..].       I   R                   owner: zone@airHandler[1..].     I   R @perimeter[1..].      I                       owner: zone@surface[1..].        I                       owner: zone  sgdist[0-4].targTy  I       integer number    run start time  sgdist[0-4].targTi  I       integer number    run start time    sgdist[0-4].f[0]  I       number            monthly-hourly    sgdist[0-4].f[1]  I       number            monthly-hourly@door[1..].           I                       owner: surface  sgdist[0-4].targTy  I       integer number    run start time  sgdist[0-4].targTi  I       integer number    run start time    sgdist[0-4].f[0]  I       number            monthly-hourly    sgdist[0-4].f[1]  I       number            monthly-hourly@window[1..].         I                       owner: surface  sgdist[0-4].targTy  I       integer number    run start time  sgdist[0-4].targTi  I       integer number    run start time    sgdist[0-4].f[0]  I       number            monthly-hourly    sgdist[0-4].f[1]  I       number            monthly-hourly@shade[1..].          I   R                   owner: window@sgdist[1..].         I                       owner: window@construction[1..].   I     @layer[1..].          I                       owner: construction@material[1..].       I     @reportFile[1..].     I     @export file[1..].    I     @report[1..].         I                       owner: reportFile@export[1..].         I                       owner: export file@reportCol[1..].      I   R                   owner: report@exportCol[1..].      I   R                   owner: export@holiday[1..].        I     @znRes[1..].              R @zhx[1..].                R                   owner: zone@xsurf[1..].              R @mass[1..].               R @massType[1..].           R
```

hidden by rob, 7-26-92: This info should be part of annotation of -p report in appendix B. And, it is stale.

Examples of subscripts of all arrays within records:

window[].sgdist[]:used from 0 to window.nsgdist-1; entries 0 and 1 are supplied by the program ...(explain); entries 2 up result, in order, from SGDIST statement groups for the window in the input file.

mass[].bc[]: characteristics and heat flows for surfaces of mass.  bc[0] = inside surface, bc[1] = outside.

-->
surface\[\].sgdist\[\].f\[\]: f\[0\] is winter solar coupling fraction; f\[1\] is summer.
