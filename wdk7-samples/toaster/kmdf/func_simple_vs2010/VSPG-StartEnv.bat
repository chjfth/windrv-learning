@echo off
REM If you have special environment variable(env-var) to set for you own .bat files,
REM you can set it here.

echo Loading Env-vars from "%~f0"

: Set input params to ToasterStampInf.bat which is called in post-build action.
call :SetEnvVar vspg_InfTemplateName=kmdf_toast_simple
call :SetEnvVar vspg_InfOldWord=toaster.
call :SetEnvVar vspg_InfNewWord=%TargetName%.

exit /b



:SetEnvVar
  set setcmd=set %*
  echo.  %setcmd%
  %setcmd%
exit /b

