@echo off
REM If you have special environment variable(env-var) to set for you own .bat files,
REM you can set it here.

echo Loading Env-vars from "%~f0"

call :SetEnvVar dir_stock_WdfCoInstaller01009=%SolutionDir%\..\..\_share\WdfCoInstaller\1.9

exit /b

REM =============================
REM ====== Functions Below ======
REM =============================

:SetEnvVar
  set setcmd=set %*
  echo.  %setcmd%
  %setcmd%
exit /b

