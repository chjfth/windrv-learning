/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:

    public.h

Abstract:

    This module contains the common declarations shared by driver
    and user applications.


Revision History:
	2020.01 Jimm Chen: Use eye-catching GUIDS {1111...}, {2222....} 

--*/

//
// Define an Interface Guid for bus enumerator class.
// This GUID is used to register (IoRegisterDeviceInterface) 
// an instance of an interface so that enumerator application 
// can send an ioctl to the bus driver.
//
DEFINE_GUID (GUID_DEVINTERFACE_BUSENUM_TOASTER,
        0x1111bb00, 0x6A0C, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
		//{1111bb00-6A0C-11d2-B841-00C04FAD5171}
  
//
// Define an Interface Guid for toaster device class.
// This GUID is used to register (IoRegisterDeviceInterface) 
// an instance of an interface so that user application 
// can control the toaster device.
//

DEFINE_GUID (GUID_DEVINTERFACE_TOASTER, 
		0x11110000, 0x72B2, 0x11d2, 0xB8, 0x52, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
		//{11110000-72B2-11d2-B852-00C04FAD5171}

//
// Define a Setup Class GUID for Toaster Class. This is same
// as the TOASTSER CLASS guid in the INF files.
//
DEFINE_GUID (GUID_DEVCLASS_TOASTER, 
		0x22222222, 0x6A01, 0x11d2, 0xB8, 0x41, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
		//{22222222-6A01-11d2-B841-00C04FAD5171}

//
// Define a WMI GUID to get busenum info. (note: this should match those in .mof)
//
DEFINE_GUID (TOASTER_BUS_WMI_STD_DATA_GUID, 
		0x1111bb01, 0x8F12, 0x11d2, 0xB8, 0x54, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
		//{1111bb01-8F12-11d2-B854-00C04FAD5171}

//
// Define a WMI GUID to get toaster device info. (note: this should match those in .mof)
//
DEFINE_GUID (TOASTER_WMI_STD_DATA_GUID, 
		0x11110001, 0x6DD3, 0x11d2, 0xB8, 0x44, 0x00, 0xC0, 0x4F, 0xAD, 0x51, 0x71);
		//{11110001-6DD3-11d2-B844-00C04FAD5171}

//
// Define a WMI GUID to represent device arrival notification WMIEvent class. (note: this should match those in .mof)
//
DEFINE_GUID (TOASTER_NOTIFY_DEVICE_ARRIVAL_EVENT, 
        0x1cdaff1, 0xc901, 0x45b4, 0xb3, 0x59, 0xb5, 0x54, 0x27, 0x25, 0xe2, 0x9c);
		//{01CDAFF1-C901-45b4-B359-B5542725E29C}


//
// GUID definition are required to be outside of header inclusion pragma to avoid
// error during precompiled headers.
//

#ifndef __PUBLIC_H
#define __PUBLIC_H

#define BUS_HARDWARE_IDS L"{11111111-6A01-11d2-B841-00C04FAD5171}\\MsToaster\0"
#define BUS_HARDWARE_IDS_LENGTH sizeof (BUS_HARDWARE_IDS) // =100
	// Chj: This naming is bad, it is actually toaster child-device's hardware Id.

#define BUSENUM_COMPATIBLE_IDS L"{11111111-6A01-11d2-B841-00C04FAD5171}\\MsCompatibleToaster\0"
#define BUSENUM_COMPATIBLE_IDS_LENGTH sizeof(BUSENUM_COMPATIBLE_IDS)


#define FILE_DEVICE_BUSENUM         FILE_DEVICE_BUS_EXTENDER

#define BUSENUM_IOCTL(_index_) \
    CTL_CODE (FILE_DEVICE_BUSENUM, _index_, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_BUSENUM_PLUGIN_HARDWARE               BUSENUM_IOCTL (0x0) // vbus
#define IOCTL_BUSENUM_UNPLUG_HARDWARE               BUSENUM_IOCTL (0x1) // vbus
#define IOCTL_BUSENUM_EJECT_HARDWARE                BUSENUM_IOCTL (0x2) // vbus
#define IOCTL_TOASTER_DONT_DISPLAY_IN_UI_DEVICE     BUSENUM_IOCTL (0x3) // toaster
#define IOCTL_TOASTER_WANT_DISPLAY_IN_UI_DEVICE     BUSENUM_IOCTL (0x4) // toaster
#define IOCTL_BUSENUM_WAKE_UP_CHILD                 BUSENUM_IOCTL (0x7) // Chj added, for Vbus

//
//  Data structure used in PlugIn and UnPlug ioctls
//
#pragma warning(disable:4200)
typedef struct _BUSENUM_PLUGIN_HARDWARE
{
    // sizeof(struct _BUSENUM_HARDWARE) = 8 , confirmed on VS2010
    //
    __in ULONG Size;                          
    
    // Unique serial number of the device to be enumerated.
    // Enumeration will be failed if another device on the 
    // bus has the same serial number.
	//
    __in ULONG SerialNo;
    
    // An array of (zero terminated wide character strings). The array itself
    //  also null terminated (ie, MULTI_SZ)
    //    
    __in  WCHAR   HardwareIDs[]; 
                                                                        
} BUSENUM_PLUGIN_HARDWARE, *PBUSENUM_PLUGIN_HARDWARE;

#pragma warning(default:4200)
typedef struct _BUSENUM_UNPLUG_HARDWARE
{
    // sizeof (struct _REMOVE_HARDWARE)
    //
    __in ULONG Size;                                    

    // Serial number of the device to be plugged out    
    //
    ULONG   SerialNo;
    
    ULONG Reserved[2];    

} BUSENUM_UNPLUG_HARDWARE, *PBUSENUM_UNPLUG_HARDWARE;

typedef struct _BUSENUM_EJECT_HARDWARE
{
    // sizeof (struct _EJECT_HARDWARE)
    //
    __in ULONG Size;                                    

    // Serial number of the device to be ejected
    //
    ULONG   SerialNo;
    
    ULONG Reserved[2];    

} BUSENUM_EJECT_HARDWARE, *PBUSENUM_EJECT_HARDWARE;


#endif



