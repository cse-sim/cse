set(source
  rcdef.cpp
  $<TARGET_OBJECTS:cse_libstubs>
)

list(APPEND source ${cse_common_source})


set(headers
  ../cnglob.h
  ../dmpak.h
  dtypes.h
  ../lookup.h
  ../messages.h
  ../rmkerr.h
  ../xiopak.h
)

include_directories(
  ..
  .
)


add_executable(RCDEF ${source} ${headers})
target_compile_features(RCDEF PRIVATE cxx_std_17)
target_compile_definitions(RCDEF PRIVATE NOVRR)
target_link_libraries(RCDEF PRIVATE cse_common_interface)
