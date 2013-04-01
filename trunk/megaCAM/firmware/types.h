#ifndef TYPES_H
#define TYPES_H

/****** FIXME:
sizeof(unsigned char)=1
sizeof(unsigned short)=2
sizeof(unsigned int)=2
sizeof(unsigned long)=4
sizeof(unsigned long long)=8
******/

typedef unsigned long uint32;
typedef unsigned int uint16;
typedef unsigned char uint8;
#ifndef bool
typedef unsigned char bool;
#endif

#ifndef null
# define null 0
#endif

#ifndef true	
#	define true		1
#endif

#ifndef false
#	define false	0
#endif

#ifndef NULL
# define NULL 0
#endif

#ifndef TRUE	
#	define TRUE		1
#endif

#ifndef FALSE
#	define FALSE	0
#endif


#endif