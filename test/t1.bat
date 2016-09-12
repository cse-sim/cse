:: Test CSE for no changes
@echo off

set errorflag=F
call Run1 600 %1

echo.
if not '%errorflag%' == 'F' echo FAIL!
if     '%errorflag%' == 'F' echo All cases OK!
set errorflag=
echo.



