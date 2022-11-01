#include <windows.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include <stdio.h>
#include <tchar.h>

const TCHAR *SetupApi_ErrText(DWORD winerr);

void Print_SetupApiError(const TCHAR *apiname, DWORD winerr=-1);

const TCHAR *Cfmgr_ErrText(DWORD cmerr);

void Print_CfmgrError(const TCHAR *apiname, DWORD cmerr=-1);
