@echo off
REM If you have special environment variable(env-var) to set for you own .bat files,
REM you can set it here.

echo Loading Env-vars from "%~f0"

call :SetEnvVar vspg_InfTemplateName=kmdf_toast_simple

exit /b

:SetEnvVar
  set setcmd=set %*
  echo.  %setcmd%
  %setcmd%
exit /b
