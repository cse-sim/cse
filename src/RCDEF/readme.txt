RCDEF

RCDEF is a batch executable that generates several header and implementation
files for CSE.

Typical CSE input files --

cndtypes.def
cnunits.def
dtlims.def
cnfields.def
cnrecs.def

During the build process, the input files are pre-processed to
yield files xxx.i

Command line argument details are documented in rcdef.cpp.

For debugging, the following can be pasted into e.g. Visual Studio
to correctly invoke the program.

cndtypes.i cnunits.i dtlims.i cnfields.i cnrecs.i . NUL NUL NUL .
