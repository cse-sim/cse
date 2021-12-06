include(initialize-submodules)
initialize_submodules()

# Penumbra
add_subdirectory(penumbra)

set_target_properties(awning PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(glfw PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(uninstall PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(glad PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(penumbra PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(tess2 PROPERTIES FOLDER Dependencies/Penumbra)

# Kiva
set(KIVA_TESTING OFF CACHE BOOL "" FORCE)
set(KIVA_EXE_BUILD OFF CACHE BOOL "" FORCE)
set(KIVA_COVERAGE OFF CACHE BOOL "" FORCE)
set(KIVA_3D OFF CACHE BOOL "" FORCE)

option( KIVA_GROUND_PLOT "Build ground plotting library" OFF )

add_subdirectory(kiva)

set_target_properties(libkiva PROPERTIES FOLDER Dependencies/Kiva)

if (KIVA_GROUND_PLOT)
  set_target_properties(groundplot PROPERTIES FOLDER Dependencies/Kiva/GroundPlot)
  set_target_properties(boost_filesystem PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(boost_system PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(mgl-static PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(png_static PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(genfiles PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(zlibstatic PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
endif()

# HPWHsim
set(HPWHSIM_OMIT_TESTTOOL ON CACHE BOOL "HPWHsim: Do not build testing code" FORCE)
set(HPWHSIM_ABRIDGED ON CACHE BOOL "HPWHsim: omit code not used by CSE" FORCE)
add_subdirectory(HPWHsim)

set_target_properties(libHPWHsim PROPERTIES FOLDER Dependencies/HPWHsim)
if (NOT HPWHSIM_OMIT_TESTTOOL)
  set_target_properties(results_directory PROPERTIES FOLDER Dependencies/Test)
endif()

# btwxt
add_subdirectory( btwxt)



