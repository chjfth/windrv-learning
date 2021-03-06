/*++
Copyright (c) Microsoft Corporation.  All rights reserved.
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    Ioctl.c

Abstract:
    USB device driver for OSR USB-FX2 Learning Kit

Environment:
    Kernel mode only
--*/

#include <osrusbfx2.h>

#if defined(EVENT_TRACING)
#include "ioctl.tmh"
#endif

#pragma alloc_text(PAGE, OsrFxEvtIoDeviceControl)
#pragma alloc_text(PAGE, AbortPipes)
#pragma alloc_text(PAGE, ResetPipe)
#pragma alloc_text(PAGE, ResetDevice)
#pragma alloc_text(PAGE, ReenumerateDevice)
#pragma alloc_text(PAGE, GetBarGraphState)
#pragma alloc_text(PAGE, SetBarGraphState)
#pragma alloc_text(PAGE, GetSevenSegmentState)
#pragma alloc_text(PAGE, SetSevenSegmentState)
#pragma alloc_text(PAGE, GetSwitchState)

VOID
OsrFxEvtIoDeviceControl(
    __in WDFQUEUE   Queue,
    __in WDFREQUEST Request,
    __in size_t     OutputBufferLength,
    __in size_t     InputBufferLength,
    __in ULONG      IoControlCode    
    )
/*++
Routine Description:
    This event is called when the framework receives IRP_MJ_DEVICE_CONTROL
    requests from the system.

Arguments:

    Queue - Handle to the framework queue object that is associated
            with the I/O request.
    Request - Handle to a framework request object.

    OutputBufferLength - length of the request's output buffer,
                        if an output buffer is available.
    InputBufferLength - length of the request's input buffer,
                        if an input buffer is available.

    IoControlCode - the driver-defined or system-defined I/O control code
                    (IOCTL) that is associated with the request.
--*/
{
    WDFDEVICE           device;
    PDEVICE_CONTEXT     pDevContext;
    size_t              bytesReturned = 0;
    PBAR_GRAPH_STATE    barGraphState = NULL;
    PSWITCH_STATE       switchState = NULL;
    PUCHAR              sevenSegment = NULL;
    BOOLEAN             requestPending = FALSE;
    NTSTATUS            status = STATUS_INVALID_DEVICE_REQUEST;

    UNREFERENCED_PARAMETER(InputBufferLength);
    UNREFERENCED_PARAMETER(OutputBufferLength);

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "--> OsrFxEvtIoDeviceControl\n");
    //
    // initialize variables
    //
    device = WdfIoQueueGetDevice(Queue);
    pDevContext = GetDeviceContext(device);

    switch(IoControlCode) {

    case IOCTL_OSRUSBFX2_GET_CONFIG_DESCRIPTOR: {
		unsigned char confd_all[1024];
        PUSB_CONFIGURATION_DESCRIPTOR   configurationDescriptor = (PUSB_CONFIGURATION_DESCRIPTOR)confd_all;
        USHORT                          size_confd_all = 0;
		void *p_outputbuffer = NULL;
		const size_t size_confd0 = sizeof(USB_CONFIGURATION_DESCRIPTOR);
		size_t size_user_obuffer = 0;

        //
        // First get the size of the config descriptor
        //
        status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                                    pDevContext->UsbDevice,
                                    NULL,
                                    &size_confd_all);

        if (status != STATUS_BUFFER_TOO_SMALL) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "WdfUsbTargetDeviceRetrieveConfigDescriptor failed 0x%x\n", status);
            break;
        }

        //
        // Get the buffer - make sure the buffer is big enough
        //
//         status = WdfRequestRetrieveOutputBuffer(Request,
//                                         (size_t)requiredSize,  // MinimumRequired
//                                         &configurationDescriptor,
//                                         NULL);
		// Chj: modified the code to allow return only standalone-device-descriptor(only 9 bytes required).

		status = WdfRequestRetrieveOutputBuffer(Request, 1, &p_outputbuffer, &size_user_obuffer);
        if(!NT_SUCCESS(status)){
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestRetrieveOutputBuffer failed 0x%x (first-try)!\n", status);
            break;
        }

        status = WdfUsbTargetDeviceRetrieveConfigDescriptor(
                                        pDevContext->UsbDevice,
                                        configurationDescriptor,
                                        &size_confd_all);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "WdfUsbTargetDeviceRetrieveConfigDescriptor failed 0x%x (Unexpect!)\n", status);
            break;
        }

		if(size_user_obuffer<size_confd0) {
			TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
				"WdfRequestRetrieveOutputBuffer failed, user buffer<%d (not enough for a standalone-device-descriptor)!\n", size_confd0);

			if(size_user_obuffer>0)
				*(unsigned char*)p_outputbuffer = confd_all[0]; // no effect to app-layer caller
			bytesReturned = 1; 
				// [Win7] Actually, when Windows sees you return STATUS_BUFFER_TOO_SMALL, 
				// it will always report *lpBytesReturned=0 to the app-layer caller,
				// and, data written to p_outputbuffer[] is also discarded(not seen by caller).

			status = STATUS_BUFFER_TOO_SMALL;
		}
		else if(size_user_obuffer<size_confd_all) {
			// can only return standalone-device-descriptor(9 bytes)
			memcpy(p_outputbuffer, confd_all, size_confd0);
			bytesReturned = size_confd0;
			status = STATUS_BUFFER_OVERFLOW;
		}
		else {
			// return all device-descriptor cluster 
			memcpy(p_outputbuffer, confd_all, size_confd_all);
			bytesReturned = size_confd_all;
			status = STATUS_SUCCESS;
		}

        break;
		} // case

    case IOCTL_OSRUSBFX2_RESET_DEVICE:

        status = ResetDevice(device);
        break;

    case IOCTL_OSRUSBFX2_REENUMERATE_DEVICE:
        //
        // call our function to reenumerate the  device
        //
        status = ReenumerateDevice(pDevContext);

        bytesReturned = 0;
        break;

    case IOCTL_OSRUSBFX2_GET_BAR_GRAPH_DISPLAY:
        //
        // Make sure the caller's output buffer is large enough
        //  to hold the state of the bar graph
        //
        status = WdfRequestRetrieveOutputBuffer(Request,
                            sizeof(BAR_GRAPH_STATE),
                            &barGraphState,
                            NULL);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "User's output buffer is too small for this IOCTL, expecting an BAR_GRAPH_STATE\n");
            bytesReturned = sizeof(BAR_GRAPH_STATE);
            break;
        }
        //
        // Call our function to get the bar graph state
        //
        status = GetBarGraphState(pDevContext, barGraphState);

        //
        // If we succeeded return the user their data
        //
        if (NT_SUCCESS(status)) {

            bytesReturned = sizeof(BAR_GRAPH_STATE);

        } else {

            bytesReturned = 0;

        }
        break;

    case IOCTL_OSRUSBFX2_SET_BAR_GRAPH_DISPLAY:

        status = WdfRequestRetrieveInputBuffer(Request,
                            sizeof(BAR_GRAPH_STATE),
                            &barGraphState,
                            NULL);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "User's input buffer is too small for this IOCTL, expecting an BAR_GRAPH_STATE\n");
            bytesReturned = sizeof(BAR_GRAPH_STATE);
            break;
        }

        //
        // Call our routine to set the bar graph state
        //
        status = SetBarGraphState(pDevContext, barGraphState);

        //
        // There's no data returned for this call
        //
        bytesReturned = 0;
        break;

    case IOCTL_OSRUSBFX2_GET_7_SEGMENT_DISPLAY:

        status = WdfRequestRetrieveOutputBuffer(Request,
                            sizeof(UCHAR),
                            &sevenSegment,
                            NULL);

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "User's output buffer is too small for this IOCTL, expecting an UCHAR\n");
            bytesReturned = sizeof(UCHAR);
            break;
        }

        //
        // Call our function to get the 7 segment state
        //
        status = GetSevenSegmentState(pDevContext, sevenSegment);

        //
        // If we succeeded return the user their data
        //
        if (NT_SUCCESS(status)) {
            bytesReturned = sizeof(UCHAR);
        } else {
            bytesReturned = 0;
        }
        break;

    case IOCTL_OSRUSBFX2_SET_7_SEGMENT_DISPLAY:

        status = WdfRequestRetrieveInputBuffer(Request,
                            sizeof(UCHAR),
                            &sevenSegment,
                            NULL);
        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "User's input buffer is too small for this IOCTL, expecting an UCHAR\n");
            bytesReturned = sizeof(UCHAR);
            break;
        }

        //
        // Call our routine to set the 7 segment state
        //
        status = SetSevenSegmentState(pDevContext, sevenSegment);

        //
        // There's no data returned for this call
        //
        bytesReturned = 0;
        break;

    case IOCTL_OSRUSBFX2_READ_SWITCHES:

        status = WdfRequestRetrieveOutputBuffer(Request,
                            sizeof(SWITCH_STATE),
                            &switchState,
                            NULL);// BufferLength

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "User's output buffer is too small for this IOCTL, expecting a SWITCH_STATE\n");
            bytesReturned = sizeof(SWITCH_STATE);
            break;

        }

        // Call our routine to get the state of the switches
        status = GetSwitchState(pDevContext, switchState);

        // If successful, return the user their data
        //
        if (NT_SUCCESS(status)) {
            bytesReturned = sizeof(SWITCH_STATE);
        } else {
            // Don't return any data
            bytesReturned = 0;
        }
        break;

    case IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE:
        //
        // Forward the request to an interrupt message queue and dont complete
        // the request until an interrupt from the USB device occurs.
        //
        status = WdfRequestForwardToIoQueue(Request, pDevContext->InterruptMsgQueue);
        if (NT_SUCCESS(status)) {
            requestPending = TRUE;
        }
        break;

	case IOCTL_OSRUSBFX2_EnableDelayIdle:
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "== Ioctl Enable DelayIdle ==\n");
		pDevContext->DelayIdle = TRUE;
		status = STATUS_SUCCESS; bytesReturned = 0;
		break;
	case IOCTL_OSRUSBFX2_DisableDelayIdle:
		TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "== Ioctl Disable DelayIdle ==\n");
		pDevContext->DelayIdle = FALSE;
		status = STATUS_SUCCESS; bytesReturned = 0;
		break;

    default :
        status = STATUS_INVALID_DEVICE_REQUEST;
        break;
    }

    if (requestPending == FALSE) {
        WdfRequestCompleteWithInformation(Request, status, bytesReturned);
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "<-- OsrFxEvtIoDeviceControl\n");

    return;
}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
ResetPipe(                // not used in this program
    __in WDFUSBPIPE Pipe
    )
/*++
Routine Description:
    This routine resets the pipe.

Arguments:
    Pipe - framework pipe handle

Return Value:
    NT status value
--*/
{
    NTSTATUS          status;
    PAGED_CODE();

    //
    //  This routine synchronously submits a URB_FUNCTION_RESET_PIPE
    // request down the stack.
    //
    status = WdfUsbTargetPipeResetSynchronously(Pipe,
                                WDF_NO_HANDLE, // WDFREQUEST
                                NULL // PWDF_REQUEST_SEND_OPTIONS
                                );

    if (NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "ResetPipe - success\n");
        status = STATUS_SUCCESS;
    }
    else {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL, "ResetPipe - failed\n");
    }

    return status;
}

VOID
StopAllPipes(
    IN PDEVICE_CONTEXT DeviceContext
    )
{
    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe),
                                 WdfIoTargetCancelSentIo);           
    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkReadPipe),
                                 WdfIoTargetCancelSentIo);        
    WdfIoTargetStop(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkWritePipe),
                                 WdfIoTargetCancelSentIo);
}

VOID
StartAllPipes(
    IN PDEVICE_CONTEXT DeviceContext
    )
{
    WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->InterruptPipe));
    WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkReadPipe));
    WdfIoTargetStart(WdfUsbTargetPipeGetIoTarget(DeviceContext->BulkWritePipe));
}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
ResetDevice(
    __in WDFDEVICE Device
    )
/*++
Routine Description:
    This routine calls WdfUsbTargetDeviceResetPortSynchronously to reset the device if it's still
    connected.

Arguments:
    Device - Handle to a framework device

Return Value:
    NT status value
--*/
{
    PDEVICE_CONTEXT pDeviceContext;
    NTSTATUS status;
    PAGED_CODE();
 
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "--> ResetDevice\n");

    pDeviceContext = GetDeviceContext(Device);

    StopAllPipes(pDeviceContext);
    
    status = WdfUsbTargetDeviceResetPortSynchronously(pDeviceContext->UsbDevice);
    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL, "ResetDevice failed - 0x%x\n", status);
    }
    
    StartAllPipes(pDeviceContext);
    
    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "<-- ResetDevice\n");
    return status;
}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
AbortPipes(                       // This function is not used, so removed in WDK10.
    __in WDFDEVICE Device
    )
/*++

Routine Description

    sends an abort pipe request on all open pipes.

Arguments:

    Device - Handle to a framework device

Return Value:

    NT status value

--*/
{
    UCHAR              i;
    ULONG              count;
    NTSTATUS           status;
    PDEVICE_CONTEXT    pDevContext;

    PAGED_CODE();

    //
    // initialize variables
    //
    pDevContext = GetDeviceContext(Device);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "--> AbortPipes \n");

    count = WdfUsbInterfaceGetNumConfiguredPipes(pDevContext->UsbInterface);

    for (i = 0; i < count; i++) {
        WDFUSBPIPE pipe;

        pipe = WdfUsbInterfaceGetConfiguredPipe(
            pDevContext->UsbInterface,
            i, //PipeIndex,
            NULL
            );
        TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "Aborting open pipe %d\n", i);

        status = WdfUsbTargetPipeAbortSynchronously(pipe,
                                    WDF_NO_HANDLE, // WDFREQUEST
                                    NULL);//PWDF_REQUEST_SEND_OPTIONS

        if (!NT_SUCCESS(status)) {
            TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "WdfUsbTargetPipeAbortSynchronously failed %x\n", status);
            break;
        }
    }

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_IOCTL, "<-- AbortPipes \n");

    return STATUS_SUCCESS;
}


__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS 
ReenumerateDevice(
    __in PDEVICE_CONTEXT DevContext
    )
/*++

Routine Description

    This routine resets the OSR USB FX2 development board. This should
    result in the default driver being loaded the next time
    the board is plugged in

Arguments:

    pDevContext - One of our device extensions

Return Value:

    NT status value

--*/
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS        sendOptions;
    GUID                            activity;
    
    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,"--> ReenumerateDevice\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(
                                  &sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT
                                  );

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
                                         &sendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT
                                         );
              
    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_REENUMERATE, // Request
                                        0, // Value
                                        0); // Index


    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                        DevContext->UsbDevice,
                                        WDF_NO_HANDLE, // Optional WDFREQUEST
                                        &sendOptions,
                                        &controlSetupPacket,
                                        NULL, // MemoryDescriptor
                                        NULL); // BytesTransferred

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "ReenumerateDevice: Failed to Reenumerate - 0x%x \n", status);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,"<-- ReenumerateDevice\n");

    //
    // Send event to eventlog
    //

    activity = DeviceToActivityId(WdfObjectContextGetObject(DevContext));
    EventWriteDeviceReenumerated(&activity,
                                 DevContext->DeviceName,
                                 DevContext->Location,
                                 status);
   
    return status;

}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS 
GetBarGraphState(
    __in PDEVICE_CONTEXT DevContext, 
    __out PBAR_GRAPH_STATE BarGraphState
    )
/*++

Routine Description

    This routine gets the state of the bar graph on the board

Arguments:

    DevContext - One of our device extensions

Return Value:

    NT status value

--*/
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS        sendOptions;
    WDF_MEMORY_DESCRIPTOR memDesc;
    ULONG    bytesTransferred;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "--> GetBarGraphState\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(
                                  &sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT
                                  );

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
                                         &sendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT
                                         );

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        USBFX2LK_READ_BARGRAPH_DISPLAY, // Request
                                        0, // Value
                                        0); // Index

    //
    // Set the buffer to 0, the board will OR in everything that is set
    //
    BarGraphState->BarsAsUChar = 0;


    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
                                                BarGraphState,
                                                sizeof(BAR_GRAPH_STATE));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                        DevContext->UsbDevice,
                                        WDF_NO_HANDLE, // Optional WDFREQUEST
                                        &sendOptions,
                                        &controlSetupPacket,
                                        &memDesc,
                                        &bytesTransferred);

    if(!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "GetBarGraphState: Failed - 0x%x \n", status);

    } else {

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
            "GetBarGraphState: LED mask is 0x%x\n", BarGraphState->BarsAsUChar);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "<-- GetBarGraphState\n");

    return status;

}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS 
SetBarGraphState(
    __in PDEVICE_CONTEXT DevContext, 
    __in PBAR_GRAPH_STATE BarGraphState
    )
/*++

Routine Description

    This routine sets the state of the bar graph on the board

Arguments:

    DevContext - One of our device extensions

Return Value:

    NT status value

--*/
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS        sendOptions;
    WDF_MEMORY_DESCRIPTOR memDesc;
    ULONG    bytesTransferred;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "--> SetBarGraphState\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(
                                  &sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT
                                  );

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
                                         &sendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT
                                         );

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_SET_BARGRAPH_DISPLAY, // Request
                                        0, // Value
                                        0); // Index

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
                                                BarGraphState,
                                                sizeof(BAR_GRAPH_STATE));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                        DevContext->UsbDevice,
                                        NULL, // Optional WDFREQUEST
                                        &sendOptions,
                                        &controlSetupPacket,
                                        &memDesc,
                                        &bytesTransferred);

    if(!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                        "SetBarGraphState: Failed - 0x%x \n", status);

    } else {

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
            "SetBarGraphState: LED mask is 0x%x\n", BarGraphState->BarsAsUChar);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "<-- SetBarGraphState\n");

    return status;

}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS 
GetSevenSegmentState(
    __in PDEVICE_CONTEXT DevContext, 
    __out PUCHAR SevenSegment
    )
/*++
Routine Description
    This routine gets the state of the 7 segment display on the board
    by sending a synchronous control command.

    NOTE: It's not a good practice to send a synchronous request in the
          context of the user thread because if the transfer takes long
          time to complete, you end up holding the user thread.

          I'm choosing to do synchronous transfer because a) I know this one
          completes immediately b) and for demonstration.

Arguments:
    DevContext - One of our device extensions

Return Value:
    NT status value
--*/
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS        sendOptions;
    
    WDF_MEMORY_DESCRIPTOR memDesc;
    ULONG    bytesTransferred;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "GetSetSevenSegmentState: Enter\n");

    PAGED_CODE();

    WDF_REQUEST_SEND_OPTIONS_INIT(
                                  &sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT
                                  );

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
                                         &sendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT
                                         );

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        USBFX2LK_READ_7SEGMENT_DISPLAY, // Request
                                        0, // Value
                                        0); // Index

    //
    // Set the buffer to 0, the board will OR in everything that is set
    //
    *SevenSegment = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
                                    SevenSegment,
                                    sizeof(UCHAR));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                    DevContext->UsbDevice,
                                    NULL, // Optional WDFREQUEST
                                    &sendOptions,
                                    &controlSetupPacket,
                                    &memDesc,
                                    &bytesTransferred);

    if(!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
            "GetLEDState: Failed to get 7 Segment state - 0x%x \n", status);
    } else {

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
            "GetLEDState: 7 Segment mask is 0x%x\n", *SevenSegment);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "GetSetSevenSegmentState: Exit\n");
    return status;
}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS 
SetSevenSegmentState(
    __in PDEVICE_CONTEXT DevContext, 
    __in PUCHAR SevenSegment
    )
/*++
Routine Description
    This routine sets the state of the 7 segment display on the board

Arguments:
    DevContext - One of our device extensions

Return Value:
    NT status value
--*/
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS        sendOptions;
    WDF_MEMORY_DESCRIPTOR memDesc;
    ULONG    bytesTransferred;

    PAGED_CODE();

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "--> SetSevenSegmentState\n");

    WDF_REQUEST_SEND_OPTIONS_INIT(
                                  &sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT
                                  );

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
                                         &sendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT
                                         );

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestHostToDevice,
                                        BmRequestToDevice,
                                        USBFX2LK_SET_7SEGMENT_DISPLAY, // Request
                                        0, // Value
                                        0); // Index

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
                                    SevenSegment,
                                    sizeof(UCHAR));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                                DevContext->UsbDevice,
                                                NULL, // Optional WDFREQUEST
                                                &sendOptions,
                                                &controlSetupPacket,
                                                &memDesc,
                                                &bytesTransferred);

    if(!NT_SUCCESS(status)) {

        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
            "SetLEDState: Failed to set 7 Segment state - 0x%x \n", status);

    } else {

        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
            "SetLEDState: 7 Segment mask is 0x%x\n", *SevenSegment);

    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "<-- SetSevenSegmentState\n");

    return status;

}

__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS 
GetSwitchState(
    __in PDEVICE_CONTEXT DevContext, 
    __in PSWITCH_STATE SwitchState
    )
/*++
Routine Description
    This routine gets the state of the switches on the board

Arguments:
    DevContext - One of our device extensions

Return Value:
    NT status value
--*/
{
    NTSTATUS status;
    WDF_USB_CONTROL_SETUP_PACKET    controlSetupPacket;
    WDF_REQUEST_SEND_OPTIONS        sendOptions;
    WDF_MEMORY_DESCRIPTOR memDesc;
    ULONG    bytesTransferred;

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "--> GetSwitchState\n");

    PAGED_CODE();

    WDF_REQUEST_SEND_OPTIONS_INIT(
                                  &sendOptions,
                                  WDF_REQUEST_SEND_OPTION_TIMEOUT
                                  );

    WDF_REQUEST_SEND_OPTIONS_SET_TIMEOUT(
                                         &sendOptions,
                                         DEFAULT_CONTROL_TRANSFER_TIMEOUT
                                         );

    WDF_USB_CONTROL_SETUP_PACKET_INIT_VENDOR(&controlSetupPacket,
                                        BmRequestDeviceToHost,
                                        BmRequestToDevice,
                                        USBFX2LK_READ_SWITCHES, // Request
                                        0, // Value
                                        0); // Index

    SwitchState->SwitchesAsUChar = 0;

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memDesc,
                                    SwitchState,
                                    sizeof(SWITCH_STATE));

    status = WdfUsbTargetDeviceSendControlTransferSynchronously(
                                                DevContext->UsbDevice,
                                                NULL, // Optional WDFREQUEST
                                                &sendOptions,
                                                &controlSetupPacket,
                                                &memDesc,
                                                &bytesTransferred);

    if(!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                "GetSwitchState: Failed to Get switches - 0x%x \n", status);

    } else {
        TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL,
            "GetSwitchState: Switch mask is 0x%x\n", SwitchState->SwitchesAsUChar);
    }

    TraceEvents(TRACE_LEVEL_VERBOSE, DBG_IOCTL, "<-- GetSwitchState\n");
    return status;
}


VOID
OsrUsbIoctlGetInterruptMessage( // Chj: better named: OsrUsb_WaitEp1_complete
    __in WDFDEVICE Device,
	__in NTSTATUS ReaderStatus // this new param is from wdk10 update.
    )
/*++
Routine Description
    This method handles the completion of the pended request for the IOCTL
    IOCTL_OSRUSBFX2_GET_INTERRUPT_MESSAGE.

Arguments:
    Device - Handle to a framework device.
--*/
{
    NTSTATUS            status;
    WDFREQUEST          request;
    PDEVICE_CONTEXT     pDevContext;
    size_t              bytesReturned = 0;
    PSWITCH_STATE       switchState = NULL;

    pDevContext = GetDeviceContext(Device);

    do {
        //
        // Check if there are any pending requests in the Interrupt Message Queue.
        // If a request is found then complete the pending request.
        //
        status = WdfIoQueueRetrieveNextRequest(pDevContext->InterruptMsgQueue, &request);

        if (NT_SUCCESS(status)) {
            status = WdfRequestRetrieveOutputBuffer(request,
                                                    sizeof(SWITCH_STATE),
                                                    &switchState,
                                                    NULL);// BufferLength

            if (!NT_SUCCESS(status)) {

                TraceEvents(TRACE_LEVEL_ERROR, DBG_IOCTL,
                    "User's output buffer is too small for this IOCTL, expecting a SWITCH_STATE\n");
                bytesReturned = 0; // sizeof(SWITCH_STATE); // fix for WDK10 sample

            } else {
                //
                // Copy the state information saved by the continuous reader.
                //
				if(NT_SUCCESS(ReaderStatus)) {
					switchState->SwitchesAsUChar = pDevContext->CurrentSwitchState;
					bytesReturned = sizeof(SWITCH_STATE);
				} else {
					bytesReturned = 0;
				}
            }

            WdfRequestCompleteWithInformation(request, 
				NT_SUCCESS(status) ? ReaderStatus : status, 
				bytesReturned);

            status = STATUS_SUCCESS; // to retrieve next wdfrequest from queue.

        } else if (status != STATUS_NO_MORE_ENTRIES) {
            KdPrint(("WdfIoQueueRetrieveNextRequest got error-status %08x\n", status));
        }

        request = NULL;

    } while (status == STATUS_SUCCESS);

    return;
}


