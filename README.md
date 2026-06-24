[![Build and Test](https://github.com/cse-sim/cse/workflows/Build%20and%20Test/badge.svg)](https://github.com/cse-sim/cse/actions?query=branch%3Amain)

## CSE

CSE is a general-purpose building simulation engine for modeling annual building energy use for heating, cooling, ventilation, and lighting. Originally built for California residential energy code compliance, it has since evolved into a broader platform used by researchers, standards developers, and software integrators.

### Documentation

The [CSE Documentation Web Site](https://cse-sim.github.io/cse) offers two options for viewing the documentation:

- [Single page](https://cse-sim.github.io/cse/single_page/)
- [Multi page](https://cse-sim.github.io/cse/)

### Issues and Issue Reporting

All known issues are listed on our [Issue Tracker]. New issues can be reported there, as well.

[Issue Tracker]: https://github.com/cse-sim/cse/issues

### Development

CSE is configured as a CMake project. Windows (MSVC) and macOS (Clang) are fully supported. Linux support is in progress. To build, run the appropriate script from the root directory:

- **Windows:** `build.bat`
- **macOS/Linux:** `build.sh`

All build products (e.g., `CSE.exe`) will be placed in a directory called `build`.

#### Testing

Automated testing of your build can be executed by running the following from the `build` directory:

`ctest -C Release`

#### Dependencies

- Microsoft Visual C++ or Clang
- CMake
- Python (via [uv](https://docs.astral.sh/uv/), required for building documentation)

See [doc/README.md](doc/README.md) for documentation build instructions.
