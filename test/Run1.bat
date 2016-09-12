:: run1.bat -- run 1 CSE case

if exist %1.rep del %1.rep
set APP=CSEd
if /I '%2' == 'REL' set APP=CSE
..\run\%APP% -x"! " -t1 %1

if not errorlevel 1 goto runOK
echo %1		fail
goto fail1

: runOK
WCMP %1.rep ref\%1.rep !
if not errorlevel 1 goto outOK1
echo %1		results differ
if not defined DIFF set DIFF=bc4
:: windiff ref\%1.rep %1.rep
call "%DIFF%" ref\%1.rep %1.rep

if /I NOT '%3' == 'UPD' goto fail1
attrib ref\%1.rep -r
copy /y %1.rep ref\%1.rep

:fail1
set errorflag=T
goto done

:outOK1
echo %1		OK

:done


