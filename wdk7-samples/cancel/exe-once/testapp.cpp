/*++
Copyright (c) Microsoft Corporation.  All rights reserved.
	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.

Module Name:
	testapp.cpp (chj updated)

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
#include <assert.h>
#include <process.h>
#include <malloc.h>
#include <tchar.h>
#include <conio.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <strsafe.h>

#include "testapp.h"

#ifdef USE_CreateThread
	typedef DWORD THREADFUNC_RET_TYPE;
#else
	typedef unsigned int THREADFUNC_RET_TYPE;
#endif

//
// Globals
//

HANDLE hDevice;
BOOLEAN ExitFlag = FALSE;
bool g_CallCancelIo = false;

HANDLE hThreads[MAXTHREADS], hThreadsDone[MAXTHREADS];
DWORD thread_ids[MAXTHREADS];
//
// function prototypes
//

VOID CALLBACK CompletionRoutine(DWORD errorcode, DWORD bytesTransfered, LPOVERLAPPED ov);

THREADFUNC_RET_TYPE WINAPI Reader(PVOID);

BOOLEAN SetupDriverName(__inout_bcount_full(BufferLength) PCHAR DriverLocation,
	__in ULONG BufferLength);


#define COUNT(ar) (sizeof(ar)/sizeof(ar[0]))

TCHAR* now_timestr(TCHAR buf[], int bufchars, bool ymd=false)
{
	SYSTEMTIME st = {0};
	GetLocalTime(&st);
	buf[0]=_T('['); buf[1]=_T('\0'); buf[bufchars-1] = _T('\0');
	if(ymd) {
#if _MSC_VER >= 1400 // VS2005+
		_sntprintf_s(buf, bufchars-1, _TRUNCATE, _T("%s%04d-%02d-%02d "), buf, 
			st.wYear, st.wMonth, st.wDay);
#else
		_sntprintf(buf, bufchars-1, _T("%s%04d-%02d-%02d "), buf, 
			st.wYear, st.wMonth, st.wDay);
#endif
	}
#if _MSC_VER >= 1400 // VS2005+
	_sntprintf_s(buf, bufchars-1, _TRUNCATE, _T("%s%02d:%02d:%02d.%03d]"), buf,
		st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#else
	_sntprintf(buf, bufchars-1, _T("%s%02d:%02d:%02d]"), buf,
		st.wHour, st.wMinute, st.wSecond);
#endif
	return buf;
}

void timeprint(const TCHAR *fmt, ...)
{
	static DWORD s_millisec_was = GetTickCount();
	
	DWORD millisec_now = GetTickCount();
	if(millisec_now-s_millisec_was>=1000) {
		printf(".\n"); // an extra line to indicate 1+ seconds has ellapsed
	}
	s_millisec_was = millisec_now;
	
	TCHAR buf[1000] = {0};
	now_timestr(buf, COUNT(buf));

	int prefixlen = (int)_tcslen(buf);

	va_list args;
	va_start(args, fmt);
#if _MSC_VER >= 1400 // VS2005+
	_vsntprintf_s(buf+prefixlen, COUNT(buf)-3-prefixlen, _TRUNCATE, fmt, args);
	prefixlen = (int)_tcslen(buf);
	_tcsncpy_s(buf+prefixlen, 2, TEXT("\r\n"), _TRUNCATE); // add trailing \r\n
#else
	_vsntprintf(buf+prefixlen, COUNT(buf)-3-prefixlen, fmt, args);
	prefixlen = _tcslen(buf);
	_tcsncpy(buf+prefixlen, TEXT("\r\n"), 2); // add trailing \r\n
#endif
	va_end(args);

	printf("%s", buf);
}


typedef int (*PROC_thread_done)(HANDLE hThreadEnd, int array_idx, void *context);

bool Wait_ThreadsDone(int array_size, HANDLE arhThreadsInput[], HANDLE arhThreadsDone[], 
	bool wait_all, int timeout_millisec,
	PROC_thread_done proc_thread_end=0, void *context=0)
{
	// arhThreadsInput[] allows NULL element, so you can call this function in a cycle
	// without shuffling arhThreadsInput's elements.
	DWORD millisec_start = GetTickCount();
	int i, valid_threads = 0, new_done = 0;

	HANDLE hDumbEvent = CreateEvent(NULL, TRUE, FALSE, NULL); // a dumb unsignaled event

	// check for non-null input hThreads(valid_threads)
	for(i=0; i<array_size; i++)
	{
		if(arhThreadsInput[i])
			valid_threads++;
		else
			arhThreadsInput[i] = hDumbEvent; // to please WaitForMultipleObjects
	}

	for(; new_done<valid_threads;)
	{
		DWORD win_millisec = timeout_millisec>=0 ? timeout_millisec : INFINITE;
		DWORD waitre = WaitForMultipleObjects(array_size, arhThreadsInput, FALSE, win_millisec);
		if(waitre>=WAIT_OBJECT_0 && waitre<WAIT_OBJECT_0+array_size)
		{
			i = waitre - WAIT_OBJECT_0;
			arhThreadsDone[i] = arhThreadsInput[i];
			arhThreadsInput[i] = hDumbEvent;
			new_done++;

			proc_thread_end(arhThreadsDone[i], i, context);
		}
		else if(waitre==WAIT_TIMEOUT)
			break;
		else
			assert(0);

		if(!wait_all && new_done>0)
			break;

		// check timeout
		if(timeout_millisec<0)
			continue; // wait forever
		if(timeout_millisec==0)
			break; // no wait
		if(GetTickCount()-millisec_start >= (DWORD)timeout_millisec)
			break; // time out

//		Sleep(100);
	}

	CloseHandle(hDumbEvent);

	if(new_done==valid_threads)
		return true; // all input threads done
	else
		return false;
}

int print_thread_done(HANDLE hThreadEnd, int array_idx, void *context)
{
	DWORD *ar_threadid = (DWORD*)context;
	timeprint("(tid=%d)WaitForSingleObject returns success for this thread.\n", ar_threadid[array_idx]);
	return 0;
}

//
// Main function
//
VOID __cdecl
main(
	__in ULONG argc,
	__in_ecount(argc) PCHAR argv[]
	)
{
	ULONG i; //, Id;
	ULONG   NumberOfThreads = 1;
	DWORD errNum = 0;
	TCHAR driverLocation[MAX_PATH] = {'\0'};
	unsigned char extra_delay_seconds = 5;
	BOOL b = FALSE;

	if (argc >= 2  && (argv[1][0] == '-' || isalpha((unsigned char)argv[1][0])))
	{
		puts("Usage:testapp <NumberOfThreads>\n");
		return;
	}
	else if (argc >= 2 && ((NumberOfThreads = atoi(argv[1])) > MAXTHREADS))
	{
		printf("Invalid option:Only a maximum of %d threads allowed.\n", MAXTHREADS);
		return;
	}

	if(argc==3)
	{
		extra_delay_seconds = (unsigned char)atoi(argv[2]);
	}

	//
	// Try to connect to driver.  If this fails, try to load the driver
	// dynamically.
	//
	if ((hDevice = CreateFile("\\\\.\\CancelSamp",
								 GENERIC_READ|GENERIC_WRITE,
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
				GENERIC_READ|GENERIC_WRITE,
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

	// Tell driver our extra_delay_seconds:
	DWORD nbWritten = 0;
	OVERLAPPED ovlp = {0};
	ovlp.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	b = WriteFile(hDevice, &extra_delay_seconds, 1, &nbWritten, &ovlp);
	if(!b || nbWritten!=1) {
		printf("Error! Underlying driver does not support setting extra-delay-seconds.\n");
		ExitProcess(2);
	}
	CloseHandle(ovlp.hEvent);

	printf("==============================================================\n");
	printf("WDK cancel-once demo, compiled on: %s %s\n", __DATE__, __TIME__);
	printf("Chj note: Each worker thread will do ReadFileEx() only once.\n");
	printf("Each ReadFileEx will be delayed %d seconds by the driver.\n", extra_delay_seconds);
	printf("You can wait silently for IO completion, or ,\n");
	printf("  'q' - let worker thread quit before IO completion\n");
	printf("  'c' - let worker thread call CancelIo before IO completion\n");
	printf("  'x' - let surprise close device handle before IO completion\n");
	printf("==============================================================\n");

	printf("Number of threads : %d\n", NumberOfThreads);

	for(i=0; i < NumberOfThreads; i++)
	{
#ifdef USE_CreateThread // This is not defined.
		// Don't use CreateThread(), because you use printf in the thread.
		hThreads[i] = CreateThread( NULL,      // security attributes
									0,         // initial stack size
									Reader,    // thread function
									NULL,      // arg to Reader thread
									0,         // creation flags
									&thread_ids[i]); // returned thread id
#else
		// Use _beginthreadex instead
		hThreads[i] = (HANDLE)_beginthreadex(NULL, // security attributes
			0,         // initial stack size
			Reader,    // thread function
			NULL,      // arg to Reader thread
			0,         // creation flags
			(unsigned int*)&thread_ids[i]); // returned thread id
#endif
		if ( NULL == hThreads[i] ) {
			timeprint(" Error CreateThread[%d] Failed: %d\n", i, GetLastError());
			ExitProcess ( 1 );
		}
		else {
			timeprint("(tid=%d)Thread created.\n", thread_ids[i]);
		}
		Sleep(10);
	}

	int quitchar = 0;
	for(;;)
	{
		if(_kbhit()) 
		{
			quitchar = _getch();
			if(quitchar=='q' || quitchar=='c' || quitchar=='x') {
				timeprint("Got keyboard input: %c\n", quitchar);
				break;
			}
		}
		
		bool all_done = Wait_ThreadsDone(NumberOfThreads, hThreads, hThreadsDone, 
			true, 100, print_thread_done, thread_ids);
		if(all_done) {
			quitchar = 0;
			break;
		}
	}

	if(quitchar)
	{
		if(quitchar=='q' || quitchar=='c') 
		{
			ExitFlag = TRUE; // tell worker thread to quit

			if(quitchar=='c')
				g_CallCancelIo = true;

			Wait_ThreadsDone(NumberOfThreads, hThreads, hThreadsDone, 
				true, -1, print_thread_done, thread_ids);

			timeprint("Closing device handle.\n");
			CloseHandle(hDevice);
		}
		else if(quitchar=='x') // surprise close device handle
		{
			timeprint("Closing device handle(surprise).\n");
			b = CloseHandle(hDevice); 
			if(b)
				timeprint("CloseHandle() returns success.\n");
			else
				timeprint("CloseHandle() returns error, winerr=%d.\n", GetLastError);

			Wait_ThreadsDone(NumberOfThreads, hThreads, hThreadsDone, 
				true, -1, print_thread_done, thread_ids);
		}
		else
			assert(0);
	}
	else
	{
		timeprint("Worker threads end with normal flow.\n");
	}

	for(i=0; i < NumberOfThreads; i++)
		CloseHandle(hThreadsDone[i]);

	b = TRUE;
//	BOOL b = ManageDriver(DRIVER_NAME, driverLocation, DRIVER_FUNC_REMOVE); // Unload the driver.  
	timeprint("Done. Not removing %s driver.\n", DRIVER_NAME);

	ExitProcess(b ? 0 : 1);
}

THREADFUNC_RET_TYPE WINAPI Reader(PVOID dummy )
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
		timeprint("(tid=%d)ReadFileEx failed, winerr=%d\n", tid, GetLastError());
		ExitThread(0);
	}
	
	bool io_complete = false;
	while(!ExitFlag) // busy sleep and check ExitFlag
	{
		DWORD re = SleepEx(100, TRUE);
		if(re==WAIT_IO_COMPLETION)
		{
			timeprint("(tid=%d)SleepEx() returns with WAIT_IO_COMPLETION.\n", tid);
			io_complete = true;
			break;
		}
	}
	
	if(g_CallCancelIo)
	{
		timeprint("(tid=%d)Calling CancelIo()...\n", tid);
		BOOL b = CancelIo(hDevice);
		if(b)
			timeprint("(tid=%d)CancelIo() returns success(cancel will take effect).\n", tid);
		else {
			DWORD winerr = GetLastError();
			timeprint("(tid=%d)CancelIo() returns with winerr=%d(cancel in vain).\n", tid, winerr);
		}
	}

	timeprint("(tid=%d)Calling ExitThread(0). %s\n", tid, io_complete?"":"(without IO completion)");
	ExitThread(0);
}


VOID CALLBACK CompletionRoutine(
	DWORD errorcode,
	DWORD bytesTransfered,
	LPOVERLAPPED ov
	)
{
	UNREFERENCED_PARAMETER(ov);

	DWORD tid = GetCurrentThreadId();
	if(!errorcode)
	{
		timeprint("(tid=%d)CompletionRoutine read: %d bytes\n", tid, bytesTransfered);
	}
	else
	{
		timeprint("(tid=%d)CompletionRoutine got error: winerr=%d\n", tid, errorcode);
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

