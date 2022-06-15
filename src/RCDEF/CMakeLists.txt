set(source
  ../CVPAK.CPP
  ../DMPAK.CPP
  ../ENVPAK.CPP
  ../LOOKUP.CPP
  ../MESSAGES.CPP
  ../MSGTBL.CPP
  ../RCDEF.CPP
  ../RMKERR.CPP
  ../STRPAK.CPP
  ../TDPAK.CPP
  ../XIOPAK.CPP
)

set(headers
  ../CNGLOB.H
  ../DMPAK.H
  dtypes.h
  ../LOOKUP.H
  ../MESSAGES.H
  ../RMKERR.H
  ../XIOPAK.H
)

include_directories(
  ..
  .
)

add_definitions(-DNOVRR)

add_executable(RCDEF ${source} ${headers})

# Adds target compile flags for gcc only. This can be use to suppress warnings in the future
# Turn on build cse in 32-bit. Some libraries might be needed to work properly
target_compile_options(RCDEF PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:
  -m32>
)

# Adds target link flags
# Turn on linking 32-bit system.
target_link_options(RCDEF PRIVATE
  $<$<CXX_COMPILER_ID:GNU>:
  -m32>
)

target_compile_features(RCDEF PRIVATE cxx_std_17)
