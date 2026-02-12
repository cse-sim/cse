
git branch --contains %1

@echo.
@choice /c yn /n /m "Delete branch '%1' (y/n)? "
@goto %ERRORLEVEL%
:1
::git branch -d %1
git branch -d %1
@goto done
:2
@echo.
@echo No action
:done


