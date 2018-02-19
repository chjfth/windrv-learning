#ifndef __my_dbgprint_h_
#define __my_dbgprint_h_

#ifdef __cplusplus
extern"C"{
#endif


#if DBG

extern int g_dbgseq;

#undef KdPrint
#define KdPrint(_x_) \
	DbgPrint("[KT%d] ", ++g_dbgseq); \
	DbgPrint _x_

#endif // DBG




#ifdef __cplusplus
} // extern"C"{
#endif

#endif
