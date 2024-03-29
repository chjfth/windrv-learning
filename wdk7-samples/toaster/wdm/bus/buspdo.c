/*++
Copyright (c) 1990-2000    Microsoft Corporation All Rights Reserved

Module Name:
    BusPdo.c

Abstract:
    This module handles plug & play calls for the *child* PDO.

Author:

Environment:
    kernel mode only

Notes:

Revision History:
--*/

#include "busenum.h"

#define VENDORNAME L"Microsoft_"
#define MODEL       L"Eliyas_Toaster_"

#ifdef ALLOC_PRAGMA
#pragma alloc_text (PAGE, Bus_PDO_PnP)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceCaps)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceId)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceText)
#pragma alloc_text (PAGE, Bus_PDO_QueryResources)
#pragma alloc_text (PAGE, Bus_PDO_QueryResourceRequirements)
#pragma alloc_text (PAGE, Bus_PDO_QueryDeviceRelations)
#pragma alloc_text (PAGE, Bus_PDO_QueryBusInformation)
#pragma alloc_text (PAGE, Bus_GetDeviceCapabilities)
#pragma alloc_text (PAGE, Bus_PDO_QueryInterface)

#endif

NTSTATUS
Bus_PDO_PnP (
    __in PDEVICE_OBJECT       DeviceObject,
    __in PIRP                 Irp,
    __in PIO_STACK_LOCATION   IrpStack,
    __in PPDO_DEVICE_DATA     DeviceData	// Chj: PDO user-data for toaster devnode(the child)
    )
/*++
Routine Description:
    Handle requests from the Plug & Play system for the devices on the BUS
--*/
{
    NTSTATUS                status = 0;
    PAGED_CODE ();

    //
    // NB: Because we are a bus enumerator, we have no one to whom we could
    // defer these irps.  Therefore we do not pass them down but merely return them.
    //
	
    switch (IrpStack->MinorFunction) 
	{{
    case IRP_MN_START_DEVICE:
        // Here we do what ever initialization and ``turning on'' that is
        // required to allow others to access this device.
        // Power up the device.
        //
        DeviceData->DevicePowerState = PowerDeviceD0;
        SET_NEW_PNP_STATE(DeviceData, Started);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_STOP_DEVICE:
        // Here we shut down the device and give up and unmap any resources
        // we acquired for the device.
        //
        SET_NEW_PNP_STATE(DeviceData, Stopped);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_STOP_DEVICE:
        // No reason here why we can't stop the device.
        // If there were a reason we should speak now, because answering success
        // here may result in a stop device irp.
        //
        SET_NEW_PNP_STATE(DeviceData, StopPending);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_STOP_DEVICE:
        // The stop was canceled.  Whatever state we set, or resources we put
        // on hold in anticipation of the forthcoming STOP device IRP should be
        // put back to normal.  Someone, in the long list of concerned parties,
        // has failed the stop device query.
        //
        // First check to see whether you have received cancel-stop
        // without first receiving a query-stop. This could happen if someone
        // above us fails a query-stop and passes down the subsequent cancel-stop.
        //
        if (StopPending == DeviceData->DevicePnPState)
        {
            // We did receive a query-stop, so restore.
            RESTORE_PREVIOUS_PNP_STATE(DeviceData);
        }
        status = STATUS_SUCCESS;// We must not fail this IRP.
        break;

    case IRP_MN_QUERY_REMOVE_DEVICE:
        // Check to see whether the device can be removed safely.
        // If not fail this request. This is the last opportunity to do so.
        //
        if (DeviceData->ToasterInterfaceRefCount){
            // Somebody is still using our interface. We must fail the remove-request.
            //
            status = STATUS_UNSUCCESSFUL;
            break;
        }

        SET_NEW_PNP_STATE(DeviceData, RemovePending);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_CANCEL_REMOVE_DEVICE:
        // Clean up a remove that did not go through.
        //
        // First check to see whether you have received cancel-remove
        // without first receiving a query-remove. This could happen if
        // someone above us fails a query-remove and passes down the subsequent cancel-remove.
        //
        if (RemovePending == DeviceData->DevicePnPState)
        {
            // We did receive a query-remove, so restore.
            RESTORE_PREVIOUS_PNP_STATE(DeviceData);
        }
        status = STATUS_SUCCESS; // We must not fail this IRP.
        break;

    case IRP_MN_SURPRISE_REMOVAL:
        // We should stop all access to the device and relinquish all the
        // resources. Let's just mark that it happened and we will do
        // the cleanup later in IRP_MN_REMOVE_DEVICE.
        //
        SET_NEW_PNP_STATE(DeviceData, SurpriseRemovePending);
        status = STATUS_SUCCESS;
        break;

    case IRP_MN_REMOVE_DEVICE:
        // DeviceData->Present is set to true when the pdo is exposed via PlugIn IOCTL.
        // It is set to FALSE when a UnPlug IOCTL is received.
        // We will delete the PDO only after we have reported to the
        // Plug and Play manager that it's missing.
        //
		// Chj: 分两种情况进行处理。
		// 第一种: toaster 子设备被 enum -u 1 强行删除, 或被 surprise-remove.
		// 第二种：在 devmgmt.msc 里头禁用(disable) toaster 子设备。
		// if/else 代码块修剪过，更容易看清两条平行的分支, 但并未改变原始写法的逻辑。
		
        if (DeviceData->ReportedMissing) 
		{
			// toaster 子设备被删除，需要同时销毁 PDO 

            PFDO_DEVICE_DATA fdoData;
            SET_NEW_PNP_STATE(DeviceData, Deleted);

            // Remove the PDO from the list and decrement the count of PDO.
            // Don't forget to synchronize access to the FDO data.
            // If the parent FDO is deleted before child PDOs, the ParentFdo
            // pointer will be NULL. This could happen if the child PDO
            // is in a SurpriseRemovePending state when the parent FDO
            // is removed.
            //
            if (DeviceData->ParentFdo) {
                fdoData = ParentFDO_FROM_PDO(DeviceData);
                ExAcquireFastMutex (&fdoData->Mutex);
                RemoveEntryList (&DeviceData->Link);
                fdoData->NumPDOs--;
                ExReleaseFastMutex (&fdoData->Mutex);
            }

            // Free up resources associated with PDO and delete it.
            status = Bus_DestroyPdo(DeviceObject, DeviceData);
        }
		else
		{
			// 设备管理器中 *disable* 子设备，将执行此处代码，此情况下不销毁 PDO 。
			// 正因为不删除 PDO ，设备管理器中才能继续显示那个设备节点，只不过显示为 disable 的样子，
			// --在 WinXP 中是在设备节点图标上覆盖一个小红叉，Win7 是覆盖一个用圆圈圈起的朝下箭头。

			ASSERT(DeviceData->Present); // chj clarify
            // When the device is *disabled* , the PDO transitions from
            // RemovePending to NotStarted. We shouldn't delete the PDO because
            // a) the device is still present on the bus,
            // b) we haven't reported missing to the PnP manager.
            //
            SET_NEW_PNP_STATE(DeviceData, NotStarted);
            status = STATUS_SUCCESS;
        }
        break;

    case IRP_MN_QUERY_CAPABILITIES:
        // Return the capabilities of a device, such as whether the device
        // can be locked or ejected..etc
        //
        status = Bus_PDO_QueryDeviceCaps(DeviceData, Irp);
        break;

    case IRP_MN_QUERY_ID:
        // Query the IDs of the device
        Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_TRACE,
                ("\tQueryId Type: %s\n",
                DbgDeviceIDString(IrpStack->Parameters.QueryId.IdType)));

        status = Bus_PDO_QueryDeviceId(DeviceData, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_RELATIONS:

        Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_TRACE,
            ("\tQueryDeviceRelation Type: %s\n",DbgDeviceRelationString(\
                    IrpStack->Parameters.QueryDeviceRelations.Type)));

        status = Bus_PDO_QueryDeviceRelations(DeviceData, Irp);
        break;

    case IRP_MN_QUERY_DEVICE_TEXT:
        status = Bus_PDO_QueryDeviceText(DeviceData, Irp);
        break;

    case IRP_MN_QUERY_RESOURCES:
        status = Bus_PDO_QueryResources(DeviceData, Irp);
        break;

    case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
        status = Bus_PDO_QueryResourceRequirements(DeviceData, Irp);
        break;

    case IRP_MN_QUERY_BUS_INFORMATION:
        status = Bus_PDO_QueryBusInformation(DeviceData, Irp);
        break;

    case IRP_MN_DEVICE_USAGE_NOTIFICATION:
        // OPTIONAL for bus drivers.
        // This bus drivers any of the bus's descendants
        // (child device, child of a child device, etc.) do not
        // contain a memory file namely paging file, dump file,
        // or hibernation file. So we  fail this Irp.
        //
        status = STATUS_UNSUCCESSFUL;
        break;

    case IRP_MN_EJECT:

        //
        // For the device to be ejected, the device must be in the D3
        // device power state (off) and must be unlocked
        // (if the device supports locking). Any driver that returns success
        // for this IRP must wait until the device has been ejected before
        // completing the IRP.
        //
        DeviceData->Present = FALSE;

        status = STATUS_SUCCESS;
        break;

    case IRP_MN_QUERY_INTERFACE:
        //
        // This request enables a driver to export a direct-call
        // interface to other drivers. A bus driver that exports
        // an interface must handle this request for its child
        // devices (child PDOs).
        //
        status = Bus_PDO_QueryInterface(DeviceData, Irp);
        break;

    case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
        // OPTIONAL for bus drivers.
        // The PnP Manager sends this IRP to a device
        // stack so filter and function drivers can adjust the
        // resources required by the device, if appropriate.
		//break;
        status = 0x1111; // debug

    case IRP_MN_QUERY_PNP_DEVICE_STATE:
		status = 0x2222; // debug
        //
        // OPTIONAL for bus drivers.
        // The PnP Manager sends this IRP after the drivers for
        // a device return success from the IRP_MN_START_DEVICE
        // request. The PnP Manager also sends this IRP when a
        // driver for the device calls IoInvalidateDeviceState.
        // break;

    case IRP_MN_READ_CONFIG:
		status = 0x3333; // debug
    case IRP_MN_WRITE_CONFIG:
		status = 0x4444; // debug
        //
        // Bus drivers for buses with configuration space must handle
        // this request for their child devices. Our devices don't
        // have a config space.
        // break;

    case IRP_MN_SET_LOCK:
		status = 0x5555; // debug
        //
        // Our device is not a lockable device
        // so we don't support this Irp.
        // break;

    default: // Chj:PWDM2 p437 列的 *ignore* 那种情形。
        // Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_TRACE,("\t Not handled\n"));
        // For PnP requests to the PDO that we do not understand we should
        // return the IRP WITHOUT setting the status or information fields.
        // These fields may have already been set by a filter (eg acpi).
        status = Irp->IoStatus.Status;
        break;
	}}

    Irp->IoStatus.Status = status;
    IoCompleteRequest (Irp, IO_NO_INCREMENT);

    return status;
}

NTSTATUS
Bus_PDO_QueryDeviceCaps(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++
Routine Description:
    When a device is enumerated, but before the function and
    filter drivers are loaded for the device, the PnP Manager
    sends an IRP_MN_QUERY_CAPABILITIES request to the parent
    bus driver for the device. The bus driver must set any
    relevant values in the DEVICE_CAPABILITIES structure and
    return it to the PnP Manager.

Arguments:
    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:
    NT STATUS
--*/
{
    PIO_STACK_LOCATION      stack;
    PDEVICE_CAPABILITIES    deviceCapabilities;
    DEVICE_CAPABILITIES     parentCapabilities;
    NTSTATUS                status;
    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    // Get the packet. 拿到输出缓冲区指针
    deviceCapabilities = stack->Parameters.DeviceCapabilities.Capabilities;

    // Set the capabilities.
    //
    if (deviceCapabilities->Version != 1 ||
            deviceCapabilities->Size < sizeof(DEVICE_CAPABILITIES))
    {
       return STATUS_UNSUCCESSFUL;
    }

    // Get the device capabilities of the parent
    //
    status = Bus_GetDeviceCapabilities(
        ParentFDO_FROM_PDO(DeviceData)->NextLowerDriver, &parentCapabilities);

    if (!NT_SUCCESS(status)) {
        Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_TRACE,
            ("\tQueryDeviceCaps failed\n"));
        return status;
    }

	// Chj: 本段解释: 要参考父设备的  DEVICE_CAPABILITIES 的理由。
    //
    // The entries in the DeviceState array are based on the capabilities
    // of the parent devnode. These (parent) entries signify the highest-powered
    // state that the device can support for the corresponding system state.
    // A (child device) driver can specify a lower(less-powered) state than the
    // bus driver.  For eg: Suppose the toaster bus controller supports
    // D0, D2, and D3; and the Toaster Device supports D0, D1, D2, and D3.
    // Following the above rule, the device *cannot* specify D1 as one of
    // it's power state. // [2018-02-18]Chj: 那所谓 *cannot*, 其实质约束体现在哪里?
    // A driver(of child device) can make the rules more restrictive but cannot loosen them.
    // First copy the parent's S to D state mapping
    //
    RtlCopyMemory(
        deviceCapabilities->DeviceState,
        parentCapabilities.DeviceState,
        (PowerSystemShutdown + 1) * sizeof(DEVICE_POWER_STATE)
        ); // 拷贝的是数组 DEVICE_CAPABILITIES.DeviceState[POWER_SYSTEM_MAXIMUM]
    //
    // Adjust the caps to what your device supports.
    // Our device just supports D0 and D3. (喂，下面明明说可以支持 D1 的, 注释和代码有矛盾!)
    //
    deviceCapabilities->DeviceState[PowerSystemWorking] = PowerDeviceD0;
	//
    if (deviceCapabilities->DeviceState[PowerSystemSleeping1] != PowerDeviceD0)
        deviceCapabilities->DeviceState[PowerSystemSleeping1] = PowerDeviceD1;
	//
    if (deviceCapabilities->DeviceState[PowerSystemSleeping2] != PowerDeviceD0)
        deviceCapabilities->DeviceState[PowerSystemSleeping2] = PowerDeviceD3;
	//
    if (deviceCapabilities->DeviceState[PowerSystemSleeping3] != PowerDeviceD0)
        deviceCapabilities->DeviceState[PowerSystemSleeping3] = PowerDeviceD3;

    // We can wake the system from D1
	// Chj: "physical (child) device" can wake the system when the child devnode is in D1 state.
    deviceCapabilities->DeviceWake = PowerDeviceD1;

    // Specifies whether the device hardware supports the D1 and D2
    // power state. Set these bits explicitly.
    //
    deviceCapabilities->DeviceD1 = TRUE; // Yes we can
    deviceCapabilities->DeviceD2 = FALSE;

    // Specifies whether the device can respond to an external wake
    // signal while in the D0, D1, D2, and D3 state.
    // Set these bits explicitly.
    //
    deviceCapabilities->WakeFromD0 = FALSE;
    deviceCapabilities->WakeFromD1 = TRUE; //Yes we can
    deviceCapabilities->WakeFromD2 = FALSE;
    deviceCapabilities->WakeFromD3 = FALSE;

    // We have no latencies
    deviceCapabilities->D1Latency = 0;
    deviceCapabilities->D2Latency = 0;
    deviceCapabilities->D3Latency = 0;

    // Ejection supported
    deviceCapabilities->EjectSupported = TRUE;

    // This flag specifies whether the device's hardware is disabled.
    // The PnP Manager only checks this bit right after the device is
    // enumerated. Once the device is started, this bit is ignored.
    //
    deviceCapabilities->HardwareDisabled = FALSE;

    // Our simulated device can be physically removed.
    //
    deviceCapabilities->Removable = TRUE;

	// Setting it to TRUE prevents the warning dialog from appearing
    // whenever the device is surprise removed.
    //
    deviceCapabilities->SurpriseRemovalOK = TRUE;

    // We don't support system-wide unique IDs.
    deviceCapabilities->UniqueID = FALSE;

    // Specify whether the Device Manager should suppress all
    // installation pop-ups except required pop-ups such as
    // "no compatible drivers found."
    //
    deviceCapabilities->SilentInstall = FALSE;

    // Specifies an address indicating where the device is located
    // on its underlying bus. The interpretation of this number is
    // bus-specific. If the address is unknown or the bus driver
    // does not support an address, the bus driver leaves this
    // member at its default value of 0xFFFFFFFF. In this example
    // the location address is same as instance id.
    //
    deviceCapabilities->Address = DeviceData->SerialNo;    // + 10; // chj test

    // UINumber specifies a number associated with the device that can
    // be displayed in the user interface.
    //
    deviceCapabilities->UINumber = DeviceData->SerialNo;

    return STATUS_SUCCESS;
}

NTSTATUS
Bus_PDO_QueryDeviceId(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++
Routine Description:
    Bus drivers must handle BusQueryDeviceID requests for their
    child devices (child PDOs). Bus drivers can handle requests
    BusQueryHardwareIDs, BusQueryCompatibleIDs, and BusQueryInstanceID
    for their child devices.

    When returning more than one ID for hardware IDs or compatible IDs,
    a driver should list the IDs in the order of most specific to most
    general to facilitate choosing the best driver match for the device.

    Bus drivers should be prepared to handle this IRP for a child device
    immediately after the device is enumerated.

Arguments:
    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:
    NT STATUS
--*/
{
    PIO_STACK_LOCATION      stack;
    PWCHAR                  buffer;
    ULONG                   length;
    NTSTATUS                status = STATUS_SUCCESS;
    ULONG_PTR               result;
    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->Parameters.QueryId.IdType) 
	{{
    case BusQueryDeviceID:
        // DeviceID is unique string to identify a device.
        // This can be the same as the hardware ids (which requires a multisz).

        buffer = DeviceData->HardwareIDs; 
			// Sample: "{B85B7C50-6A01-11d2-B841-00C04FAD5171}\MsToaster"

		while(*buffer++ || *buffer++); // concise method from LeiWei
//      while (*(buffer++)) {
//          while (*(buffer++)) {
//              ;
//          }
//      }

        status = RtlULongPtrSub((ULONG_PTR)buffer, (ULONG_PTR)DeviceData->HardwareIDs, &result);
        if (!NT_SUCCESS(status)) {
           break;
		}
        length = (ULONG)result;

        buffer = ExAllocatePoolWithTag (PagedPool, length, BUSENUM_POOL_TAG);

        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        RtlCopyMemory (buffer, DeviceData->HardwareIDs, length);
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        break;

    case BusQueryInstanceID:
        // total length = number (10 digits to be safe (2^32)) + null wide char
        //
        length = 11 * sizeof(WCHAR);

        buffer = ExAllocatePoolWithTag (PagedPool, length, BUSENUM_POOL_TAG);
        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }
        RtlStringCchPrintfW(buffer, length/sizeof(WCHAR), L"%02d", DeviceData->SerialNo /*+20*/); // chj test
        Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_INFO,
                     ("\tInstanceID: %ws\n", buffer));
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        break;


    case BusQueryHardwareIDs:
        // A device has at least one hardware id.
        // In a list of hardware IDs (multi_sz string) for a device,
        // DeviceId is the most specific and should be first in the list.

        buffer = DeviceData->HardwareIDs; 
			// Sample: "{B85B7C50-6A01-11d2-B841-00C04FAD5171}\MsToaster"

        while (*(buffer++)) {
            while (*(buffer++)) {
                ;
            }
        }

        status = RtlULongPtrSub((ULONG_PTR)buffer, (ULONG_PTR)DeviceData->HardwareIDs, &result);
        if (!NT_SUCCESS(status)) {
           break;
        }
        length = (ULONG)result;

        buffer = ExAllocatePoolWithTag (PagedPool, length, BUSENUM_POOL_TAG);
        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }

        RtlCopyMemory (buffer, DeviceData->HardwareIDs, length);
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        break;

    case BusQueryCompatibleIDs:
        // The generic ids for installation of this pdo.
        //
        length = BUSENUM_COMPATIBLE_IDS_LENGTH;
        buffer = ExAllocatePoolWithTag (PagedPool, length, BUSENUM_POOL_TAG);
        if (!buffer) {
           status = STATUS_INSUFFICIENT_RESOURCES;
           break;
        }
        RtlCopyMemory (buffer, BUSENUM_COMPATIBLE_IDS, length);
        Irp->IoStatus.Information = (ULONG_PTR) buffer;
        break;

    default:
        status = Irp->IoStatus.Status;

	}}
    return status;
}

NTSTATUS
Bus_PDO_QueryDeviceText(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++
Routine Description:
    The PnP Manager uses this IRP to get a device's
    description or location information. This string
    is displayed in the "found new hardware" pop-up
    window if no INF match is found for the device.
    Bus drivers are also encouraged to return location
    information for their child devices, but this information
    is optional.

Arguments:
    DeviceData - Pointer to the PDO's device extension.
    Irp        - Pointer to the irp.

Return Value:
    NT STATUS
--*/
{
    PWCHAR  buffer;
    USHORT  length;
    PIO_STACK_LOCATION   stack;
    NTSTATUS    status;

    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->Parameters.QueryDeviceText.DeviceTextType) 
	{{
    case DeviceTextDescription:
        // Check to see if any filter driver has set any information.
        // If so then remain silent otherwise add your description.
        // This string must be localized to support various languages.

        switch (stack->Parameters.QueryDeviceText.LocaleId) 
		{{
        case 0x00000407 : // German
              // Localize the device text.
              // Until we implement let us fallthru to English

        default: // for all other languages, fallthru to English
        case 0x00000409 : // English
            if (!Irp->IoStatus.Information) {
                // 10 for number of digits in the serial number
                length  = (USHORT) \
                (wcslen(VENDORNAME) + 1 + wcslen(MODEL) + 1 + 10) * sizeof(WCHAR);
                buffer = ExAllocatePoolWithTag (PagedPool,
                                            length, BUSENUM_POOL_TAG);
                if (buffer == NULL ) {
                    status = STATUS_INSUFFICIENT_RESOURCES;
                    break;
                }

                RtlStringCchPrintfW(buffer, length/sizeof(WCHAR), 
					L"%ws%ws%02d", VENDORNAME, MODEL, // e.g. "Microsoft_Eliyas_Toaster_01"
                    DeviceData->SerialNo);
					// Chj: When the driver(.inf/.sys) for those toaster devices is not installed on the system, 
					// Windows user will see the device name as "Microsoft_Eliyas_Toaster_01" with yellow exclamation icon.
                Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_TRACE,
                    ("\tDeviceTextDescription :%ws\n", buffer));

                Irp->IoStatus.Information = (ULONG_PTR) buffer;
            }
            status = STATUS_SUCCESS;
            break;
		}} // end of LocalID switch
        break;

    case DeviceTextLocationInformation:
	{
		// Chj add location text demo:
#define LOCTEXT "buspdo.c answers this location for PDO"
#define MAKE_L_TEXT(str) L ## str
#define MAKE_LEXP_TEXT(str) MAKE_L_TEXT(str)
		WCHAR loctext[] = MAKE_LEXP_TEXT(LOCTEXT); // result in L"buspdo.c answers this location for PDO"
		buffer = ExAllocatePoolWithTag (PagedPool,	sizeof(loctext), BUSENUM_POOL_TAG);
		if (buffer == NULL) {
			status = STATUS_INSUFFICIENT_RESOURCES;
			break;
		}
		RtlStringCchCopyW(buffer, sizeof(loctext)/sizeof(WCHAR), loctext);

		Bus_KdPrint_Cont (DeviceData, BUS_DBG_PNP_TRACE,
			("\tDeviceTextLocationInformation: %s\n", LOCTEXT));

		Irp->IoStatus.Information = (ULONG_PTR) buffer;
		status = STATUS_SUCCESS;
		break;
	}

    default:
        status = Irp->IoStatus.Status;
        break;
	}}

    return status;

}

NTSTATUS
Bus_PDO_QueryResources(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in PIRP   Irp
    )
/*++

Routine Description:

    The PnP Manager uses this IRP to get a device's
    boot configuration resources. The bus driver returns
    a resource list in response to this IRP, it allocates
    a CM_RESOURCE_LIST from paged memory. The PnP Manager
    frees the buffer when it is no longer needed.

Arguments:

    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:

    NT STATUS

--*/
{
#if 0
    PCM_RESOURCE_LIST resourceList;
    PCM_FULL_RESOURCE_DESCRIPTOR frd;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR prd;
    ULONG  resourceListSize;
#endif
    PAGED_CODE ();

    UNREFERENCED_PARAMETER(DeviceData);

    return Irp->IoStatus.Status;

    //
    // Following code shows how to provide
    // boot I/O port resource to a device.
    //
#if 0

    resourceListSize = sizeof(CM_RESOURCE_LIST);

    resourceList = ExAllocatePoolWithTag (PagedPool,
                        resourceListSize, BUSENUM_POOL_TAG);

    if (resourceList == NULL ) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory( resourceList, resourceListSize );
    resourceList->Count = 1;
    frd = &resourceList->List[0];

    frd->InterfaceType = InterfaceTypeUndefined;
    frd->BusNumber = 0;

    //
    // Set the number of Partial Resource
    // Descriptors in this FRD.
    //

    frd->PartialResourceList.Count = 1;

    //
    // Get pointer to first Partial Resource
    // Descriptor in this FRD.
    //
    prd = &frd->PartialResourceList.PartialDescriptors[0];
    prd->Type = CmResourceTypePort;

    prd->ShareDisposition = CmResourceShareShared;

    prd->Flags = CM_RESOURCE_PORT_IO | CM_RESOURCE_PORT_16_BIT_DECODE;

    prd->u.Port.Start.QuadPart = 0xBAD; // some random port number
    prd->u.Port.Length = 1;
    Irp->IoStatus.Information = (ULONG_PTR)resourceList;
    return STATUS_SUCCESS;
#endif
}

NTSTATUS
Bus_PDO_QueryResourceRequirements(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++

Routine Description:

    The PnP Manager uses this IRP to get a device's alternate
    resource requirements list. The bus driver returns a resource
    requirements list in response to this IRP, it allocates an
    IO_RESOURCE_REQUIREMENTS_LIST from paged memory. The PnP
    Manager frees the buffer when it is no longer needed.

Arguments:

    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:

    NT STATUS

--*/
{
#if 0
    PIO_RESOURCE_REQUIREMENTS_LIST  resourceList;
    PIO_RESOURCE_DESCRIPTOR descriptor;
    ULONG resourceListSize;
#endif
    NTSTATUS status;

    UNREFERENCED_PARAMETER(DeviceData);
    UNREFERENCED_PARAMETER(Irp);

    PAGED_CODE ();

    //
    // Reporting a I/O port resource may lead to code 12
    // error on IA64 systems because toaster bus is root
    // enumerated and the resource reported here falls into
    // ISA bus. Since no devices claim the ownership of ISA bus
    // on IA64 system, the system fails to allocate this resource.
    // Enabling this code might work on X86 and x64 systems currently,
    // but if the support for ISA is dropped completely in the future
    // then it will start to fail again. So it's a good idea to not
    // claim hardware resources like this unless the driver has a
    // knowledge of or control over a piece of hardware that will
    // actually consume these resources.
    //
#if 0
    //
    // Note the IO_RESOURCE_REQUIREMENTS_LIST structure includes
    // IO_RESOURCE_LIST  List[1]; if we specify more than one
    // resource, we must include IO_RESOURCE_LIST size
    // in the  resourceListSize calculation.
    //
    resourceListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);

    resourceList = ExAllocatePoolWithTag (
                       PagedPool,
                       resourceListSize,
                       BUSENUM_POOL_TAG);

    if (resourceList == NULL ) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        return status;
    }

    RtlZeroMemory( resourceList, resourceListSize );

    resourceList->ListSize = resourceListSize;

    //
    // Initialize the list header.
    //

    resourceList->AlternativeLists = 1;
    resourceList->InterfaceType = InterfaceTypeUndefined;
    resourceList->BusNumber = 0;
    resourceList->List[0].Version = 1;
    resourceList->List[0].Revision = 1;
    resourceList->List[0].Count = 1;
    descriptor = resourceList->List[0].Descriptors;

    //
    // Fill in the information about the ports your device
    // can use.
    //

    descriptor->Type = CmResourceTypePort;
    descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
    descriptor->Flags = CM_RESOURCE_PORT_IO|CM_RESOURCE_PORT_16_BIT_DECODE;
    descriptor->u.Port.Length = 1;
    descriptor->u.Port.Alignment = 0x01;
    descriptor->u.Port.MinimumAddress.QuadPart = 0;
    descriptor->u.Port.MaximumAddress.QuadPart = 0xFFFF;
    Irp->IoStatus.Information = (ULONG_PTR)resourceList;
#endif
    status = STATUS_SUCCESS;

    return status;
}

NTSTATUS
Bus_PDO_QueryDeviceRelations(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++
Routine Description:
    The PnP Manager sends this IRP to gather information about
    devices with a relationship to the specified device.
    Bus drivers must handle this request for TargetDeviceRelation
    for their child devices (child PDOs).

    If a driver returns relations in response to this IRP,
    it allocates a DEVICE_RELATIONS structure from paged
    memory containing a count and the appropriate number of
    device object pointers. The PnP Manager frees the structure
    when it is no longer needed. If a driver replaces a
    DEVICE_RELATIONS structure allocated by another driver,
    it must free the previous structure.

    A driver must reference the PDO of any device that it
    reports in this IRP (ObReferenceObject). The PnP Manager
    removes the reference when appropriate.

Arguments:
    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:
    NT STATUS
--*/
{
	// Chj: PWDM2 p443 列出了几乎相同的代码。
	// PnP manager 发这个请求，说白了就是向一个 Dstack 查询谁是 PDO 。

    PIO_STACK_LOCATION   stack;
    PDEVICE_RELATIONS deviceRelations;
    NTSTATUS status;
    PAGED_CODE ();

    stack = IoGetCurrentIrpStackLocation (Irp);

    switch (stack->Parameters.QueryDeviceRelations.Type) 
	{{
    case TargetDeviceRelation:

        deviceRelations = (PDEVICE_RELATIONS) Irp->IoStatus.Information;
        if (deviceRelations) {
            //
            // Only PDO can handle this request. Somebody above
            // is not playing by rule.
            //
            ASSERTMSG("Someone above is handling TagerDeviceRelation", !deviceRelations);
        }

        deviceRelations = (PDEVICE_RELATIONS)
                ExAllocatePoolWithTag (PagedPool,
                                        sizeof(DEVICE_RELATIONS),
                                        BUSENUM_POOL_TAG);
        if (!deviceRelations) {
                status = STATUS_INSUFFICIENT_RESOURCES;
                break;
        }

        //
        // There is only one PDO pointer in the structure
        // for this relation type. The PnP Manager removes
        // the reference to the PDO when the driver or application
        // un-registers for notification on the device.
        //

        deviceRelations->Count = 1;
        deviceRelations->Objects[0] = DeviceData->Self;
        ObReferenceObject(DeviceData->Self);

        status = STATUS_SUCCESS;
        Irp->IoStatus.Information = (ULONG_PTR) deviceRelations;
        break;

    case BusRelations: // Not handled by PDO
    case EjectionRelations: // optional for PDO
    case RemovalRelations: // // optional for PDO
    default:
        status = Irp->IoStatus.Status;
	}}

    return status;
}

NTSTATUS
Bus_PDO_QueryBusInformation(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++
Routine Description:
    The PnP Manager uses this IRP to request the type and
    instance number of a device's parent bus. Bus drivers
    should handle this request for their child devices (PDOs).

Arguments:
    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:
    NT STATUS
--*/
{
    PPNP_BUS_INFORMATION busInfo;
    UNREFERENCED_PARAMETER(DeviceData);
    PAGED_CODE ();

    busInfo = ExAllocatePoolWithTag (PagedPool, sizeof(PNP_BUS_INFORMATION),
                                        BUSENUM_POOL_TAG);

    if (busInfo == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    busInfo->BusTypeGuid = GUID_DEVCLASS_TOASTER;

    // Some buses have a specific INTERFACE_TYPE value,
    // such as PCMCIABus, PCIBus, or PNPISABus.
    // For other buses, especially newer buses like TOASTER, the bus
    // driver sets this member to PNPBus.
    //
    busInfo->LegacyBusType = PNPBus;

    // This is an hypothetical bus
    //
    busInfo->BusNumber = 0;

    Irp->IoStatus.Information = (ULONG_PTR)busInfo;

    return STATUS_SUCCESS;
}

NTSTATUS
Bus_PDO_QueryInterface(
    __in PPDO_DEVICE_DATA     DeviceData,
    __in  PIRP   Irp
    )
/*++
Routine Description:
    This requests enables a driver to export proprietary interface
    to other drivers. This function and the following 5 routines
    are meant to show how a typical interface is exported.
    Note: This and many other routines in this sample are not required if
    someone is using this sample for just device enumeration purpose.

Arguments:
    DeviceData - Pointer to the PDO's device extension.
    Irp          - Pointer to the irp.

Return Value:
    NT STATUS
--*/
{
   PIO_STACK_LOCATION irpStack;
   PTOASTER_INTERFACE_STANDARD toasterInterfaceStandard;
   GUID *interfaceType;
   NTSTATUS    status = STATUS_SUCCESS;
   PAGED_CODE();

   irpStack = IoGetCurrentIrpStackLocation(Irp);
   interfaceType = (GUID *) irpStack->Parameters.QueryInterface.InterfaceType;
   
   if (IsEqualGUID(interfaceType, (PVOID) &GUID_TOASTER_INTERFACE_STANDARD)) 
   {
      if (irpStack->Parameters.QueryInterface.Size <
                    sizeof(TOASTER_INTERFACE_STANDARD)
                    && irpStack->Parameters.QueryInterface.Version != 1) {
         return STATUS_INVALID_PARAMETER;
      }

      toasterInterfaceStandard = (PTOASTER_INTERFACE_STANDARD)
                                irpStack->Parameters.QueryInterface.Interface;

      toasterInterfaceStandard->InterfaceHeader.Context = DeviceData;

      // Fill in the exported functions
      //
      toasterInterfaceStandard->InterfaceHeader.InterfaceReference   =
                        (PINTERFACE_REFERENCE) Bus_InterfaceReference;
      toasterInterfaceStandard->InterfaceHeader.InterfaceDereference =
                        (PINTERFACE_DEREFERENCE) Bus_InterfaceDereference;
      toasterInterfaceStandard->GetCrispinessLevel = Bus_GetCrispinessLevel;
      toasterInterfaceStandard->SetCrispinessLevel = Bus_SetCrispinessLevel;
      toasterInterfaceStandard->IsSafetyLockEnabled = Bus_IsSafetyLockEnabled;

      // Must take a reference before returning
      //
      Bus_InterfaceReference(DeviceData);
   } else {
        //
        // Interface type not supported
        //
        status = Irp->IoStatus.Status;
   }
   return status;
}

BOOLEAN
Bus_GetCrispinessLevel(
    __in   PVOID Context,
    __out  PUCHAR Level
    )
/*++
Routine Description:
    This routine gets the current crispiness level of the toaster.
Arguments:
    Context        pointer to  PDO device extension
    Level          crispiness level of the device
Return Value:
    TRUE or FALSE
--*/
{
    UNREFERENCED_PARAMETER(Context);
	//
    // Validate the context to see if it's really a pointer
    // to PDO's device extension. You can store some kind
    // of signature in the PDO for this purpose

    Bus_KdPrint ((PPDO_DEVICE_DATA)Context, BUS_DBG_PNP_TRACE,
                                    ("GetCrispinessLevel\n"));
    *Level = 10;
    return TRUE;
}

BOOLEAN
Bus_SetCrispinessLevel(
    __in   PVOID Context,
    __in   UCHAR Level
    )
/*++
Routine Description:
    This routine sets the current crispiness level of the toaster.
Arguments:
    Context        pointer to  PDO device extension
    Level          crispiness level of the device
Return Value:
    TRUE or FALSE
--*/
{
    UNREFERENCED_PARAMETER(Context);
    UNREFERENCED_PARAMETER(Level);

    Bus_KdPrint ((PPDO_DEVICE_DATA)Context, BUS_DBG_PNP_TRACE,
                                    ("SetCrispinessLevel\n"));
    return TRUE;
}

BOOLEAN
Bus_IsSafetyLockEnabled(
    __in PVOID Context
    )
/*++
Routine Description:
    Routine to check whether safety lock is enabled
Arguments:
    Context        pointer to  PDO device extension
Return Value:
    TRUE or FALSE
--*/
{
    UNREFERENCED_PARAMETER(Context);
    
    Bus_KdPrint ((PPDO_DEVICE_DATA)Context, BUS_DBG_PNP_TRACE,
                                    ("IsSafetyLockEnabled\n"));
    return TRUE;
}

VOID
Bus_InterfaceReference (
   __in PVOID Context
   )
/*++
Routine Description:
    This routine increments the refcount. We check this refcount
    during query_remove decide whether to allow the remove or not.
Arguments:
    Context        pointer to  PDO device extension
Return Value:
--*/
{
    InterlockedIncrement((LONG *)&((PPDO_DEVICE_DATA)Context)->ToasterInterfaceRefCount);
}

VOID
Bus_InterfaceDereference (
   __in PVOID Context
   )
/*++
Routine Description:
    This routine decrements the refcount. We check this refcount
    during query_remove decide whether to allow the remove or not.
Arguments:
    Context        pointer to  PDO device extension
Return Value:
--*/
{
    InterlockedDecrement((LONG *)&((PPDO_DEVICE_DATA)Context)->ToasterInterfaceRefCount);
}


NTSTATUS
Bus_GetDeviceCapabilities(
    __in  PDEVICE_OBJECT          DeviceObject,
    __out PDEVICE_CAPABILITIES    DeviceCapabilities
    )
/*++
Routine Description:
    This routine sends the get capabilities irp to the given stack

Arguments:
    DeviceObject        A device object in the stack whose capabilities we want
    DeviceCapabilites   Where to store the answer

Return Value:
    NTSTATUS
--*/
{
    IO_STATUS_BLOCK     ioStatus;
    KEVENT              pnpEvent;
    NTSTATUS            status;
    PDEVICE_OBJECT      targetObject;
    PIO_STACK_LOCATION  irpStack;
    PIRP                pnpIrp;
    PAGED_CODE();

    // Initialize the capabilities that we will send down
    //
    RtlZeroMemory( DeviceCapabilities, sizeof(DEVICE_CAPABILITIES) );
    DeviceCapabilities->Size = sizeof(DEVICE_CAPABILITIES);
    DeviceCapabilities->Version = 1;
    DeviceCapabilities->Address = ULONG_MAX;
    DeviceCapabilities->UINumber = ULONG_MAX;

    // Initialize the event
    KeInitializeEvent( &pnpEvent, NotificationEvent, FALSE );

    targetObject = IoGetAttachedDeviceReference( DeviceObject );

    // Build an Irp
    //
    pnpIrp = IoBuildSynchronousFsdRequest(
        IRP_MJ_PNP,
        targetObject,
        NULL,
        0,
        NULL,
        &pnpEvent,
        &ioStatus
        );
    if (pnpIrp == NULL) {
        status = STATUS_INSUFFICIENT_RESOURCES;
        goto GetDeviceCapabilitiesExit;
    }

    // Pnp Irps all begin life as STATUS_NOT_SUPPORTED;
    pnpIrp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    // Get the top of stack
    irpStack = IoGetNextIrpStackLocation( pnpIrp );

    // Set the top of stack
    RtlZeroMemory( irpStack, sizeof(IO_STACK_LOCATION ) );
    irpStack->MajorFunction = IRP_MJ_PNP;
    irpStack->MinorFunction = IRP_MN_QUERY_CAPABILITIES;
    irpStack->Parameters.DeviceCapabilities.Capabilities = DeviceCapabilities;

    // Call the driver
    //
    status = IoCallDriver( targetObject, pnpIrp );
    if (status == STATUS_PENDING) {
        // Block until the irp comes back.
        // Important thing to note here is when you allocate
        // the memory for an event in the stack you must do a
        // KernelMode wait instead of UserMode to prevent
        // the stack from getting paged out.
        //
        KeWaitForSingleObject(
            &pnpEvent,
            Executive,
            KernelMode,
            FALSE,
            NULL
            );
        status = ioStatus.Status;
    }

GetDeviceCapabilitiesExit:
    // Done with reference
    ObDereferenceObject( targetObject );

    // Done
    return status;
}

