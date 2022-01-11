@echo off
setlocal EnableDelayedExpansion

set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%
set _vspgINDENTS=%_vspgINDENTS%.
call :Echos START from %batdir%

REM ==========================


REM
REM >>>>>>>> set parameters here
REM
set inx_list=kmdf_toast_simple kmdf_toast_clsinstlr
set oldword=@toaster@
set newword=%TargetName%
REM
REM <<<<<<<< set parameters here
REM


REM For each inx template listed above, call ToasterStampInf on them to get 
REM corresponding .inf files in %TargetDir%.

if "%dir_toaster_inf_template%" == "" (
	call :Echos [ERROR] Env-var dir_toaster_inf_template not defined by from outer bat yet.
	exit /b 4
)

if "%stampinf_ARCH%" == "" (
	call :Echos [ERROR] Env-var stampinf_ARCH not defined by outer bat yet.
	exit /b 4
)

for %%i in (%inx_list%) do (
  
  set inx_stem=%%i
  set inf_filename=%TargetName%--!inx_stem!.inf
  
  call :Echos ToasterStampInf "!inx_stem!.inx" to "!inf_filename!" ...
  
  set execmd=call "%dir_toaster_inf_template%\ToasterStampInf.bat"^
    "%dir_toaster_inf_template%\!inx_stem!.inx"^
    "%TargetDir%\!inf_filename!"^
    %oldword%^
    %newword%^
    "-a %stampinf_ARCH% -k 1.9 -v 1.0.0.1"
  
  call :EchoExec %execmd%
  !execmd!
  
  if errorlevel 1 exit /b 4
)



REM =============================
REM ====== Functions Below ======
REM =============================

:Echos
  echo %_vspgINDENTS%[%batfilenam%] %*
exit /b

:EchoExec
  echo %_vspgINDENTS%[%batfilenam%] EXEC: %*
exit /b
