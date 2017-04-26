:: run beyond compare 4

if exist "c:\program files\beyond compare 4\bcomp.exe" (
  @c:\"program files\beyond compare 4\bcomp" %*
) else (
  @c:\"program files (x86)\beyond compare 4\bcomp" %*
)