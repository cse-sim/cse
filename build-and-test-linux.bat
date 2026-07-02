@echo off
rem Builds and tests CSE on Ubuntu via Docker, for use from Windows without a
rem separate Linux machine or VM. See build-and-test-linux.sh for the
rem macOS/Linux equivalent, and Dockerfile.linux-dev for image details.
rem
rem Pins --platform linux/amd64 to match the x86_64 CI runners (see
rem .github/workflows/build-and-test.yml).
rem
rem Build output lands in builds\linux-docker\ (already git-ignored, alongside
rem builds\<Debug|Release> from build.sh/build.bat) and is also logged to
rem builds\linux-docker\build-and-test.log for later review.

setlocal

set BUILD_DIR=builds/linux-docker
if not exist "builds\linux-docker" mkdir "builds\linux-docker"

docker build --platform linux/amd64 -f Dockerfile.linux-dev -t cse-linux-dev .
if %errorlevel% neq 0 exit /B %errorlevel%

docker run --rm --platform linux/amd64 -v "%cd%":/src -w /src cse-linux-dev bash -c "set -e && cmake -DBUILD_ARCHITECTURE=64 -DCOMPILER_ID=gcc -DCONFIGURATION=Release -DBUILD_DIRECTORY=%BUILD_DIR% -DBUILD_DOCS_WITH_ALL=ON -P cmake/configure-and-build.cmake && cd %BUILD_DIR% && ctest -C Release -j \"$(nproc)\" --output-on-failure -E shadetest" > "builds\linux-docker\build-and-test.log" 2>&1
set RESULT=%errorlevel%
type "builds\linux-docker\build-and-test.log"
exit /B %RESULT%
