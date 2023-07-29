set(source
  rcdef.cpp
  ../lookup.cpp
  ../dmpak.cpp
  ../messages.cpp
  ../msgtbl.cpp
  ../rmkerr.cpp
  ../strpak.cpp
  ../xiopak.cpp
  ../libstubs.cpp
)

set(headers
  ../cnglob.h
  ../lookup.h
  ../dmpak.h
  ../messages.h
  ../rmkerr.h
  ../xiopak.h
)

include_directories(
  .
  ..
)


add_executable(RCDEF ${source} ${headers})
target_compile_features(RCDEF PRIVATE cxx_std_17)
target_compile_definitions(RCDEF PRIVATE NODTYPES NOVRR)
target_link_libraries(RCDEF PRIVATE cse_common_interface)
