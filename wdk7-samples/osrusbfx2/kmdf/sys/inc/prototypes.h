#ifdef __cplusplus
extern"C" {
#endif


DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD EvtDeviceAdd;

EVT_WDF_DEVICE_CONTEXT_CLEANUP EvtDriverContextCleanup;
EVT_WDF_DEVICE_PREPARE_HARDWARE EvtDevicePrepareHardware;

EVT_WDF_DEVICE_RELEASE_HARDWARE EvtDeviceReleaseHardware; // chj test

EVT_WDF_IO_QUEUE_IO_READ EvtIoRead;
EVT_WDF_IO_QUEUE_IO_WRITE EvtIoWrite;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL EvtIoDeviceControl;

EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtRequestReadCompletionRoutine;
EVT_WDF_REQUEST_COMPLETION_ROUTINE EvtRequestWriteCompletionRoutine;

// chj test:
EVT_WDF_FILE_CLEANUP EvtFileobjectCleanup;
EVT_WDF_FILE_CLOSE   EvtFileobjectClose;

EVT_WDF_DEVICE_D0_ENTRY EvtDeviceD0Entry;
EVT_WDF_DEVICE_D0_EXIT EvtDeviceD0Exit;


#ifdef __cplusplus
} // extern"C" {
#endif
