#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "usart.h"
	
#if (DEBUG) /* only for debug */
void print(const char *format, ...) {
	va_list args;
	char buf[32];
	
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	usart_dbg_string(buf);
}
#endif

/*
** @ USART output string
**/
void usart_output_str(const char * buf) {
	while(*buf != 0) {
		UART_WRITE(*(buf ++));
	}	
}

/*
** @ USART ouput pgm string
**/
void usart_output_str_pgm(prog_uchar * buf) {
	char cache;
	
	do {
		cache = pgm_read_byte(buf ++);
		if(cache != 0) {
			UART_WRITE(cache);
		}
	} while (cache != 0);
}

/*
** @ USART ouput hex-number
**/
void usart_output_hex(unsigned long hex) {
	//char buffer[16];
	
	//memset(buffer, 0, sizeof(buffer));
	//sprintf(buffer, "%lx", hex);
	//usart_output_str(buffer);
#if (1)
	int i;
	bool output = FALSE;
	unsigned char buf;
	
	for(i = 7; i >= 0; i --) {
		buf = (hex >> (i * 4)) & 0x0f;
		if(output == FALSE) {
			if((buf != 0) || (i == 0)) {
				output = TRUE;
			}
		}
		if(output == TRUE) {
			if(buf < 0x0a) {
				UART_WRITE(0x30 + buf);
			} else {
				UART_WRITE(0x57 + buf);
			}
		}
	}
#endif	
}

/*
** @ USART ouput dec-num
** @ from range 0-99999
**/
void usart_output_dec(unsigned long dec) {
	char buffer[16];
	
	sprintf(buffer, "%ld", dec);
	usart_output_str(buffer);
}



