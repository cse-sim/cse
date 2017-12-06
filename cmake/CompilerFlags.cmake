if (MSVC AND NOT ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel"))

  ## TODO:
  #  - Check with Chip regarding other build types
  #  - Add precompiled header flags (/Yu"cnglob.h" /Fp"xxxx\CSEd.PCH")
  #  - Figure out where other flags are coming from in CMake (e.g., Zc:*, /analyze-, /Gd /TP, etc.)

  # Final flags are CommonFlags + build-specific flags (e.g., ReleaseFlags, DebugFlags)

  # '*' indicates CMake default option
  # '+' indicates default compiler behavior

  set(CommonFlags
    /DWIN32     #*
  # /D_WINDOWS  #*
    /D_CONSOLE  #
    /DINCNE     # CSE-specific
    /W3         #*Warning level
    /GR         #*Enable Run-Time Type Information TODO: keep?
    /EHsc       #*Specifies the model of exception handling (sc options).
    /nologo     # Suppresses display of sign-on banner.
  # /fp:precise #+Specifies floating-point behavior.
  )

  string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CommonFlags}")

  set(ReleaseFlags
    /DNDEBUG    #*
  # /MD         #*Creates a multithreaded DLL using MSVCRT.lib.
    /MT         # Creates a multithreaded executable file using LIBCMT.lib.
    /O2         #*Creates fast code.
    /Ob2        #*Controls inline expansion (level 2).
  )

  string(REPLACE ";" " " CMAKE_CXX_FLAGS_RELEASE "${ReleaseFlags}")

  set(DebugFlags
    /D_DEBUG    #*
  # /MDd        #*Creates a debug multithreaded DLL using MSVCRT.lib.
    /MTd        # Creates a debug multithreaded executable file using LIBCMTD.lib.
    /Zi         #*Includes debug information in a program database compatible with Edit and Continue.
  # /Ob0        #*Controls inline expansion (level 0 -- disabled).
    /Od         #*Disables optimization.
    /RTC1       #*Enables run-time error checking.
    /Gm         # Enables minimal rebuild.
  )

  string(REPLACE ";" " " CMAKE_CXX_FLAGS_DEBUG "${DebugFlags}")

  set(MinSizeRelFlags
    /DNDEBUG    #*
  # /MD         #*Creates a multithreaded DLL using MSVCRT.lib.
    /MT         # Creates a multithreaded executable file using LIBCMT.lib.
    /O1         #*Creates small code.
    /Ob1        #*Controls inline expansion (level 1).
  )

  string(REPLACE ";" " " CMAKE_CXX_FLAGS_MINSIZEREL "${MinSizeRelFlags}")

  set(RelWithDebInfoFlags
    /DNDEBUG    #*
  # /MD         #*Creates a multithreaded DLL using MSVCRT.lib.
    /MT         # Creates a multithreaded executable file using LIBCMT.lib.
    /Zi         #*Includes debug information in a program database compatible with Edit and Continue.
    /O2         #*Creates fast code.
    /Ob1        #*Controls inline expansion (level 1).
    /Gm         # Enables minimal rebuild.
  )

  string(REPLACE ";" " " CMAKE_CXX_FLAGS_RELWITHDEBINFO "${RelWithDebInfoFlags}")

endif()
