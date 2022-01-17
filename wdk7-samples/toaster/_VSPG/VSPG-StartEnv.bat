@echo off
REM If you have special environment variable(env-var) to set for you own .bat files,
REM you can set it here.

echo Loading Env-vars from "%~f0"

call :SetEnvVar dir_stock_WdfCoInstaller01009=%SolutionDir%\..\..\_share\WdfCoInstaller\1.9

call :SetEnvVar dir_toaster_inf_template=%SolutionDir%\inf-template

if "%PlatformName%"=="x64" (
	
	REM set `stampinf_ARCH` used by stampinf.exe
	set stampinf_ARCH=AMD64
	
	REM set `wdfdll_subdirname` so to know which WdfCoInstaller01009.dll to use
	set wdfdll_subdirname=x64
	
	REM set `inf2cat_osnick` used by inf2cat.exe, for the /os: param .
	set inf2cat_osnick=XP_X64
) else (
	set stampinf_ARCH=x86
	set wdfdll_subdirname=x86
	set inf2cat_osnick=XP_X86
)

exit /b

REM =============================
REM ====== Functions Below ======
REM =============================

:SetEnvVar
  set setcmd=set %*
  echo.  %setcmd%
  %setcmd%
exit /b

