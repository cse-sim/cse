add_library(cse_libstubs OBJECT libstubs.cpp)

target_include_directories(cse_libstubs PUBLIC
  "${CMAKE_SOURCE_DIR}/src/RCDEF" # stubs don't need legit dtypes.h
  "${CMAKE_SOURCE_DIR}/src"
)

target_compile_features(cse_libstubs PUBLIC cxx_std_17)
target_link_libraries(cse_libstubs PRIVATE cse_common_interface)


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
  "${HPWHsim_SOURCE_DIR}/src"
  "${HPWHsim_BINARY_DIR}/src"
  "${btwxt_SOURCE_DIR}/src"
)

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
  cse_common_interface
)

source_group("Source Files" FILES ${source})
source_group("Header Files" FILES ${headers})
source_group("Def Files" FILES ${defs})

target_link_libraries(CSE ${libs})

add_dependencies(CSE version_header)

set(exeDir "${CMAKE_SOURCE_DIR}/msvc") # TODO: Change for other platforms. Alternatively, Bin/..?

if ("${CSE_BUILD_ARCHITECTURE}" STREQUAL "64")
  set_target_properties(CSE PROPERTIES
    OUTPUT_NAME "CSE64"
  )
endif()

set_target_properties(CSE PROPERTIES
  DEBUG_POSTFIX "d"
)

foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_CAPS)
    set_target_properties(CSE PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_CAPS} ${exeDir})
endforeach()
