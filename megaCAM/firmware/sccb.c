/*
============================================================
	名称: sccb.c
	描述: SCCB 总线协议
	作者: L.H.R		jk36125@gmail.com
	时间: 2009-12-10 9:44:51
	版本: 1.0
	备注: 无
============================================================
*/
#include <avr/io.h>
#include <util/delay.h>

#include "board.h"
#include "sccb.h"
#include "trace.h"

// i2c device address
static uint8 dev_raddr = 0;
static uint8 dev_waddr = 0;

#define I2C_STARTHOLD_DELAY		4
#define I2C_CLKLOW_DELAY		5
#define I2C_CLKHIGH_DELAY		4
#define I2C_DATHOLD_DELAY		1
#define I2C_DATSETUP_DELAY		1
#define I2C_STOPHOLD_DELAY		4
#define I2C_BUSFREE_DELAY		5
#define I2C_LOWSUBDHOLD_DELAY	1 //(I2C_CLKLOW_DELAY - I2C_DATHOLD_DELAY)

#define I2C_SCK_OUTPUT()	DDRD |= BOARD_CAM_SCK;\
							PORTD |= BOARD_CAM_SCK
#define I2C_SDA_OUTPUT()	DDRD |= BOARD_CAM_SDA
#define I2C_SDA_INPUT()		DDRD &= ~BOARD_CAM_SDA;\
							PORTD |= BOARD_CAM_SDA
#define I2C_SDA_LOW()		PORTD &= ~BOARD_CAM_SDA
#define I2C_SDA_HIGH()		PORTD |= BOARD_CAM_SDA
#define I2C_SCK_LOW()		PORTD &= ~BOARD_CAM_SCK
#define I2C_SCK_HIGH()		PORTD |= BOARD_CAM_SCK
#define I2C_SDA()			(PIND & BOARD_CAM_SDA)

#define I2C_DELAY(us)	_delay_us(us * 10)

// send i2c START signal
void i2c_start() {
	// Start condition: CLK = H, DAT = H -> L
	//io_config_output(host->io_host, host->dat_flag);
	//io_config_output(host->io_host, host->clk_flag);
	I2C_SCK_OUTPUT();
	I2C_SDA_OUTPUT();
	
	/*  SDA  L/H -> H -> L -> L
 	 *	SCL	   L -> H -> H -> L
 	 * */
	//io_set(host->io_host, host->dat_flag);
	I2C_SDA_HIGH();
	//host->delay_ns(host->time_para.dat_hold);
	I2C_DELAY(I2C_DATHOLD_DELAY);
	//io_set(host->io_host, host->clk_flag);
	I2C_SCK_HIGH();
	//io_clear(host->io_host, host->dat_flag);
	I2C_SDA_LOW();
	//host->delay_ns(host->time_para.start_hold);
	I2C_DELAY(I2C_STARTHOLD_DELAY);
	//io_clear(host->io_host, host->clk_flag);
	I2C_SCK_LOW();
}

// send i2c STOP signal
void i2c_stop() {
	// Stop condition: CLK = H, DAT = L -> H
	//io_config_output(host->io_host, host->dat_flag);
	//io_config_output(host->io_host, host->clk_flag);	
	I2C_SCK_OUTPUT();
	I2C_SDA_OUTPUT();
	
	/*	SDA	 L/H -> L -> L -> H
 	 *	SCL	   L -> L -> H -> H	
 	 * */
	//io_clear(host->io_host, host->dat_flag);
	I2C_SDA_LOW();
	//host->delay_ns(host->time_para.clk_low_sub_dat_hold);
	I2C_DELAY(I2C_LOWSUBDHOLD_DELAY);
	//io_set(host->io_host, host->clk_flag);
	I2C_SCK_HIGH();
	//host->delay_ns(host->time_para.stop_hold);
	I2C_DELAY(I2C_STOPHOLD_DELAY);
	//io_set(host->io_host, host->dat_flag);
	I2C_SDA_HIGH();
	//host->delay_ns(host->time_para.bus_free);
	I2C_DELAY(I2C_BUSFREE_DELAY);
}

//*----------------------------------------------------------------------------
//*  Name     : sim_i2c_write_bit()
//*  Brief    : Write a bit on I2C bus
//*  Argument : bit = 0 or bit > 0
//*  Return   : None
//*----------------------------------------------------------------------------
static void i2c_write_bit(bool bit) {			   		  	
	//host->delay_ns(host->time_para.dat_hold);
	I2C_DELAY(I2C_DATHOLD_DELAY);
	if(bit) {
		//io_set(host->io_host, host->dat_flag);
		I2C_SDA_HIGH();
	} else {
		//io_clear(host->io_host, host->dat_flag);
		I2C_SDA_LOW();
	}
	// remain clock low
	//host->delay_ns(host->time_para.clk_low_sub_dat_hold);
	I2C_DELAY(I2C_LOWSUBDHOLD_DELAY);
	// clock high
	//io_set(host->io_host, host->clk_flag);
	I2C_SCK_HIGH();
	//host->delay_ns(host->time_para.clk_high);
	I2C_DELAY(I2C_CLKHIGH_DELAY);
	//io_clear(host->io_host, host->clk_flag);	
	I2C_SCK_LOW();
}



//*----------------------------------------------------------------------------
//*  Name     : sim_i2c_read_bit()
//*  Brief    : read a bit from I2C bus
//*  Argument : None
//*  Return   : bit value: 0 or 1
//*----------------------------------------------------------------------------
static uint8 i2c_read_bit() {
	uint8 state;
	
	//host->delay_ns(host->time_para.clk_low);
	I2C_DELAY(I2C_CLKLOW_DELAY);
	//io_set(host->io_host, host->clk_flag);
	I2C_SCK_HIGH();
	//host->delay_ns(host->time_para.clk_high);
	I2C_DELAY(I2C_CLKHIGH_DELAY);
	//state = io_get_input(host->io_host, host->dat_flag);
	state = I2C_SDA();
	//io_clear(host->io_host, host->clk_flag);
	I2C_SCK_LOW();
	return state;
}

// i2c write one byte
int i2c_writeb(uint8 dat) {
	int i;
	
	//io_config_output(host->io_host, host->dat_flag);
	I2C_SDA_OUTPUT();
	for( i = 0; i < 8; i++) {
		i2c_write_bit((dat << i) & 0x80);
	}
	//io_config_input(host->io_host, host->dat_flag);
	I2C_SDA_INPUT();
	// check ack
	if(i2c_read_bit()) {
		print("call to I2C NOACK\r\n");
		// no ack
		return -1;
	} else {
		// ack
		return 0;
	}
}

// i2c read one byte
uint8 i2c_readb(uint8 wack) {
	uint8 buf = 0;
	int i;
	
	//io_config_input(host->io_host, host->dat_flag);
	I2C_SDA_INPUT();
	for( i = 0; i < 8; i ++) {
		buf <<= 1;
		if(i2c_read_bit()) {
			buf |= 1;
		}
	}
	// ack or not
	//io_config_output(host->io_host, host->dat_flag);
	I2C_SDA_OUTPUT();
	if(wack == TRUE) {
		i2c_write_bit(0);
	} else {
		i2c_write_bit(1);
	}
	return buf;
}


//*----------------------------------------------------------------------------
//*  Name     : sccb_set_dev_addr()
//*  Brief    : Set device address, for OV7660/OV7670, it's 0x42
//*  Argument : Device address
//*  Return   : None
//*----------------------------------------------------------------------------
void sccb_set_dev_addr(uint8 device_address) {	
	dev_waddr = device_address & 0xfe;
	dev_raddr  = device_address | 0x01;
}



//*----------------------------------------------------------------------------
//*  Name     : sccb_write_byte()
//*  Brief    : Write single byte on SCCB bus
//*  Argument : dat, internal address addr
//*  Return   : Error = -1, OK = 0
//*----------------------------------------------------------------------------
int sccb_writeb(uint8 addr, uint8 dat) {
	int state;
	
	// write start condition
	i2c_start();
	
	// write device address
	state = i2c_writeb(dev_waddr);
	if(state < 0) {
		print("call to SCCBW: write device address\r\n");
		goto stop_tran;
	}
	
	// write internal address
	state = i2c_writeb(addr);
	if(state < 0) {
		print("call to SCCBW: write internal address\r\n");
		goto stop_tran;
	}
	// write data
	state = i2c_writeb(dat);
stop_tran:
	// write stop condition
	i2c_stop();
	return state;
}



//*----------------------------------------------------------------------------
//*  Name     : sccb_read_byte()
//*  Brief    : Read a byte from internal address
//*  Argument : Internal address addr
//*  Return   : Data
//*----------------------------------------------------------------------------
int sccb_readb(uint8 addr) {
	int state;
	
	// dummy write for address
	i2c_start();
	state = i2c_writeb(dev_waddr);
	if(state < 0) {
		print("call to SCCBR: write device address\r\n");
		goto stop_tran;
	}
	state = i2c_writeb(addr);
	if(state < 0) {
		print("call to SCCBR: write internal address\r\n");
		goto stop_tran;
	}
	i2c_stop();
	
	// random read
	i2c_start();
	state = i2c_writeb(dev_raddr);
	if(state < 0) {
		print("call to SCCBR: write device address\r\n");
		goto stop_tran;
	}
	state = i2c_readb(FALSE);
stop_tran:
	i2c_stop();
	return state;
}




