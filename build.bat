@echo off
cmake -P cmake/configure.cmake
IF %ERRORLEVEL% NEQ 0 (
    exit %ERRORLEVEL%
)
cmake -P cmake/build.cmake
IF %ERRORLEVEL% NEQ 0 (
    exit %ERRORLEVEL%
)
IF "%1"=="" (
  PAUSE
)
