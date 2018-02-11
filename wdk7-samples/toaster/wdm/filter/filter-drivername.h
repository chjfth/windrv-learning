#ifndef __FILTER_DRIVERNAME_H_
#define __FILTER_DRIVERNAME_H_

#define DRIVERNAME "filter.sys: "

#if BUS_LOWER

#undef DRIVERNAME
#define DRIVERNAME "BFdoLwr.sys: "

#endif

#if BUS_UPPER

#undef DRIVERNAME
#define DRIVERNAME "BFdoUpr.sys: "

#endif

#if DEVICE_LOWER

#undef DRIVERNAME
#define DRIVERNAME "DevLower.sys: "

#endif

#if DEVICE_UPPER

#undef DRIVERNAME
#define DRIVERNAME "DevUpper.sys: "

#endif

#if CLASS_LOWER

#undef DRIVERNAME
#define DRIVERNAME "ClsLower.sys: "

#endif

#if CLASS_UPPER

#undef DRIVERNAME
#define DRIVERNAME "ClsUpper.sys: "

#endif

#endif // __FILTER_DRIVERNAME_H_


