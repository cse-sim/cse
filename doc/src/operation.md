# Operation

## Command Line

CSE is invoked from the command prompt or from a batch file using the following command:

        CSE *inputfile* {*switches*}

where:

*inputfile*
:   specifies the name of the text input file for the run(s). If the filename has an extension other than ".cse" (the default), it must be included. The name of the file with weather data for the simulation(s) is given in this file (wfName= statement, see [Weather Data Items](#top-weather-data-items)).


*{switches}*
:   indicates zero or more of the following:

-   -D*name* defines the preprocessor symbol *name* with the value "". Useful for testing with `#ifdef name`, to invoke variations in the simulation without changing the input file. The CSE preprocessor is described "[The Preprocessor](#the-preprocessor)".

-   -D*name*=*value* defines the preprocessor symbol *name* with the specified *value*. *Name* can then be used in the input file to allow varying the simulation without changing the input file -- see "[The Preprocessor](#the-preprocessor)" for more information. The entire switch should be enclosed in quotes if it contains any spaces -- otherwise the command processor will divide it into two arguments and CSE will not understand it.

-   -U*name* undefines the preprocessor symbol *name*.

-   -i*path;path;path* specifies directories where CSE looks for input and include files.

-   -b batch mode: suppresses all prompts for user input.  Currently there are no prompts implemented in CSE.  -b is retained in case prompts are added in the future.  It is good practice to include -b in batch script CSE invocations.

-   -n suppresses screen display of warning messages.  When -n is specified, warning messages are reported only to the error file.

-   -p display all the class and member names that can be "probed" or accessed in CSE expressions. "Probes" are described in "[Probes](#probes)". Use with command processor redirection operator "&gt;" to obtain a report in a file. *Inputfile* may be given or omitted when -p is given.

-   -q similar to -p, but displays additional member names that cannot be probed or would not make sense to probe in an input file (development aid).

-   -c list all input names.

-   -x specifies report test prefix; see TOP repTestPfx. The -x command line setting takes precedence over the input file value, if any.

-   -t*nn* specifies internal testing option bits (development aid).

## Locating Files

As with any program, in order to invoke CSE, the directory containing CSE.EXE must be the current directory, or that directory must be on the operating system path, or you must type the directory path before CSE.

A CSE simulation requires a weather file. The name of the weather file is given in the CSE input file (`wfName=` statement, see [Weather Data Items](#top-weather-data-items)). The weather file must be in one of the same three places: current directory, directory containing CSE.EXE, or a directory on the operating system path; or, the directory path to the file must be given in the `wfName=` statement in the usual pathName syntax. ?? Appears that file must be in current directory due to file locating bugs 2011-07

<!--
TODO: Check file locating methods.  Path.find() etc.  Update here as needed re weather file etc.
-->

The CSE input file, named on the CSE command line, must be in the current directory or the directory path to it must be included in the command line.

Included input files, named in `#include` preprocessor directives (see "[The Preprocessor](#the-preprocessor)") in the input file, must be in the current directory or the path to them must be given in the `#include` directive. In particular, CSE will NOT automatically look for included files in the directory containing the input file. The default extension for included files is ".CSE".

Output files created by default by CSE (error message file, primary report and export files) will be in the same directory as the input file; output files created by explicit command in the input file (additional report and/or export files) will be in the current directory unless another path is explicitly specified in the command creating the file.

## Output File Names

If any error or warning messages are generated, CSE puts them in a file with the same name and path as the input file and extension .ERR, as well as on the screen and, usually, in the primary (default) report file. The exception is errors in the command line: these appear only on the screen. If there are no error or warning messages, any prior file with this name is deleted.

By default, CSE generates an output report file with the same name and path as the input file, and extension ".REP". This file may be examined with a text editor and/or copied to an ASCII printer. If any exports are specified, they go by default into a file with the same name and path as the input file and extension ".EXP".

In response to specifications in the input file, CSE can also generate additional report and export files with user-specified names. The default extensions for these are .REP and .CSV respectively and the default directory is the current directory; other paths and extensions may be specified. For more information on report and export files, see REPORTFILE and EXPORTFILE in "[Input Data](#input-data)".

## Errorlevel

CSE sets the command processor ERRORLEVEL to 2 if any error occurs in the session. This should be tested in batch files that invoke CSE, to prevent use of the output reports if the run was not satisfactory. The ERRORLEVEL is NOT set if only warning messages that do not suppress or abort the run occur, but such messages DO create the .ERR file.

<!--
REST OF FILE (7 PAGES) IS ALL HIDDEN TEST 7-92 rob
-->

<!--
### Results Files

    This section hidden by rob 7-26-92 becuase 1) it doesn't explain what its doing (showing examples of the reports, I guess) 2) it generates a lot of blank paper - need to use small line spacing to show a whole report page in a box on one page, or something 3) style is wrong: no monospace font; .5 lines between lines (may be my change).  Probably need to use reports without page formatting, or copy them in and doctor them.

    Run log (LOG)

    **Error! Not a valid filename.** Input echo (INP)

    Run summary (SUM)

    Zone Energy Balance (ZEB)

    **Error! Not a valid filename.**

    Zone Statistics (ZST)

    Zone Data Dump (ZDD)

    **Error! Not a valid filename.**

    Meter Report (MTR)

    User Defined Table (UDT)
-->

<!--

### Error Files

*Hidden by rob 7-26-92.  Suggest explaining, then showing a copied-in and doctored example.*

Error Messages (ERR)

**Error! Not a valid filename.**

### External File Interface

### Hourly files

## Helpful Hints

### Memory

-->

<!--
*Hidden by rob, 7-26-92: it doesn't say anything. When restored, need to address extended memory version here.*

If a user is running out of memory, there are a series of steps to try to minimize memory usage.

-->

