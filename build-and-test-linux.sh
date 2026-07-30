#!/bin/bash

# Builds and tests CSE on Ubuntu via Docker, for use from macOS or Linux
# without a separate Linux machine or VM. See build-and-test-linux.bat for
# the Windows equivalent, and Dockerfile.linux-dev for image details.
#
# Pins --platform linux/amd64 to match the x86_64 CI runners (see
# .github/workflows/build-and-test.yml); this also sidesteps some
# architecture-specific issues that only show up on arm64 Linux.
#
# Build output lands in builds/linux-docker/ (already git-ignored, alongside
# builds/<Debug|Release> from build.sh/build.bat) and is also logged to
# builds/linux-docker/build-and-test.log for later review.

set -e

cd "$(dirname "${BASH_SOURCE[0]}")"

BUILD_DIR=builds/linux-docker
mkdir -p "$BUILD_DIR"

docker build --platform linux/amd64 -f Dockerfile.linux-dev -t cse-linux-dev .

docker run --rm --platform linux/amd64 -v "$(pwd)":/src -w /src cse-linux-dev bash -c "
  set -e
  cmake -DBUILD_ARCHITECTURE=64 -DCOMPILER_ID=gcc -DCONFIGURATION=Release \
    -DBUILD_DIRECTORY=$BUILD_DIR -DBUILD_DOCS_WITH_ALL=ON -P cmake/configure-and-build.cmake
  cd $BUILD_DIR
  ctest -C Release -j \"\$(nproc)\" --output-on-failure -E shadetest
" 2>&1 | tee "$BUILD_DIR/build-and-test.log"
exit "${PIPESTATUS[0]}"
