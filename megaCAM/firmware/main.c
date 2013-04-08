#include <stdio.h>
#include <string.h>

#include <avr/sleep.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>

#include "board.h"
#include "usart.h"
#include "camera.h"
#include "sccb.h"
#include "trace.h"

/* strings which write into flash space */
static prog_uchar str0[] = "\r\n****************************************\r\n";
static prog_uchar str1[] = "        megaCAM by AliveHex \r\n";
static prog_uchar str2[] = "        v1.2 Build ";
static prog_uchar str3[] = "\r\n>";
static prog_uchar str4[] = "console mode disable, enter package mode\r\n";
//static prog_uchar str5[] = "press enter to enable console mode ... \r\n";


int main() {
	/* open USART with default baudrate */
	UART_OPEN(BOARD_UART_BAUDRATE);	
	
	/* config the FIFO and Camera IOs */
	FIFO_IO_INIT();
	CAM_IO_INIT();
	FIFO_WE_LOW();
	FIFO_OE_HIGH();
	
	/* output welcome message and baudrate config */
	usart_output_str_pgm(str0);
	usart_output_str_pgm(str1);
	usart_output_str_pgm(str2);
	usart_output_str(__DATE__);
	usart_output_str_pgm(str0);
	//usart_output_str_pgm(str5);
	
	/* listen packages, NEVER returns */
	cmd_package_listen();
	return 0;
}
