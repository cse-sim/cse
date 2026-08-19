# SPDX-FileCopyrightText: © 2025 Big Ladder Software <info@bigladdersoftware.com>
# SPDX-License-Identifier: BSD-3-Clause

macro(add_submodule submodule_name)
    # Clone a submodule and add its subdirectory to the build (if the corresponding target hasn't already been added)
    set(options) # None
    set(one_value_args
            TARGET_NAME           # Specify if target name is different from the submodule name
            PARENT_SUBMODULE_PATH # Specify if you want to leverage a submodule outside of this project's vendor directory
    )
    set(multi_value_args
            CACHE_ON # Set options from submodule as "ON" in the cache
            CACHE_OFF # Set options from submodule as "OFF" in the cache
            MARK_AS_ADVANCED # Mark certain options from the submodule project as advanced (to hide from higher level CMake GUI)
    )
    cmake_parse_arguments(add_${submodule_name}_args "${options}" "${one_value_args}" "${multi_value_args}" ${ARGN})

    if (DEFINED add_${submodule_name}_args_TARGET_NAME)
        set(target_name ${add_${submodule_name}_args_TARGET_NAME})
    else ()
        set(target_name "${submodule_name}")
    endif ()

    if (DEFINED add_${submodule_name}_args_PARENT_SUBMODULE_PATH)
        set(submodule_path "${add_${submodule_name}_args_PARENT_SUBMODULE_PATH}/vendor/${submodule_name}")
    else ()
        set(submodule_path "${CMAKE_CURRENT_SOURCE_DIR}/${submodule_name}")
    endif ()

    # Clone repository
    if (GIT_FOUND AND NOT EXISTS "${submodule_path}/.git")
        message(STATUS "Cloning submodule \"${submodule_name}\"")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init ${submodule_path}
                WORKING_DIRECTORY ${submodule_path}
                RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if (NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "Unable to update submodule \"${submodule_name}\"")
        endif ()
    endif ()

    # Cache on -- must run before add_subdirectory: these are typically options the
    # submodule's own CMakeLists.txt reads via option()/if() to decide what to build,
    # so forcing them after add_subdirectory would be too late to have any effect.
    if (DEFINED add_${submodule_name}_args_CACHE_ON)
        foreach (variable ${add_${submodule_name}_args_CACHE_ON})
            set(${variable} ON CACHE BOOL "" FORCE)
        endforeach ()
    endif ()

    # Cache off -- see Cache on above
    if (DEFINED add_${submodule_name}_args_CACHE_OFF)
        foreach (variable ${add_${submodule_name}_args_CACHE_OFF})
            set(${variable} OFF CACHE BOOL "" FORCE)
        endforeach ()
    endif ()

    # Add subdirectory
    if (NOT TARGET ${target_name})
        if (NOT EXISTS "${submodule_path}")
            message(FATAL_ERROR "Submodule directory \"${submodule_path}\" does not exist")
        endif ()
        add_subdirectory(${submodule_path})
    endif ()

    # Mark as advanced -- runs after add_subdirectory since some of these variables are
    # only declared (as cache entries) by the submodule's own CMakeLists.txt
    if (DEFINED add_${submodule_name}_args_MARK_AS_ADVANCED)
        foreach (variable ${add_${submodule_name}_args_MARK_AS_ADVANCED})
            mark_as_advanced(${variable})
        endforeach ()
    endif ()

endmacro()
