if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(${PROJECT_NAME}_GIT_SUBMODULE "Check submodules during build" ON)
    if(${PROJECT_NAME}_GIT_SUBMODULE)
        message(STATUS "${PROJECT_NAME} submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
          WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
          RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

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
set(HPWHSIM_OMIT_TESTTOOL ON CACHE BOOL "Do not build HPWHsim testing code" FORCE)
add_subdirectory(HPWHsim)

set_target_properties(libHPWHsim PROPERTIES FOLDER Dependencies/HPWHsim)

