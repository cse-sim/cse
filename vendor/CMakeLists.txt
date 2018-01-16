if (CMAKE_VERSION VERSION_LESS 3.2)
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "")
else()
    # UPDATE_DISCONNECTED 1: Don't look for remote updates on sub-sequent builds (e.g., git pull)
    # Change to 0 to check for updates to branch
    set(UPDATE_DISCONNECTED_IF_AVAILABLE "UPDATE_DISCONNECTED 1")
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

download_project(PROJ                HPWHsim
                 GIT_REPOSITORY      https://github.com/EcotopeResearch/HPWHsim.git
                 GIT_TAG             v1.4.1
                 DOWNLOAD_DIR        "${CMAKE_CURRENT_BINARY_DIR}/HPWHsim-dl"
                 SOURCE_DIR          "${CMAKE_CURRENT_SOURCE_DIR}/HPWHsim"
                 BINARY_DIR          "${CMAKE_CURRENT_BINARY_DIR}/HPWHsim"
                 ${UPDATE_DISCONNECTED_IF_AVAILABLE}
)

add_subdirectory(${HPWHsim_SOURCE_DIR})
