:: run beyond compare 4
@echo off
if exist "c:\program files\beyond compare 4\BCompare.exe" (
  @c:\"program files\beyond compare 4\BCompare.exe" %*
) else (
  @c:\"program files (x86)\beyond compare 4\BCompare.exe" %*
)
