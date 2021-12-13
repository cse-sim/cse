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
target_compile_features(RCDEF PRIVATE cxx_std_17)
