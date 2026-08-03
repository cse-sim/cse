include(initialize-submodules)

# Googletest -- added first, before any submodule that might vendor its own copy of
# add_submodule() and shadow this one. CMake macros are global, not scoped to the
# directory that defines them, so a same-named macro loaded later (e.g. HPWHsim's own)
# would silently replace this one for any add_submodule() call below that point. Adding
# it here first, with our preferred settings, also means the add_subdirectory'd projects
# below that check `if (NOT TARGET gtest)` themselves correctly see it as already provided.
if (NOT TARGET gtest)
  # Prevent GoogleTest from overriding our compiler/linker options
  # when building with Visual Studio
  add_submodule(googletest TARGET_NAME gtest
    CACHE_ON gtest_force_shared_crt BUILD_GTEST BUILD_GMOCK
    CACHE_OFF INSTALL_GTEST
    MARK_AS_ADVANCED BUILD_GTEST BUILD_GMOCK INSTALL_GTEST
  )
  set_target_properties(gtest PROPERTIES FOLDER Dependencies/Test)
  set_target_properties(gmock PROPERTIES FOLDER Dependencies/Test)
  set_target_properties(gtest_main PROPERTIES FOLDER Dependencies/Test)
  set_target_properties(gmock_main PROPERTIES FOLDER Dependencies/Test)
endif ()

# Penumbra
add_submodule(penumbra
  MARK_AS_ADVANCED
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

set_target_properties(glfw PROPERTIES FOLDER Dependencies/Penumbra/GLFW)
set_target_properties(glad PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(penumbra PROPERTIES FOLDER Dependencies/Penumbra)
set_target_properties(tess2 PROPERTIES FOLDER Dependencies/Penumbra)

# Kiva
option( KIVA_GROUND_PLOT "Build ground plotting library" OFF )

add_submodule(kiva TARGET_NAME libkiva
  CACHE_OFF
    KIVA_TESTING
    KIVA_EXE_BUILD
    KIVA_COVERAGE
    KIVA_3D
    BUILD_TESTING # Pollution from eigen
  MARK_AS_ADVANCED
    KIVA_3D
    KIVA_EXE_BUILD
    KIVA_COVERAGE
    KIVA_GROUND_PLOT
    KIVA_STATIC_LIB
    KIVA_TESTING
    ENABLE_OPENMP
    # Should be marked in future Kiva update
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

set_target_properties(libkiva PROPERTIES FOLDER Dependencies/Kiva)
set_target_properties(fmt PROPERTIES FOLDER Dependencies/Kiva)

if (KIVA_GROUND_PLOT)
  set_target_properties(groundplot PROPERTIES FOLDER Dependencies/Kiva/GroundPlot)
  set_target_properties(mgl-static PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(png_static PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
  set_target_properties(genfiles PROPERTIES FOLDER Dependencies/Kiva/GroundPlot/Dependencies)
endif()

# HPWHsim
set(HPWHSIM_OMIT_TESTTOOL ON CACHE BOOL "HPWHsim: Do not build testing code" FORCE)
set(HPWHSIM_ABRIDGED ON CACHE BOOL "HPWHsim: omit code not used by CSE" FORCE)
add_submodule(HPWHsim
  MARK_AS_ADVANCED HPWHSIM_OMIT_TESTTOOL HPWHSIM_ABRIDGED
)

set_target_properties(HPWHsim PROPERTIES FOLDER Dependencies/HPWHsim)
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
