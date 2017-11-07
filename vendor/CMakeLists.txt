if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
endif()

include("${PROJECT_SOURCE_DIR}/cmake/DownloadProject.cmake")
download_project(PROJ                Penumbra
                 GIT_REPOSITORY      https://github.com/cse-sim/penumbra.git
                 GIT_TAG             master
                 DOWNLOAD_DIR        "${CMAKE_CURRENT_BINARY_DIR}/Penumbra-dl"
                 SOURCE_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/Penumbra"
                 BINARY_DIR          "${CMAKE_CURRENT_BINARY_DIR}/Penumbra"
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${Penumbra_SOURCE_DIR})

include("${PROJECT_SOURCE_DIR}/cmake/DownloadProject.cmake")
download_project(PROJ                HPWHsim
                 GIT_REPOSITORY      https://github.com/EcotopeResearch/HPWHsim.git
                 GIT_TAG             cmake
                 DOWNLOAD_DIR        "${CMAKE_CURRENT_BINARY_DIR}/HPWHsim-dl"
                 SOURCE_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/HPWHsim"
                 BINARY_DIR          "${CMAKE_CURRENT_BINARY_DIR}/HPWHsim"
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${HPWHsim_SOURCE_DIR})
