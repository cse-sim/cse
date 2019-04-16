@echo off
cmake -P cmake/build.cmake
IF "%1"=="" (
  PAUSE
)
