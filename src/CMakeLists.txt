# Compile RCDEF
add_subdirectory(RCDEF)

# Run C Preprocessor on *.DEF files
macro(c_preprocess file)
  add_custom_command(
    OUTPUT "${file}.i"
    COMMAND "${CMAKE_C_COMPILER}" -c -EP -nologo -I "${CMAKE_CURRENT_SOURCE_DIR}" -Tc "${CMAKE_CURRENT_SOURCE_DIR}/${file}.def" > "${file}.i"
    MAIN_DEPENDENCY "${CMAKE_CURRENT_SOURCE_DIR}/${file}.def"
  )
endmacro()

c_preprocess(cndtypes)
c_preprocess(cnunits)
c_preprocess(dtlims)
c_preprocess(cnfields)
c_preprocess(cnrecs)

# Run RCDEF
add_custom_command(
  OUTPUT
    "${CMAKE_CURRENT_SOURCE_DIR}/DTYPES.H"
    "${CMAKE_CURRENT_SOURCE_DIR}/RCCN.H"
    "${CMAKE_CURRENT_SOURCE_DIR}/DTTAB.CPP"
    "${CMAKE_CURRENT_SOURCE_DIR}/SRFD.CPP"
    "${CMAKE_CURRENT_SOURCE_DIR}/UNTAB.CPP"
    "${CMAKE_CURRENT_SOURCE_DIR}/rcdef.sum"
  COMMAND RCDEF "${CMAKE_CURRENT_BINARY_DIR}/cndtypes.i" "${CMAKE_CURRENT_BINARY_DIR}/cnunits.i" "${CMAKE_CURRENT_BINARY_DIR}/dtlims.i" "${CMAKE_CURRENT_BINARY_DIR}/cnfields.i" "${CMAKE_CURRENT_BINARY_DIR}/cnrecs.i"    . NUL NUL NUL  .
  DEPENDS cndtypes.i cnunits.i dtlims.i cnfields.i cnrecs.i
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)

set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
foreach(CompilerFlag ${CompilerFlags})
  string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
endforeach()

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
  DMPAK.CPP
  DTTAB.CPP
  ENVPAK.CPP
  EXMAN.CPP
  GMUTIL.CPP
  hpwhwrap.cpp
  IMPF.CPP
  LOOKUP.CPP
  MESSAGES.CPP
  MMPAK.CPP
  MSGTBL.CPP
  MSPAK.CPP
  nummeth.cpp
  PGPAK.CPP
  PP.CPP
  PSYCHRO.CPP
  PVCalc.cpp
  RMKERR.CPP
  RUNDATA.CPP
  SHADING.CPP
  SLPAK.CPP
  SRFD.CPP
  StdAfx.cpp
  STRPAK.CPP
  SYTB.CPP
  TDPAK.CPP
  TIMER.CPP
  UNTAB.CPP
  VRPAK.CPP
  WFPAK.CPP
  XIOPAK.CPP
  XMODULE.CPP
  YACAM.CPP
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
  CSE.H
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
  DTYPES.H
  ENVPAK.H
  EXMAN.H
  GMPAK.H
  ../vendor/hpwh/HPWH.hh
  IMPF.H
  IRATS.H
  LOOKUP.H
  MESSAGES.H
  MMPAK.H
  MSGHANS.H
  mspak.h
  NUMMETH.H
  PGPAK.H
  PP.H
  PSYCHRO.H
  PVCalc.h
  RCCN.H
  RMKERR.H
  SLPAK.H
  SRD.H
  STRPAK.H
  SYTB.H
  TDPAK.H
  TIMER.H
  VECPAK.H
  VRPAK.H
  WFPAK.H
  XIOPAK.H
  XMODULE.h
  YACAM.H
)

include_directories("${CMAKE_SOURCE_DIR}/src" "${CMAKE_SOURCE_DIR}/vendor" "${CMAKE_SOURCE_DIR}/vendor/penumbra/include")

add_definitions(-DINCNE)

link_directories("${CMAKE_SOURCE_DIR}/vendor/penumbra/lib")

add_executable(CSE ${source} ${headers})

set(libs
  glad
  penumbra
  glfw3
  tess2
)

set(libsd
  gladd
  penumbrad
  glfw3d
  tess2d
)

target_link_libraries(CSE
  "$<$<CONFIG:Debug>:${libsd}>"
  "$<$<CONFIG:RelWithDebInfo>:${libsd}>"
  "$<$<CONFIG:Release>:${libs}>"
  "$<$<CONFIG:MinSizeRel>:${libs}>"
)
