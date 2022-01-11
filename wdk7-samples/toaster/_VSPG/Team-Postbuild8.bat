@echo off
setlocal EnableDelayedExpansion
REM Called as this:
REM <this>.bat $(SolutionDir) $(ProjectDir) $(BuildConf) $(PlatformName) $(TargetDir) $(TargetFileNam) $(TargetName) $(IntrmDir)

set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%

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

call :Echos START from %batdir%

REM ==== Prelude Above ====

REM ================ StampInf ================

if "%PlatformName%"=="x64" (
	set stampinf_ARCH=AMD64
	set wdfdll_subdirname=x64
) else (
	set stampinf_ARCH=x86
	set wdfdll_subdirname=x86
)

if "" == "%vspg_InfTemplateName%" (
  call :Echos Env-var vspg_InfTemplateName is empty, no stampinf action taken.
  goto :DONE_STAMPINF
)

if "" == "%vspg_InfOldWord%" (
  call :Echos [ERROR] Env-var vspg_InfOldWord is empty.
  exit /b 4
)
if "" == "%vspg_InfNewWord%" (
  call :Echos [ERROR] Env-var vspg_InfNewWord is empty.
  exit /b 4
)

REM Now determine inf filename. 
if "%TargetName%" == %vspg_InfTemplateName% (
  set inf_filename=%TargetName%.inf
) else (
  set inf_filename=%TargetName%--%vspg_InfTemplateName%.inf
)

call :Echos calling ToasterStampInf.bat ...
set subcmd=call "%SolutionDir%\_VSPG\ToasterStampInf.bat"^
  "%ProjectDir%\..\inf-template\%vspg_InfTemplateName%.inx"^
  "%TargetDir%\%inf_filename%"^
  %vspg_InfOldWord%^
  %vspg_InfNewWord%^
  "-a %stampinf_ARCH% -k 1.9 -v 1.0.0.1"
REM call :Echos %subcmd%
%subcmd%
if errorlevel 1 exit /b 4

:DONE_STAMPINF

REM ======== Copy WDF CoInstaller DLL to output folder ========

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
