/*++
Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:
    Toaster.h

Abstract:
    Header file for the toaster driver modules.

Environment:
    Kernel mode
--*/


#if !defined(_TOASTER_H_)
#define _TOASTER_H_

#ifdef __cplusplus
extern"C"{
#endif

#include <VisualDDKHelpers.h>

#include <ntddk.h>
#include <wdf.h>
#include <usbdi.h>
#include <wdfusb.h> // test

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include <wmilib.h>
#include <initguid.h>
#include "driver.h"
#include "public.h"

#define TOASTER_POOL_TAG (ULONG) 'saoT'

#define MOFRESOURCENAME L"ToasterWMI"

//
// The device extension for the device object
//
typedef struct _FDO_DATA
{
    WDFWMIINSTANCE WmiDeviceArrivalEvent;

    BOOLEAN     WmiPowerDeviceEnableRegistered;

    TOASTER_INTERFACE_STANDARD BusInterface;
	//
	// Chj: 虽然此成员名字叫 "Bus"Interface, 但并不是对应 GUID_DEVINTERFACE_BUSENUM_TOASTER,
	// 而是对应 GUID_DEVINTERFACE_TOASTER. 此处的 "Bus" 字眼应理解为: 由 bus driver 所创建
	// 的 PDO 所提供的 interface , 而非针对 bus-devnode 的接口. 

}  FDO_DATA, *PFDO_DATA;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FDO_DATA, ToasterFdoGetData)


//
// Connector Types
//

#define TOASTER_WMI_STD_I8042 0
#define TOASTER_WMI_STD_SERIAL 1
#define TOASTER_WMI_STD_PARALEL 2
#define TOASTER_WMI_STD_USB 3

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD ToasterEvtDeviceAdd;

EVT_WDF_DEVICE_CONTEXT_CLEANUP ToasterEvtDeviceContextCleanup;
//VOID ToasterEvtDeviceContextCleanup(WDFDEVICE Device);


EVT_WDF_DEVICE_D0_ENTRY ToasterEvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT ToasterEvtDeviceD0Exit;
EVT_WDF_DEVICE_PREPARE_HARDWARE ToasterEvtDevicePrepareHardware;
EVT_WDF_DEVICE_RELEASE_HARDWARE ToasterEvtDeviceReleaseHardware;

EVT_WDF_DEVICE_SELF_MANAGED_IO_INIT ToasterEvtDeviceSelfManagedIoInit;

//
// Io events callbacks.
//
EVT_WDF_IO_QUEUE_IO_READ ToasterEvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE ToasterEvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL ToasterEvtIoDeviceControl;
EVT_WDF_DEVICE_FILE_CREATE ToasterEvtDeviceFileCreate;
EVT_WDF_FILE_CLOSE ToasterEvtFileClose;

NTSTATUS
ToasterWmiRegistration(
    __in WDFDEVICE Device
    );

//
// Power events callbacks
//
EVT_WDF_DEVICE_ARM_WAKE_FROM_S0 ToasterEvtDeviceArmWakeFromS0;
EVT_WDF_DEVICE_ARM_WAKE_FROM_SX ToasterEvtDeviceArmWakeFromSx;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_S0 ToasterEvtDeviceDisarmWakeFromS0;
EVT_WDF_DEVICE_DISARM_WAKE_FROM_SX ToasterEvtDeviceDisarmWakeFromSx;
EVT_WDF_DEVICE_WAKE_FROM_S0_TRIGGERED ToasterEvtDeviceWakeFromS0Triggered;
EVT_WDF_DEVICE_WAKE_FROM_SX_TRIGGERED ToasterEvtDeviceWakeFromSxTriggered;

PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    );

//
// WMI event callbacks
//
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceStdDeviceDataQueryInstance;
EVT_WDF_WMI_INSTANCE_QUERY_INSTANCE EvtWmiInstanceToasterControlQueryInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceStdDeviceDataSetInstance;
EVT_WDF_WMI_INSTANCE_SET_INSTANCE EvtWmiInstanceToasterControlSetInstance;
EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceToasterControlSetItem;
EVT_WDF_WMI_INSTANCE_SET_ITEM EvtWmiInstanceStdDeviceDataSetItem;
EVT_WDF_WMI_INSTANCE_EXECUTE_METHOD EvtWmiInstanceToasterControlExecuteMethod;

NTSTATUS
ToasterFireArrivalEvent(
    __in WDFDEVICE  Device
    );


extern ULONG DebugLevel;


#ifdef __cplusplus
} // extern"C"{
#endif

#endif  // _TOASTER_H_

