#include <usart.h>

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#if (0)
void usart_open(unsigned long baudrate) {
	unsigned int UBR0 = 0;
	
	PORTD |= (BOARD_DBG_TXD | BOARD_DBG_RXD);
	DDRD |= BOARD_DBG_TXD;
	DDRD &= ~BOARD_DBG_RXD;
	
	UBRR0L = (F_CPU / (unsigned long)baudrate / 8 - 1) % 256;
    UBRR0H = (F_CPU / (unsigned long)baudrate / 8 - 1) / 256;
	UBR0 = F_CPU / baudrate / 8 - 1;
	//UBRR0L = UBR0 & 0xFF;
	//UBRR0H = (UBR0 >> 8) & 0xFF;
		
	UCSR0B = (1 << 3) | (1 << 4 );
    UCSR0C = (3 << 1);
	UCSR0A = (1 << 1);
	
	//print("%d, %d, %d\r\n", UBRR0L, UBRR0H, UBR0);
}
#endif
	
#if (DEBUG)
void print(const char *format, ...) {
	va_list args;
	char buf[32];
	
	va_start(args, format);
	vsprintf(buf, format, args);
	va_end(args);
	usart_dbg_string(buf);
}
#endif

void usart_output_str(const char * buf) {
	while(*buf != 0) {
		UART_WRITE(*(buf ++));
	}	
}

void usart_output_str_pgm(prog_uchar * buf) {
	char cache;
	
	do {
		cache = pgm_read_byte(buf ++);
		if(cache != 0) {
			UART_WRITE(cache);
		}
	} while (cache != 0);
}

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

/* output dec from 0-99999 */
void usart_output_dec(unsigned long dec) {
	char buffer[16];
	
	sprintf(buffer, "%ld", dec);
	usart_output_str(buffer);
}


int ISR(USART_RX_vect) {  
	char data;   
		  
	data = UDR0;  
	UART_WRITE(data);
	return 0;
}  
