// Define C++ new, delete 

#include <ntddk.h>

void * ufcom_new(size_t count) 
{
//	PAGED_CODE();
	return ExAllocatePoolWithTag(PagedPool, count, '++ct');
}

void ufcom_delete(void *object) 
{
//	PAGED_CODE();
	if(!object)
		return;
	ExFreePoolWithTag(object, 'mcfu');
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
