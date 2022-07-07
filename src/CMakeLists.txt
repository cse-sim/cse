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
  ancrec.cpp
  ashwat.cpp
  ashwface.cpp
  battery.cpp
  brfw.cpp
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
  cvpak.cpp
  datfcns.cpp
  dhwcalc.cpp
  dhwsolar.cpp
  dmpak.cpp
  dttab.cpp
  envpak.cpp
  exman.cpp
  foundation.cpp
  gmutil.cpp
  geometry.cpp
  hvac.cpp
  impf.cpp
  lookup.cpp
  messages.cpp
  msgtbl.cpp
  mspak.cpp
  nummeth.cpp
  pgpak.cpp
  pp.cpp
  psychro.cpp
  pvcalc.cpp
  rmkerr.cpp
  shading.cpp
  solar.cpp
  srfd.cpp
  strpak.cpp
  sytb.cpp
  tdpak.cpp
  timer.cpp
  untab.cpp
  vrpak.cpp
  wfpak.cpp
  xiopak.cpp
  yacam.cpp
)

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
  brfw.h
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
  csevrsn.h
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
  dtypes.h
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
  rccn.h
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
)

if(USE_XMODULE) # Runtime Share Library interface - Not used since Version 0.899. (Windows only)
  list(APPEND source XMODULE.CPP)
  list(APPEND headers XMODULE.H)
endif()

include_directories(
  "${CMAKE_SOURCE_DIR}/src"
  "${CMAKE_SOURCE_DIR}/vendor"
  "${CMAKE_SOURCE_DIR}/vendor/penumbra/include"
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
