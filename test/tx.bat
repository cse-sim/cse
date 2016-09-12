:: Test CSE for no changes
@echo off

set errorflag=F
call Run1 600 %1 %2
call Run1 930 %1 %2
call Run1 960 %1 %2
call Run1 2Zone %1 %2
call Run1 3ZoneAirNet %1 %2
call Run1 3ZoneAirNetCSW %1 %2
call Run1 Fresno %1 %2
call Run1 Autosize %1 %2
call Run1 masstest1 %1 %2
:: call Run1 12P21M %1 %2
:: call Run1 1ZCZMU %1 %2
call Run1 Z2SG %1 %2
call Run1 zonetest4W %1 %2
call Run1 zt5d %1 %2
call Run1 qw5ds %1 %2
call Run1 izfantest %1 %2
:: call Run1 2700x %1 %2
call Run1 2zun %1 %2
call Run1 1zattic %1 %2
call Run1 2zattic %1 %2
call Run1 actest1bL %1 %2
call Run1 ashptest2 %1 %2
call Run1 herv %1 %2
call Run1 oavtest2 %1 %2
call Run1 bgtest %1 %2
call Run1 ashwat1 %1 %2
call Run1 dhw02 %1 %2
call Run1 dhwX %1 %2
call Run1 dhwDU %1 %2
call Run1 dhw_C %1 %2
call Run1 pvtest %1 %2


echo.
if not '%errorflag%' == 'F' echo FAIL!
if     '%errorflag%' == 'F' echo All cases OK!
set errorflag=
echo.



