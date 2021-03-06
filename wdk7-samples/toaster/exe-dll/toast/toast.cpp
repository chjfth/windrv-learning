/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    TOAST.C (Chj rename it to toast.cpp)

Abstract:
    Lists all the toaster device and interfaces present in the system
    and opens the last interface to send an invalidate device Ioctl request
    or read requests.

Environment:
    usermode console application

Revision History:
  Eliyas Yakub  Nov 2, 1998
  2022.01 Updated to Unicode version, Jimm Chen.
--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include <initguid.h>
#include <tchar.h>
#include <stdio.h>
#include <string.h>
#include <winioctl.h>
#include "public.h"
#include <conio.h>
// #include <dontuse.h>

#define USAGE  \
	_T("Usage: Toast [-h|-s]\n")\
	_T("      -h|-s : hide|show from Device Manager UI\n")

enum findmethod_st { find_setupclass=0, find_ifaceclass=1 };

BOOL PrintToasterDeviceInfo(findmethod_st);

INT __cdecl
_tmain(
    __in ULONG argc,
    __in_ecount(argc) TCHAR *argv[]
    )
{
    HDEVINFO                            hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA            deviceInterfaceData;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0, bytes=0;
    HANDLE                              file;
    int                                 i, ch;
    char                                buffer[10];
	BOOL                                bDontShowInDevmgmt = FALSE;
	BOOL                                bWantShowInDevmgmt = FALSE;

    if(argc == 2) {
        if(argv[1][0] == _T('-')) {
			char c = argv[1][1];
            if(c==_T('h') || c==_T('H')) {
                bDontShowInDevmgmt = TRUE;
			} else if (c==_T('s') || c==_T('S')) {
				bWantShowInDevmgmt = TRUE;
            } else {
                _tprintf(_T("%s"), USAGE);
                exit(0);
            }
        }
        else {
            _tprintf(_T("%s"), USAGE);
            exit(0);
        }
    }

    //
    // Print a list of devices of Toaster Class
    //
	_tprintf(_T("Try finding toaster devices by device-setup-class...\n"));
    if(!PrintToasterDeviceInfo(find_setupclass))
    {
		_tprintf(_T("Try finding toaster devices by device-interface-class...\n"));

		if(!PrintToasterDeviceInfo(find_ifaceclass))
		{
			_tprintf(_T("No toaster devices present.\n"));
			return 0;
		}
    }

    //
    // Open a handle to the device interface information set of all
    // present toaster class interfaces.
    //

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&GUID_DEVINTERFACE_TOASTER,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
                       (DIGCF_PRESENT | // Only Devices present
                       DIGCF_DEVICEINTERFACE)); // Function class devices.
    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        _tprintf(_T("SetupDiGetClassDevs failed: %x\n"), GetLastError());
        return 0;
    }

    deviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    _tprintf(_T("\nList of Toaster Device Interfaces\n"));
    _tprintf(_T("---------------------------------\n"));

    i = 0;

    //
    // Enumerate devices of toaster class
    //

    for(;;) 
	{
        if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&GUID_DEVINTERFACE_TOASTER,
                                 i, //
                                 &deviceInterfaceData)) 
		{
            if(deviceInterfaceDetailData) {
                free (deviceInterfaceDetailData);
                deviceInterfaceDetailData = NULL;
            }

            //
            // Allocate a function class device data structure to
            // receive the information about this particular device.
            //

            //
            // First find out required length of the buffer
            //

            if(!SetupDiGetDeviceInterfaceDetail (
                    hardwareDeviceInfo,
                    &deviceInterfaceData,
                    NULL, // probing so no output buffer yet
                    0, // probing so output buffer length of zero
                    &requiredLength,
                    NULL)) 
			{ // not interested in the specific dev-node
                if(ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
                    _tprintf(_T("SetupDiGetDeviceInterfaceDetail failed %d\n"), GetLastError());
                    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                    return FALSE;
                }

            }

            predictedLength = requiredLength;

            deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(predictedLength);

            if(deviceInterfaceDetailData) {
                deviceInterfaceDetailData->cbSize = sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
            } else {
                _tprintf(_T("Couldn't allocate %d bytes for device interface details.\n"), predictedLength);
                SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                return FALSE;
            }

            if (! SetupDiGetDeviceInterfaceDetail (
                       hardwareDeviceInfo,
                       &deviceInterfaceData,
                       deviceInterfaceDetailData,
                       predictedLength,
                       &requiredLength,
                       NULL)) 
			{
                _tprintf(_T("Error in SetupDiGetDeviceInterfaceDetail\n"));
                SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
                free (deviceInterfaceDetailData);
                return FALSE;
            }

			// Chj: print out the so-called DevOpenPath for this enumerated devnode.
            _tprintf(_T("(%d) %s\n"), ++i,
                    deviceInterfaceDetailData->DevicePath);
        }
        else if (ERROR_NO_MORE_ITEMS != GetLastError()) 
		{
			// weird error case
            free (deviceInterfaceDetailData);
            deviceInterfaceDetailData = NULL;
            continue;
        }
        else
            break;

    }

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);

    if(!deviceInterfaceDetailData)
    {
        _tprintf(_T("No device interfaces present\n"));
        return 0;
    }

    //
    // Open the last toaster device interface
    //
    _tprintf(_T("\nOpening the last-enumerated interface:\n %s\n"),
                    deviceInterfaceDetailData->DevicePath);

    file = CreateFile ( deviceInterfaceDetailData->DevicePath,
                        GENERIC_READ | GENERIC_WRITE,
                        0,
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL);

    if (INVALID_HANDLE_VALUE == file) {
        _tprintf(_T("Error in CreateFile: %x"), GetLastError());
        free (deviceInterfaceDetailData);
        return 4;
    }

    //
    // Invalidate the Device State
    //
    if(bDontShowInDevmgmt || bWantShowInDevmgmt)
    {
        if (!DeviceIoControl (file,
                bDontShowInDevmgmt ? IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE : IOCTL_TOASTER_WANT_DISPLAY_IN_UI_DEVICE,
                NULL, 0,
                NULL, 0,
                &bytes, NULL)) {
            _tprintf(_T("Invalidate device request failed:0x%x\n"), GetLastError());
            free (deviceInterfaceDetailData);
            CloseHandle(file);
            return 4;
        }
        _tprintf(_T("\nRequest to hide the device completed successfully\n"));
    }

    //
    // Read/Write to the toaster device.
    //
    _tprintf(_T("\nPress 'q' to exit, any other key to read...\n"));
    fflush(stdin);
    ch = _getche();

    while(tolower(ch) != 'q' )
    {

        if(!ReadFile(file, buffer, sizeof(buffer), &bytes, NULL))
        {
            _tprintf(_T("Error in ReadFile: %x"), GetLastError());
            break;
        }
        _tprintf(_T("Read Successful\n"));
        ch = _getche();
    }

    free (deviceInterfaceDetailData);
    CloseHandle(file);
    return 0;
}


BOOL
PrintToasterDeviceInfo(findmethod_st fm)
{
    HDEVINFO        hdi;
    DWORD           dwIndex=0;
    SP_DEVINFO_DATA deid;
    BOOL            fSuccess=FALSE;
	TCHAR           szCompInstanceId[MAX_PATH] = {};
	TCHAR           szCompDescription[MAX_PATH] = {};
	TCHAR           szFriendlyName[MAX_PATH] = {};
    DWORD           dwRegType;
    BOOL            fFound=FALSE;

    // get a list of all devices of class 'GUID_DEVCLASS_TOASTER'
	if(fm==find_setupclass)
		hdi = SetupDiGetClassDevs(&GUID_DEVCLASS_TOASTER, NULL, NULL, DIGCF_PRESENT);
	else
		hdi = SetupDiGetClassDevs(&GUID_DEVINTERFACE_TOASTER, NULL, NULL, DIGCF_DEVICEINTERFACE|DIGCF_PRESENT);

    if (INVALID_HANDLE_VALUE != hdi)
    {

        // enumerate over each device
        while (deid.cbSize = sizeof(SP_DEVINFO_DATA),
               SetupDiEnumDeviceInfo(hdi, dwIndex, &deid))
        {
            dwIndex++;

            // the right thing to do here would be to call this function
            // to get the size required to hold the instance ID and then
            // to call it second time with a buffer large enough for that size.
            // However, that would tend to obscure the control flow in
            // the sample code. Lets keep things simple by keeping the
            // buffer large enough.

            // get the device instance ID
            fSuccess = SetupDiGetDeviceInstanceId(hdi, &deid,
                                                  szCompInstanceId,
                                                  ARRAYSIZE(szCompInstanceId), 
												  NULL);
            if (fSuccess)
            {
                // get the description for this instance
                fSuccess = SetupDiGetDeviceRegistryProperty(hdi, &deid,
                                                     SPDRP_DEVICEDESC,
                                                     &dwRegType,
                                                     (BYTE*) szCompDescription,
                                                     sizeof(szCompDescription),
                                                     NULL);
                if (fSuccess)
                {
                    memset(szFriendlyName, 0, MAX_PATH);
                    SetupDiGetDeviceRegistryProperty(hdi, &deid,
                                                     SPDRP_FRIENDLYNAME,
                                                     &dwRegType,
                                                     (BYTE*) szFriendlyName,
                                                     sizeof(szFriendlyName),
                                                     NULL);
                    fFound = TRUE;
                    _tprintf(_T("Instance ID : %s\n"), szCompInstanceId);
                    _tprintf(_T("Description : %s\n"), szCompDescription);
                    _tprintf(_T("FriendlyName: %s\n\n"), szFriendlyName);
                }
            }
        }

        // release the device info list
        SetupDiDestroyDeviceInfoList(hdi);
    }

    if(fFound)
        return TRUE;
    else
        return FALSE;
}


