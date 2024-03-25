@echo off
if "%parent%"=="" set parent=%~0
if "%console_mode%"=="" (set console_mode=1& for %%x in (%cmdcmdline%) do if /i "%%~x"=="/c" set console_mode=0)

set arch="32"
if "%1"=="64" (set arch="%1")

set compiler="msvc"
if "%2"=="clang" (set compiler="clang")


cmake -DBUILD_ARCHITECTURE=%arch% -DCONFIGURATION=Release -DCOMPILER_ID=%compiler% -P cmake/configure-and-build.cmake
if %errorlevel% neq 0 (
  if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )
  exit /B %errorlevel%
)

if "%parent%"=="%~0" ( if "%console_mode%"=="0" pause )