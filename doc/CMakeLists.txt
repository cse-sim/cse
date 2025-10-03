find_program(UV_EXECUTABLE NAMES uv REQUIRED)

if(CSE_BUILD_DOCUMENTATION STREQUAL "BuildWithAll")
    set(build_with_all_string "ALL")
endif()

add_custom_target(
        cse_documentation
        ${build_with_all_string}
        COMMAND ${UV_EXECUTABLE} run mkdocs build
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

add_test(NAME documentation.coverage
        COMMAND ${UV_EXECUTABLE} run coverage.py --path_to_cse $<TARGET_FILE:CSE> --path_to_input_data "docs/input-data"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src"
)