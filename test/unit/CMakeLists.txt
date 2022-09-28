set(source
      cse_tests.unit.cpp
      strpak.unit.cpp
      xiopak.unit.cpp
      $<TARGET_OBJECTS:cse_libstubs>
   )
   
list(APPEND source ${cse_common_source})

add_executable(cse_tests ${source})

target_include_directories(cse_tests PRIVATE
  "${CMAKE_SOURCE_DIR}/src/RCDEF" # stubs don't need legit dtypes.h
  "${CSE_SOURCE_DIR}/src"
  "${CSE_BINARY_DIR}/src"
  "${gtest_SOURCE_DIR}/include"
)

target_link_libraries(cse_tests PRIVATE gtest)
target_compile_features(cse_tests PUBLIC cxx_std_17)
target_compile_definitions(cse_tests PRIVATE NOVRR)

include(GoogleTest)

gtest_discover_tests(cse_tests TEST_SUFFIX .Unit)
