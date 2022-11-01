@echo off
if "%parent%"=="" set parent=%~0
if "%console_mode%"=="" (set console_mode=1& for %%x in (%cmdcmdline%) do if /i "%%~x"=="/c" set console_mode=0)

set archB="x32"
if "%1"=="x64" (set archB="%1")

cmake -DARCHITECTUREBUILD=%archB% -P cmake/configure.cmake
if %errorlevel% neq 0 (
  if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )
  exit /B %errorlevel%
)

cmake -DARCHITECTUREBUILD=%archB% -P cmake/build.cmake
if %errorlevel% neq 0 (
  if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )
  exit /B %errorlevel%
)

if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )