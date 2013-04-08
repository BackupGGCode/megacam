#include <util/delay.h>
#include <avr/interrupt.h>

#include "camera.h"
#include "usart.h"
#include "sccb.h"
#include "trace.h"

/* strings which goes into flash space */
static prog_uchar str1[] = "Camera chip: ";
static prog_uchar str2[] = "OV7640\r\n";
static prog_uchar str3[] = "unknown";

/* internal functions */
static void reset_fifo_read(void);
static int camera_config(void);

/* internal macros */
#define PINVSYNC()		(PIND & BOARD_CAM_VSYNC)

/*
** @ capture one frame image into FIFO
**/
int cam_pick_image(void) {
#if (0)	// interrupt is toooooooo slow
	frame_start = FALSE;
	frame_stop = FALSE;
	
	// config the extern interrupt
	cli();
	EICRA = 0x3; // INT0 rasing
	EIMSK = 1; // enable interrupt
	sei();
	while(frame_start == FALSE);
	while(frame_stop == FALSE);
	cli();
#endif
	
	while(PINVSYNC() == 0); // wait last frame end
	while(PINVSYNC()); // wait for frame intervel
	FIFO_WE_HIGH(); // frame start
	while(PINVSYNC() == 0); // wait for frame end
	FIFO_WE_LOW();
	//print("image is good in fifo\r\n");
	return 0;
}

/*
** @ read image from FIFO and write to USART
** @ start: start pixel
** @ len: number of bytes to read
**/
void cam_read(unsigned long start, unsigned long len) {
	unsigned long i;
	
	FIFO_OUTPUT_ENABLE(); // #RE low too
	reset_fifo_read();
	
	for(i = 0; i < len; i ++) {
		FIFO_RCLK_LOW();
		if(i >= start) {
			UART_WRITE(FIFO_READ());
		}
		FIFO_RCLK_HIGH();
	}
	
	FIFO_OUTPUT_DISABLE(); //#RE high too
}

/*
** @ expand read image function
** @ width, height: original image width and height
¡Á¡Á @ x, y: clipping image x(width) and y(length)
**/
void ex_cam_read(uint16 width, uint16 height, uint16 x, uint16 y) {
	unsigned long i;
	unsigned long j;
	
	FIFO_OUTPUT_ENABLE(); // #RE low too
	reset_fifo_read();
	
	for(i = 0; i < height; i ++) {
		for(j = 0; j < width; j ++) {
			FIFO_RCLK_LOW();
			if(j < x && i < y) {
				UART_WRITE(FIFO_READ());
			}
			FIFO_RCLK_HIGH();
		}
	}	
	FIFO_OUTPUT_DISABLE(); //#RE high too
}

/* reset FIFO read pointer */
static void reset_fifo_read() {
	int i;
	
	FIFO_RRST_LOW();
	// write some dummy read clocks
	for(i = 0; i < 8; i ++) {		
		FIFO_RCLK_LOW();
		FIFO_RCLK_HIGH();
	}
	FIFO_RRST_HIGH();
}

/* read chip id to identify Camera chip */
static int camera_config() {
	uint16 chip_id;
	
	sccb_set_dev_addr(0x42);
	chip_id = (sccb_readb(0x0a) << 8) | sccb_readb(0x0b);
	
	switch(chip_id) {
	case 0x7660:
		print("chip OV7660\r\n");
		break;
	case 0x7648:
		print("chip OV7640\r\n");
		//sccb_writeb(0x11, 1); // clock div
		//sccb_writeb(0x14, 0x24); // QVGA
		//sccb_writeb(0x28, 0x60); // B&W, progressive scan
		//sccb_writeb(0x71, 0x40); // PCLK gated by HREF
		break;
	default:
		print("unknown chip id:0x%x\r\n", chip_id);
		return -1;
	}
	return 0;
}



