/*++
Step4: This steps shows:
       1) How to register Read and Write events on the default queue.
       2) Retrieve memory from read and write request, format the
          requests and send it to USB target.
--*/

#include <VisualDDKHelpers.h>
#include <ntddk.h>
#include <wdf.h>

#include "prototypes.h"
#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <usbdi.h>
#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdfusb.h>
#include <initguid.h>

DEFINE_GUID(GUID_DEVINTERFACE_OSRUSBFX2, // Generated using guidgen.exe
   0x573e8c73, 0xcb4, 0x4471, 0xa1, 0xbf, 0xfa, 0xb2, 0x6c, 0x31, 0xd3, 0x84);
// {573E8C73-0CB4-4471-A1BF-FAB26C31D384}

#define IOCTL_INDEX                     0x800
#define FILE_DEVICE_OSRUSBFX2          0x65500
#define USBFX2LK_SET_BARGRAPH_DISPLAY 0xD8
#define BULK_OUT_ENDPOINT_INDEX        1
#define BULK_IN_ENDPOINT_INDEX         2
#define IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY CTL_CODE(FILE_DEVICE_OSRUSBFX2,\
                                                    IOCTL_INDEX + 5, \
                                                    METHOD_BUFFERED, \
                                                    FILE_WRITE_ACCESS)
typedef struct _DEVICE_CONTEXT {
  WDFUSBDEVICE      UsbDevice;
  WDFUSBINTERFACE   UsbInterface;
  WDFUSBPIPE        BulkReadPipe;
  WDFUSBPIPE        BulkWritePipe;
} DEVICE_CONTEXT, *PDEVICE_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(DEVICE_CONTEXT, GetDeviceContext)


extern"C" NTSTATUS
DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath
    )
{
    WDF_DRIVER_CONFIG       config;
    NTSTATUS                status;

    KdPrint(("[osrusbfx2]DriverEntry of Step4\n"));

    WDF_DRIVER_CONFIG_INIT(&config, EvtDeviceAdd);

    status = WdfDriverCreate(DriverObject,
                        RegistryPath,
                        WDF_NO_OBJECT_ATTRIBUTES, 
                        &config,     
                        WDF_NO_HANDLE 
                        );

    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDriverCreate failed 0x%x\n", status));
    }

    return status;
}

NTSTATUS
EvtDeviceAdd(
    IN WDFDRIVER        Driver,
    IN PWDFDEVICE_INIT  DeviceInit
    )
{
	PAGED_CODE();

    WDF_OBJECT_ATTRIBUTES               attributes;
    NTSTATUS                            status;
    WDFDEVICE                           device;
    PDEVICE_CONTEXT                     pDevContext;
    WDF_PNPPOWER_EVENT_CALLBACKS        pnpPowerCallbacks;
    WDF_IO_QUEUE_CONFIG                 ioQueueConfig;
    UNREFERENCED_PARAMETER(Driver);

	KdPrint(("[osrusbfx2]EvtDeviceAdd()\n"));

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);
    pnpPowerCallbacks.EvtDevicePrepareHardware = EvtDevicePrepareHardware;
	pnpPowerCallbacks.EvtDeviceReleaseHardware = EvtDeviceReleaseHardware;
	pnpPowerCallbacks.EvtDeviceD0Entry = EvtDeviceD0Entry;
	pnpPowerCallbacks.EvtDeviceD0Exit = EvtDeviceD0Exit;
    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

	// chj test:
	WDF_FILEOBJECT_CONFIG foConfig;
	WDF_FILEOBJECT_CONFIG_INIT(&foConfig, NULL, EvtFileobjectClose, EvtFileobjectCleanup);
	WdfDeviceInitSetFileObjectConfig(DeviceInit, &foConfig, NULL);

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, DEVICE_CONTEXT);

    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreate failed 0x%x\n", status));
        return status;
    }

    pDevContext = GetDeviceContext(device);

    status = WdfDeviceCreateDeviceInterface(device,
                                (LPGUID) &GUID_DEVINTERFACE_OSRUSBFX2,
                                NULL);// Reference String
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfDeviceCreateDeviceInterface failed 0x%x\n", status));
        return status;
    }

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
                                    WdfIoQueueDispatchParallel);

    ioQueueConfig.EvtIoDeviceControl = EvtIoDeviceControl;
    ioQueueConfig.EvtIoRead = EvtIoRead;
    ioQueueConfig.EvtIoWrite = EvtIoWrite;

    status = WdfIoQueueCreate(device,
                         &ioQueueConfig,
                         WDF_NO_OBJECT_ATTRIBUTES,
                         WDF_NO_HANDLE);
    if (!NT_SUCCESS(status)) {
        KdPrint(("WdfIoQueueCreate failed  %!STATUS!\n", status));
        return status;
    }

    return status;
}

void chj_try_usbapi_1(WDFUSBDEVICE usbdevice)
{
	USB_DEVICE_DESCRIPTOR dev_dscpr;
	WdfUsbTargetDeviceGetDeviceDescriptor(usbdevice, &dev_dscpr);

	char buf_cfg_dscpr[1000];
	USHORT bufsize = sizeof(buf_cfg_dscpr);
	WdfUsbTargetDeviceRetrieveConfigDescriptor(usbdevice, buf_cfg_dscpr, &bufsize);

	WDFUSBINTERFACE usbinterface = WdfUsbTargetDeviceGetInterface(usbdevice, 0); // ok

	int altindex_now = WdfUsbInterfaceGetConfiguredSettingIndex(usbinterface);

	int cfg_pipes = WdfUsbInterfaceGetNumConfiguredPipes(usbinterface); // always 0

	WDF_USB_PIPE_INFORMATION pipeinfo; WDF_USB_PIPE_INFORMATION_INIT(&pipeinfo);
	WDFUSBPIPE usbpipe0 = WdfUsbInterfaceGetConfiguredPipe(usbinterface,
		0,//BULK_IN_ENDPOINT_INDEX, (PT850 mod)
		&pipeinfo); // return NULL because of not SelectConfig yet.
}

void chj_try_usbapi_2(WDFUSBDEVICE usbdevice, WDFUSBINTERFACE usbinterface)
{
	int altindex_now = WdfUsbInterfaceGetConfiguredSettingIndex(usbinterface);

	int cfg_pipes = WdfUsbInterfaceGetNumConfiguredPipes(usbinterface);
}

NTSTATUS
EvtDevicePrepareHardware(
    IN WDFDEVICE    Device,
    IN WDFCMRESLIST ResourceList,
    IN WDFCMRESLIST ResourceListTranslated
    )
{
	PAGED_CODE();

    NTSTATUS                            status;
    PDEVICE_CONTEXT                     pDeviceContext;
    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS configParams;

    UNREFERENCED_PARAMETER(ResourceList);
    UNREFERENCED_PARAMETER(ResourceListTranslated);

	KdPrint(("[osrusbfx2]EvtDevicePrepareHardware() wdfdevice=0x%p\n", Device));

	pDeviceContext = GetDeviceContext(Device);

    //
    // Create the USB device if it is not already created.
    //
    if (pDeviceContext->UsbDevice == NULL) {

        status = WdfUsbTargetDeviceCreate(Device,
                                    WDF_NO_OBJECT_ATTRIBUTES,
                                    &pDeviceContext->UsbDevice);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfUsbTargetDeviceCreate failed 0x%x\n", status));        
            return status;
        }
    }

	chj_try_usbapi_1(pDeviceContext->UsbDevice);

	// chj try USB selective suspend:
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS idleSettings;
	WDF_DEVICE_POWER_POLICY_IDLE_SETTINGS_INIT(&idleSettings, 
		IdleCannotWakeFromS0 // IdleUsbSelectiveSuspend
		);
	idleSettings.IdleTimeout = 10000; // 10 seconds idle timeout
	// idleSettings.UserControlOfIdleSettings = IdleDoNotAllowUserControl; // a test
	status = WdfDeviceAssignS0IdleSettings(Device, &idleSettings);
	if (!NT_SUCCESS(status)) {
		// typical: STATUS_POWER_STATE_INVALID(0x2d3)
		KdPrint( ("WdfDeviceAssignS0IdleSettings failed 0x%x\n", status));
		return status;
	}

    WDF_USB_DEVICE_SELECT_CONFIG_PARAMS_INIT_SINGLE_INTERFACE(&configParams);

    status = WdfUsbTargetDeviceSelectConfig(pDeviceContext->UsbDevice,
                                        WDF_NO_OBJECT_ATTRIBUTES,
                                        &configParams); // [in,out]
    if(!NT_SUCCESS(status)) {
		// Chj: When VisualDDK debugging a Win7 VM(USB device is attached to the 
		// VMware Workstation VM), it sometimes fails here, don't know why?
		// Retry loading the driver usually gets the cure.
		// Retry method 1: From devmgmt.msc, Disable then Enable the device.
		// Retry method 2: Unplug USB cable then plug it in again.
        KdPrint(("WdfUsbTargetDeviceSelectConfig failed 0x%x\n", status));
        return status;
    }

    pDeviceContext->UsbInterface =  
                configParams.Types.SingleInterface.ConfiguredUsbInterface;

	chj_try_usbapi_2(pDeviceContext->UsbDevice, pDeviceContext->UsbInterface);

    pDeviceContext->BulkReadPipe = WdfUsbInterfaceGetConfiguredPipe(
                                                  pDeviceContext->UsbInterface,
                                                  0,//BULK_IN_ENDPOINT_INDEX, (PT850 mod)
                                                  NULL);// pipeInfo

    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkReadPipe);
    
    pDeviceContext->BulkWritePipe = WdfUsbInterfaceGetConfiguredPipe(
                                                  pDeviceContext->UsbInterface,
                                                  1,//BULK_OUT_ENDPOINT_INDEX, (PT850 mod)
                                                  NULL);// pipeInfo

    WdfUsbTargetPipeSetNoMaximumPacketSizeCheck(pDeviceContext->BulkWritePipe);
        
    return status;
}

NTSTATUS
EvtDeviceReleaseHardware(
	IN WDFDEVICE    Device,
	IN WDFCMRESLIST ResourceListTranslated
	)
{
	PAGED_CODE();
	UNREFERENCED_PARAMETER(ResourceListTranslated);

	KdPrint(("[osrusbfx2]EvtDeviceReleaseHardware() wdfdevice=0x%p\n", Device));

	return STATUS_SUCCESS;
}

VOID
EvtIoDeviceControl(
    IN WDFQUEUE   Queue,
    IN WDFREQUEST Request,
    IN size_t     OutputBufferLength,
    IN size_t     InputBufferLength,
    IN ULONG      IoControlCode    
    )
{
	PAGED_CODE();

    WDFDEVICE                           device;
    PDEVICE_CONTEXT                     pDevContext;
    size_t                              bytesTransferred = 0;
    NTSTATUS                            status;
    WDF_USB_CONTROL_SETUP_PACKET        controlSetupPacket;
    WDF_MEMORY_DESCRIPTOR               memDesc;
    WDFMEMORY                           memory;
    WDF_REQUEST_SEND_OPTIONS            sendOptions;
     
    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);
    
    device = WdfIoQueueGetDevice(Queue);
    pDevContext = GetDeviceContext(device);

    switch(IoControlCode) {

    case IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY:

        if(InputBufferLength < sizeof(UCHAR)) {
            status = STATUS_BUFFER_OVERFLOW;
            bytesTransferred = sizeof(UCHAR);
            break;
        } 

        status = WdfRequestRetrieveInputMemory(Request, &memory);
        if (!NT_SUCCESS(status)) {
            KdPrint(("WdfRequestRetrieveMemory failed 0x%x", status));
            break;
        }

        WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_SET_BARGRAPH_DISPLAY, // Request
                                        0, // Value
                                        0); // Index                                                        

        WDF_MEMORY_DESCRIPTOR_INIT_HANDLE(&memDesc, memory, NULL);

       //
       // Send the I/O with a timeout to avoid hanging the calling 
       // thread indefinitely.
       //
        WDF_REQUEST_SEND_OPTIONS_INIT(&sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT);

        WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(&sendOptions,
                                         WDF_REL_TIMEOUT_IN_MS(100));
    
        status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                        pDevContext->UsbDevice, 
                                        NULL, // Optional WDFREQUEST
                                        &sendOptions, // PWDF_REQUEST_SEND_OPTIONS
                                        &controlSetupPacket,
                                        &memDesc,
                                        (PULONG)&bytesTransferred);
        if (!NT_SUCCESS(status)) {
            KdPrint(("SendControlTransfer failed 0x%x", status));
            break;
        }
        break;

    default:
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    WdfRequestCompleteWithInformation(Request, status, bytesTransferred);

    return;
}

VOID 
EvtIoRead(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )   
{
	PAGED_CODE();

    WDFUSBPIPE                  pipe;
    NTSTATUS                    status;
    WDFMEMORY                   reqMemory;
    PDEVICE_CONTEXT             pDeviceContext;
    BOOLEAN                     ret;
    
    UNREFERENCED_PARAMETER(Length);

    pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
    
    pipe = pDeviceContext->BulkReadPipe;
    
    status = WdfRequestRetrieveOutputMemory(Request, &reqMemory);
    if(!NT_SUCCESS(status)){
        goto Exit;
    }
   
    status = WdfUsbTargetPipeFormatRequestForRead(pipe,
                                        Request,
                                        reqMemory,
                                        NULL // Offsets
                                        ); 
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(Request,
                            EvtRequestReadCompletionRoutine,
                            pipe);
    ret = WdfRequestSend(Request, 
                    WdfUsbTargetPipeGetIoTarget(pipe), 
                    WDF_NO_SEND_OPTIONS);
    
    if (ret == FALSE) {
        status = WdfRequestGetStatus(Request);
        goto Exit;
    } else {
        return;
    }
   
Exit:
    WdfRequestCompleteWithInformation(Request, status, 0);

    return;
}

VOID
EvtRequestReadCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
{    
//	PAGED_CODE(); // seen DISPATCH_LEVEL

    NTSTATUS    status;
    size_t      bytesRead = 0;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    status = CompletionParams->IoStatus.Status;
    
    usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
    
    bytesRead =  usbCompletionParams->Parameters.PipeRead.Length;
    
    if (NT_SUCCESS(status)){
        KdPrint(("Number of bytes read: %I64d\n", (INT64)bytesRead));  
    } else {
        KdPrint(("Read failed - request status 0x%x , UsbdStatus 0x%x %s\n",
                status, usbCompletionParams->UsbdStatus,
				status==STATUS_CANCELLED ? "(I/O cancelled)" : ""
				));
    }

	// chj test >>>
	size_t bytesRet = 0;
	void *pmem = WdfMemoryGetBuffer(usbCompletionParams->Parameters.PipeRead.Buffer, &bytesRet);
		// peak into the returned content

	WDFMEMORY mem_out; size_t outsizeRet = 0;
	WdfRequestRetrieveOutputMemory(Request, &mem_out);
	void *pmem_out = WdfMemoryGetBuffer(mem_out, &outsizeRet);

	ASSERT(pmem_out == pmem);

	ASSERT(mem_out == usbCompletionParams->Parameters.PipeRead.Buffer);
	// chj test <<<

    WdfRequestCompleteWithInformation(Request, status, bytesRead);

    return;
}

bool chj_try_usbreset(WDFUSBDEVICE usbdevice, WDFMEMORY reqMemory)
{
	size_t bytes = 0;
	const char *pInBuf = (char*)WdfMemoryGetBuffer(reqMemory, &bytes);

	if(bytes==1 && *pInBuf=='R') // 0x52
	{
		WdfUsbTargetDeviceResetPortSynchronously(usbdevice);
		return true;
	}
	else if(bytes==1 && *pInBuf=='S') // 0x53
	{
		WdfUsbTargetDeviceCyclePortSynchronously(usbdevice);
		return true;
	}
	else
		return false;
}

VOID 
EvtIoWrite(
    IN WDFQUEUE         Queue,
    IN WDFREQUEST       Request,
    IN size_t           Length
    )   
{
	PAGED_CODE();

    NTSTATUS                    status;
    WDFUSBPIPE                  pipe;
    WDFMEMORY                   reqMemory;
    PDEVICE_CONTEXT             pDeviceContext;
    BOOLEAN                     ret;
    
    UNREFERENCED_PARAMETER(Length);

    pDeviceContext = GetDeviceContext(WdfIoQueueGetDevice(Queue));
    
    pipe = pDeviceContext->BulkWritePipe;

    status = WdfRequestRetrieveInputMemory(Request, &reqMemory);
    if(!NT_SUCCESS(status)){
        goto Exit;
    }

	if(chj_try_usbreset(pDeviceContext->UsbDevice, reqMemory)) {
		WdfRequestCompleteWithInformation(Request, STATUS_SUCCESS, 0);
		return;
	}

    status = WdfUsbTargetPipeFormatRequestForWrite(pipe,
                                              Request,
                                              reqMemory,
                                              NULL); // Offset
    if (!NT_SUCCESS(status)) {
        goto Exit;
    }

    WdfRequestSetCompletionRoutine(
                            Request,
                            EvtRequestWriteCompletionRoutine,
                            pipe);
    ret = WdfRequestSend(Request, 
                    WdfUsbTargetPipeGetIoTarget(pipe), 
                    WDF_NO_SEND_OPTIONS);
    if (ret == FALSE) {
        status = WdfRequestGetStatus(Request);
        goto Exit;
    } else {
        return;
    }

Exit:    
    WdfRequestCompleteWithInformation(Request, status, 0);

    return;
}

VOID
EvtRequestWriteCompletionRoutine(
    IN WDFREQUEST                  Request,
    IN WDFIOTARGET                 Target,
    PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
    IN WDFCONTEXT                  Context
    )
{
//	PAGED_CODE(); // seen DISPATCH_LEVEL

    NTSTATUS    status;
    size_t      bytesWritten = 0;
    PWDF_USB_REQUEST_COMPLETION_PARAMS usbCompletionParams;

    UNREFERENCED_PARAMETER(Target);
    UNREFERENCED_PARAMETER(Context);

    status = CompletionParams->IoStatus.Status;

    usbCompletionParams = CompletionParams->Parameters.Usb.Completion;
    
    bytesWritten =  usbCompletionParams->Parameters.PipeWrite.Length;
    
    if (NT_SUCCESS(status)){
        KdPrint(("Number of bytes written: %I64d\n", (INT64)bytesWritten));        
    } else {
        KdPrint(("Write failed: request Status 0x%x UsbdStatus 0x%x\n", 
                status, usbCompletionParams->UsbdStatus));
    }

    WdfRequestCompleteWithInformation(Request, status, bytesWritten);

    return;
}

void EvtFileobjectCleanup(WDFFILEOBJECT  FileObject)
{
	KdPrint(("[osrusbfx2]EvtFileobjectCleanup() wdffileobject=0x%p\n", FileObject));
}	

void EvtFileobjectClose(WDFFILEOBJECT  FileObject)
{
	KdPrint(("[osrusbfx2]EvtFileobjectClose() wdffileobject=0x%p\n", FileObject));
}	


PCHAR
DbgDevicePowerString(
    IN WDF_POWER_DEVICE_STATE Type
    )
{
    PAGED_CODE();

    switch (Type)
    {
    case WdfPowerDeviceInvalid:
        return "WdfPowerDeviceInvalid";
    case WdfPowerDeviceD0:
        return "WdfPowerDeviceD0";
    case PowerDeviceD1:
        return "WdfPowerDeviceD1";
    case WdfPowerDeviceD2:
        return "WdfPowerDeviceD2";
    case WdfPowerDeviceD3:
        return "WdfPowerDeviceD3";
    case WdfPowerDeviceD3Final:
        return "WdfPowerDeviceD3Final";
    case WdfPowerDevicePrepareForHibernation:
        return "WdfPowerDevicePrepareForHibernation";
    case WdfPowerDeviceMaximum:
        return "PowerDeviceMaximum";
    default:
        return "UnKnown Device Power State";
    }
}

NTSTATUS
EvtDeviceD0Entry(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   RecentPowerState
    )
{
	PAGED_CODE();
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(RecentPowerState);

    KdPrint(("[osrusbfx2]EvtDeviceD0Entry - coming from %s\n",
              DbgDevicePowerString(RecentPowerState)));

    return STATUS_SUCCESS;
}

NTSTATUS
EvtDeviceD0Exit(
    IN WDFDEVICE                Device,
    IN WDF_POWER_DEVICE_STATE   PowerState
    )
{
    UNREFERENCED_PARAMETER(Device);
    UNREFERENCED_PARAMETER(PowerState);
    PAGED_CODE();

    KdPrint(("[osrusbfx2]EvtDeviceD0Exit - to %s\n",
              DbgDevicePowerString(PowerState)));

    return STATUS_SUCCESS;
}
