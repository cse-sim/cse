set(source
  rcdef.cpp
  $<TARGET_OBJECTS:cse_libstubs>
)

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
target_link_libraries(RCDEF PRIVATE libcse)
target_compile_features(RCDEF PRIVATE cxx_std_17)
