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

#define TRACE "FUNC: %s\n",__func__

#define DEBUG "FILE: %s\n\tFUNC: %s\tLINE: %d\n",__FILE__,__func__,__LINE__

/*
	Simple log function, put msg to stderr
	FUNCTION WORKS IFF DEBUG_MODE is defined


	PARAMS
	@IN msg - log message

	RETURN:
	This is a void function
*/
void __log__(const char *msg,...);

/*
	Simple trace calling funcion

	FUNCTION WORKS IFF TRACE_MODE is defined

	PARAMS
	@IN msg - trace message

	RETURN:
	This is a void function
*/
void __trace_call__(const char *msg,...);

#endif
