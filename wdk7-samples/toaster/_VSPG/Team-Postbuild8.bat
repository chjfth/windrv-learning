@echo off
setlocal EnableDelayedExpansion
REM Called as this:
REM <this>.bat $(SolutionDir) $(ProjectDir) $(BuildConf) $(PlatformName) $(TargetDir) $(TargetFileNam) $(TargetName) $(IntrmDir)

set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%
set _vspgINDENTS=%_vspgINDENTS%.
call :Echos START from %batdir%

REM _SolutionDir_ has double-quotes around, SolutionDir has no quotes.
set _SolutionDir_=%1&set SolutionDir=%~1
set _ProjectDir_=%2&set ProjectDir=%~2
set _BuildConf_=%3&set BuildConf=%~3
set _PlatformName_=%4&set PlatformName=%~4
set _TargetDir_=%5&set TargetDir=%~5
set _TargetFilenam_=%6&set TargetFilenam=%~6
set _TargetName_=%7&set TargetName=%~7
set _IntrmDir_=%8&set IntrmDir=%~8

REM == debugging purpose ==
REM call :EchoVar SolutionDir
REM call :EchoVar ProjectDir
REM call :EchoVar _BuildConf_
REM call :EchoVar BuildConf
REM call :EchoVar IntrmDir
REM call :EchoVar TargetDir
REM call :EchoVar PlatformName
REM call :EchoVar TargetName

REM ==== Prelude Above ====

REM ================ StampInf ================

set fp_stampinfs=%ProjectDir%\StampInfs.bat

if exist "%fp_stampinfs%" (
	call "%fp_stampinfs%"
	if errorlevel 1 exit /b 4
) else (
	call :Echos Not doing stampinf, bcz not found "%fp_stampinfs%"
)



REM ======== Copy WDF CoInstaller DLL to output folder ========

if "%wdfdll_subdirname%" == "" (
	call :Echos [ERROR] Env-var wdfdll_subdirname not defined by outer bat yet.
	exit /b 4
)

set targetWdfDll=%TargetDir%\WdfCoInstaller01009.dll

call :EchoVar dir_stock_WdfCoInstaller01009
if not defined dir_stock_WdfCoInstaller01009 goto :DONE_COPY_WDFDLL

set copycmd=copy "%dir_stock_WdfCoInstaller01009%\%wdfdll_subdirname%\WdfCoInstaller01009.dll" "%TargetDir%"
if not exist "%targetWdfDll%" (
	call :EchoExec %copycmd%
	%copycmd%
)

:DONE_COPY_WDFDLL


goto :END

REM =============================
REM ====== Functions Below ======
REM =============================

:Echos
  echo %_vspgINDENTS%[%batfilenam%] %*
exit /b

:EchoExec
  echo %_vspgINDENTS%[%batfilenam%] EXEC: %*
exit /b

:EchoVar
  REM Env-var double expansion trick from: https://stackoverflow.com/a/1200871/151453
  set _Varname=%1
  for /F %%i in ('echo %_Varname%') do echo %_vspgINDENTS%[%batfilenam%] %_Varname% = !%%i!
exit /b

:SetErrorlevel
  REM Usage example:
  REM call :SetErrorlevel 4
exit /b %1

:END
