#ifndef __my_dbgprint_h_
#define __my_dbgprint_h_

#ifdef __cplusplus
extern"C"{
#endif



#if DBG

void my_PrintTimestampPrefix(void);

#undef KdPrint
#define KdPrint(_x_) \
	my_PrintTimestampPrefix(); \
	DbgPrint _x_

#endif // DBG




#ifdef __cplusplus
} // extern"C"{
#endif

#endif
