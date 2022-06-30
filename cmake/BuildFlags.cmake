if (MSVC AND NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))

  # Final flags are Common + build-specific flags (e.g., Release, Debug)

  # '*' indicates CMake default option
  # '+' indicates default compiler behavior

  #================#
  # Compiler flags #
  #================#

  if(USE_X64)
    message("64 is working")
    set(CompileCommon
    #  /DWIN64     #*
    # /D_WINDOWS  #*
      /D_CONSOLE  #
      /DINCNE     # CSE-specific
      /W3         #*Warning level
      /GR         #*Enable Run-Time Type Information TODO: keep?
      /EHsc       #*Specifies the model of exception handling (sc options).
      /nologo     # Suppresses display of sign-on banner.
      /fp:precise #+Specifies floating-point behavior.
      /fp:except- # Specifies floating-point behavior.
    #  /arch:IA32  # Specifies the architecture for code generation (no special instructions).
    )
  else()
    message("You are using 32")
    set(CompileCommon
      /DWIN32     #*
    # /D_WINDOWS  #*
      /D_CONSOLE  #
      /DINCNE     # CSE-specific
      /W3         #*Warning level
      /GR         #*Enable Run-Time Type Information TODO: keep?
      /EHsc       #*Specifies the model of exception handling (sc options).
      /nologo     # Suppresses display of sign-on banner.
      /fp:precise #+Specifies floating-point behavior.
      /fp:except- # Specifies floating-point behavior.
      /arch:IA32  # Specifies the architecture for code generation (no special instructions).
    )
  endif()


  string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CompileCommon}")

  set(CompileRelease
  # /DNDEBUG    #* TODO: Add back?
  # /MD         #*Creates a multithreaded DLL using MSVCRT.lib.
    /MT         # Creates a multithreaded executable file using LIBCMT.lib.
    /GF         # Enables string pooling.
    /GL         # Enables whole program optimization.
    /Gy         # Enables function-level linking.
  # /O2         #*Creates fast code.
  # /Ob2        #*Controls inline expansion (level 2).
    /Oi         # Generates intrinsic functions.
    /Ot         # Favors fast code.
    /Ox         # Uses maximum optimization (/Ob2gity /Gs).
    /Oy         # Omits frame pointer (x86 only).
    /Gd         # Uses the __cdecl calling convention (x86 only).
    /Zi         #*Generates complete debugging information.
    /GS-        # Buffers security check.
    /MP         # Compiles multiple source files by using multiple processes.
  )
  string(REPLACE ";" " " CMAKE_CXX_FLAGS_RELEASE "${CompileRelease}")

  set(CompileDebug
    /D_DEBUG    #*
  # /MDd        #*Creates a debug multithreaded DLL using MSVCRT.lib.
    /MTd        # Creates a debug multithreaded executable file using LIBCMTD.lib.
    /Zi         #*Generates complete debugging information.
  # /Ob0        #*Controls inline expansion (level 0 -- disabled).
    /Od         #*Disables optimization.
    /RTC1       #*Enables run-time error checking.
  )
  string(REPLACE ";" " " CMAKE_CXX_FLAGS_DEBUG "${CompileDebug}")

  #==============#
  # Linker flags #
  #==============#

  if(USE_X64)
    set(LinkCommon
      /NOLOGO     # Suppresses the startup banner.
      /DYNAMICBASE#Specifies whether to generate an executable image that can be randomly rebased at load time by using the address space layout randomization (ASLR) feature.
      /MACHINE:X64 # Specifies the target platform.
    )
  else()
    set(LinkCommon
      /NOLOGO     # Suppresses the startup banner.
      /DYNAMICBASE#Specifies whether to generate an executable image that can be randomly rebased at load time by using the address space layout randomization (ASLR) feature.
      /MACHINE:X86 # Specifies the target platform.
    )
  endif()
  string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS "${LinkCommon}")

  set(LinkRelease
    /LTCG       # Specifies link-time code generation.
    /DEBUG	    # Creates debugging information.
    /MAP        # Creates a mapfile.
    /INCREMENTAL:NO # Controls incremental linking.
  )
  string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS_RELEASE "${LinkRelease}")

  set(LinkDebug
    /DEBUG	    # Creates debugging information.
    /MAP        # Creates a mapfile.
    /INCREMENTAL # Controls incremental linking.
  )
  string(REPLACE ";" " " CMAKE_EXE_LINKER_FLAGS_DEBUG "${LinkDebug}")



endif()
