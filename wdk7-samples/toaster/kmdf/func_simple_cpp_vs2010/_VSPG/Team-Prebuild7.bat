@echo off
setlocal EnableDelayedExpansion
REM VSPG-PreBuild7.bat $(SolutionDir) $(ProjectDir) $(Configuration) $(PlatformName) $(TargetDir) $(TargetFileName) $(TargetName)
REM ==== boilerplate code >>>
REM
set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%
set SolutionDir=%~1
set SolutionDir=%SolutionDir:~0,-1%
set ProjectDir=%~2
set ProjectDir=%ProjectDir:~0,-1%
REM BuildConf : Debug | Release
set BuildConf=%~3
set _BuildConf_=%3
REM PlatformName : Win32 | x64
set PlatformName=%4
REM TargetDir is the EXE/DLL output directory
set TargetDir=%~5
set _TargetDir_=%5
set TargetDir=%TargetDir:~0,-1%
REM TargetFilenam is the EXE/DLL output name (varname chopping trailing 'e', means "no path prefix")
set TargetFilenam=%~6
set TargetName=%~7
REM
rem call :Echos START for %ProjectDir%
REM
REM ==== boilerplate code <<<<

REM// Check that WDKPATH env-var in defined, otherwise, assert error.

if "%WDKPATH%"=="" (
  echo.
  call :Echos [ERROR] Env-var WDKPATH is empty. It must be set to the directory containing WDK 7.1
  echo.
  exit /b 4
)


goto :END

REM =============================
REM ====== Functions Below ======
REM =============================

:SetErrorlevel
exit /b %1

:Echos
  echo [%batfilenam%] %*
exit /b

:EchoExec
  echo [%batfilenam%] EXEC: %*
exit /b

:EchoVar
  REM Env-var double expansion trick from: https://stackoverflow.com/a/1200871/151453
  set _Varname=%1
  for /F %%i in ('echo %_Varname%') do echo [%batfilenam%] %_Varname% = !%%i!
exit /b

:SetErrorlevel
  REM Usage example:
  REM call :SetErrorlevel 4
exit /b %1

:END
