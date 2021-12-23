setlocal
: This is a function.
: Converts a .inx to .inf by calling stampinf, and at the same time, replace a word in the output inf.
:
: Param1: Input inx filepath
: Param2: Output inf filepath
: Param3: Old word to find (must not contain space char)
: Param4: Replace with this new word (must not contain space char)
: Param5: Stampinf extra params, like "-a AMD64 -k 1.9 -v 1.0.0.1"
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

call :ReplaceInFile %Oldword% %Newword% %InputInx% %OutputInf% 

if not exist "%OutputInf%" (
	echo ERROR: %0 Cannot generate intermediate inx-file: %tempFilepath%
	exit /b 4
)

REM Now %OutputInf% is still an inx, we have to call stampinf on it.

set execmd=stampinf -f %OutputInf% %StampinfExParams%
%execmd%
if errorlevel 1 (
	echo ERROR: Stampinf execution fail.
	exit /b 4
)

exit /b 0

:ReplaceInFile
REM Thanks to:
REM https://stackoverflow.com/questions/23075953/batch-script-to-find-and-replace-a-string-in-text-file-without-creating-an-extra/23076141
REM https://ss64.com/nt/for_f.html
REM Param1~4 same meaning as the containing bat.
  echo off
  set "search=%1"
  set "replace=%2"
  set "oldfile=%3"
  set "newfile=%4"
  (for /f delims^=^ eol^= %%i in (%oldfile%) do (
    set "line=%%i"
    setlocal enabledelayedexpansion
    set "line=!line:%search%=%replace%!"
    echo !line!
    endlocal
  ))>"%newfile%"
