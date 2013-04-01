#ifndef BOARD_H
#define BOARD_H

#define BOARD_MCK					F_CPU
#define BOARD_UART_BAUDRATE			230400

// DEBUG
#define BOARD_DBG_RXD	(1 << 0)	// PD0
#define BOARD_DBG_TXD	(1 << 1)	// PD1

// CAMERA
#define BOARD_CAM_SCK		(1 << 4)	// PD4
#define BOARD_CAM_SDA		(1 << 3)	// PD3
#define BOARD_CAM_VSYNC		(1 << 2)	// PD2,INT0

// FIFO
#define BOARD_FIFO_WE		(1 << 3) 	// PB3
#define BOARD_FIFO_RRST		(1 << 5)	// PD5
#define BOARD_FIFO_RCLK		(1 << 6)	// PD6
#define BOARD_FIFO_OE		(1 << 7)	// PD7



#endif // BOARD_H