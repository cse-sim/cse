include(initialize-submodules)
initialize_submodules()

# btwxt
add_subdirectory( btwxt)

# Penumbra
add_subdirectory(penumbra)

set_target_properties(awning PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(glfw PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(uninstall PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(glad PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(penumbra PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(tess2 PROPERTIES FOLDER Dependencies/Penumbra)

mark_as_advanced(
  BUILD_PENUMBRA_TESTING
  PENUMBRA_COVERAGE
  PENUMBRA_STATIC_LIB
  # Should be marked in future Penumbra update
  GLFW_BUILD_DOCS
  GLFW_BUILD_EXAMPLES
  GLFW_BUILD_TESTS
  GLFW_DOCUMENT_INTERNALS
  GLFW_INSTALL
  GLFW_USE_HYBRID_HPG
  GLFW_VULKAN_STATIC
  LIB_SUFFIX
  USE_MSVC_RUNTIME_LIBRARY_DLL
)

# Kiva
set(KIVA_TESTING OFF CACHE BOOL "" FORCE)
set(KIVA_EXE_BUILD OFF CACHE BOOL "" FORCE)
set(KIVA_COVERAGE OFF CACHE BOOL "" FORCE)
set(KIVA_3D OFF CACHE BOOL "" FORCE)

option( KIVA_GROUND_PLOT "Build ground plotting library" OFF )

add_subdirectory(kiva)

set_target_properties(libkiva PROPERTIES FOLDER Dependencies/Kiva)
set_target_properties(fmt PROPERTIES FOLDER Dependencies/Kiva)

mark_as_advanced(
  KIVA_3D
  KIVA_EXE_BUILD
  KIVA_COVERAGE
  KIVA_GROUND_PLOT
  KIVA_STATIC_LIB
  KIVA_TESTING
  ENABLE_OPENMP
)

# Should be marked in future Kiva update
mark_as_advanced(
  FMT_TEST
  FMT_CUDA_TEST
  FMT_DEBUG_POSTFIX
  FMT_DOC
  FMT_FUZZ
  FMT_MODULE
  FMT_INSTALL
  FMT_OS
  FMT_PEDANTIC
  FMT_WERROR
  FMT_INC_DIR
)

if (KIVA_GROUND_PLOT)
  set_target_properties(groundplot PROPERTIES FOLDER Dependencies/Kiva/GroundPlot)
  set_target_properties(boost_filesystem PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(boost_system PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(mgl-static PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(png_static PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(genfiles PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(zlibstatic PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
endif()

# HPWHsim
set(HPWHSIM_OMIT_TESTTOOL ON CACHE BOOL "HPWHsim: Do not build testing code" FORCE)
set(HPWHSIM_ABRIDGED ON CACHE BOOL "HPWHsim: omit code not used by CSE" FORCE)
mark_as_advanced(
  HPWHSIM_OMIT_TESTTOOL
  HPWHSIM_ABRIDGED
)
add_subdirectory(HPWHsim)

set_target_properties(libHPWHsim PROPERTIES FOLDER Dependencies/HPWHsim)
if (NOT HPWHSIM_OMIT_TESTTOOL)
  set_target_properties(results_directory PROPERTIES FOLDER Dependencies/Test)
endif()

set_target_properties(btwxt PROPERTIES FOLDER Dependencies)
mark_as_advanced(
  BTWXT_COVERAGE
  BTWXT_STATIC_LIB
  BUILD_BTWXT_TESTING
  GIT_SUBMODULE # Should go away with Btwxt update
)

# Creates a multithreaded executable file using LIBCMT.lib.
target_compile_options(penumbra PUBLIC $<$<CXX_COMPILER_ID:MSVC>:$<$<CONFIG:RELEASE>:/MT>$<$<CONFIG:DEBUG>:/MTd /RTC1 /Od>>)
target_compile_options(libHPWHsim PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/EHsc $<$<CONFIG:RELEASE>:/MT>$<$<CONFIG:DEBUG>:/MTd>>)
target_compile_options(libkiva PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /EHsc $<$<CONFIG:RELEASE>:/MT>$<$<CONFIG:DEBUG>:/MTd>>)
target_compile_options(btwxt PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /EHsc $<$<CONFIG:RELEASE>:/MT>$<$<CONFIG:DEBUG>:/MTd>>)
target_compile_options(fmt PRIVATE $<$<CXX_COMPILER_ID:MSVC>: /EHsc>)


if (NOT TARGET gtest)
  # Prevent GoogleTest from overriding our compiler/linker options
  # when building with Visual Studio
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  set(BUILD_GTEST ON CACHE BOOL "" FORCE MARK)
  set(BUILD_GMOCK ON CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  mark_as_advanced(BUILD_GTEST BUILD_GMOCK INSTALL_GTEST)
  add_subdirectory(googletest)
  set_target_properties(gtest PROPERTIES FOLDER Dependencies/Test)
  set_target_properties(gmock PROPERTIES FOLDER Dependencies/Test)
  set_target_properties(gtest_main PROPERTIES FOLDER Dependencies/Test)
  set_target_properties(gmock_main PROPERTIES FOLDER Dependencies/Test)
endif ()

