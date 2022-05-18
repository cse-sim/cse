# Compile RCDEF
add_subdirectory(RCDEF)

# Run C Preprocessor on *.DEF files
set(defs
  CNDTYPES.DEF
  CNUNITS.DEF
  DTLIMS.DEF
  CNFIELDS.DEF
  CNRECS.DEF
)

# User Option
option(USE_XMODULE "Add XMODULE to the build." OFF)

macro(c_preprocess defFile)
  string(TOLOWER "${defFile}" file)
  string(REPLACE ".def" "" file "${file}")
  add_custom_command(
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${file}.i"
    COMMAND ${CMAKE_COMMAND}
    ARGS
      -Dcompiler_path=${CMAKE_C_COMPILER}
      -Dinput_path="${CMAKE_CURRENT_SOURCE_DIR}/${defFile}"
      -Doutput_path="${CMAKE_CURRENT_BINARY_DIR}/${file}.i"
      -Dinclude_dir="${CMAKE_CURRENT_SOURCE_DIR}"
      -Dcompiler_name=${CMAKE_CXX_COMPILER_ID}
      -P "${PROJECT_SOURCE_DIR}/cmake/preprocess.cmake"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${defFile}"
  )
endmacro()

foreach(defFile ${defs})
  c_preprocess(${defFile})
endforeach()

# Run RCDEF
add_custom_command(
  OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/dtypes.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/rccn.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/dttab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/srfd.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/untab.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/rcdef.sum"
  COMMAND RCDEF "${CMAKE_CURRENT_BINARY_DIR}/cndtypes.i" "${CMAKE_CURRENT_BINARY_DIR}/cnunits.i" "${CMAKE_CURRENT_BINARY_DIR}/dtlims.i" "${CMAKE_CURRENT_BINARY_DIR}/cnfields.i" "${CMAKE_CURRENT_BINARY_DIR}/cnrecs.i"    . NUL NUL NUL  .
  DEPENDS RCDEF "${CMAKE_CURRENT_BINARY_DIR}/cndtypes.i" "${CMAKE_CURRENT_BINARY_DIR}/cnunits.i" "${CMAKE_CURRENT_BINARY_DIR}/dtlims.i" "${CMAKE_CURRENT_BINARY_DIR}/cnfields.i" "${CMAKE_CURRENT_BINARY_DIR}/cnrecs.i"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

# Set CSE Version
add_custom_target(version_header
  DEPENDS ${PROJECT_SOURCE_DIR}/src/csevrsn.h.in
  COMMAND ${CMAKE_COMMAND}
   ARGS -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
   -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
   -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
 -P "${PROJECT_SOURCE_DIR}/cmake/CSEVersion.cmake"
)

set_target_properties(version_header PROPERTIES FOLDER Dependencies)

# Set source files
set(source
  ANCREC.CPP
  ashwat.cpp
  ashwface.cpp
  BATTERY.cpp
  BRFW.CPP
  CGCOMP.CPP
  CGDEBUG.CPP
  CGENBAL.CPP
  CGRESULT.CPP
  CGSOLAR.CPP
  CGWTHR.CPP
  CNAH1.CPP
  CNAH2.CPP
  CNAUSZ.CPP
  CNCP.CPP
  CNCOIL.CPP
  CNCULT.CPP
  CNCULT2.CPP
  CNCULT3.CPP
  CNCULT4.CPP
  CNCULT5.CPP
  CNCULT6.CPP
  CNFAN.CPP
  CNGUTS.CPP
  CNHP.CPP
  CNLOADS.CPP
  CNTP.CPP
  CNZTU.CPP
  CPGBUILD.CPP
  CPGPRPUT.CPP
  CPNAT.CPP
  CSE.cpp
  CUEVAL.CPP
  CUL.CPP
  CUPARSE.CPP
  CUPROBE.CPP
  CUTOK.CPP
  CVPAK.CPP
  DATFCNS.CPP
  DHWCalc.cpp
  DHWSolar.cpp
  DMPAK.CPP
  dttab.cpp
  ENVPAK.CPP
  EXMAN.CPP
  Foundation.cpp
  GMUTIL.CPP
  geometry.cpp
  HVAC.CPP
  IMPF.CPP
  LOOKUP.CPP
  MESSAGES.CPP
  MSGTBL.CPP
  MSPAK.CPP
  nummeth.cpp
  PGPAK.CPP
  PP.CPP
  PSYCHRO.CPP
  PVCalc.cpp
  RMKERR.CPP
  SHADING.CPP
  solar.cpp
  srfd.cpp
  STRPAK.CPP
  SYTB.CPP
  TDPAK.CPP
  TIMER.CPP
  untab.cpp
  VRPAK.CPP
  WFPAK.CPP
  XIOPAK.CPP
  YACAM.CPP
)

set(precomp
  StdAfx.cpp
)

set(rcFile
  cse.rc
)

set(headers
  ANCREC.H
  ashwat.h
  ashwface.h
  BRF.H
  BRFW.H
  CGWTHR.H
  CNCULT.H
  CNCULTI.H
  CNDEFNS.H
  CNGLOB.H
  CNGUTS.H
  CPGBUILD.H
  CPRINT.H
  CSE.h
  CSEFace.H
  csevrsn.h
  CUEVAL.H
  CUEVF.H
  CUL.H
  CUPARSE.H
  CUPARSEI.H
  CUPARSEX.H
  CUTOK.H
  CVPAK.H
  DATFCNS.H
  DMPAK.H
  dtypes.h
  ENVPAK.H
  EXMAN.H
  Foundation.h
  GMPAK.H
  geometry.h
  HVAC.H
  IMPF.H
  IRATS.H
  LOOKUP.H
  MESSAGES.H
  MSGHANS.H
  mspak.h
  NUMMETH.H
  PGPAK.H
  PP.H
  PSYCHRO.H
  rccn.h
  RMKERR.H
  solar.h
  SRD.H
  STRPAK.H
  SYTB.H
  TDPAK.H
  TIMER.H
  VECPAK.H
  VRPAK.H
  WFPAK.H
  XIOPAK.H
  YACAM.H
)

if(USE_XMODULE) # Runtime Share Library interface - Not used since Version 0.899. (Windows only)
  list(APPEND source XMODULE.CPP)
  list(APPEND headers XMODULE.H)
endif()

include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/vendor"
  "${CMAKE_SOURCE_DIR}/vendor/Penumbra/include"
  "${CMAKE_SOURCE_DIR}/vendor/HPWHsim/src"
  "${CMAKE_BINARY_DIR}/vendor/HPWHsim/src"
  "${btwxt_SOURCE_DIR}/src"
)

# Precompiled Headers
macro(addMSVCPrecompiledHeader PrecompiledHeader PrecompiledSourceVar SourcesVar)
  if(MSVC)
    get_filename_component(PrecompiledBasename ${PrecompiledHeader} NAME_WE)
    set(PrecompiledBinary "$(IntDir)/${PrecompiledBasename}.pch")
    set(Sources ${${SourcesVar}})
    set(PrecompiledSource ${${PrecompiledSourceVar}})

    set_source_files_properties(${PrecompiledSource}
                                PROPERTIES COMPILE_FLAGS "/Yc\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_OUTPUTS "${PrecompiledBinary}")
    set_source_files_properties(${Sources}
                                PROPERTIES COMPILE_FLAGS "/Yu\"${PrecompiledHeader}\" /Fp\"${PrecompiledBinary}\""
                                           OBJECT_DEPENDS "${PrecompiledBinary}")
    # Add precompiled header to SourcesVar
    list(APPEND ${SourcesVar} ${PrecompiledSource})
  endif()
endmacro()

addMSVCPrecompiledHeader("cnglob.h" precomp source)

# Final executable/linking
add_executable(CSE ${source} ${rcFile} ${headers})
target_compile_features(CSE PUBLIC cxx_std_17)

if(USE_XMODULE)
  target_compile_definitions(CSE PRIVATE SUPPURT_XMODULE)
endif()

set(libs
  glad
  penumbra
  glfw
  tess2
  libHPWHsim
  libkiva
  btwxt
)

source_group("Source Files" FILES ${source})
source_group("Header Files" FILES ${headers})
source_group("Def Files" FILES ${defs})

target_link_libraries(CSE ${libs})

add_dependencies(CSE version_header)

set(exeDir "${CMAKE_SOURCE_DIR}/msvc") # TODO: Change for other platforms. Alternatively, Bin/..?

set_target_properties(CSE PROPERTIES
  DEBUG_POSTFIX "d"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_CAPS)
    set_target_properties(CSE PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_CAPS} ${exeDir})
endforeach()

#cotire(CSE)
