/*++
Copyright (c) Microsoft Corporation.  All rights reserved.
    THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
    KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
    IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
    PURPOSE.

Module Name:
    Interrupt.c

Abstract:
    This modules has routines configure a continuous reader on an
    interrupt pipe to asynchronously read toggle switch states.

Environment:
    Kernel mode
--*/

#include <osrusbfx2.h>

#if defined(EVENT_TRACING)
#include "interrupt.tmh"
#endif


__drv_requiresIRQL(PASSIVE_LEVEL)
NTSTATUS
OsrFxConfigContReaderForInterruptEndPoint(
    __in PDEVICE_CONTEXT DeviceContext
    )
/*++
Routine Description:
    This routine configures a continuous reader on the
    interrupt endpoint. It's called from the PrepareHarware event.

Arguments:

Return Value:
    NT status value
--*/
{
    WDF_USB_CONTINUOUS_READER_CONFIG contReaderConfig;
    NTSTATUS status;

    WDF_USB_CONTINUOUS_READER_CONFIG_INIT(&contReaderConfig,
                                          OsrFxEvtUsbInterruptPipeReadComplete,
                                          DeviceContext,    // Context
                                          sizeof(UCHAR));   // TransferLength
	contReaderConfig.EvtUsbTargetPipeReadersFailed = OsrFxEvtUsbInterruptReadersFailed; // WDK10 udpate
	
	//
    // Reader requests are not posted to the target automatically.
    // Driver must explicitly call WdfIoTargetStart to kick start the reader.
    // In this sample, it's done in D0Entry.
    // By default, framework queues two requests to the target endpoint. 
    // Driver can configure up to 10 requests with CONFIG macro.
    //
    status = WdfUsbTargetPipeConfigContinuousReader(DeviceContext->InterruptPipe,
                                                    &contReaderConfig);

    if (!NT_SUCCESS(status)) {
        TraceEvents(TRACE_LEVEL_ERROR, DBG_PNP,
                    "OsrFxConfigContReaderForInterruptEndPoint failed %x\n",
                    status);
        return status;
    }

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_PNP,
		"OsrFxConfigContReaderForInterruptEndPoint success.\n",
		status);

    return status;
}

VOID
OsrFxEvtUsbInterruptPipeReadComplete(
    WDFUSBPIPE  Pipe,
    WDFMEMORY   Buffer,
    size_t      NumBytesTransferred,
    WDFCONTEXT  Context
    )
/*++
Routine Description:
    This the completion routine of the continuous reader. This can
    called *concurrently* on multiprocessor system if there are
    more than one readers configured. So make sure to protect
    access to global resources.

Arguments:
    Buffer - This buffer is freed when this call returns.
             If the driver wants to delay processing of the buffer, it
             can take an additional reference.

    Context - Provided in the WDF_USB_CONTINUOUS_READER_CONFIG_INIT macro
--*/
{
    PUCHAR          switchState = NULL;
    WDFDEVICE       device;
    PDEVICE_CONTEXT pDeviceContext = Context;
	NTSTATUS status;
	BOOLEAN isok = 0, is_call_stopidle = FALSE;

    UNREFERENCED_PARAMETER(Pipe);

    device = WdfObjectContextGetObject(pDeviceContext);

    //
    // Make sure that there is data in the read packet.  Depending on the device
    // specification, it is possible for it to return a 0 length read in
    // certain conditions.
    //

    if (NumBytesTransferred == 0) {
        TraceEvents(TRACE_LEVEL_WARNING, DBG_INIT,
                    "OsrFxEvtUsbInterruptPipeReadComplete Zero length read "
                    "occurred on the Interrupt Pipe's Continuous Reader\n"
                    );
        return;
    }
    ASSERT(NumBytesTransferred == sizeof(UCHAR));

    switchState = WdfMemoryGetBuffer(Buffer, NULL);

    TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
                "OsrFxEvtUsbInterruptPipeReadComplete SwitchState %x\n",
                *switchState);

    pDeviceContext->CurrentSwitchState = *switchState;

	// chj >>>
	if(pDeviceContext->DelayIdle)
	{
		WdfSpinLockAcquire(pDeviceContext->spinlock); // sync with a spinlock.
		status = STATUS_SUCCESS;
		if(!pDeviceContext->isIdleStopped)
		{
			pDeviceContext->isIdleStopped = TRUE;
			status = WdfDeviceStopIdle(device, FALSE);
			is_call_stopidle = TRUE;
		}
		WdfSpinLockRelease(pDeviceContext->spinlock);

		if(is_call_stopidle)
		{
			if(NT_SUCCESS(status)) {
				TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "WdfDeviceStopIdle() success(0x%x)\n", status);
				// May show success with STATUS_PENDING(0x103), but no problem running on.
			} else {
				TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, "WdfDeviceStopIdle() failed!(0x%x)\n", status);
			}
		}

		isok = WdfTimerStart(pDeviceContext->TimerToResumeIdle, 
			WDF_REL_TIMEOUT_IN_MS(pDeviceContext->DelayIdleMillisec));
	}
	// chj <<

    //
    // Handle(=Complete) any pending Interrupt Message IOCTLs. 
	// Note that the OSR USB device will generate an interrupt message(=endpoint 1 data) 
	// when the the device resumes from a low power state. 
    // So %if% the Interrupt Message IOCTL was sent after the device           // 操, 什么 %if%? 在 USB Resume 后故意从 EP1 送出上一个字节, 这明明就是 FX2 固件的行为嘛!
    // has gone to a low power state, the pending Interrupt Message IOCTL will
    // get completed in the function call below, before the user twiddles the
    // dip switches on the OSR USB device. If this is not the desired behavior
    // for your driver, then you could handle this condition by maintaining a
    // state variable on D0Entry to track interrupt messages caused by power up.
    // --这段话的意思我看懂了, 虽然表达很含糊.
    OsrUsbIoctlGetInterruptMessage(device, STATUS_SUCCESS);
}


BOOLEAN // from wdk10 update
OsrFxEvtUsbInterruptReadersFailed(
	__in WDFUSBPIPE Pipe,
	__in NTSTATUS status,
	__in USBD_STATUS UsbdStatus
	)
{
	WDFDEVICE device = WdfIoTargetGetDevice(WdfUsbTargetPipeGetIoTarget(Pipe));
	PDEVICE_CONTEXT pDeviceContext = GetDeviceContext(device);
	LARGE_INTEGER delay;

	UNREFERENCED_PARAMETER(UsbdStatus);

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT, 
		"Got OsrFxEvtUsbInterruptReadersFailed(). status=0x%x, UsbdStatus=0x%X\n", status, UsbdStatus);

	//
	// Clear the current switch state.
	//
	pDeviceContext->CurrentSwitchState = 0;

	//
	// Service the pending interrupt switch change request
	//
	OsrUsbIoctlGetInterruptMessage(device, status);

	delay.QuadPart = WDF_REL_TIMEOUT_IN_MS(10);
	KeDelayExecutionThread(KernelMode, FALSE, &delay);
	return TRUE; // try to restart ctreader
}

VOID
EvtTimer_ResumeIdle(WDFTIMER  timer)
{
	WDFDEVICE device = (WDFDEVICE)WdfTimerGetParentObject(timer);
	PDEVICE_CONTEXT pDevContext = GetDeviceContext(device);

	PAGED_CODE();

	// TODO: sync with a spinlock.
	WdfSpinLockAcquire(pDevContext->spinlock);

	if(pDevContext->isIdleStopped==TRUE) // to be safe, imagining OsrFxEvtUsbInterruptPipeReadComplete() could be passive-level
	{
		WdfDeviceResumeIdle(device); // this reset's WDF's USB idle timer
		pDevContext->isIdleStopped = FALSE;
	}

	WdfSpinLockRelease(pDevContext->spinlock);

	TraceEvents(TRACE_LEVEL_INFORMATION, DBG_INIT,
		"WdfDeviceResumeIdle() called. Tell WDF to restart counting %d millisec before idle.\n", 
		pDevContext->WdfIdleMillisec);
}

