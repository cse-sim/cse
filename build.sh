#!/bin/bash

set -e

cmake -DBUILD_ARCHITECTURE=64 -DCONFIGURATION=Release -P cmake/configure-and-build.cmake