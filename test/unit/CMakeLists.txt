set(source
      cse_tests.unit.cpp
      strpak.unit.cpp
      xiopak.unit.cpp
      cvpak.unit.cpp
	  nummeth.unit.cpp
      ${CSE_SOURCE_DIR}/src/libstubs.cpp
      ${CSE_BINARY_DIR}/src/untab.cpp
      ${CSE_BINARY_DIR}/src/dttab.cpp
   )

set_source_files_properties(
        "${CSE_BINARY_DIR}/src/untab.cpp"
        "${CSE_BINARY_DIR}/src/dttab.cpp"
        PROPERTIES GENERATED TRUE
)

list(APPEND source ${cse_common_source})
add_executable(cse_tests ${source})
add_dependencies(cse_tests generate_rcdef_outputs)
target_compile_definitions(cse_tests PRIVATE SOURCE_DIR="${CMAKE_SOURCE_DIR}")

target_include_directories(cse_tests PRIVATE
  "${CSE_SOURCE_DIR}/src"
  "${CSE_BINARY_DIR}/src"
  "${gtest_SOURCE_DIR}/include"
)

target_link_libraries(cse_tests PRIVATE gtest cse_common_interface)
target_compile_features(cse_tests PUBLIC cxx_std_17)

include(GoogleTest)

gtest_discover_tests(cse_tests TEST_SUFFIX .Unit)
