/*++
Copyright (c) Microsoft Corporation.  All rights reserved.

    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:

    toastmon.c

Abstract: 
          This sample demonstrates how to register PnP event notification
          for an interface class, how to open the target device(toaster child)
          in the callback(due to PnP event notification)
		  and register and respond to device change notification.
          
          To schedule sending Read and Write requests to the target device
          the sample uses a "passive" timer. This feature enables getting 
          a timer callback at PASSIVE_LEVEL without having to create and 
          queue a WDFWORKITEM objects on its own.		  
          
Environment:
    Kernel mode
--*/

#include <VisualDDKHelpers.h>
#include "toastmon.h"
#include <initguid.h>
#include <wdmguid.h>
#include <wmistr.h>

#include "public.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, ToastMon_EvtDeviceAdd)
#pragma alloc_text (PAGE, ToastMon_EvtDeviceContextCleanup)
#pragma alloc_text (PAGE, ToastMon_PnpNotifyInterfaceChange)
#pragma alloc_text (PAGE, ToastMon_EvtIoTargetQueryRemove)
#pragma alloc_text (PAGE, ToastMon_EvtIoTargetRemoveCanceled)
#pragma alloc_text (PAGE, ToastMon_EvtIoTargetRemoveComplete)
#endif


NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
/*++
Routine Description:
    Installable driver initialization entry point.
    This entry point is called directly by the I/O system.
Arguments:
    DriverObject - pointer to the driver object
    RegistryPath - pointer to a unicode string representing the path,
                   to driver-specific key in the registry.
Return Value:
    STATUS_SUCCESS
--*/
{
    NTSTATUS            status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG   config;

    KdPrint(("ToastMon Driver Sample - Driver Framework Edition.\n"));
    KdPrint(("Built %s %s\n", __DATE__, __TIME__));

    //
    // Initialize driver config to control the attributes that
    // are global to the driver. Note that framework by default
    // provides a driver unload routine. If you create any resources
    // in the DriverEntry and want to be cleaned in driver unload,
    // you can override that by manually setting the EvtDriverUnload in the
    // config structure. In general xxx_CONFIG_INIT macros are provided to
    // initialize most commonly used members.
    //

    WDF_DRIVER_CONFIG_INIT(
        &config,
        ToastMon_EvtDeviceAdd
        );
    //
    // Create a framework driver object to represent our driver.
    //
    status = WdfDriverCreate(
        DriverObject,
        RegistryPath,
        WDF_NO_OBJECT_ATTRIBUTES, // Driver Attributes
        &config,          // Driver Config Info
        WDF_NO_HANDLE
        );

    if (!NT_SUCCESS(status)) {
        KdPrint( ("[toastmon]WdfDriverCreate failed with status 0x%x\n", status));
    }

    return status;
}

NTSTATUS
ToastMon_EvtDeviceAdd(
    IN WDFDRIVER Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
/*++
Routine Description:
    ToastMon_EvtDeviceAdd is called by the framework in response to AddDevice
    call from the PnP manager. We create and initialize a device object to
    represent a new instance of toaster device.

Arguments:
    Driver - Handle to a framework driver object created in DriverEntry
    DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:
    NTSTATUS
--*/
{
    WDF_OBJECT_ATTRIBUTES           attributes;
    NTSTATUS                        status = STATUS_SUCCESS;
    WDFDEVICE                       device;
    PDEVICE_EXTENSION               deviceExtension;

/* Test code:
	void *p1=(void*)0x912345678, *p2=Driver;
	KdPrint(( "[%%p]p1=%p , p2=%p\n", p1, p2 ));
	KdPrint(( "[%%X]p1=0x%x , p2=0x%x\n", p1, p2 ));
On Win7 x64, we'll get:
	[%p]p1=0000000912345678 , p2=0000057FEF692B38
	[%X]p1=0x12345678 , p2=0xef692b38
--so a stack element from va_args() is at least 8-bytes.
*/
    KdPrint( ("[toastmon]ToastMon_EvtDeviceAdd routine\n"));
    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    //
    // Specify the size of device extension where we track per device
    // context.
    //
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_EXTENSION);

    attributes.EvtCleanupCallback = ToastMon_EvtDeviceContextCleanup;

    //
    // Create a framework device object.This call will in turn create
    // a WDM deviceobject, attach to the lower stack and set the
    // appropriate flags and attributes.
    //
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint( ("[toastmon]WdfDeviceCreate failed with Status 0x%x\n", status));
        return status;
    }

    //
    // Get the DeviceExtension and initialize it.
    //
    deviceExtension = GetDeviceExtension(device);

    deviceExtension->WdfDevice = device;

    //
    // Create a collection to store information about target devices.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfCollectionCreate(&attributes,
                                &deviceExtension->TargetDeviceCollection);
    if (!NT_SUCCESS(status))
    {
        KdPrint( ("[toastmon]WdfCollectionCreate failed with status 0x%x\n", status));
        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfWaitLockCreate(&attributes,
                               &deviceExtension->TargetDeviceCollectionLock);
    if (!NT_SUCCESS(status))
    {
        KdPrint( ("[toastmon]WdfWaitLockCreate failed with status 0x%x\n", status));
        return status;
    }

    //
    // Register for TOASTER device interface change notification.
    // We will get GUID_DEVICE_INTERFACE_ARRIVAL and               // 定义于 inc\ddk\wdmguid.h
    // GUID_DEVICE_INTERFACE_REMOVAL notification when the toaster
    // device is started and removed.
    // Framework doesn't provide a WDF interface to register for interface change notification. 
    // --上头这句话好像强调的是: *只有* WDM 层面的函数(IoRegisterPlugPlayNotification)
	//   才提供 interface-change 通知功能, 因此只好用 WDM API 了.
	//
	// However if the target device is opened (later) by symbolic-link using
    // IoTarget, framework (自动进行) registers itself EventCategoryTargetDeviceChange
    // notification on the handle and responds to the PnP notifications.
    //
    // Note that as soon as you register, arrival notification will be sent
    // about all existing toaster devices even before this device is started. So if
    // you cannot handle these notification before start then you should register this in
    // PrepareHardware or SelfManagedIoInit callback.
    // You must unregister this notification when the device is removed in the
    // DeviceContextCleanup callback. 
	// This call takes a reference on the driverobject. // this call 指的是 IoRegisterPlugPlayNotification 吗?
    // So if you don't unregister it will prevent the driver from unloading.
    //
    status = IoRegisterPlugPlayNotification (
        EventCategoryDeviceInterfaceChange, // 注意：此处不是用 EventCategoryTargetDeviceChange
        PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
        (PVOID)&GUID_DEVINTERFACE_TOASTER,  // 监视[提供 GUID_DEVINTERFACE_TOASTER 接口的设备的]生成和销毁事件
        WdfDriverWdmGetDriverObject(WdfDeviceGetDriver(device)),
        ToastMon_PnpNotifyInterfaceChange, // (PDRIVER_NOTIFICATION_CALLBACK_ROUTINE) // pointer implicit conversion ok for C.
        (PVOID)deviceExtension,
        &deviceExtension->NotificationHandle);

    if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]RegisterPnPNotifiction failed: 0x%x\n", status));
        return status;
    }

    RegisterForWMINotification(deviceExtension);

    return status;
}

VOID
ToastMon_EvtDeviceContextCleanup(
    IN WDFDEVICE Device
    )
/*++
Routine Description:

EvtDeviceContextCleanup event callback must perform any operations that are
   necessary before the specified device is removed. The framework calls
   the driver's EvtDeviceRemove callback when the PnP manager sends    // 注释错误: EvtDeviceRemove
   an IRP_MN_REMOVE_DEVICE request to the driver stack. Function driver
   typically undo whatever they did in EvtDeviceAdd callback - free
   structures, cleanup collections, etc.

Arguments:
    Device - Handle to a framework device object.
--*/
{
    PDEVICE_EXTENSION           deviceExtension;
    KdPrint( ("[toastmon]ToastMon_EvtDeviceContextCleanup\n"));
    PAGED_CODE();

    deviceExtension = GetDeviceExtension(Device);

    //
    // Unregister the interface notification
    //
    if(deviceExtension->NotificationHandle) {
        IoUnregisterPlugPlayNotification(deviceExtension->NotificationHandle);
    }

    //
    // TargetDeviceCollection will get deleted automatically when
    // the Device is deleted due to the association we made when
    // we created the object in EvtDeviceAdd. (之前用 .ParentObject 关联的)
    //
    // Any targets remaining in the collection will also be automatically closed
    // and deleted.
    //
    deviceExtension->TargetDeviceCollection = NULL;

    UnregisterForWMINotification(deviceExtension);

    return;
}

NTSTATUS
ToastMon_PnpNotifyInterfaceChange(
    IN  PDEVICE_INTERFACE_CHANGE_NOTIFICATION NotificationStruct,
    IN  PVOID  Context
    )
/*++
Routine Description:

    This routine is the PnP "interface change notification" callback routine.

	This gets called on a Toaster triggered device interface arrival or removal.
	// Chj: This gets called when a device supporting GUID_DEVINTERFACE_TOASTER arrives or disappears.
      - Interface arrival corresponds to a Toaster device being STARTED
      - Interface removal corresponds to a Toaster device being REMOVED

    On arrival:
      - Create a IoTarget and open it by using the symbolic-link. WDF will
        register for EventCategoryTargetDeviceChange notification on the fileobject
        so it can cleanup whenever associated device is removed. // 意思是 WDF 会自动做这个注册动作

    On removal:
      - This callback is a NO-OP for interface removal because framework registers
        for PnP EventCategoryTargetDeviceChange callbacks and
        uses that callback to clean up when the associated toaster device goes away.

Arguments:

    NotificationStruct  - Structure defining the change.

    Context -    pointer to the device extension.
                 (supplied as the "context" when we registered for this callback)
Return Value:

    STATUS_SUCCESS - always, even if something goes wrong.

    status is only used during query removal notifications and the OS ignores other cases
--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PDEVICE_EXTENSION           deviceExtension = Context;
    WDFIOTARGET                 ioTarget;

    PAGED_CODE();

    KdPrint(("[toastmon]Entered ToastMon_PnpNotifyInterfaceChange\n"));

    //
    // Verify that interface class is a toaster device interface.
    //
    ASSERT(IsEqualGUID( (LPGUID)&(NotificationStruct->InterfaceClassGuid),
                      (LPGUID)&GUID_DEVINTERFACE_TOASTER));

    //
    // Check the callback event (type).
    //
    if(IsEqualGUID( (LPGUID)&(NotificationStruct->Event),
                     (LPGUID)&GUID_DEVICE_INTERFACE_ARRIVAL )) 
	{
        KdPrint(("[toastmon]Arrival Notification\n"));

        status = Toastmon_OpenDevice((WDFDEVICE)deviceExtension->WdfDevice,
                                     (PUNICODE_STRING)NotificationStruct->SymbolicLinkName,
                                     &ioTarget);
        if (!NT_SUCCESS(status)) {
            KdPrint( ("[toastmon]Unable to open control device 0x%x\n", status));
            return status;
        }

        //
        // Add this one to the collection.
        //
        WdfWaitLockAcquire(deviceExtension->TargetDeviceCollectionLock, NULL);

        //
        // WdfCollectionAdd takes a reference on the request object and removes
        // it when you call WdfCollectionRemove.
        //
        status = WdfCollectionAdd(deviceExtension->TargetDeviceCollection, ioTarget);
        if (!NT_SUCCESS(status)) {
            KdPrint( ("[toastmon]WdfCollectionAdd failed 0x%x\n", status));
            WdfObjectDelete(ioTarget); // Delete will also close the target
        }

        WdfWaitLockRelease(deviceExtension->TargetDeviceCollectionLock);

    } 
	else 
	{
        KdPrint(("[toastmon]Removal Interface Notification\n"));
    }
    return STATUS_SUCCESS;
}


NTSTATUS
Toastmon_OpenDevice(
    WDFDEVICE Device,
    PUNICODE_STRING SymbolicLink,
    WDFIOTARGET *Target
    )
/*++
Routine Description:
    Open the I/O target and preallocate any resources required
    to communicate with the target device.
--*/
{
    NTSTATUS                    status = STATUS_SUCCESS;
    PTARGET_DEVICE_INFO         targetDeviceInfo = NULL;
    WDF_IO_TARGET_OPEN_PARAMS   openParams;
    WDFIOTARGET                 ioTarget;
    WDF_OBJECT_ATTRIBUTES       attributes;
    PDEVICE_EXTENSION           deviceExtension = GetDeviceExtension(Device);
    WDF_TIMER_CONFIG            wdfTimerConfig;
    
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, TARGET_DEVICE_INFO); // 明明应该叫 IOTARGET_INFO

    status = WdfIoTargetCreate(deviceExtension->WdfDevice, // =Device
                            &attributes,
                            &ioTarget);
    if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfIoTargetCreate failed 0x%x\n", status));
        return status;
    }

    targetDeviceInfo = GetTargetDeviceInfo(ioTarget);
    targetDeviceInfo->DeviceExtension = deviceExtension;

    //
    // Warning: It's not recommended to open the targetdevice
    // from a pnp notification callback routine, because if
    // the target device initiates any kind of PnP action as
    // a result of this open, the PnP manager could deadlock.
    // You should queue a workitem to do that.
    // For example, SWENUM devices in conjunction with KS
    // initiate an enumeration of a device when you do the
    // open on the device interface.
	//
    // We can open the target device here because we know the
    // toaster function driver doesn't trigger any pnp action. // 没看懂， toaster 触发 PnP action 是什么意思？
    //

    WDF_IO_TARGET_OPEN_PARAMS_INIT_OPEN_BY_NAME(
        &openParams,
        SymbolicLink,
        STANDARD_RIGHTS_ALL);

    openParams.ShareAccess = FILE_SHARE_WRITE | FILE_SHARE_READ;
    //
    // Framework provides default action for all of these if you don't register
    // these callbacks -it will close the handle to the target when the device is
    // being query-removed and reopen it if the query-remove fails.
    // In this sample, we use a periodic timers to post requests to the target.
    // So we need to register these callbacks so that we can start and stop
    // the timer when the state of the target device changes. Since we are
    // registering these callbacks, we are now responsible for closing and
    // reopening the target.
    //
    openParams.EvtIoTargetQueryRemove = ToastMon_EvtIoTargetQueryRemove;
    openParams.EvtIoTargetRemoveCanceled = ToastMon_EvtIoTargetRemoveCanceled;
    openParams.EvtIoTargetRemoveComplete = ToastMon_EvtIoTargetRemoveComplete;
	//
    status = WdfIoTargetOpen(ioTarget, &openParams);

    if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfIoTargetOpen failed with status 0x%x\n", status));
        WdfObjectDelete(ioTarget);
        return status;
    }
   
    KdPrint(("[toastmon]Target Device=0x%p, PDO=0x%p, Fileobject=0x%p, Filehandle=0x%p\n",
                        WdfIoTargetWdmGetTargetDeviceObject(ioTarget),
                        WdfIoTargetWdmGetTargetPhysicalDevice(ioTarget),
                        WdfIoTargetWdmGetTargetFileObject(ioTarget),
                        WdfIoTargetWdmGetTargetFileHandle(ioTarget)));

    //
    // Create two request objects - one for read and one for write.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = ioTarget;

    status = WdfRequestCreate(&attributes,
                              ioTarget,
                              &targetDeviceInfo->ReadRequest);

    if (!NT_SUCCESS(status)) {
        WdfObjectDelete(ioTarget);
        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = ioTarget;

    status = WdfRequestCreate(&attributes,
                            ioTarget,
                            &targetDeviceInfo->WriteRequest);

    if (!NT_SUCCESS(status)) {
        WdfObjectDelete(ioTarget);
        return status;
    }

    //
    // Create a passive timer to post requests to the I/O target.
    //
    WDF_TIMER_CONFIG_INIT(&wdfTimerConfig,
                          Toastmon_EvtTimerPostRequests);
     
    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, TIMER_CONTEXT);

    // Make IoTarget as parent of the timer to prevent the ioTarget
    // from deleted until the DPC has run to completion.
    //
    attributes.ParentObject = ioTarget;

    // By specifying WdfExecutionLevelPassive the framework will invoke
    // the timer callback Toastmon_EvtTimerPostRequests at PASSIVE_LEVEL.
    //
    attributes.ExecutionLevel = WdfExecutionLevelPassive;

    // Setting the AutomaticSerialization to FALSE prevents
    // WdfTimerCreate to fail if the parent device object's 
    // execution level is set to WdfExecutionLevelPassive.       // 何意？
    //
    wdfTimerConfig.AutomaticSerialization = FALSE;
    
    status = WdfTimerCreate(&wdfTimerConfig,
                       &attributes,
                       &targetDeviceInfo->TimerForPostingRequests
                       );

    if(!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfTimerCreate failed 0x%x\n", status));
        WdfObjectDelete(ioTarget);
        return status;
    }

    GetTimerContext(targetDeviceInfo->TimerForPostingRequests)->IoTarget = ioTarget;

    // Start the passive timer. The first timer will be queued after 1ms interval and
    // after that it will be requeued in the timer callback function. 
    // The value of 1 ms (lowest timer resolution allowed on NT) is chosen here so 
    // that timer would fire right away.
    //
    WdfTimerStart(targetDeviceInfo->TimerForPostingRequests,
                                        WDF_REL_TIMEOUT_IN_MS(1));

    *Target = ioTarget;

    return status;
}

NTSTATUS
ToastMon_EvtIoTargetQueryRemove(
    WDFIOTARGET IoTarget
)
/*++
Routine Description:
    Called when the Target device receives IRP_MN_QUERY_REMOVE.
    This happens when somebody disables, ejects or uninstalls the target
    device driver in user mode. Here close the handle to the
    target device. If the system fails to remove the device for
    some reason, you will get RemoveCancelled callback where
    you can reopen and continue to interact with the target device.
--*/
{
    PTARGET_DEVICE_INFO         targetDeviceInfo = NULL;

    PAGED_CODE();

    targetDeviceInfo = GetTargetDeviceInfo(IoTarget);

    KdPrint(("[toastmon]Device Removal (query remove) Notification\n"));

    //
    // Stop the timer 
    //

    WdfTimerStop(targetDeviceInfo->TimerForPostingRequests, TRUE);

    WdfIoTargetCloseForQueryRemove(IoTarget);

//	return STATUS_UNSUCCESSFUL; // Memo: This will cause `enum -e 1` to be denied.

	return STATUS_SUCCESS;
}

VOID
ToastMon_EvtIoTargetRemoveCanceled(
    WDFIOTARGET IoTarget
    )
/*++
Routine Description:
    Called when the Target device received IRP_MN_CANCEL_REMOVE.
    This happens if another app or driver talking to the target
    device doesn't close handle or veto query-remove notification.
--*/
{
    PTARGET_DEVICE_INFO         targetDeviceInfo = NULL;
    WDF_IO_TARGET_OPEN_PARAMS   openParams;
    NTSTATUS status;

    PAGED_CODE();

    KdPrint(("[toastmon]Device Removal (remove cancelled) Notification\n"));

    targetDeviceInfo = GetTargetDeviceInfo(IoTarget);

    //
    // Reopen the Target.
    //
    WDF_IO_TARGET_OPEN_PARAMS_INIT_REOPEN(&openParams);

    status = WdfIoTargetOpen(IoTarget, &openParams);

    if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfIoTargetOpen failed 0x%x\n", status));
        WdfObjectDelete(IoTarget);
        return;
    }

    //
    // Restart the timer.
    //
    WdfTimerStart(targetDeviceInfo->TimerForPostingRequests,
                                        WDF_REL_TIMEOUT_IN_SEC(1));
}

VOID
ToastMon_EvtIoTargetRemoveComplete(
    WDFIOTARGET IoTarget
)
/*++
Routine Description:
    Called when the Target device is removed ( either the target
    received IRP_MN_REMOVE_DEVICE or IRP_MN_SURPRISE_REMOVAL)
--*/
{
    PDEVICE_EXTENSION      deviceExtension;
    PTARGET_DEVICE_INFO    targetDeviceInfo = NULL;

    KdPrint(("[toastmon]Device Removal (remove complete) Notification\n"));

    PAGED_CODE();

    targetDeviceInfo = GetTargetDeviceInfo(IoTarget);
    deviceExtension = targetDeviceInfo->DeviceExtension;

    //
    // Stop the timer 
    //
    WdfTimerStop(targetDeviceInfo->TimerForPostingRequests, TRUE);

    //
    // Remove the target device from the collection.
    //
    WdfWaitLockAcquire(deviceExtension->TargetDeviceCollectionLock, NULL);

    WdfCollectionRemove(deviceExtension->TargetDeviceCollection, IoTarget);

    WdfWaitLockRelease(deviceExtension->TargetDeviceCollectionLock);

    //
    // Finally delete the target.
    //
    WdfObjectDelete(IoTarget);

    return;
}

VOID
Toastmon_EvtTimerPostRequests(
    IN WDFTIMER Timer
    )
/*++
Routine Description:
    Passive timer event to post read and write requests.
--*/
{
    NTSTATUS status;

    PTARGET_DEVICE_INFO       targetInfo;
    
    WDFIOTARGET         ioTarget = GetTimerContext(Timer)->IoTarget;

    targetInfo = GetTargetDeviceInfo(ioTarget);

    //
    // Even though this routine and the completion routine check the 
    // ReadRequest/WriteRequest field outside a lock, no harm is done. 
    // Depending on how far the completion-routine has run, timer 
    // may miss an opportunity to post a request. Even if we use a lock, 
    // this race condition will still exist. 
    //

    //
    // Send a read request to the target device
    //
    if(targetInfo->ReadRequest) {
        status = ToastMon_PostReadRequests(ioTarget);
        if (!NT_SUCCESS(status)) {
            ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    //
    // Send a write request to the target device
    //
    if(targetInfo->WriteRequest) {
        status = ToastMon_PostWriteRequests(ioTarget);
        if (!NT_SUCCESS(status)) {
            ASSERT(status == STATUS_INSUFFICIENT_RESOURCES);
        }
    }

    //
    // Restart the passive timer.
    //
    WdfTimerStart(targetInfo->TimerForPostingRequests,
                                      WDF_REL_TIMEOUT_IN_SEC(1));

    return;
}

NTSTATUS
ToastMon_PostReadRequests(
    IN WDFIOTARGET IoTarget
    )
/*++
Routine Description:
    Called by the timer callback to send a read request to the target device.

Return Value:
    NT Status code - only failure expected is STATUS_INSUFFICIENT_RESOURCES
--*/
{
    WDFREQUEST                  request;
    NTSTATUS                    status;
    PTARGET_DEVICE_INFO       targetInfo;
    WDFMEMORY               memory;
    WDF_OBJECT_ATTRIBUTES       attributes;

    targetInfo = GetTargetDeviceInfo(IoTarget);

    request = targetInfo->ReadRequest;

    //
    // Allocate memory for read. Ideally I should have allocated the memory upfront along
    // with the request because the sizeof the memory buffer is constant.
    // But for demonstration, I have chosen to allocate a memory
    // object everytime I send a request down and delete it when the request
    // is completed.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    status = WdfMemoryCreate(
                        &attributes,
                        NonPagedPool,
                        DRIVER_TAG,
                        READ_BUF_SIZE,
                        &memory,
                        NULL); // buffer pointer

    if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfMemoryCreate failed 0x%x\n", status));
        return status;
    }

    status = WdfIoTargetFormatRequestForRead(
                                IoTarget,
                                request,
                                memory,
                                NULL, // Buffer offset
                                NULL); // OutputBufferOffset
   if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfIoTargetFormatRequestForRead failed 0x%x\n", status));
        return status;
    }

    WdfRequestSetCompletionRoutine(request,
                   Toastmon_ReadRequestCompletionRoutine,
                   targetInfo);

    //
    // Clear the ReadRequest field in the context to avoid
    // being reposted even before the request completes.
    // This will be reset in the complete routine when the request completes.
	//
	// "avoid being reposted" 的意思是, 防止前一次的 read-request 未完成前, (由于下一个定时时间点到)又再启动一次 read-request.
	// 本程序不希望出现并行的两个 read-request. 将 targetInfo->ReadRequest 设成 NULL; 可以达到此目的.
    //
    targetInfo->ReadRequest = NULL;

    if(WdfRequestSend(request, IoTarget, WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(request);
        KdPrint(("[toastmon]WdfRequestSend failed 0x%x\n", status));
        targetInfo->ReadRequest = request;
    }

    return status;
}

NTSTATUS
ToastMon_PostWriteRequests(
    IN WDFIOTARGET IoTarget
    )
/*++
Routine Description:
    Called by the timer callback to send a write request to the target device.

Return Value:
    NT Status code - only failure expected is STATUS_INSUFFICIENT_RESOURCES
--*/
{
    WDFREQUEST                  request;
    NTSTATUS                    status;
    PTARGET_DEVICE_INFO       targetInfo;
    WDFMEMORY               memory;
    WDF_OBJECT_ATTRIBUTES       attributes;

    targetInfo = GetTargetDeviceInfo(IoTarget);

    request = targetInfo->WriteRequest;

    //
    // Allocate memory for write. Ideally I should have allocated the memory upfront along
    // with the request because the sizeof the memory buffer is constant.
    // But for demonstration, I have chosen to allocate a memory
    // object everytime I send a request down and delete it when the request
    // is completed.
    //
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    status = WdfMemoryCreate(
                        &attributes,
                        NonPagedPool,
                        DRIVER_TAG,
                        WRITE_BUF_SIZE,
                        &memory,
                        NULL); // buffer pointer

    if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfMemoryCreate failed 0x%x\n", status));
        return status;
    }

    status = WdfIoTargetFormatRequestForWrite(
                                IoTarget,
                                request,
                                memory,
                                NULL, // Buffer offset
                                NULL); // OutputBufferOffset
   if (!NT_SUCCESS(status)) {
        KdPrint(("[toastmon]WdfIoTargetFormatRequestForWrite failed 0x%x\n", status));
        return status;
    }

    WdfRequestSetCompletionRoutine(request,
                   Toastmon_WriteRequestCompletionRoutine,
                   targetInfo);

    //
    // Clear the WriteRequest field in the context to avoid
    // being reposted even before the request completes.
    // This will be reset in the complete routine when the request completes.
    //
    targetInfo->WriteRequest = NULL;

    if(WdfRequestSend(request, IoTarget, WDF_NO_SEND_OPTIONS) == FALSE) {
        status = WdfRequestGetStatus(request);
        KdPrint(("[toastmon]WdfRequestSend failed 0x%x\n", status));
        targetInfo->WriteRequest = request;
    }
    return status;
}

VOID
Toastmon_ReadRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
/*++
Routine Description:
    Completion Routine

Arguments:
    CompletionParams - Contains the results of the transfer such as IoStatus, Length, Buffer, etc.

    Context - context value specified in the WdfRequestSetCompletionRoutine
--*/
{
    WDF_REQUEST_REUSE_PARAMS    params;
    PTARGET_DEVICE_INFO       targetInfo;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Context);

    targetInfo = GetTargetDeviceInfo(Target);
    
    //
    // Delete the memory object because we create a new one every time we post
    // the request. For perf reason, it would be better to preallocate the memory
    // object once.
    // Also for driver created requests, do not call WdfRequestRetrieve functions to get
    // the buffers. They can be called only for requests delivered by the queue.
    //
    WdfObjectDelete(CompletionParams->Parameters.Read.Buffer);

    //
    // Scrub the request for reuse.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

    status = WdfRequestReuse(Request, &params);
    ASSERT(NT_SUCCESS(status));
    //
    // RequestReuse zero all the values in structure pointed by CompletionParams.
    // So you must get all the information from completion params before
    // calling RequestReuse.

    targetInfo->ReadRequest = Request; 
		// 该赋值将被定时器回调函数 Toastmon_EvtTimerPostRequests 里头的代码检测到, 
		// 重新启动一轮 ToastMon_PostReadRequests.

    // Don't repost the request in the completion routine because it may lead to recursion
    // if the driver below completes the request synchronously.
    return;
}

VOID
Toastmon_WriteRequestCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
/*++
Routine Description:
    Completion Routine

Arguments:
    CompletionParams - Contains the results of the transfer such as IoStatus, Length, Buffer, etc.

    Context - context value specified in the WdfRequestSetCompletionRoutine
--*/
{
    WDF_REQUEST_REUSE_PARAMS    params;
    PTARGET_DEVICE_INFO       targetInfo;
    NTSTATUS status;

    UNREFERENCED_PARAMETER(Context);

    targetInfo = GetTargetDeviceInfo(Target);
    
    //
    // Delete the memory object because we create a new one every time we post
    // the request. For perf reason, it would be better to preallocate even memory object.
    // Also for driver created requests, do not call WdfRequestRetrieve functions to get
    // the buffers. WdfRequestRetrieveBuffer functions can be called only for requests
    // delivered by the queue.
    //
    WdfObjectDelete(CompletionParams->Parameters.Write.Buffer);

    //
    // Scrub the request for reuse.
    //
    WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);

    status = WdfRequestReuse(Request, &params);
    ASSERT(NT_SUCCESS(status));

    //
    // RequestReuse zero all the values in structure pointed by CompletionParams.
    // So you must get all the information from completion params before
    // calling RequestReuse.
    //

    targetInfo->WriteRequest = Request;

    //
    // Don't repost the request in the completion routine because it may lead to recursion
    // if the driver below completes the request synchronously.
    //    
    return;
}


