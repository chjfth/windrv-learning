// Define C++ new, delete 

#include <ntddk.h>

#define TOASTER_DEVICE_POOL_TAG '++dT' // Poolmon will show "Td++"


void * ufcom_new(size_t count) 
{
//	PAGED_CODE();
	return ExAllocatePoolWithTag(PagedPool, count, TOASTER_DEVICE_POOL_TAG);
}

void ufcom_delete(void *object) 
{
//	PAGED_CODE();
	if(!object)
		return;
	ExFreePoolWithTag(object, TOASTER_DEVICE_POOL_TAG);
}


void *__cdecl operator new(size_t count) 
{
	return ufcom_new(count);
}

void __cdecl operator delete(void *object) 
{
	return ufcom_delete(object);
}


/*
void *__cdecl operator new[](size_t count) 
{
	return ufcom_new(count); // WDK7 test result: this will not get called.
}

void __cdecl operator delete[](void *object) 
{
	return ufcom_delete(object); // WDK7 test result: this will not get called.
}

*/
