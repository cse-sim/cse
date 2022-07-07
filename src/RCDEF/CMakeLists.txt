set(source
  ../cvpak.cpp
  ../dmpak.cpp
  ../envpak.cpp
  ../lookup.cpp
  ../messages.cpp
  ../msgtbl.cpp
  ../rcdef.cpp
  ../rmkerr.cpp
  ../strpak.cpp
  ../tdpak.cpp
  ../xiopak.cpp
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

add_definitions(-DNOVRR)

add_executable(RCDEF ${source} ${headers})
target_compile_features(RCDEF PRIVATE cxx_std_17)
