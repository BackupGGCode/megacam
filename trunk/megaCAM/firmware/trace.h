#ifndef _TRACE_H_
#define _TRACE_H_

#if (DEBUG)
extern void print(const char *format, ...);
#else 
#define print(...)	while(0) {}
#endif

#endif