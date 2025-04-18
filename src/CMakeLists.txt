# cse/src CMakeLists.txt -- Build rcdef, run rcdef, build CSE

# Build RCDEF
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

# Implemented for NREL, but generally useful (if documented for others!).
option(USE_BINRES "Enable binary results output." OFF) # TODO (MP) Windows io header was replaced for C-style file io fopen, fclose, etc. These changes were not tested.

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
      -Dcompiler_arch=${CSE_BUILD_ARCHITECTURE}
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
    "${CMAKE_CURRENT_BINARY_DIR}/dtypes.h"
    "${CMAKE_CURRENT_BINARY_DIR}/rccn.h"
    "${CMAKE_CURRENT_BINARY_DIR}/dttab.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/srfd.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/untab.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/rcdef.sum"
  COMMAND RCDEF "${CMAKE_CURRENT_BINARY_DIR}/cndtypes.i" "${CMAKE_CURRENT_BINARY_DIR}/cnunits.i" "${CMAKE_CURRENT_BINARY_DIR}/dtlims.i" "${CMAKE_CURRENT_BINARY_DIR}/cnfields.i" "${CMAKE_CURRENT_BINARY_DIR}/cnrecs.i"    . NUL NUL NUL  .
  DEPENDS RCDEF "${CMAKE_CURRENT_BINARY_DIR}/cndtypes.i" "${CMAKE_CURRENT_BINARY_DIR}/cnunits.i" "${CMAKE_CURRENT_BINARY_DIR}/dtlims.i" "${CMAKE_CURRENT_BINARY_DIR}/cnfields.i" "${CMAKE_CURRENT_BINARY_DIR}/cnrecs.i"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
)

# Needed to establish dependency for other targets that depend on generated RCDEF outputs
add_custom_target(generate_rcdef_outputs DEPENDS
        "${CSE_BINARY_DIR}/src/dtypes.h"
        "${CSE_BINARY_DIR}/src/rccn.h"
        "${CSE_BINARY_DIR}/src/dttab.cpp"
        "${CSE_BINARY_DIR}/src/srfd.cpp"
        "${CSE_BINARY_DIR}/src/untab.cpp"
)

# Set CSE Version
add_custom_target(version_header
  DEPENDS "${PROJECT_SOURCE_DIR}/src/csevrsn.in.h"
  BYPRODUCTS "${PROJECT_BINARY_DIR}/src/csevrsn.h"
  COMMAND ${CMAKE_COMMAND}
   ARGS -DPROJECT_SOURCE_DIR=${PROJECT_SOURCE_DIR}
   -DPROJECT_BINARY_DIR=${PROJECT_BINARY_DIR}
   -DGIT_EXECUTABLE=${GIT_EXECUTABLE}
 -P "${PROJECT_SOURCE_DIR}/cmake/CSEVersion.cmake"
)

set_target_properties(version_header PROPERTIES FOLDER Dependencies)

# Set source files
set(source
  ancrec.cpp
  ashwat.cpp
  ashwface.cpp
  battery.cpp
  cgcomp.cpp
  cgdebug.cpp
  cgenbal.cpp
  cgresult.cpp
  cgsolar.cpp
  cgwthr.cpp
  cnah1.cpp
  cnah2.cpp
  cnausz.cpp
  cncp.cpp
  cncoil.cpp
  cncult.cpp
  cncult2.cpp
  cncult3.cpp
  cncult4.cpp
  cncult5.cpp
  cncult6.cpp
  cnfan.cpp
  cnguts.cpp
  cnhp.cpp
  cnloads.cpp
  cntp.cpp
  cnztu.cpp
  cpgbuild.cpp
  cpgprput.cpp
  cpnat.cpp
  cse.cpp
  cueval.cpp
  cul.cpp
  cuparse.cpp
  cuprobe.cpp
  curvemap.cpp
  cutok.cpp
  datfcns.cpp
  dhwcalc.cpp
  dhwsolar.cpp
  dttab.cpp
  exman.cpp
  foundation.cpp
  gmutil.cpp
  geometry.cpp
  hvac.cpp
  impf.cpp
  mspak.cpp
  nummeth.cpp
  pgpak.cpp
  pp.cpp
  pvcalc.cpp
  shading.cpp
  solar.cpp
  sytb.cpp
  timer.cpp
  vrpak.cpp
  wfpak.cpp
  yacam.cpp
  culstr.natvis
  "${CMAKE_CURRENT_BINARY_DIR}/srfd.cpp"
  "${CMAKE_CURRENT_BINARY_DIR}/untab.cpp"
)

list(APPEND source ${cse_common_source})

set(precomp
  StdAfx.cpp
)

set(rcFile
  cse.rc
)

set(headers
  ancrec.h
  ashwat.h
  ashwface.h
  brf.h
  cgwthr.h
  cncult.h
  cnculti.h
  cndefns.h
  cnglob.h
  cnguts.h
  cpgbuild.h
  cprint.h
  cse.h
  cseface.h
  cueval.h
  cuevf.h
  cul.h
  cuparse.h
  cuparsei.h
  cuparsex.h
  cutok.h
  curvemap.h
  cvpak.h
  datfcns.h
  dmpak.h
  envpak.h
  exman.h
  foundation.h
  gmpak.h
  geometry.h
  hvac.h
  impf.h
  irats.h
  lookup.h
  messages.h
  msghans.h
  mspak.h
  nummeth.h
  pgpak.h
  pp.h
  psychro.h
  rmkerr.h
  solar.h
  srd.h
  strpak.h
  sytb.h
  tdpak.h
  timer.h
  vecpak.h
  vrpak.h
  wfpak.h
  xiopak.h
  yacam.h
  "${CMAKE_CURRENT_BINARY_DIR}/csevrsn.h"
  "${CMAKE_CURRENT_BINARY_DIR}/dtypes.h"
  "${CMAKE_CURRENT_BINARY_DIR}/rccn.h"
)

if(USE_XMODULE) # Runtime Share Library interface - Not used since Version 0.899. (Windows only)
  list(APPEND source xmodule.cpp)
  list(APPEND headers xmodule.h)
endif()

if(USE_BINRES) # CSE Binary results file writer
  list(APPEND source brfw.cpp)
  list(APPEND headers brfw.h)
endif()

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

target_include_directories(CSE PRIVATE
  "${CSE_BINARY_DIR}/src"
  "${CSE_SOURCE_DIR}/src"
  "${penumbra_SOURCE_DIR}/include"
)

target_compile_features(CSE PUBLIC cxx_std_17)

if(USE_XMODULE)
  target_compile_definitions(CSE PRIVATE SUPPURT_XMODULE)
endif()

if(USE_BINRES)
  target_compile_definitions(CSE PRIVATE BINRES)
endif()

set(libs
  glad
  penumbra
  glfw
  tess2
  HPWHsim
  libkiva
  btwxt
  cse_common_interface
)

source_group("Source Files" FILES ${source})
source_group("Header Files" FILES ${headers})
source_group("Def Files" FILES ${defs})

target_link_libraries(CSE PRIVATE ${libs})

if(KIVA_GROUND_PLOT)
  set_source_files_properties(foundation.cpp foundation.h PROPERTIES COMPILE_DEFINITIONS GROUND_PLOT)
  target_link_libraries(CSE PUBLIC groundplot)
endif()

add_dependencies(CSE version_header)

if (${CSE_COMPILER_NAME} STREQUAL "msvc" AND ${CSE_BUILD_ARCHITECTURE} STREQUAL "32")
  set(executable_name "CSE")
  set(debug_postfix "d")
else()
  set(executable_name "cse-${CSE_COMPILER_NAME}-${CSE_BUILD_ARCHITECTURE}")
  set(debug_postfix "-debug")
endif ()

set_target_properties(CSE PROPERTIES
  OUTPUT_NAME "${executable_name}"
)

set_target_properties(CSE PROPERTIES
  DEBUG_POSTFIX "${debug_postfix}"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_CAPS)
    set_target_properties(CSE PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_CAPS} ${CSE_EXECUTABLE_DIRECTORY})
endforeach()

