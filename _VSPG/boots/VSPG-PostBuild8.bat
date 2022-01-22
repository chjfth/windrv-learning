@echo off
setlocal EnableDelayedExpansion
REM Called as this:
REM <this>.bat $(SolutionDir) $(ProjectDir) $(BuildConf) $(PlatformName) $(TargetDir) $(TargetFileNam) $(TargetName) $(IntrmDir)
REM ==== boilerplate code >>>
REM
set batfilenam=%~n0%~x0
set bootsdir=%~dp0
set bootsdir=%bootsdir:~0,-1%
call "%bootsdir%\PathSplit.bat" "%bootsdir%" userbatdir __temp
set _vspgINDENTS=%_vspgINDENTS%.

set SolutionDir=%~1
set ProjectDir=%~2
REM BuildConf : Debug | Release
set BuildConf=%~3
set _BuildConf_=%3
REM PlatformName : Win32 | x64
set PlatformName=%~4
REM TargetDir is the EXE/DLL output directory
set TargetDir=%~5
set _TargetDir_=%5
REM TargetFilenam is the EXE/DLL output name (varname chopping trailing 'e', means "no path prefix")
set TargetFilenam=%~6
set TargetName=%~7
set IntrmDir=%~8
REM ==== boilerplate code <<<<


call :Echos called with params: 
call :EchoVar bootsdir
call :EchoVar SolutionDir
call :EchoVar ProjectDir
call :EchoVar BuildConf
call :EchoVar PlatformName
call :EchoVar IntrmDir
call :EchoVar TargetDir
call :EchoVar TargetFilenam
call :EchoVar TargetName

REM Try to call some PostBuild bat-s  from one of five predefined directories,
REM whichever is encountered first. But if none found, just do nothing.

set SubbatSearchDirs=^
  "%ProjectDir%"^
  "%ProjectDir%\_VSPG"^
  "%SolutionDir%"^
  "%SolutionDir%\_VSPG"^
  "%userbatdir%"

REM ==== Call Team-Postbuild8.bat if exist. ====
call "%bootsdir%\SearchAndExecSubbat.bat" Greedy0 Team-PostBuild8.bat %VSPG_VSIDE_ParamsPack% %SubbatSearchDirs%
if errorlevel 1 exit /b 4

REM ==== Call Personal-Postbuild8.bat if exist. ====
call "%bootsdir%\SearchAndExecSubbat.bat" Greedy0 Personal-PostBuild8.bat %VSPG_VSIDE_ParamsPack% %SubbatSearchDirs%
if errorlevel 1 exit /b 4

REM ==== Call PostBuild-CopyOutput4.bat if exist. ====
REM If you need this bat, just copy it from ..\samples\PostBuild-CopyOutput4.bat.sample,
REM and tune some variables there to meet your need..

call "%bootsdir%\SearchAndExecSubbat.bat" Greedy0 PostBuild-CopyOutput4.bat^
  """%BuildConf%"" ""%PlatformName%"" ""%TargetDir%"" ""%TargetName%"""^
  %SubbatSearchDirs%
if errorlevel 1 exit /b 4


goto :END

REM =============================
REM ====== Functions Below ======
REM =============================

REM %~n0%~x0 is batfilenam
:Echos
  echo %_vspgINDENTS%[%batfilenam%] %*
exit /b 0

:EchoExec
  echo %_vspgINDENTS%[%batfilenam%] EXEC: %*
exit /b 0

:EchoVar
  REM Env-var double expansion trick from: https://stackoverflow.com/a/1200871/151453
  set _Varname=%1
  for /F %%i in ('echo %_Varname%') do echo %_vspgINDENTS%[%batfilenam%] %_Varname% = !%%i!
exit /b 0

:SetErrorlevel
  REM Usage example:
  REM call :SetErrorlevel 4
exit /b %1

:END
exit /b %ERRORLEVEL%
