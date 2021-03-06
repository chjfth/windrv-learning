setlocal EnableDelayedExpansion
set batfilenam=%~n0%~x0
set batdir=%~dp0
set batdir=%batdir:~0,-1%
set _vspgINDENTS=%_vspgINDENTS%.
call :Echos START from %batdir%

: This is a function.
: Converts a .inx to .inf by calling stampinf, and at the same time, replace a word in the output inf.
:
: Param1: Input inx filepath
: Param2: Output inf filepath
: Param3: Old word to find (must not contain space char)
: Param4: Replace with this new word (must not contain space char)
: Param5: Stampinf extra params, like "-a AMD64 -k 1.9 -v *"
: 
: Issue: Whether filepath/filename is space tolerant, not verified.

set InputInx=%~1
set OutputInf=%~2
set Oldword=%~3
set Newword=%~4
set StampinfExParams=%~5

if "%StampinfExParams%"=="" (
	echo ERROR: %0 missing parameters.
	exit /b 4
)

call :Echos For inf-file, will replace "%Oldword%" to "%Newword%" .

REM call :EchoExec "%bootsdir%\ReplaceInFile.bat" "%Oldword%" "%Newword%" "%InputInx%" "%OutputInf%"
call "%bootsdir%\ReplaceInFile.bat" "%Oldword%" "%Newword%" "%InputInx%" "%OutputInf%"
if errorlevel 1 exit /b 4


if not exist "%OutputInf%" (
	echo ERROR: %0 Cannot generate intermediate inx-file: %tempFilepath%
	exit /b 4
)

REM Now %OutputInf% is still an inx, we have to call stampinf on it.

set execmd=stampinf -f "%OutputInf%" %StampinfExParams%
call :EchoExec %execmd%

%execmd%
if errorlevel 1 (
	echo ERROR: Stampinf execution fail.
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
