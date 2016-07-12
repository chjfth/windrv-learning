/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

Module Name:

	testapp.c

Abstract:

Environment:

	User mode Win32 console application
--*/

//
// Annotation to indicate to prefast that this is nondriver user-mode code.
//
												  
#include <DriverSpecs.h>
__user_code  

#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strsafe.h>

#include "testapp.h"

//
// Globals
//

HANDLE hDevice;
BOOLEAN ExitFlag = FALSE;
bool g_CallCancelIo = false;
HANDLE hThreads[MAXTHREADS];

//
// function prototypes
//

VOID CALLBACK CompletionRoutine(
	DWORD errorcode,
	DWORD bytesTransfered,
	LPOVERLAPPED ov
	);

DWORD
WINAPI Reader(
	PVOID
	);

BOOLEAN
SetupDriverName(
	__inout_bcount_full(BufferLength) PCHAR DriverLocation,
	__in ULONG BufferLength
	);

//
// Main function
//

VOID __cdecl
main(
	__in ULONG argc,
	__in_ecount(argc) PCHAR argv[]
	)
{
	ULONG i, Id;
	ULONG   NumberOfThreads = 1;
	DWORD errNum = 0;
	TCHAR driverLocation[MAX_PATH] = {'\0'};

	if (argc >= 2  && (argv[1][0] == '-' || isalpha((unsigned char)argv[1][0])))
	{
		puts("Usage:testapp <NumberOfThreads>\n");
		return;
	}
	else if (argc >= 2 && ((NumberOfThreads = atoi(argv[1])) > MAXTHREADS))
	{
		printf("Invalid option:Only a maximun of %d threads allowed.\n", MAXTHREADS);
		return;
	}

	//
	// Try to connect to driver.  If this fails, try to load the driver
	// dynamically.
	//
	if ((hDevice = CreateFile("\\\\.\\CancelSamp",
								 GENERIC_READ,
								 0,
								 NULL,
								 OPEN_EXISTING,
								 FILE_FLAG_OVERLAPPED,
								 NULL
								 )) == INVALID_HANDLE_VALUE) 
	{
		errNum = GetLastError();

		if (errNum != ERROR_FILE_NOT_FOUND) {
			printf("CreateFile failed!  Error = %d\n", errNum);
			return ;
		}

		// Setup full path to driver name.
		if (!SetupDriverName(driverLocation, sizeof(driverLocation))) {
			return ;
		}

		//
		// Install driver.
		//
		if (!ManageDriver(DRIVER_NAME,
						  driverLocation,
						  DRIVER_FUNC_INSTALL
						  )) 
		{
			printf("Unable to install driver. \n");

			// Error - remove driver.
			ManageDriver(DRIVER_NAME,
						 driverLocation,
						 DRIVER_FUNC_REMOVE
						 );
			return;
		}

		//
		// Try to open the newly installed driver.
		//
		hDevice = CreateFile( "\\\\.\\CancelSamp",
				GENERIC_READ,
				0,
				NULL,
				OPEN_EXISTING,
				FILE_FLAG_OVERLAPPED,
				NULL);

		if ( hDevice == INVALID_HANDLE_VALUE )
		{
			printf ( "Error: CreatFile Failed(2nd-try) : %d\n", GetLastError());
			return;
		}
	}

	printf("============================================================\n");
	printf("Chj Note: Each worker thread will do ReadFileEx() only once.\n");
	printf("============================================================\n");

	printf("Number of threads : %d\n", NumberOfThreads);

	printf("<Enter 'q' or 'c', then Enter to exit gracefully>\n");

	for(i=0; i < NumberOfThreads; i++)
	{
		hThreads[i] = CreateThread( NULL,      // security attributes
									0,         // initial stack size
									Reader,    // thread function
									NULL,      // arg to Reader thread
									0,         // creation flags
									(LPDWORD)&Id); // returned thread id

		if ( NULL == hThreads[i] ) {
			printf( " Error CreateThread[%d] Failed: %d\n", i, GetLastError());
			ExitProcess ( 1 );
		}
		else {
			printf("(tid=%d)Thread created.\n", Id);
		}
	}

	int quitchar = getchar();

	if(quitchar=='q' || quitchar=='c') // do graceful thread-exit
	{
		if(quitchar=='c')
			g_CallCancelIo = true;
			
		ExitFlag = TRUE;
		WaitForMultipleObjects(NumberOfThreads, hThreads, TRUE, INFINITE);
		
		printf("WaitForMultipleObjects has returned for all worker threads.\n");
		
		CloseHandle(hDevice);
	}
	else // do force-exit
	{
		CloseHandle(hDevice); // close device handle while it is still in use
		WaitForMultipleObjects( NumberOfThreads, hThreads, TRUE, INFINITE);
	}

	for(i=0; i < NumberOfThreads; i++)
		CloseHandle(hThreads[i]);

	//
	// Unload the driver.  Ignore any errors.
	//
	BOOLEAN b = TRUE;
//	BOOLEAN b = ManageDriver(DRIVER_NAME, driverLocation, DRIVER_FUNC_REMOVE);
	printf("Done. Not removing %s driver.\n", DRIVER_NAME);

	ExitProcess(b ? 0 : 1);
}


DWORD WINAPI Reader(PVOID dummy )
{
	ULONG data = 0x11223344;
	OVERLAPPED ov;
	DWORD tid = GetCurrentThreadId();

	UNREFERENCED_PARAMETER(dummy);

	ZeroMemory( &ov, sizeof(ov) );
	ov.Offset = 0;
	ov.OffsetHigh = 0;

	if (!ReadFileEx(hDevice, (PVOID)&data, sizeof(ULONG),  &ov, CompletionRoutine))
	{
		printf ( "(tid=%d)ReadFileEx failed, winerr=%d\n", tid, GetLastError());
		ExitThread(0);
	}
	
	bool io_complete = false;
	while(!ExitFlag)
	{
		DWORD re = SleepEx(100, TRUE);
		if(re==WAIT_IO_COMPLETION)
		{
			printf("(tid=%d)SleepEx() returned with WAIT_IO_COMPLETION. Work done.\n", tid);
			io_complete = true;
			break;
		}
	}
	
	if(g_CallCancelIo)
	{
		printf("(tid=%d)Calling CancelIo()...\n", tid);
		BOOL b = CancelIo(hDevice);
		if(b)
			printf("(tid=%d)Called  CancelIo(). Success(cancel will take effect).\n", tid);
		else {
			DWORD winerr = GetLastError();
			printf("(tid=%d)Called  CancelIo(). Fail with winerr=%d(cancel in vain).\n", tid, winerr);
		}
	}

	printf("(tid=%d)Calling ExitThread(0). %s\n", tid, io_complete?"":"(without IO completion)");
	ExitThread(0);
}


VOID CALLBACK CompletionRoutine(
	DWORD errorcode,
	DWORD bytesTransfered,
	LPOVERLAPPED ov
	)
{
	UNREFERENCED_PARAMETER(ov);

	if(!errorcode)
	{
		fprintf(stdout, "Thread %d read: %d bytes\n",
			GetCurrentThreadId(), bytesTransfered);
	}
	else
	{
		fprintf(stdout, "Thread %d CompletionRoutine got error: winerr=%d\n",
			GetCurrentThreadId(), errorcode);
	}
	return;
}


BOOLEAN
SetupDriverName(
	__inout_bcount_full(BufferLength) PCHAR DriverLocation,
	__in ULONG BufferLength
	)
{
	HANDLE fileHandle;
	DWORD driverLocLen = 0;

	// Get the current directory.
	driverLocLen = GetCurrentDirectory(BufferLength,
									   DriverLocation
									   );
	if (driverLocLen == 0) {
		printf("GetCurrentDirectory failed!  Error = %d \n", GetLastError());
		return FALSE;
	}

	// Setup path name to driver file.
	if (FAILED( StringCbCat(DriverLocation, BufferLength, "\\"DRIVER_NAME".sys") )) {
		return FALSE;
	}

	//
	// Insure driver file is in the specified directory.
	//
	if ((fileHandle = CreateFile(DriverLocation,
								 GENERIC_READ,
								 0,
								 NULL,
								 OPEN_EXISTING,
								 FILE_ATTRIBUTE_NORMAL,
								 NULL
								 )) == INVALID_HANDLE_VALUE) 
	{
		printf("%s.sys is not loaded.\n", DRIVER_NAME);
		return FALSE;
	}

	// Close open file handle.
	if (fileHandle) {
		CloseHandle(fileHandle);
	}

	return TRUE;
}   // SetupDriverName
