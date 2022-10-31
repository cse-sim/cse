set(source
  rcdef.cpp
  ../cvpak.cpp
  ../dmpak.cpp
  ../lookup.cpp
  ../messages.cpp
  ../msgtbl.cpp
  ../rmkerr.cpp
  ../strpak.cpp
  ../tdpak.cpp
  ../xiopak.cpp
  $<TARGET_OBJECTS:cse_libstubs>
)

set(headers
  ../cnglob.h
  ../dmpak.h
  dtypes.h
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