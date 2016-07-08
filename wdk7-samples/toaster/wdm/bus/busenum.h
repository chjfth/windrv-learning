/*++

Copyright (c) 1990-2000 Microsoft Corporation All Rights Reserved

Module Name:

    BUSENUM.H

Abstract:

    This module contains the common private declarations
    for the Toaster Bus enumerator.

Author:

Environment:

    kernel mode only

Notes:


Revision History:


--*/
#include <VisualDDKHelpers.h>
#include <ntddk.h>
#include <wmilib.h> // required for WMILIB_CONTEXT
#include <initguid.h> // required for GUID definitions
#include <ntintsafe.h>
#include "driver.h"
#include "public.h"

//
// Let us use newly introduced (Windows Server 2003 DDK) safe string function to avoid
// security issues related buffer overrun.
// The advantages of the RtlStrsafe functions include:
// 1) The size of the destination buffer is always provided to the
// function to ensure that the function does not write past the end of
// the buffer.
// 2) Buffers are guaranteed to be null-terminated, even if the
// operation truncates the intended result.
//

//
// In this driver we are using a safe version swprintf, which is
// RtlStringCchPrintfW. To use strsafe function on 9x, ME, and Win2K Oses, we
// have to define NTSTRSAFE_LIB before including this header file and explicitly
// linking to ntstrsafe.lib. If your driver is just targeted for XP and above, there is
// no need to define NTSTRSAFE_LIB and link to the lib.
//
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include <dontuse.h>

#ifndef BUSENUM_H
#define BUSENUM_H

#define BUSENUM_POOL_TAG (ULONG) 'suBT'

//
// Debugging Output Levels
//

#define BUS_DBG_ALWAYS                  0x00000000

#define BUS_DBG_STARTUP_SHUTDOWN_MASK   0x0000000F
#define BUS_DBG_SS_NOISE                0x00000001
#define BUS_DBG_SS_TRACE                0x00000002
#define BUS_DBG_SS_INFO                 0x00000004
#define BUS_DBG_SS_ERROR                0x00000008

#define BUS_DBG_PNP_MASK                0x000000F0
#define BUS_DBG_PNP_NOISE               0x00000010
#define BUS_DBG_PNP_TRACE               0x00000020
#define BUS_DBG_PNP_INFO                0x00000040
#define BUS_DBG_PNP_ERROR               0x00000080

#define BUS_DBG_IOCTL_MASK              0x00000F00
#define BUS_DBG_IOCTL_NOISE             0x00000100
#define BUS_DBG_IOCTL_TRACE             0x00000200
#define BUS_DBG_IOCTL_INFO              0x00000400
#define BUS_DBG_IOCTL_ERROR             0x00000800

#define BUS_DBG_POWER_MASK              0x0000F000
#define BUS_DBG_POWER_NOISE             0x00001000
#define BUS_DBG_POWER_TRACE             0x00002000
#define BUS_DBG_POWER_INFO              0x00004000
#define BUS_DBG_POWER_ERROR             0x00008000

#define BUS_DBG_WMI_MASK                0x000F0000
#define BUS_DBG_WMI_NOISE               0x00010000
#define BUS_DBG_WMI_TRACE               0x00020000
#define BUS_DBG_WMI_INFO                0x00040000
#define BUS_DBG_WMI_ERROR               0x00080000

extern int g_seq;

#if DBG
#define BUS_DEFAULT_DEBUG_OUTPUT_LEVEL 0x000FFFFF

#define Bus_KdPrint(_d_,_l_, _x_) \
            if (!(_l_) | (_d_)->DebugLevel & (_l_)) { \
               DbgPrint ("[%d]BusEnum.SYS: ", g_seq++); \
               DbgPrint _x_; \
            }

#define Bus_KdPrint_Cont(_d_,_l_, _x_) \
            if (!(_l_) | (_d_)->DebugLevel & (_l_)) { \
               DbgPrint _x_; \
            }

#define Bus_KdPrint_Def(_l_, _x_) \
            if (!(_l_) | BusEnumDebugLevel & (_l_)) { \
               DbgPrint ("[%d]BusEnum.SYS: ", g_seq++); \
               DbgPrint _x_; \
            }

#define DbgRaiseIrql(_x_,_y_) KeRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_) KeLowerIrql(_x_)
#else

#define BUS_DEFAULT_DEBUG_OUTPUT_LEVEL 0x0
#define Bus_KdPrint(_d_, _l_, _x_)
#define Bus_KdPrint_Cont(_d_, _l_, _x_)
#define Bus_KdPrint_Def(_l_, _x_)
#define DbgRaiseIrql(_x_,_y_)
#define DbgLowerIrql(_x_)

#endif

extern ULONG BusEnumDebugLevel;

//
// These are the states a PDO or FDO transition upon
// receiving a specific PnP Irp. Refer to the PnP Device States
// diagram in DDK documentation for better understanding.
//

typedef enum _DEVICE_PNP_STATE_et {

    NotStarted = 0,         // Not started yet
    Started,                // Device has received the START_DEVICE IRP
    StopPending,            // Device has received the QUERY_STOP IRP
    Stopped,                // Device has received the STOP_DEVICE IRP
    RemovePending,          // Device has received the QUERY_REMOVE IRP
    SurpriseRemovePending,  // Device has received the SURPRISE_REMOVE IRP
    Deleted,                // Device has received the REMOVE_DEVICE IRP
    UnKnown                 // Unknown state

} DEVICE_PNP_STATE_et;


typedef struct _GLOBALS {

    // Path to the driver's Services Key in the registry
    UNICODE_STRING RegistryPath;

} GLOBALS;


extern GLOBALS Globals;

//
// Structure for reporting data to WMI
//

typedef struct _TOASTER_BUS_WMI_STD_DATA {

    // The error Count
    //
    UINT32   ErrorCount;

    // Debug Print Level
    //
    UINT32  DebugPrintLevel;

} TOASTER_BUS_WMI_STD_DATA, * PTOASTER_BUS_WMI_STD_DATA;


//
// A common header for the device extensions of the PDOs and FDO
//
typedef struct _COMMON_DEVICE_DATA
{
    // A back pointer to the device object for which this is the extension
    PDEVICE_OBJECT  Self;

    // This flag helps distinguish between PDO and FDO
    BOOLEAN         IsFDO;

    // We track the state of the device with every PnP Irp
    // that affects the device through these two variables.
	//
    DEVICE_PNP_STATE_et DevicePnPState;
    DEVICE_PNP_STATE_et PreviousPnPState;

    ULONG           DebugLevel;

    // Stores the current system power state
    SYSTEM_POWER_STATE  SystemPowerState;

    // Stores current device power state
    DEVICE_POWER_STATE  DevicePowerState;

} COMMON_DEVICE_DATA, *PCOMMON_DEVICE_DATA;

//
// The device extension for the PDOs.
// That's of the toaster device which this bus driver enumerates.
//

typedef struct _PDO_DEVICE_DATA
{
    #pragma warning(suppress:4201)
    COMMON_DEVICE_DATA;

    // A back pointer to the bus
    PDEVICE_OBJECT  ParentFdo;

    // An array of (zero terminated wide character strings).
    // The array itself also null terminated
    __drv_aliasesMem
    PWCHAR      HardwareIDs;

    // Unique serial number of the device on the bus
    ULONG SerialNo;

    // Link point to hold all the PDOs for a single bus together
    LIST_ENTRY  Link;

    // Present is set to TRUE when the PDO is exposed via PlugIn IOCTL,
    // and set to FALSE when a UnPlug IOCTL is received.
    // We will delete the PDO in IRP_MN_REMOVE only after we have reported
    // to the Plug and Play manager that it's missing.
    //
    BOOLEAN     Present;
    BOOLEAN     ReportedMissing;
    UCHAR       Reserved[2]; // for 4 byte alignment

    // Used to track the interfaces handed out to other drivers.
    // If this value is non-zero, we fail query-remove.
    //
    ULONG       ToasterInterfaceRefCount;

    //
    // In order to reduce the complexity of the driver, I choose not
    // to use any queues for holding IRPs when the system tries to
    // rebalance resources to accommodate a new device, and stops our
    // device temporarily. But in a real world driver this is required.
    // If you hold Irps then you should also support Cancel and
    // Cleanup functions. The function driver demonstrates these techniques.
    //
    // The queue where the incoming requests are held when
    // the device is stopped for resource rebalance.

    //LIST_ENTRY          PendingQueue;

    // The spin lock that protects access to  the queue

    //KSPIN_LOCK          PendingQueueLock;

} PDO_DEVICE_DATA, *PPDO_DEVICE_DATA;


//
// The device extension of the bus itself.  From whence the PDO's are born.
//
typedef struct _FDO_DEVICE_DATA
{
    #pragma warning(suppress:4201)
    COMMON_DEVICE_DATA;

    PDEVICE_OBJECT  UnderlyingPDO;
	//
    // The underlying bus PDO and the actual device object to which our
    // FDO is attached

    PDEVICE_OBJECT  NextLowerDriver;

    // List of PDOs created so far
    LIST_ENTRY      ListOfPDOs;

    // The PDOs currently enumerated.
    ULONG           NumPDOs;

    // A synchronization for access to the device extension.
    FAST_MUTEX      Mutex;

    // The number of IRPs sent from the bus to the underlying device object
    ULONG           OutstandingIO; // Biased to 1

    // On remove device plug & play request we must wait until all outstanding
    // requests have been completed before we can actually delete the device
    // object. This event is when the Outstanding IO count goes to zero
    //
    KEVENT          RemoveEvent;

    // This event is set when the Outstanding IO count goes to 1.
    KEVENT          StopEvent;

    // The name returned from IoRegisterDeviceInterface,
    // which is used as a handle for IoSetDeviceInterfaceState.

    UNICODE_STRING      InterfaceName;

    // WMI Information
    //
    WMILIB_CONTEXT         WmiLibInfo;

    TOASTER_BUS_WMI_STD_DATA   StdToasterBusData;

} FDO_DEVICE_DATA, *PFDO_DEVICE_DATA;

#define ParentFDO_FROM_PDO(pdoData) \
          ((PFDO_DEVICE_DATA) (pdoData)->ParentFdo->DeviceExtension)

#define INITIALIZE_PNP_STATE(_Data_)    \
        (_Data_)->DevicePnPState =  NotStarted;\
        (_Data_)->PreviousPnPState = NotStarted;

#define SET_NEW_PNP_STATE(_Data_, _state_) \
        (_Data_)->PreviousPnPState =  (_Data_)->DevicePnPState;\
        (_Data_)->DevicePnPState = (_state_);

#define RESTORE_PREVIOUS_PNP_STATE(_Data_)   \
        (_Data_)->DevicePnPState =   (_Data_)->PreviousPnPState;\

//
// Prototypes of functions defined in busenum.c
//

DRIVER_INITIALIZE DriverEntry;

__drv_dispatchType(IRP_MJ_CREATE)
__drv_dispatchType(IRP_MJ_CLOSE)
DRIVER_DISPATCH Bus_CreateClose;

__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
DRIVER_DISPATCH Bus_IoCtl;

DRIVER_UNLOAD Bus_DriverUnload;

VOID
Bus_IncIoCount (
    __in PFDO_DEVICE_DATA   Data
    );

VOID
Bus_DecIoCount (
    __in PFDO_DEVICE_DATA   Data
    );

//
// Defined in PNP.C
//

PCHAR
PnPMinorFunctionString (
    UCHAR MinorFunction
);

DRIVER_ADD_DEVICE Bus_AddDevice;

DRIVER_DISPATCH Bus_SendIrpSynchronously;

__drv_dispatchType(IRP_MJ_PNP)
DRIVER_DISPATCH Bus_PnP;

IO_COMPLETION_ROUTINE Bus_CompletionRoutine;

VOID
Bus_InitializePdo (
    __drv_in(__drv_aliasesMem)
    PDEVICE_OBJECT      Pdo,
    PFDO_DEVICE_DATA    FdoData
    );

NTSTATUS
Bus_PlugInDevice (
    PBUSENUM_PLUGIN_HARDWARE   PlugIn,
    ULONG                       PlugInLength,
    PFDO_DEVICE_DATA            DeviceData
    );

NTSTATUS
Bus_UnPlugDevice (
    PBUSENUM_UNPLUG_HARDWARE   UnPlug,
    PFDO_DEVICE_DATA            DeviceData
    );

NTSTATUS
Bus_EjectDevice (
    PBUSENUM_EJECT_HARDWARE     Eject,
    PFDO_DEVICE_DATA            FdoData
    );

void
Bus_RemoveFdo (
    __in PFDO_DEVICE_DATA    FdoData
    );

NTSTATUS
Bus_DestroyPdo (
    PDEVICE_OBJECT      Device,
    PPDO_DEVICE_DATA    PdoData
    );


NTSTATUS
Bus_FDO_PnP (
    __in PDEVICE_OBJECT       DeviceObject,
    __in PIRP                 Irp,
    __in PIO_STACK_LOCATION   IrpStack,
    __in PFDO_DEVICE_DATA     DeviceData
    );


NTSTATUS
Bus_StartFdo (
    __in  PFDO_DEVICE_DATA            FdoData,
    __in  PIRP   Irp );

PCHAR
DbgDeviceIDString(
    BUS_QUERY_ID_TYPE Type
    );

PCHAR
DbgDeviceRelationString(
    __in DEVICE_RELATION_TYPE Type
    );

//
// Defined in Power.c
//

// TODO: JakobL: Notice: Deliberately not added:
NTSTATUS
Bus_FDO_Power (
    PFDO_DEVICE_DATA    FdoData,
    PIRP                Irp
    );

// TODO: JakobL: Notice: Deliberately not added:
NTSTATUS
Bus_PDO_Power (
    PPDO_DEVICE_DATA    PdoData,
    PIRP                Irp
    );

__drv_dispatchType(IRP_MJ_POWER)
DRIVER_DISPATCH Bus_Power;

PCHAR
PowerMinorFunctionString (
    UCHAR MinorFunction
);

PCHAR
DbgSystemPowerString(
    __in SYSTEM_POWER_STATE Type
    );

PCHAR
DbgDevicePowerString(
    __in DEVICE_POWER_STATE Type
    );

//
// Defined in BusPDO.c
//

NTSTATUS
Bus_PDO_PnP (
    __in PDEVICE_OBJECT       DeviceObject,
    __in PIRP                 Irp,
    __in PIO_STACK_LOCATION   IrpStack,
    __in PPDO_DEVICE_DATA     DeviceData
    );

NTSTATUS
Bus_PDO_QueryDeviceCaps(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

NTSTATUS
Bus_PDO_QueryDeviceId(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );


NTSTATUS
Bus_PDO_QueryDeviceText(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

NTSTATUS
Bus_PDO_QueryResources(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

NTSTATUS
Bus_PDO_QueryResourceRequirements(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

NTSTATUS
Bus_PDO_QueryDeviceRelations(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

NTSTATUS
Bus_PDO_QueryBusInformation(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

NTSTATUS
Bus_GetDeviceCapabilities(
    __in  PDEVICE_OBJECT          DeviceObject,
    __out PDEVICE_CAPABILITIES    DeviceCapabilities
    );

NTSTATUS
Bus_PDO_QueryInterface(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp );

BOOLEAN
Bus_GetCrispinessLevel(
    __in   PVOID Context,
    __out  PUCHAR Level
    );
BOOLEAN
Bus_SetCrispinessLevel(
    __in   PVOID Context,
    __in  UCHAR Level
    );
BOOLEAN
Bus_IsSafetyLockEnabled(
    __in PVOID Context
    );
VOID
Bus_InterfaceReference (
   __in PVOID Context
   );
VOID
Bus_InterfaceDereference (
   __in PVOID Context
   );

//
// Defined in WMI.C
//
NTSTATUS
Bus_WmiRegistration(
    __in PFDO_DEVICE_DATA               FdoData
);

NTSTATUS
Bus_WmiDeRegistration (
    __in PFDO_DEVICE_DATA               FdoData
);

__drv_dispatchType(IRP_MJ_SYSTEM_CONTROL)
DRIVER_DISPATCH Bus_SystemControl;

WMI_SET_DATAITEM_CALLBACK Bus_SetWmiDataItem;

WMI_SET_DATABLOCK_CALLBACK Bus_SetWmiDataBlock;

WMI_QUERY_DATABLOCK_CALLBACK Bus_QueryWmiDataBlock;

WMI_QUERY_REGINFO_CALLBACK Bus_QueryWmiRegInfo;

#endif

