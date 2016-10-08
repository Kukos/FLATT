#ifndef LOG_H
#define LOG_H

/*
	Wrapper to smiple log errors

	Author: Michal Kukowski
	email: michalkukowski10@gmail.com

	LICENCE: GPL3

*/

/*
	Modes to debug:
	DEBUG_MODE -> if defined, log

	TRACE_MODE -> if defined trace_call works
*/

#define __TRACE__ "[TRACE]\tFUNC: %s\n",__func__

#define __ERROR__ "[ERROR]\tFILE: %s\n\tFUNC: %s\tLINE: %d\n",__FILE__,__func__,__LINE__

#define __LOG__ "[LOG]\tFUNC: %s\tLINE: %d\n",__func__,__LINE__

#define TRACE(...) \
    do{ \
        __trace_call__(__TRACE__, ##__VA_ARGS__); \
    }while(0)


#define LOG(msg, ...) \
    do{ \
        __log__(__LOG__); \
        __log__("\t"); \
        __log__(msg, ##__VA_ARGS__); \
    }while(0)

#define ERROR(msg, errno, ...) \
    do{ \
        __log__(__ERROR__); \
        __log__("\t"); \
        __log__(msg, ##__VA_ARGS__); \
        \
        return errno; \
    }while(0)
/*
	Simple log function, put msg to stderr
	FUNCTION WORKS IFF DEBUG_MODE is defined


	PARAMS
	@IN msg - log message

	RETURN:
	This is a void function
*/
void __log__(const char *msg, ...);

/*
	Simple trace calling funcion

	FUNCTION WORKS IFF TRACE_MODE is defined

	PARAMS
	@IN msg - trace message

	RETURN:
	This is a void function
*/
void __trace_call__(const char *msg, ...);

#endif
