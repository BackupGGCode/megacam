#ifndef USART_H
#define USART_H

#include <avr/io.h>
#include <avr/pgmspace.h>

#include "types.h"
#include "board.h"

#define UART_OPEN(baudrate) \
	PORTD |= (BOARD_DBG_TXD | BOARD_DBG_RXD); \
	DDRD |= BOARD_DBG_TXD; \
	DDRD &= ~BOARD_DBG_RXD; \
	UBRR0L= (F_CPU / (unsigned long)baudrate / 8 - 1) % 256; \
    UBRR0H= (F_CPU / (unsigned long)baudrate / 8 - 1) / 256; \
    UCSR0B = (1 << 3) | (1 << 4 ); \
    UCSR0C = (3 << 1); \
	UCSR0A = (1 << 1);
	
#define UART_WRITE(c) \
	while ( !( UCSR0A & (1 << 5)) );\
	UDR0 = c

static inline uint8 UART_GETCH(void) {
	while ( !(UCSR0A & (1 << 7)) );
	return UDR0;
}

static inline int UART_GETCH_NB(void) {
	if( (UCSR0A & (1 << 7)) == 0)
		return -1;
	else
		return UDR0 & 0xff;
}

extern void usart_output_str_pgm(prog_uchar * buf);
extern void usart_output_str(const char * buf);
extern void usart_output_hex(unsigned long hex);
extern void usart_output_dec(unsigned long dec);

#endif // USART_H