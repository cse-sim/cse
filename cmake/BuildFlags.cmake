# '*' indicates CMake default option
# '+' indicates default compiler behavior

# Remove unwanted CMake defaults from global flags
if (MSVC)
  # See https://gitlab.kitware.com/cmake/cmake/-/blob/master/Modules/Platform/Windows-MSVC.cmake
  set(CMAKE_CXX_FLAGS
    /EHsc         #*Specifies the model of exception handling (sc options).
    /DWIN32       #*Windows Platform (regardless of architecture)
    /D_WINDOWS    #*
  )
  set(CMAKE_CXX_FLAGS_RELEASE
    /O2           #*Creates fast code (Og+Oi+Ot+Oy+Ob2+GF+Gy).
    # /Ob2        #*Controls inline expansion (level 2). (part of O2)
    /DNDEBUG      #*Enables or disables compilation of assertions (CSE traditionally did not define this. See https://github.com/cse-sim/cse/issues/285)
  )
  set(CMAKE_CXX_FLAGS_DEBUG
    /Ob0          #*Controls inline expansion (level 0 -- disabled).
    /Od           #*Disables optimization.
    /Zi           #*Generates complete debugging information.
    /RTC1         #*Enables run-time error checking.
)
else () # GCC or Clang or AppleClang
  # See https://gitlab.kitware.com/cmake/cmake/-/blob/master/Modules/Compiler/GNU.cmake
  set(CMAKE_CXX_FLAGS "")
  set(CMAKE_CXX_FLAGS_RELEASE
    -O3           #*Maximum optimization (see https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html#Optimize-Options).
    -DNDEBUG      #*Enables or disables compilation of assertions (CSE traditionally did not define this. See https://github.com/cse-sim/cse/issues/285)
  )
  set(CMAKE_CXX_FLAGS_DEBUG
    -g            #*Produce debugging information in the operating system’s native format.
  )
endif()

# Convert lists to space-separated strings
list(JOIN CMAKE_CXX_FLAGS " " CMAKE_CXX_FLAGS)
list(JOIN CMAKE_CXX_FLAGS_RELEASE " " CMAKE_CXX_FLAGS_RELEASE)
list(JOIN CMAKE_CXX_FLAGS_DEBUG " " CMAKE_CXX_FLAGS_DEBUG)

# Add specific flags for use in
add_library(cse_common_interface INTERFACE)

#==================#
# Compiler options #
#==================#

target_compile_options(cse_common_interface INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:
    /W3           #*Warning level
    /WX           # Turn all warnings into errors
    /GR           #*Enable Run-Time Type Information TODO: keep?
    /nologo       # Suppresses display of sign-on banner.
    /fp:precise   #+Specifies floating-point behavior.
    /fp:except-   # Specifies floating-point behavior.
    $<$<STREQUAL:"${CSE_BUILD_ARCHITECTURE}","32">:
      /arch:IA32  # Specifies the architecture for code generation (no special instructions).
    >
    # $<$<STREQUAL:"${CSE_BUILD_ARCHITECTURE}","64">:
    #   /arch:SSE2 #+Enables the use of Intel Advanced Vector Extensions instructions. Explicting setting this creates an erroneous warnings. https://stackoverflow.com/questions/1067630/sse2-option-in-visual-c-x64
    # >
    $<$<CONFIG:Release>:
      # /MD       #*Creates a multithreaded DLL using MSVCRT.lib.
      # /MT       # Creates a multithreaded executable file using LIBCMT.lib. (set through CMAKE_MSVC_RUNTIME_LIBRARY)
      /GL         # Enables whole program optimization.
      /Gd         # Uses the __cdecl calling convention (x86 only).
      /Zi         #*Generates complete debugging information.
      /GS-        # Buffers security check. (do not check)
      /MP         # Compiles multiple source files by using multiple processes.
    >
    $<$<CONFIG:Debug>:
      # /MDd      #*Creates a debug multithreaded DLL using MSVCRT.lib.
      # /MTd      # Creates a debug multithreaded executable file using LIBCMTD.lib. (set through CMAKE_MSVC_RUNTIME_LIBRARY)
    >
  >
  $<$<CXX_COMPILER_ID:GNU,Clang,AppleClang>:
    -Wall
    -Wno-invalid-offsetof # "offsetof" is essential to CSE, though it's use isn't guaranteed to be safe for non-POD structures in C++ implementations. See https://stackoverflow.com/questions/1129894/why-cant-you-use-offsetof-on-non-pod-structures-in-c.
  >
)

#======================#
# Compiler definitions #
#======================#

target_compile_definitions(cse_common_interface INTERFACE
    CSE_ARCH=${CSE_BUILD_ARCHITECTURE}    # 32 or 64
  $<$<CXX_COMPILER_ID:MSVC>:
    _CONSOLE    # MSVC Predefined Macro
  >
)

#================#
# Linker options #
#================#

target_link_options(cse_common_interface INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:    # MSVC Link Flags
    /NOLOGO         # Suppresses the startup banner.
    /DYNAMICBASE    # Specifies whether to generate an executable image that can be randomly rebased at load time by using the address space layout randomization (ASLR) feature.
    # /MACHINE:     # Specifies the target platform (allow linker to determine the machine type).
    $<$<CONFIG:Release>:        # MSVC Release Link Flags
      /LTCG         # Specifies link-time code generation.
      /DEBUG	      # Creates debugging information.
      /MAP          # Creates a mapfile.
      /INCREMENTAL:NO # Controls incremental linking.
    >
    $<$<CONFIG:Debug>:          # MSVC Debug Link Flags
      /DEBUG	      # Creates debugging information.
      /MAP          # Creates a mapfile.
      /INCREMENTAL  # Controls incremental linking.
    >
  >
  $<$<CXX_COMPILER_ID:GNU>:     # GCC Link Flags
  >
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:   # Clang Link Flags
  >
)
