@REM This sample applies to all of 
@REM Team-Prebuild.bat, Team-Postbuild.bat, Personal-Prebuild.bat, Personal-Postbuild.bat

@echo off
setlocal EnableDelayedExpansion

set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%
set _vspgINDENTS=%_vspgINDENTS%.
call :Echos START from "%batdir%"

REM == debugging purpose ==
REM call :EchoVar SolutionDir
REM call :EchoVar ProjectDir
REM call :EchoVar BuildConf
REM call :EchoVar IntrmDir
REM call :EchoVar TargetDir
REM call :EchoVar PlatformName
REM call :EchoVar TargetName

REM ==== Prelude Above ====

if "%PlatformShortName%" == "amd64" set PlatformShortName=x64

set exe2dir=%SolutionDir%\__%BuildConf%
if not exist "%exe2dir%" (
	call :EchoAndExec mkdir "%exe2dir%"
	if errorlevel 1 exit /b 4
)

REM
REM This copies compiler-generated devcon.exe to devcon-x86.exe, devcon-x64.exe etc.
REM
call :EchoAndExec copy "%ExeDllDir%\%TargetFilenam%" "%exe2dir%\%TargetName%-%PlatformShortName%.exe"
if errorlevel 1 exit /b 4

exit /b 0



REM =============================
REM ====== Functions Below ======
REM =============================

:Echos
  REM This function preserves %ERRORLEVEL% for the caller,
  REM and, LastError does NOT pollute the caller.
  setlocal & set LastError=%ERRORLEVEL%
  echo %_vspgINDENTS%[%batfilenam%] %*
exit /b %LastError%

:EchoAndExec
  echo %_vspgINDENTS%[%batfilenam%] EXEC: %*
  %*
exit /b %ERRORLEVEL%

:EchoVar
  setlocal & set Varname=%~1
  call echo %_vspgINDENTS%[%batfilenam%] %Varname% = %%%Varname%%%
exit /b 0

:SetErrorlevel
  REM Usage example:
  REM call :SetErrorlevel 4
exit /b %1

