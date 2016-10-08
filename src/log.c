#include"log.h"
#include<stdio.h>
#include<stdarg.h>

void __log__(const char *msg,...)
{
	#ifdef DEBUG_MODE
        va_list args;
        va_start (args,msg);

		vfprintf(stderr,msg,args);

		va_end(args);
	#endif
}

void __trace_call__(const char *msg,...)
{
	#ifdef TRACE_MODE
        va_list args;
        va_start (args,msg);

		vfprintf(stderr,msg,args);

		va_end(args);
	#endif
}

