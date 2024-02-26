# RCDEF CMakeLists.txt

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

add_executable(RCDEF ${source} ${headers})
target_compile_features(RCDEF PRIVATE cxx_std_17)
target_include_directories(RCDEF PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CSE_SOURCE_DIR}/src")
target_compile_definitions(RCDEF PRIVATE NODTYPES)
target_link_libraries(RCDEF PRIVATE cse_common_interface)
