if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    # UPDATE_DISCONNECTED 1: Don't look for remote updates on sub-sequent builds (e.g., git pull)
    # Change to 0 to check for updates to branch
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 0")
endif()

include(DownloadProject)
download_project(PROJ                Penumbra
                 GIT_REPOSITORY      https://github.com/bigladder/penumbra.git
                 GIT_TAG             v0.1.1
                 DOWNLOAD_DIR        "${CMAKE_CURRENT_BINARY_DIR}/Penumbra-dl"
                 SOURCE_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/Penumbra"
                 BINARY_DIR          "${CMAKE_CURRENT_BINARY_DIR}/Penumbra"
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${Penumbra_SOURCE_DIR})

set_target_properties(awning PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(glfw PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(uninstall PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(glad PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(penumbra PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(tess2 PROPERTIES FOLDER Dependencies/Penumbra)

download_project(PROJ                Kiva
                 GIT_REPOSITORY      https://github.com/bigladder/kiva.git
                 GIT_TAG             v0.5.0
                 DOWNLOAD_DIR        "${CMAKE_CURRENT_BINARY_DIR}/kiva-dl"
                 SOURCE_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/kiva"
                 BINARY_DIR          "${CMAKE_CURRENT_BINARY_DIR}/kiva"
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

set(KIVA_TESTING OFF CACHE BOOL "" FORCE)
set(KIVA_EXE_BUILD OFF CACHE BOOL "" FORCE)
set(KIVA_COVERAGE OFF CACHE BOOL "" FORCE)
set(KIVA_3D OFF CACHE BOOL "" FORCE)

option( KIVA_GROUND_PLOT "Build ground plotting library" OFF )

add_subdirectory(${Kiva_SOURCE_DIR})

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

download_project(PROJ                HPWHsim
                 GIT_REPOSITORY      https://github.com/EcotopeResearch/HPWHsim.git
                 GIT_TAG             v1.4.1
                 DOWNLOAD_DIR        "${CMAKE_CURRENT_BINARY_DIR}/HPWHsim-dl"
                 SOURCE_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/HPWHsim"
                 BINARY_DIR          "${CMAKE_CURRENT_BINARY_DIR}/HPWHsim"
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${HPWHsim_SOURCE_DIR})

set_target_properties(libHPWHsim PROPERTIES FOLDER Dependencies/HPWHsim)
set_target_properties(testTool PROPERTIES FOLDER Dependencies/HPWHsim)
