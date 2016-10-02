[2016-10-02]
When doing cl /c bulkrwr.c, I have to pass option -DWINNT, otherwise

	EVENT_CONTROL_CODE_ENABLE_PROVIDER

will not be defined(appears in evntrace.h but hidden by and #ifdef check).

