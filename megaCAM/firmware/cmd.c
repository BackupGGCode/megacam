#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <util/delay.h>

#include "usart.h"
#include "sccb.h"
#include "camera.h"
#include "types.h"
#include "trace.h"

/* internal macros */
#define UART_READ_TIMEOUT	1000
#define MAX_WIDTH_RES		640
#define MAX_HTIGHT_RES		480
#define MAX_TRANSFER_SIZE	(MAX_WIDTH_RES * MAX_HTIGHT_RES)

/* internal function */
static int usart_read(int timeout_ms);

/*
** @ listen UART command packages
**/
int cmd_package_listen() {
	cam_cmd_t cp;
	char *buf;
	int count;
	int c;
	uint16 var0[2];
	uint16 var1[2];
	
	while (1) {
		buf = (char *)&cp;
		count = 0;
		do {
			c = usart_read(UART_READ_TIMEOUT);
			if(c == -1) {
				buf = (char *)&cp;
				count = 0;
				//usart_output_str("timeout");
			} else {
				*buf ++ = c;
				count ++;
			}	
		} while (count < sizeof(cam_cmd_t));

		/* check the cmd */
		if((cp.cmd + cp.para1 + cp.para2) != cp.check_sum) {
			while (1) {
				usart_output_str("sum error: C=");
				usart_output_hex(cp.cmd + cp.para1 + cp.para2);
				usart_output_str("R=");
				usart_output_hex(cp.check_sum);
				_delay_ms(1000);
			}
			continue;
		}
		
#if (0)
		while(1) {
			usart_output_str("0x");
			usart_output_hex(cp.para1);
			usart_output_str("0x");
			usart_output_hex(cp.para2);
			usart_output_str("\r\n");
			_delay_ms(1000);
		}
#endif
		
		switch(cp.cmd) {
		case CMD_WRITE_OVREG:
			// check the addr and value
			if(cp.para1 > 0xFF || cp.para2 > 0xFF) {
				UART_WRITE(CMD_FAILED);
			} else {
				if(sccb_writeb(cp.para1, cp.para2) != 0) {
					UART_WRITE(CMD_FAILED);
				} else {
					UART_WRITE(CMD_PASS);
				}
			}
		break;
		case CMD_READ_OVREG:
			if(cp.para1 > 0xFF) {
				UART_WRITE(CMD_FAILED);
			} else {
				UART_WRITE(CMD_PASS);
				UART_WRITE(sccb_readb(cp.para1));
			}
		break;
		case CMD_CONF_OVADR:
			if(cp.para1 > 0xFF) {
				UART_WRITE(CMD_FAILED);
			} else {
				sccb_set_dev_addr(cp.para1);
				UART_WRITE(CMD_PASS);
			}
		break;
		case CMD_CONF_BDR:
			if(cp.para1 < 1600 || cp.para1 > 3000000){
				UART_WRITE(CMD_FAILED);
			} else {
				UART_WRITE(CMD_PASS);
				_delay_ms(100);
				UART_OPEN(cp.para1);
			}
		break;
		case CMD_CAPTURE:
			cam_pick_image();
			UART_WRITE(CMD_PASS);
		break;
		case CMD_ECHO:
			UART_WRITE(CMD_PASS);
			UART_WRITE('E');
		break;
		case CMD_READ:
			if(cp.para1 > MAX_TRANSFER_SIZE || cp.para2 > MAX_TRANSFER_SIZE) {
				UART_WRITE(CMD_FAILED);
				break;
			}
			UART_WRITE(CMD_PASS);
			cam_read(cp.para1, cp.para2);
		break;
		case CMD_OVHW_RESET:
			// PB4 MISO
		break;
		case CMD_EXREAD:		
			memcpy(var0, &cp.para1, sizeof(cp.para1));
			memcpy(var1, &cp.para2, sizeof(cp.para2));
			
			
			if(var0[0] > MAX_WIDTH_RES || var1[0] > MAX_WIDTH_RES || var1[0] > var0[0] ||\
			   var0[1] > MAX_HTIGHT_RES || var1[1] > MAX_HTIGHT_RES || var1[1] > var0[1]) {
				UART_WRITE(CMD_FAILED);
			}
			UART_WRITE(CMD_PASS);
#if (0)
			while(1) {
				usart_output_hex(var0[0]);
				usart_output_str(" ");
				usart_output_hex(var0[1]);
				usart_output_str(" ");
				usart_output_hex(var1[0]);
				usart_output_str(" ");
				usart_output_hex(var1[1]);
				usart_output_str("\r\n");
			}
#endif
			ex_cam_read(var0[0], var0[1], var1[0], var1[1]);
		break;
		default:
			continue;
		}
	}
	return 0;
}

/* read USART with timeout */
static int usart_read(int timeout_ms) {
	while( (UCSR0A & (1 << 7)) == 0 ) {
		-- timeout_ms;
		if(timeout_ms == 0) {
			return -1;
		}
		//_delay_ms(1);
	}
	return (UDR0 & 0xff);
}





