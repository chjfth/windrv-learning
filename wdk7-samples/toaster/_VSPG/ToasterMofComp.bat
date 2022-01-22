setlocal EnableDelayedExpansion
set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%
set _vspgINDENTS=%_vspgINDENTS%.
call :Echos START from %batdir%

: This is a function.
: Call MofComp.exe to turn xxx.mof into binary xxx.bmf .
:
: Param1: Input MOF filepath
: Param2: Output BMF filepath
: Remaning params: all passed to mofcomp.exe

set InputPath=%~1
set OutputPath=%~2
set MofcompMoreParams=%~3

if "%InputPath%"=="" (
	call :Echos [ERROR] missing input file parameter^(xxx.mof^).
	exit /b 4
)
if "%OutputPath%"=="" (
	call :Echos [ERROR] missing output file parameter^(xxx.bmf^).
	exit /b 4
)

set execmd=mofcomp %MofcompMoreParams% -B:"%OutputPath%" "%InputPath% "
call :EchoExec %execmd%
%execmd%
if errorlevel 1 (
	call :Echos [ERROR] mofcomp execution fail.
	exit /b 4
)

exit /b 0

REM =============================
REM ====== Functions Below ======
REM =============================

REM %~n0%~x0 is batfilenam
:Echos
  echo %_vspgINDENTS%[%~n0%~x0] %*
exit /b

:EchoExec
  echo %_vspgINDENTS%[%~n0%~x0] EXEC: %*
exit /b
