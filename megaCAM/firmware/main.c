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
#include "cmdline.h"
#include "trace.h"

#define CMD_BUF_LEN		32
#define LIMIT_U_TIMES	6

extern int USART_MODE_ENABLE;

static prog_uchar str0[] = "\r\n****************************************\r\n";
static prog_uchar str1[] = "        megaCAM by AliveHex \r\n";
static prog_uchar str2[] = "        v1.2 Build ";
static prog_uchar str3[] = "\r\n>";

int main() {
#if (1)
	char line_buf[CMD_BUF_LEN];
	char *buf = NULL;
	char c;
	int count = 0;
	int limit = 0;
	
	UART_OPEN(BOARD_UART_BAUDRATE);	
	
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
	
	/* init camera chip */
	//default_camera_support();	
	
	cmd_package_listen();
	
	while (1) {		
		/* skip the /r, /n and Backspace*/
		if(USART_MODE_ENABLE) {
			usart_output_str_pgm(str3);
		}
	
		buf = line_buf;
		count = 0;
		
		/* read line */
		do {
			c = UART_GETCH();
				
			if(c != '\b' && c != '\r' && c != '\n') {
				*(buf ++) = (char)c;
				count ++;
			}
			
			if(c == '\b') {
				if(buf > line_buf) {
					count --;
					buf --;
				}
			}
			
			if(USART_MODE_ENABLE) {
				if(c != '\r' && c != '\n' /*&& c != '\b'*/) {
					UART_WRITE(c);
				}
			}			
			
			/* five time input u or U, makes into ascii mode */
			if(c == 'u' || c == 'U') {
				limit ++;
				if(limit >= LIMIT_U_TIMES) {
					if(USART_MODE_ENABLE) {
						USART_MODE_ENABLE = 0;
					} else {
						USART_MODE_ENABLE = 1;
					}
					memset(line_buf, 0, CMD_BUF_LEN);
					break;
				} 
			} else {
				limit = 0;
			}	
		} while(c != '\r' && c != '\n' && count < CMD_BUF_LEN);
		*buf = 0;
		
		if(strlen(line_buf)) {
			CmdLineProcess(line_buf);	
		}
	}
#else
	// char **argv = {"read", "0", "0", "76800"};
	
	UART_OPEN(BOARD_UART_BAUDRATE);
	
	/* output welcome message and baudrate config */
	//print("\r\n********************************\r\n");
	//print("megaCAM by AliveHex \r\n");
	//print("v1.2 Build %s", __DATE__);
	//print("\r\n********************************\r\n"); 
	
	//_delay_ms(1000);
	
	cam_open();
	
	while(1) {
		//print("%d. megaCAM\r\n", i ++);
		UART_GETCH();
		cam_pick_image();
		cam_save_image();
		//read_hook(4, argv);
	}
#endif
	return 0;
}