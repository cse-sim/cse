@echo off
:: RCDEFX.BAT -- run rcdef for CSE
:: Creates dttab.cpp, untab.cpp, srfd.cpp, dtypes.h, rctypes.h, rccn.h.
:: args: %1
::    if NOX: don't execute rcdef (just preprocess input files)
::    else D,S etc rcdef debug flags

:: Preprocessing files ...
:: Preprocess data types definition file
if exist cndtypes.i del cndtypes.i
cl -c -EP -nologo -I.\ -Tccndtypes.def >cndtypes.i
if errorlevel 1 goto error
if NOT exist cndtypes.i goto error

:: Preprocess units definition file
if exist cnunits.i del cnunits.i
cl -c -EP -nologo -I.\  -Tccnunits.def >cnunits.i
if errorlevel 1 goto error
if NOT exist cnunits.i goto error

:: Preprocess fields definition file
if exist cnfields.i del cnfields.i
cl -c -EP -nologo -I.\  -Tccnfields.def >cnfields.i
if errorlevel 1 goto error
if NOT exist cnfields.i goto error

:: Preprocess record definition file
if exist cnrecs.i del cnrecs.i
cl -c -EP -nologo -I.\  -Tccnrecs.def >cnrecs.i
if errorlevel 1 goto error
if NOT exist cnrecs.i goto error

if '%1'=='NOX' goto done

:: Note: both VS2013 (RCDEF) and VS2008 (RCDEF9) produce rcdef.exe
::  They produce identical results
echo Executing rcdef ...
dir *.sum

::                        1         2          3          4        5    6   7   8   9 10  11  
rcdef\rcdef cndtypes.i cnunits.i dtlims.def cnfields.i cnrecs.i    . NUL NUL NUL  .  %1
::                                                                      :              :
::                      .  put h files into current dir ----------------:              :
::                      .  put dttab, untab, srfd.cpp into current dir ----------------:
if errorlevel 1 goto error

:: delete working files (or leave re debugging)
  if exist cndtypes.i         del cndtypes.i
  if exist cnunits.i          del cnunits.i
  if exist cnfields.i         del cnfields.i
  if exist cnrecs.i           del cnrecs.i
  if exist rcdef.err	      del rcdef.err
  goto done

:error
  if exist rcdef.sum del rcdef.sum
  set rcdefexe=
  echo cse RCDEF error !!
  exit /b 1

:done
