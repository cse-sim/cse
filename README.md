## California Simulation Engine (CSE)

CSE is a general purpose building simulation model developed primarily to perform the required calculations for the California Building Energy Code Compliance for Residential buildings ([CBECC-Res](http://www.bwilcox.com/BEES/BEES.html)) software.

### CSE User Manual

The CSE User Manual can be found on the [CSE Documentation Web Site](https://cse-sim.github.io/cse):

- [HTML Format](https://cse-sim.github.io/cse/cse-user-manual.html) (single page)
- [HTML Format](https://cse-sim.github.io/cse/cse-user-manual/index.html) (multi page)
- [PDF Format](https://cse-sim.github.io/cse/pdfs/cse-user-manual.pdf)

### Issues and Issue Reporting

All known issues are listed on our [Issue Tracker]. New issues can be reported there, as well.

[Issue Tracker]: https://github.com/cse-sim/cse/issues

### Development

CSE is configured as a CMake project. Currently, CMake is only configured to generate Microsoft Visual Studio solutions compiled with Microsoft Visual C++ (other generators and compilers will not work). A batch script is set up for automatic project setup. Simply double click or run the **build.bat** script in the root directory. All products (e.g., CSE.exe) will be placed in a directory called **msvc**.

#### Testing

Automated testing of your build can be executed by running

`ctest -C Release`

from the **msvc\\build** directory.

#### Dependencies

- Microsoft Visual Studio 2017 with Visual C++ and Support for Windows XP (v141_xp toolset)
- CMake 3.10 or later

Note: Generating the documentation requires additional tools. See [doc\\README.md](doc/README.md).
