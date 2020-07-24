@echo off
cmake -P cmake/configure.cmake
cmake -P cmake/build.cmake
IF "%1"=="" (
  PAUSE
)
