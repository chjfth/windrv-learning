@echo off
REM If you have special environment variable(env-var) to set for you own .bat files,
REM you can set it here.

echo Loading Env-vars from "%~f0"

: Set input params to ToasterMofComp.bat which is called in pre-build action.
call :SetEnvVar vspg_mofcompInput=..\bus\dynamic\busenum.mof
call :SetEnvVar vspg_mofcompOutput=..\bus\dynamic\busenum.bmf

exit /b %ERRORLEVEL%



:SetEnvVar
  set setcmd=set %*
  echo.  %setcmd%
  %setcmd%
exit /b 0

