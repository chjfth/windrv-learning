// InterfaceEnum.cpp: Registered interface enumeration sample
// Copyright (C) 2000 by Walter Oney
// All rights reserved
//
// Update quite a lot by Jimm Chen, 2016, 2022.

#include "stdafx.h"

BOOL GuidFromString(GUID* guid, LPCTSTR string);
const TCHAR * StringFromGuid(const GUID &guid);
BOOL htol(LPCTSTR& string, PDWORD presult);
BOOL htos(LPCTSTR& string, PWORD presult);
BOOL htob(LPCTSTR& string, PBYTE presult);
LPCTSTR InterfaceGuidName(GUID* guid);

///////////////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[])
{
	(void)argc; (void)argv;

	setlocale(LC_ALL, "");
	printf("Program compile date: %s (version 2.0)\n", __DATE__); // This must be 'char'
	
	// Registered device interfaces have persistent registry keys below
	// HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\DeviceClasses.
	// Enumerate those keys to learn which device interfaces have been
	// registered by any device.
	
	HKEY hkey;
	DWORD code = RegOpenKey(HKEY_LOCAL_MACHINE, REGSTR_PATH_DEVICE_CLASSES, &hkey);
	if (code)
		return code;
	
	TCHAR keyname[256];
	for (DWORD keyindex = 0; 
		RegEnumKey(hkey, keyindex, keyname, ARRAYSIZE(keyname)) == NO_ERROR; 
		++keyindex)
	{
		// for each registered interface ...
		
		// Convert the key name to a GUID. Seems like there ought to be an SDK
		// function that would do this without needing a UNICODE string first...
		
		GUID IfcguidToQuery; // Interface GUID to query for matching devnodes
		if (!GuidFromString(&IfcguidToQuery, keyname))
			continue;			// can't convert key to GUID

		// Print the header for a section of interface-GUID reports
		
		_tprintf(_T("\nIFC<%d>%s (%s):\n"), keyindex, keyname, InterfaceGuidName(&IfcguidToQuery));
		
		// Acquire a device information set to enumerate instances of this interface
		
		HDEVINFO infoset = SetupDiGetClassDevs(&IfcguidToQuery, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
		if (infoset == INVALID_HANDLE_VALUE)
		{
			// Chj Win7 memo: With an arbitrary IfcguidToQuery value, infoset will always be valid,
			// except that later SetupDiEnumDeviceInterfaces() will report nothing.
			// So not likely to get here.

			_tprintf(_T("    (Unexpected error from SetupDiGetClassDevs(), WinErr=%d)\n"), GetLastError());
			continue;
		}
		
		// Report information about these devices
		
		DWORD devindex = 0;
		SP_DEVICE_INTERFACE_DATA Difd = {sizeof(SP_DEVICE_INTERFACE_DATA)};
		for(devindex=0;; ++devindex) // for each device
		{
			// Note: using NULL in third param(&IfcguidToQuery) will fail with @err=87(ERROR_INVALID_PARAMETER)
			//
			BOOL b = SetupDiEnumDeviceInterfaces(infoset, 
				NULL, // IN, we what interface-data for EVERY device in infoset 
				&IfcguidToQuery, // IN,
				devindex, // IN
				&Difd // OUT: 
					// .InterfaceClassGuid(=input IfcguidToQuery, silly API)
					// .Flags (don't care here)
					// .Reserved. Chj: This is a CRITICAL internal ptr that determines what we can fetch from SetupDiGetDeviceInterfaceDetail().
				);
			if(b)
			{
				assert(memcmp(&IfcguidToQuery, &Difd.InterfaceClassGuid, sizeof(GUID))==0);
			}

			if(!b)
				break; // enum end

			_tprintf(_T("    <%d>devobj:\n"), devindex);

			// Obtain MORE information about the device. (calling it "detail" is again silly)
			//
			SP_DEVINFO_DATA Did = {sizeof(SP_DEVINFO_DATA)};
			TCHAR dev__Openpath[4000];
			DWORD reqout = 0;
			SP_DEVICE_INTERFACE_DETAIL_DATA *pDevIfcDetail = (SP_DEVICE_INTERFACE_DETAIL_DATA*)dev__Openpath;
			pDevIfcDetail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);
			b = SetupDiGetDeviceInterfaceDetail(infoset,
				&Difd, // IN
				pDevIfcDetail, // OUT
				ARRAYSIZE(dev__Openpath), 
				&reqout, 
				&Did // OUT: We need this Did as later API input.
				); // memo: ERROR_INSUFFICIENT_BUFFER if dev__Openpath not enough
			if(!b) {
				DWORD winerr = GetLastError();
				_tprintf(_T("    Unexpected: SetupDiGetDeviceInterfaceDetail() fails, winerr=%d!\n"), winerr);
				continue;		// unexpected result
			}
			
			// Chj: The above SetupDiGetDeviceInterfaceDetail()'s main purpose is to have Did
			// filled with a piece of opaque data which will act as an input-param to
			// later SetupDiGetDeviceRegistryProperty() call.
			// The Did is filled even if @err is ERROR_INSUFFICIENT_BUFFER - MSDN/run confirmed.
			//
			// Almost every scene of using HDEVINFO requires an accompanied SP_DEVINFO_DATA, 
			// very sluggish API design!

			// show user the devpath that can be used with CreateFile().
			_tprintf(_T("      DevOpenPath: \"%s\"\n"), pDevIfcDetail->DevicePath);

			// Print "device instance path"
			TCHAR szDevinstpath[512];
			b = SetupDiGetDeviceInstanceId(infoset, &Did, szDevinstpath, ARRAYSIZE(szDevinstpath), NULL);
			if(b) {
				_tprintf(_T("      Device-instance-path: %s\n"), szDevinstpath);
			}

			// Determine the friendly name parameter (if any) for the interface instance. This is useful
			// when displaying kernel-streaming interface information
	
			TCHAR szSetupClassGuid[80];
			_tcscpy_s(szSetupClassGuid, StringFromGuid(Did.ClassGuid));
			_tprintf(_T("      SP_DEVINFO_DATA.ClassGuid(setup-class)=%s\n"), szSetupClassGuid);

			TCHAR interfacename[512] = {0};
			HKEY interfacekey = SetupDiOpenDeviceInterfaceRegKey(infoset, &Difd, 0, KEY_READ);
				// Chj: Using Process Explorer, I can know the opened regkey path is sth like:
				//	HKLM\SYSTEM\ControlSet001\Control\DeviceClasses\{07dad660-22f1-11d1-a9f4-00c04fbbde8f}\##?#Root#SYSTEM#0000#{07dad660-22f1-11d1-a9f4-00c04fbbde8f}\#{07dad662-22f1-11d1-a9f4-00c04fbbde8f}&GLOBAL\Device Parameters

			// Print the friendly name of this "device interface".

			_tprintf(_T("      Interface friendly name: "));
			if(interfacekey!=INVALID_HANDLE_VALUE)
			{	// look for friendly name of the interface
				DWORD size = sizeof(interfacename);
				RegQueryValueEx(interfacekey, _T("FriendlyName"), 0, NULL, (BYTE*)interfacename, &size);
				if (interfacename[0])
					_tprintf(_T("\"%s\"\n"), interfacename);
				else
					_tprintf(_T("(no \"FriendlyName\" item in registry\n"));
				
				RegCloseKey(interfacekey);
					// Chj: put RegCloseKey() at end, so that when we break at either _tprintf() above,
					// we can still check into Process Explorer to know which regkey is being opened.
			}
			else 
			{
				_tprintf(_T("(no interface-regkey for this device)\n"));
			}

			// Determine and print the friendly name or description of this "device instance".
			TCHAR szFriendly[512]={0}, szDevdesc[512]={0};
			BOOL b1 = SetupDiGetDeviceRegistryProperty(infoset, &Did, SPDRP_FRIENDLYNAME, NULL, (BYTE*)szFriendly, sizeof(szFriendly), NULL);
			BOOL b2 = SetupDiGetDeviceRegistryProperty(infoset, &Did, SPDRP_DEVICEDESC, NULL, (BYTE*)szDevdesc, sizeof(szDevdesc), NULL);
			if(b1) {
				_tprintf(_T("      SPDRP_FRIENDLYNAME: %s\n"), szFriendly);
			}
			if(b2) {
				_tprintf(_T("      SPDRP_DEVICEDESC  : %s\n"), szDevdesc);
			}

			// Print more registry properties of this "device instance".
			TCHAR szValue[512] = {0};

			b = SetupDiGetDeviceRegistryProperty(infoset, &Did, SPDRP_SERVICE, NULL, (BYTE*)szValue, sizeof(szValue), NULL);
			if(b) {
				_tprintf(_T("      SPDRP_SERVICE: %s\n"), szValue);
			}

			b = SetupDiGetDeviceRegistryProperty(infoset, &Did, SPDRP_CLASSGUID, NULL, (BYTE*)szValue, sizeof(szValue), NULL);
			if(b) {
				//printf("      SPDRP_CLASSGUID(setup class): %s\n", szValue);
				int diff = _stricmp((char*)szValue, (char*)szSetupClassGuid);
				if(diff) {
					_ftprintf(stderr, _T("ASSERT ERROR: SPDRP_CLASSGUID registry-property and szSetupClassGuid mismatch.\n"));
					exit(1);
				}
			}

		}					// for each device
		
		SetupDiDestroyDeviceInfoList(infoset);
		
		if (devindex == 0)
		{
			// Chj: Registry has this Ifcguid, but SetupDiEnumDeviceInterfaces() reports none, sth weird.
			_tprintf(_T("    (No devices)\n"));
			continue;
		}
	}						// for each registered interface
	
	RegCloseKey(hkey);
	
	return 0;
}							// main

///////////////////////////////////////////////////////////////////////////////
// GuidFromString converts a string of the form {c200e360-38c5-11ce-ae62-08002b2b79ef}
// into a GUID structure. It return TRUE if the conversion was successful

BOOL GuidFromString(GUID* guid, LPCTSTR string)
{							// GuidFromString
	return *string++ == _T('{')
		&& htol(string, &guid->Data1)
		&& *string++ == _T('-')
		&& htos(string, &guid->Data2)
		&& *string++ == _T('-')
		&& htos(string, &guid->Data3)
		&& *string++ == _T('-')
		&& htob(string, &guid->Data4[0])
		&& htob(string, &guid->Data4[1])
		&& *string++ == _T('-')
		&& htob(string, &guid->Data4[2])
		&& htob(string, &guid->Data4[3])
		&& htob(string, &guid->Data4[4])
		&& htob(string, &guid->Data4[5])
		&& htob(string, &guid->Data4[6])
		&& htob(string, &guid->Data4[7])
		&& *string++ == _T('}')
		&& *string == 0;
}							// GuidFromString

const TCHAR * StringFromGuid(const GUID &guid) 
{
	static TCHAR buf[100] = {0};
	_sntprintf_s(buf, ARRAYSIZE(buf),
		_T("{%08lx-%04hx-%04hx-%02hhx%02hhx-%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx}"), 
		guid.Data1, guid.Data2, guid.Data3, 
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
	return buf;
}


///////////////////////////////////////////////////////////////////////////////
// Hexadecimal conversion routines for use by GuidFromString

BOOL htol(LPCTSTR& string, PDWORD presult)
{							// htol
	DWORD result = 0;
	for (int i = 0; i < 8; ++i)
	{						// convert hex digits
		TCHAR ch = *string++;
		BYTE hexit;
		if (ch >= _T('0') && ch <= _T('9'))
			hexit = ch - _T('0');
		else if (ch >= _T('A') && ch <= _T('F'))
			hexit = ch - (_T('A') - 10);
		else if (ch >= _T('a') && ch <= _T('f'))
			hexit = ch - (_T('a') - 10);
		else
			return FALSE;
		
		result <<= 4;
		result |= hexit;
	}						// convert hex digits
	
	*presult = result;
	return TRUE;
}							// htol

BOOL htos(LPCTSTR& string, PWORD presult)
{							// htos
	WORD result = 0;
	for (int i = 0; i < 4; ++i)
	{						// convert hex digits
		TCHAR ch = *string++;
		BYTE hexit;
		if (ch >= _T('0') && ch <= _T('9'))
			hexit = ch - _T('0');
		else if (ch >= _T('A') && ch <= _T('F'))
			hexit = ch - (_T('A') - 10);
		else if (ch >= _T('a') && ch <= _T('f'))
			hexit = ch - (_T('a') - 10);
		else
			return FALSE;
		
		result <<= 4;
		result |= hexit;
	}						// convert hex digits
	
	*presult = result;
	return TRUE;
}							// htos

BOOL htob(LPCTSTR& string, PBYTE presult)
{							// htob
	BYTE result = 0;
	for (int i = 0; i < 2; ++i)
	{						// convert hex digits
		TCHAR ch = *string++;
		BYTE hexit;
		if (ch >= _T('0') && ch <= _T('9'))
			hexit = ch - _T('0');
		else if (ch >= _T('A') && ch <= _T('F'))
			hexit = ch - (_T('A') - 10);
		else if (ch >= _T('a') && ch <= _T('f'))
			hexit = ch - (_T('a') - 10);
		else
			return FALSE;
		
		result <<= 4;
		result |= hexit;
	}						// convert hex digits
	
	*presult = result;
	return TRUE;
}							// htob

///////////////////////////////////////////////////////////////////////////////
// InterfaceGuidName returns a symbolic name for an interface GUID

LPCTSTR InterfaceGuidName(GUID* guid)
{							// InterfaceGuidName
	
	// Define a table of standard interface guids. This first redefinition of
	// DEFINE_GUID is the standard one followed by a semicolon.
	
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	GUID name = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } } ;
	
#include "StandardInterfaces.h"
	
	// Now define a lookup table for them. This second redefinition of
	// DEFINE_GUID generates the table
	
#undef DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) {&name, TEXT(#name)},
	
	struct {GUID* guid; LPCTSTR name;} guidname[] = {
#include "StandardInterfaces.h"
	};
	
	for (int i = 0; i < ARRAYSIZE(guidname); ++i)
		if (*guidname[i].guid == *guid)
			return guidname[i].name;
		
		return _T("Unknown Interface GUID");
}							// InterfaceGuidName
