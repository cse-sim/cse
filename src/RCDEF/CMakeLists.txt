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

# Added for debuging purposes to be deleted in the future
target_compile_options(RCDEF PRIVATE
  $<$<CXX_COMPILER_ID:MSVC>:/W4>
  $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
  -Wno-writable-strings -Wno-format -Wno-unused-value -Wno-undefined-inline -Wno-varargs>
)

target_compile_features(RCDEF PRIVATE cxx_std_17)
