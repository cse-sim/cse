# NMAKE file: runs RCDEF as required to generate derived CSE header and source files
# RCDEF produces output file rcdef.sum; we use this as target to determine if make is reqd

rcdef.sum : cndtypes.def cnfields.def cnrecs.def cnunits.def dtlims.def cndefns.h rcdef\rcdef.exe
    rcdefx
    @if errorlevel 1 echo Rcdefx FAIL
	  
# clean target: deletes all output files                                                                                                                            
clean :
	@del rcdef.sum
	@del dtypes.h
	@del rccn.h
	@del dttab.cpp
	@del srfd.cpp
	@del untab.cpp
	
rebuild : clean rcdef.sum

# rcdefx.mak end
	
	
