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

    printf("Number of threads : %d\n", NumberOfThreads);

    printf("Enter 'q' to exit gracefully:");

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
    }

	int quitchar = getchar();

    if(quitchar == 'q') // do graceful thread-exit
    {
        ExitFlag = TRUE;
        WaitForMultipleObjects( NumberOfThreads, hThreads, TRUE, INFINITE);
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
    BOOLEAN b = ManageDriver(DRIVER_NAME,
                 driverLocation,
                 DRIVER_FUNC_REMOVE
                 );

    ExitProcess(b ? 0 : 1);
}


DWORD WINAPI Reader(PVOID dummy )
{
    ULONG data;
    OVERLAPPED ov;
	DWORD tid = GetCurrentThreadId();

    UNREFERENCED_PARAMETER(dummy);

    while(!ExitFlag)
    {
        ZeroMemory( &ov, sizeof(ov) );
        ov.Offset = 0;
        ov.OffsetHigh = 0;

        if (!ReadFileEx(hDevice, (PVOID)&data, sizeof(ULONG),  &ov, CompletionRoutine))
        {
            printf ( "(tid %d)ReadFileEx failed, winerr=%d\n", tid, GetLastError());
            break; //ExitProcess(1);
        }
        SleepEx(INFINITE, TRUE);
    }

    printf("(tid %d)Exiting thread.\n", tid);
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

