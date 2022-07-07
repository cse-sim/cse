set(cse_test_src
      cse_tests.unit.cpp
      xiopak.unit.cpp
      $<TARGET_OBJECTS:cse_libstubs>
   )
   
add_executable(cse_tests ${cse_test_src})

target_include_directories(cse_tests PRIVATE
    "${CSE_SOURCE_DIR}/src"
    "${gtest_SOURCE_DIR}/include"
    )

target_link_libraries(cse_tests PRIVATE gtest libcse)

include(GoogleTest)

gtest_discover_tests(cse_tests TEST_SUFFIX .Unit)