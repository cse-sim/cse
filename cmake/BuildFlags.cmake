# '*' indicates CMake default option
# '+' indicates default compiler behavior

# Set global flags and remove defaults
if (MSVC)
  set(CMAKE_CXX_FLAGS "/EHsc")  #*Specifies the model of exception handling (sc options).
else ()
  set(CMAKE_CXX_FLAGS "")
endif()
set(CMAKE_CXX_FLAGS_RELEASE "")
set(CMAKE_CXX_FLAGS_DEBUG "")

add_library(cse_common_interface INTERFACE)

#==================#
# Compiler options #
#==================#

target_compile_options(cse_common_interface INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:
    /W3           #*Warning level
    # /WX         # Turn all warnings into errors
    /GR           #*Enable Run-Time Type Information TODO: keep?
    /nologo       # Suppresses display of sign-on banner.
    /fp:precise   #+Specifies floating-point behavior.
    /fp:except-   # Specifies floating-point behavior.
    /arch:IA32    # Specifies the architecture for code generation (no special instructions).

    $<$<CONFIG:Release>:
      # /MD       #*Creates a multithreaded DLL using MSVCRT.lib.
      /MT         # Creates a multithreaded executable file using LIBCMT.lib.
      /GF         # Enables string pooling.
      /GL         # Enables whole program optimization.
      /Gy         # Enables function-level linking.
      # /O2       #*Creates fast code.
      # /Ob2      #*Controls inline expansion (level 2).
      /Oi         # Generates intrinsic functions.
      /Ot         # Favors fast code.
      /Ox         # Uses maximum optimization (/Ob2gity /Gs).
      /Oy         # Omits frame pointer (x86 only).
      /Gd         # Uses the __cdecl calling convention (x86 only).
      /Zi         #*Generates complete debugging information.
      /GS-        # Buffers security check.
      /MP         # Compiles multiple source files by using multiple processes.
    >

    $<$<CONFIG:Debug>:
      # /MDd      #*Creates a debug multithreaded DLL using MSVCRT.lib.
      # /MTd      # Creates a debug multithreaded executable file using LIBCMTD.lib. (set through CMAKE_MSVC_RUNTIME_LIBRARY)
      /Zi         #*Generates complete debugging information.
      /Ob0        #*Controls inline expansion (level 0 -- disabled).
      /Od         #*Disables optimization.
      /RTC1       #*Enables run-time error checking.
    >
  >
  # Flags for GCC
  $<$<CXX_COMPILER_ID:GNU>:
    -Wall
  >
  # Flags for Clang
  $<$<CXX_COMPILER_ID:Clang,AppleClang>:
    -Wall
  >
)

#======================#
# Compiler definitions #
#======================#

target_compile_definitions(cse_common_interface INTERFACE
  INCNE       # CSE-specific
  $<$<CXX_COMPILER_ID:MSVC>:
    WIN32       #*
    _WINDOWS    #*
    _CONSOLE    #
    $<$<CONFIG:Release>:
      # NDEBUG  #* TODO: Add back?
    >
    $<$<CONFIG:Debug>:
      _DEBUG    #*
    >
  >
)

#================#
# Linker options #
#================#

target_link_options(cse_common_interface INTERFACE
  $<$<CXX_COMPILER_ID:MSVC>:    # MSVC Link Flags
    /NOLOGO         # Suppresses the startup banner.
    /DYNAMICBASE    # Specifies whether to generate an executable image that can be randomly rebased at load time by using the address space layout randomization (ASLR) feature.
    /MACHINE:X86    # Specifies the target platform.
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
