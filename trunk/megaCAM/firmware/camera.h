#ifndef _CAMERA_H_
#define _CAMERA_H_

#include <avr/io.h>

#include "board.h"
#include "types.h"

/****** ALL IMAGE IS IN B&W MODE, VGA SIZE ******/

#define VGA_IMAGE_SIZE		(640 * 480)					// VGA image size in byte
#define CACHE_BUFFER_SIZE	512							// block buffer size
#define READ_BUFFER_TIMES	(VGA_IMAGE_SIZE / CACHE_BUFFER_SIZE)

// CAMERA IOs
#define CAM_IO_INIT()		DDRD &= ~BOARD_CAM_VSYNC;\
							DDRD |= (BOARD_CAM_SCK | BOARD_CAM_SDA);\
							PORTD |= (BOARD_CAM_VSYNC | BOARD_CAM_SCK | BOARD_CAM_SDA)

#define FIFO_DAT_INIT()		DDRC &= ~0x3f;\
							DDRB &= ~((1 << 2) | (1 << 1))

// FIFO IOs
#define FIFO_IO_INIT()		FIFO_DAT_INIT();\
							DDRD |= (BOARD_FIFO_OE | BOARD_FIFO_RRST | BOARD_FIFO_RCLK);\
							DDRB |= BOARD_FIFO_WE;\
							PORTD |= (BOARD_FIFO_OE | BOARD_FIFO_RRST | BOARD_FIFO_RCLK);\
							PORTB |= BOARD_FIFO_WE

#define FIFO_RRST_LOW()		PORTD &= ~BOARD_FIFO_RRST	// enable FIFO read reset
#define FIFO_RRST_HIGH()	PORTD |= BOARD_FIFO_RRST	// disable FIFO read reset

#define FIFO_OE_LOW()		PORTD &= ~BOARD_FIFO_OE	// enbale FIFO output
#define FIFO_OE_HIGH()		PORTD |= BOARD_FIFO_OE	// disable FIFO output

#define FIFO_RCLK_LOW()		PORTD &= ~BOARD_FIFO_RCLK	// read clock low
#define FIFO_RCLK_HIGH()	PORTD |= BOARD_FIFO_RCLK	// read clock high

#define FIFO_WE_LOW()		PORTB &= ~BOARD_FIFO_WE
#define FIFO_WE_HIGH()		PORTB |= BOARD_FIFO_WE

// FIFO DATA READ
#define FIFO_READ()			((PINC & 0x3f) | ((PINB << 5) & 0xc0)) 
							
#define FIFO_OUTPUT_ENABLE()	FIFO_OE_LOW()
#define FIFO_OUTPUT_DISABLE()	FIFO_OE_HIGH()

typedef enum {
	RGB,
	BW,
} image_t;


#define CMD_WRITE_OVREG		0x80
#define CMD_READ_OVREG		0x81
#define CMD_CONF_OVADR		0x82
#define CMD_CONF_BDR		0x83
#define CMD_CAPTURE			0x84
#define CMD_ECHO			0x85
#define CMD_READ			0x86
#define CMD_OVHW_RESET		0x87
#define CMD_PASS			0x00
#define CMD_FAILED			0x01

typedef struct {
	uint32	cmd;
	uint32	para1;
	uint32	para2;
	uint32	check_sum;
} cam_cmd_t;


extern int cam_open(void);
extern int default_camera_support(void);
extern int cam_pick_image(void);	// pick one frame image, return when image is ready in FIFO
extern void cam_read_image(uint16 nbytes, uint8 * buffer); // read data from fifo
extern void cam_read(unsigned long start, unsigned long len);

#endif // ifndef _CAMERA_H_