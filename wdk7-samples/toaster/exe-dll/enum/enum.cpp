/*++
Copyright (c) Microsoft Corporation.  All rights reserved.
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    Enum.c (Chj rename it to enum.cpp, and add Unicode build)

Abstract:
        This application simulates the plugin, unplug or ejection
        of devices.

Environment:
    usermode console application

Revision History:
  Eliyas Yakub  Oct 14, 1998
--*/

#include <basetyps.h>
#include <stdlib.h>
#include <wtypes.h>
#include <setupapi.h>
#include <initguid.h>
#include <stdio.h>
#include <tchar.h>
#include <string.h>
#include <winioctl.h>
#include "public.h"
// #include <dontuse.h> // Chj: this is WDK header

//
// Prototypes
//

BOOLEAN
OpenBusInterface (
    __in       HDEVINFO                    HardwareDeviceInfo,
    __in       PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
    );


#define USAGE  _T("Usage: \n\
Enum [-p SerialNo] Plugs in a device. SerialNo must be greater than zero.\n\
     [-u SerialNo or 0] Unplugs device(s) - specify 0 to unplug all \n\
                        the devices enumerated so far.\n\
     [-e SerialNo or 0] Ejects device(s) - specify 0 to eject all \n\
                        the devices enumerated so far.\n\
     [-w SerialNo]      Wake up a toaster device.\n")

BOOLEAN     bPlugIn, bUnplug, bEject, bWake;
ULONG       SerialNo;

INT __cdecl
_tmain(
    __in ULONG argc,
    __in_ecount(argc) TCHAR *argv[]
    )
{
    HDEVINFO                    hardwareDeviceInfo;
    SP_DEVICE_INTERFACE_DATA    deviceInterfaceData;
	bool err = false;

	BUSENUM_PLUGIN_HARDWARE bh = {0};
	bh.SerialNo = 8;

    bPlugIn = bUnplug = bEject = bWake = FALSE;

    if(argc <3) {
        goto usage;
    }

    if(argv[1][0] == _T('-')) {
        if(tolower(argv[1][1]) == _T('p')) {
            if(argv[2])
                SerialNo = (USHORT)_ttol(argv[2]);
			bPlugIn = TRUE;
        }
        else if(tolower(argv[1][1]) == _T('u')) {
            if(argv[2])
                SerialNo = (ULONG)_ttol(argv[2]);
            bUnplug = TRUE;
        }
        else if(tolower(argv[1][1]) == _T('e')) {
            if(argv[2])
                SerialNo = (ULONG)_ttol(argv[2]);
            bEject = TRUE;
        }
		else if(tolower(argv[1][1]) == _T('w')) {
			if(argv[2])
				SerialNo = (ULONG)_ttol(argv[2]);
			bWake = TRUE;
		}
        else {
            goto usage;
        }
    }
    else
        goto usage;

    if(bPlugIn && 0 == SerialNo)
        goto usage;
    //
    // Open a handle to the device interface information set of all
    // present toaster bus enumerator interfaces.
    //

    hardwareDeviceInfo = SetupDiGetClassDevs (
                       (LPGUID)&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                       NULL, // Define no enumerator (global)
                       NULL, // Define no
						(
						DIGCF_PRESENT | // Only Devices present
						DIGCF_DEVICEINTERFACE // First param is interface-guid
						)
					   ); 

    if(INVALID_HANDLE_VALUE == hardwareDeviceInfo)
    {
        _tprintf(_T("SetupDiGetClassDevs failed: %x\n"), GetLastError());
        return 0;
    }

    deviceInterfaceData.cbSize = sizeof (SP_DEVICE_INTERFACE_DATA);

    if (SetupDiEnumDeviceInterfaces (hardwareDeviceInfo,
                                 0, // No care about specific PDOs
                                 (LPGUID)&GUID_DEVINTERFACE_BUSENUM_TOASTER,
                                 0, //
                                 &deviceInterfaceData))
    {
        BOOLEAN bSuccess = OpenBusInterface(hardwareDeviceInfo, &deviceInterfaceData);
		err = !bSuccess;
    } 
	else if (ERROR_NO_MORE_ITEMS == GetLastError()) 
	{
		err = true;
        _tprintf( _T("Error:Interface GUID_DEVINTERFACE_BUSENUM_TOASTER is not registered\n"));
    }

    SetupDiDestroyDeviceInfoList (hardwareDeviceInfo);
    
	return err ? 4 : 0;

usage:
    _tprintf(USAGE);
    exit(1);
}

BOOLEAN
OpenBusInterface (
    __in       HDEVINFO                    HardwareDeviceInfo,
    __in       PSP_DEVICE_INTERFACE_DATA   DeviceInterfaceData
    )
{
    HANDLE                              file;
    PSP_DEVICE_INTERFACE_DETAIL_DATA    deviceInterfaceDetailData = NULL;
    ULONG                               predictedLength = 0;
    ULONG                               requiredLength = 0;
    ULONG                               bytes;
    BUSENUM_UNPLUG_HARDWARE             unplug;
    BUSENUM_EJECT_HARDWARE              eject;
    PBUSENUM_PLUGIN_HARDWARE            hardware;
    BOOLEAN                             bSuccess;

    //
    // Allocate a function class device data structure to receive the
    // information about this particular device.
    //

    SetupDiGetDeviceInterfaceDetail (
            HardwareDeviceInfo,
            DeviceInterfaceData,
            NULL, // probing so no output buffer yet
            0,    // probing so output buffer length set to zero
            &requiredLength,
            NULL); // not interested in the specific devnode

    if(ERROR_INSUFFICIENT_BUFFER != GetLastError()) {
        _tprintf(_T("Error in SetupDiGetDeviceInterfaceDetail. WinErr=%d\n"), GetLastError());
        return FALSE;
    }

    predictedLength = requiredLength;

    deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(predictedLength);

    if(deviceInterfaceDetailData) {
        deviceInterfaceDetailData->cbSize =
                      sizeof (SP_DEVICE_INTERFACE_DETAIL_DATA);
    } else {
        _tprintf(_T("Couldn't allocate %d bytes for device interface details.\n"), predictedLength);
        return FALSE;
    }


    if (! SetupDiGetDeviceInterfaceDetail (
               HardwareDeviceInfo,
               DeviceInterfaceData,
               deviceInterfaceDetailData,
               predictedLength,
               &requiredLength,
               NULL)) 
	{
        _tprintf(_T("Error in SetupDiGetDeviceInterfaceDetail\n"));
        free (deviceInterfaceDetailData);
        return FALSE;
    }

    _tprintf(_T("Opening %s\n"), deviceInterfaceDetailData->DevicePath);

    file = CreateFile ( deviceInterfaceDetailData->DevicePath,
                        GENERIC_READ, // Only read access
                        0, // FILE_SHARE_READ | FILE_SHARE_WRITE
                        NULL, // no SECURITY_ATTRIBUTES structure
                        OPEN_EXISTING, // No special create flags
                        0, // No special attributes
                        NULL); // No template file

    if (INVALID_HANDLE_VALUE == file) {
        _tprintf(_T("CreateFile failed: 0x%x"), GetLastError());
        free (deviceInterfaceDetailData);
        return FALSE;
    }

    _tprintf(_T("Good. Bus interface opened.\n"));

    //
    // From this point on, we need to jump to the end of the routine for
    // common clean-up.  Keep track of whether we succeeded or failed, so
    // we'll know what to return to the caller.
    //
    bSuccess = FALSE;

    //
    // Enumerate Devices
    //

    if(bPlugIn) 
	{
        _tprintf(_T("SerialNo. of the device to be enumerated: %d\n"), SerialNo);

        bytes = sizeof (BUSENUM_PLUGIN_HARDWARE) + BUS_HARDWARE_IDS_LENGTH;
		hardware = (PBUSENUM_PLUGIN_HARDWARE)malloc(bytes);

        if(hardware) {
            hardware->Size = sizeof (BUSENUM_PLUGIN_HARDWARE);
            hardware->SerialNo = SerialNo;
        } else {
            _tprintf(_T("Couldn't allocate %d bytes for busenum plugin hardware structure.\n"), bytes);
            goto End;
        }

        //
        // Allocate storage for the Device ID
        //

        memcpy (hardware->HardwareIDs,
                BUS_HARDWARE_IDS,
                BUS_HARDWARE_IDS_LENGTH);

        if (!DeviceIoControl (file,
                              IOCTL_BUSENUM_PLUGIN_HARDWARE ,
                              hardware, bytes,
                              NULL, 0,
                              &bytes, NULL)) 
		{
              free (hardware);
              
			  DWORD winerr = GetLastError();
			  _tprintf(_T("PlugIn failed. DeviceIoControl() returns winerr=%d\n"), winerr);
              goto End;
        }

        free (hardware);
    }

    //
    // Removes a device if given the specific Id of the device. Otherwise this
    // ioctls removes all the devices that are enumerated so far.
    //

    if(bUnplug) 
	{
        _tprintf(_T("Unplugging device(s)....\n"));

        unplug.Size = bytes = sizeof (unplug);
        unplug.SerialNo = SerialNo;
        if (!DeviceIoControl (file,
                              IOCTL_BUSENUM_UNPLUG_HARDWARE,
                              &unplug, bytes,
                              NULL, 0,
                              &bytes, NULL)) 
		{
            _tprintf(_T("Unplug failed: 0x%x\n"), GetLastError());
			bSuccess = FALSE;
            goto End;
        }
    }

    //
    // Ejects a device if given the specific Id of the device. Otherwise this
    // ioctls ejects all the devices that are enumerated so far.
    //

    if(bEject)
    {
        _tprintf(_T("Ejecting Device(s)\n"));

        eject.Size = bytes = sizeof (eject);
        eject.SerialNo = SerialNo;
        if (!DeviceIoControl (file,
                              IOCTL_BUSENUM_EJECT_HARDWARE,
                              &eject, bytes,
                              NULL, 0,
                              &bytes, NULL)) 
		{
            _tprintf(_T("Eject failed: 0x%x\n"), GetLastError());
            goto End;
        }
    }

	if(bWake) // Chj added
	{
		_tprintf(_T("Waking device with SerialNo=%d ...\n"), SerialNo);

		BUSENUM_PLUGIN_HARDWARE wakewho;
		wakewho.Size = bytes = sizeof (wakewho);
		wakewho.SerialNo = SerialNo;
		if (!DeviceIoControl (file,
			IOCTL_BUSENUM_WAKE_UP_CHILD,
			&wakewho, bytes,
			NULL, 0,
			&bytes, NULL)) 
		{
			_tprintf(_T("Wake child failed: 0x%x\n"), GetLastError());
			bSuccess = FALSE;
			goto End;
		}
		else
		{
			_tprintf(_T("IOCTL_BUSENUM_WAKE_UP_DEVICE success. 'ToasterEvtDeviceD0Entry <- WdfPowerDeviceD1/D2' should appear from kernel DbgPrint message.\n"));
		}

	}

    _tprintf(_T("Success!!!\n"));
    bSuccess = TRUE;

End:
    CloseHandle(file);
    free (deviceInterfaceDetailData);
    return bSuccess;
}



