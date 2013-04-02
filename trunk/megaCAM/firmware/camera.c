#include <util/delay.h>
#include <avr/interrupt.h>

#include "camera.h"
#include "usart.h"
#include "sccb.h"
#include "trace.h"

//static bool volatile frame_start = FALSE; // mark for VSYNC
//static bool volatile frame_stop = FALSE;
//uint8 buffer[512];

static prog_uchar str1[] = "Camera chip: ";
static prog_uchar str2[] = "OV7640\r\n";
static prog_uchar str3[] = "unknown";

unsigned char read_fifo_byte() {
	unsigned char buf;
	
	FIFO_RCLK_LOW();
	buf = FIFO_READ();
	FIFO_RCLK_HIGH();
	return buf;
}

void reset_fifo_read() {
	int i;
	
	FIFO_RRST_LOW();
	// write some dummy read clocks
	for(i = 0; i < 8; i ++) {		
		FIFO_RCLK_LOW();
		FIFO_RCLK_HIGH();
	}
	FIFO_RRST_HIGH();
}


/*
** @ OV7640 Only now
**/
int default_camera_support(void) {
	uint16 chip_id;
	
	sccb_set_dev_addr(0x42);
	chip_id = (sccb_readb(0x0a) << 8) | sccb_readb(0x0b);
	
	/* make all default B&W, QVGA */
	usart_output_str_pgm(str1);
	switch(chip_id) {
	//case 0x7660:
	//	print("OV7660\r\n");
	//	break;
	case 0x7648:
		usart_output_str_pgm(str2);
		//sccb_writeb(0x11, 1); // clock div
		sccb_writeb(0x14, 0x24); // QVGA
		sccb_writeb(0x28, 0x60); // B&W, progressive scan
		//sccb_writeb(0x71, 0x40); // PCLK gated by HREF
		break;
	default:
		usart_output_str_pgm(str3);
		usart_output_str("[id=0x");
		usart_output_hex(chip_id);
		usart_output_str("]");
		return -1;
	}
	return 0;
}

#if (1)
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
		sccb_writeb(0x14, 0x24); // QVGA
		sccb_writeb(0x28, 0x60); // B&W, progressive scan
		//sccb_writeb(0x71, 0x40); // PCLK gated by HREF
		break;
	default:
		print("unknown chip id:0x%x\r\n", chip_id);
		return -1;
	}
	
	/*
	if(sccb_writeb(0x11, 0x40) < 0) {
		goto err;
	}
#if (0)
	if(sccb_writeb(0x12, 0x15) < 0) {
		goto err;
	}
#endif
	if(sccb_writeb(0x15, 0x20) < 0) {
		goto err;
	}
#if (0)
	if(sccb_writeb(0x40, 0xd0) < 0) {
		goto err;
	}	
#endif*/
	return 0;
}
#endif

#if (1)
int cam_open() {
	FIFO_IO_INIT();
	CAM_IO_INIT();
	FIFO_WE_LOW();
	FIFO_OE_HIGH();
	
	if(camera_config() < 0) {
		return -1;
	}
	print("wait 1S for image stable\r\n");
	_delay_ms(1000);	
	return 0;
}
#endif

// VSYNC falling
#if (0)
SIGNAL(SIG_INTERRUPT0) {
	if(frame_start == FALSE) {
		FIFO_WE_HIGH();
		frame_start = TRUE;
		//print("frame start\r\n");
	} else {
		FIFO_WE_LOW();
		EIMSK = 0; // disable interrupt
		frame_stop = TRUE;
		//print("frame stop\r\n");
	}
}
#endif


int cam_pick_image() {
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
#define PINVSYNC()		(PIND & BOARD_CAM_VSYNC)
		
	while(PINVSYNC() == 0); // wait last frame end
	while(PINVSYNC()); // wait for frame intervel
	FIFO_WE_HIGH(); // frame start
	while(PINVSYNC() == 0); // wait for frame end
	FIFO_WE_LOW();
	//print("image is good in fifo\r\n");
	return 0;
}

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


#if (1)
void cam_save_image(unsigned long len) {
	/******
		if(nbytes > total_image_bytes)
			nbytes = total_image_bytes;
		SD_POWER_OFF();
		CAM_OE_ENABLE();
		while(nbytes -- ) {
			FIFO_RCLK_LOW();
			FIFO_RCLK_HIGH();
			*(buffer ++) = PORTB;
		}
		total_image_bytes -= nbytes;
		CAM_OE_DISABLE();
		if(total_image_bytes == 0) {
			fifo_power_off();				
		}
	******/
	uint32 i, j;

#if (0)	
	FIFO_OE_LOW(); // #RE low too
	//FIFO_RCLK_LOW();
	FIFO_RRST_LOW();
	// write some dummy read clocks
	for(i = 0; i < 32; i ++) {
		FIFO_RCLK_HIGH();
		FIFO_RCLK_LOW();
	}
	FIFO_RRST_HIGH();
#else

	FIFO_OUTPUT_ENABLE(); // #RE low too
	reset_fifo_read();
	
#endif
	//print("P2 320 240 255\r\n");
	for(j = 0; j < 76800; j ++) {
		FIFO_RCLK_LOW();
		UART_WRITE(FIFO_READ());
		FIFO_RCLK_HIGH();
	}
	
#if (0)
	for(j = 0; j < 150; j ++) {
		for(i = 0; i < 512; i ++) {
			FIFO_RCLK_HIGH();
			buffer[i] = FIFO_READ();
			FIFO_RCLK_LOW();
		}
		for(i = 0; i < 512; i ++) {
			UART_WRITE(buffer[i]);
		}
	}
#endif
	//FIFO_OE_HIGH();
	FIFO_OUTPUT_DISABLE(); //#RE high too
}
#endif
