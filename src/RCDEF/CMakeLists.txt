set(source
  rcdef.cpp
  $<TARGET_OBJECTS:cse_libstubs>
)

list(APPEND source ${cse_common_source})


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


add_executable(RCDEF ${source} ${headers})
target_compile_features(RCDEF PRIVATE cxx_std_17)

  #================#
  # Compiler flags #
  #================#

target_compile_options(RCDEF PRIVATE
  # MSVC common compiler flags
  $<$<CXX_COMPILER_ID:MSVC>:
    /DWIN32       #*
    # /D_WINDOWS  #*
    /D_CONSOLE    #
    /DINCNE       # CSE-specific
    /W3           #*Warning level
    /GR           #*Enable Run-Time Type Information TODO: keep?
    /EHsc         #*Specifies the model of exception handling (sc options).
    /nologo       # Suppresses display of sign-on banner.
    /fp:precise   #+Specifies floating-point behavior.
    /fp:except-   # Specifies floating-point behavior.
    /arch:IA32    # Specifies the architecture for code generation (no special instructions).
    # MSVC Release Flags
    $<$<CONFIG:Release>:
      # /DNDEBUG  #* TODO: Add back?
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
    # # MSVC DEBUG Flags
    $<$<CONFIG:Debug>:
      /D_DEBUG    #*
      # /MDd      #*Creates a debug multithreaded DLL using MSVCRT.lib.
      /MTd        # Creates a debug multithreaded executable file using LIBCMTD.lib.
      /Zi         #*Generates complete debugging information.
      # /Ob0      #*Controls inline expansion (level 0 -- disabled).
      /Od         #*Disables optimization.
      /RTC1       #*Enables run-time error checking.
    >
  >
  # Flags for GCC
  $<$<CXX_COMPILER_ID:GNU>:
    -Wall
  >
  # Flags for Clang
  $<$<CXX_COMPILER_ID:Clang>:
    -Wall
  >
)

#==============#
# Linker flags #
#==============#

target_link_options(RCDEF PRIVATE
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
  $<$<CXX_COMPILER_ID:Clang>:   # Clang Link Flags
  >
)
target_compile_definitions(RCDEF PRIVATE NOVRR)
