@echo off
if "%parent%"=="" set parent=%~0
if "%console_mode%"=="" (set console_mode=1& for %%x in (%cmdcmdline%) do if /i "%%~x"=="/c" set console_mode=0)

if "%1"=="/x64" (cmake -DX64=ON -P cmake/configure.cmake) ELSE (cmake -P cmake/configure.cmake)
if %errorlevel% neq 0 (
  if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )
  exit /B %errorlevel%
)

if "%1"=="/x64" (cmake -DX64=ON -P cmake/build.cmake) ELSE (cmake -P cmake/build.cmake)
if %errorlevel% neq 0 (
  if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )
  exit /B %errorlevel%
)

if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )