#if (0)

//***********************************************
//	Unused, wasted functions
//***********************************************


#define CMD_BUF_LEN		32
#define LIMIT_U_TIMES	6

extern int USART_MODE_ENABLE;

char line_buf[CMD_BUF_LEN];
char *buf = NULL;
char c;
int count = 0;
int limit = 0;
int console = 0;

/* Command Line UART input */
while (1) {		
	/* skip the /r, /n and Backspace*/
	usart_output_str_pgm(str3);

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
		
		if(c != '\r' && c != '\n' /*&& c != '\b'*/) {
			UART_WRITE(c);
		}			
		
	} while(c != '\r' && c != '\n' && count < CMD_BUF_LEN);
	*buf = 0;
	
	if(strlen(line_buf)) {
		CmdLineProcess(line_buf);	
	}
}

//********************************************
//	From Console functions
//********************************************

static int bdr(int argc, char **argv);
static int dadr(int argc, char **argv);
static int wreg(int argc, char **argv);
static int rreg(int argc, char **argv);
static int cap(int argc, char **argv);
static int read(int argc, char **argv);
static int help(int argc, char **argv);
static int echo(int argc, char **argv);
static int test(int argc, char **argv);
static int mrd(int argc, char **argv);

int USART_MODE_ENABLE = 1;

#define MAX_USART_BDR	(F_CPU / 8)
#define MIN_USART_BDR	(F_CPU / 65535)
#define MAX_FIFO_LEN	(375000)
#define FAILED_CODE		0x2a	// '*'
#define PASS_CODE		0x24	// '$'

/* ymodem commands */
#define	WAIT_TIMEOUT_MS	5000 
#define MARK		0x43
#define SOH			0x01	// 128 bytes per block
#define STX			0x02	// 1024 bytes per block
#define EOT			0x04	// end package
#define ACK			0x06	// ack package
#define NACK		0x15 	// nack package
#define CAN			0x18 	// cancle transfer

tCmdLineEntry g_sCmdTable[] = {
	{"echo", echo, ""},
	{"bd", 	 bdr,  ""},
	{"ad", 	 dadr, ""},
	{"wg", 	 wreg, ""},
	{"rg",   rreg, ""},
	{"cp",   cap,  ""},
	{"rd",   read, ""},
	//{"mrd",  mrd,  ""},
	//{"ts",   test, ""},
	//{"C",	 modem, ""},
	{"help", help, ""},
	{0, 0, 0}
};

prog_uchar help_message[] = 
"\r\n\
echo [empty] return the character \'$\'\r\n\
bd [baudrate{DEC}] config the serial port baudrate\r\n\
ad [address{HEX}{BYTE}] config the camera chip address\r\n\
wg [address{HEX}{BYTE}] [value{HEX}{BYTE}] write to the camera register\r\n\
rg [address{HEX}{BYTE}] read from the camera register\r\n\
cp [empty] capture one frame picture\r\n\
rd [start-pixel{DEC}][bytes-to-read{DEC}] read picture data\r\n\
help [empty] display these messages\r\n\
";


static prog_uchar str0[] = "$$$$$$$$ delete me and anything before $$$$$$$$\r\n";
static prog_uchar str1[] = "$$$$$$$$ delete me and anything behind $$$$$$$$\r\n";
static prog_uchar str2[] = "unknown commond";
static prog_uchar str3[] = "\r\nready to transfer, press enter to cancle";
static prog_uchar str4[] = "\r\nwait time out";

static void cout(const char *str) {
	if(USART_MODE_ENABLE) {
		usart_output_str("\r\n");
		usart_output_str(str);
	}
}

static void uret(char code) {
	UART_WRITE(code);
}

#if (0)
static unsigned long string2uint32(const char *str) {
	int i;
	unsigned long temp = 0;
	unsigned long multiple = 1;
	
	for(i = (strlen(str) - 1); i >= 0; i --) {
		temp += str[i] * multiple;
		multiple *= 10;
	} 
	return temp;
}
#endif

int help_all(void) {
#if (0)	
	int i;

	for(i = 0; i < sizeof(g_sCmdTable) / sizeof(tCmdLineEntry); i ++) {
		//cout(g_sCmdTable[i].pcHelp);	
		cout(g_sCmdTable[i].pcCmd);
	}
#endif
	if(USART_MODE_ENABLE)
		usart_output_str_pgm(help_message);
	return 0;
}

int cmd_entry_hook(tCmdLineEntry *pCmdEntry, int argc, char **argv) {
	int ret;
	
	if(pCmdEntry == NULL) {
		//cout("unknown command");
		if(USART_MODE_ENABLE)
			usart_output_str_pgm(str2);
		return -1;
	}
	
	ret = pCmdEntry->pfnCmd(argc, argv);
	switch(ret) {
	case 0:
		uret(PASS_CODE);
		return 0;
	case -1:
		cout(pCmdEntry->pcHelp);
		uret(FAILED_CODE);
		return -1;
	default:
		uret(FAILED_CODE);
		return -1;
	}
}

static int echo(int argc, char **argv) {
	return 0;
}

static int help(int argc, char **argv) {
#if (0)	
	int i;
	
	if (argc < 2) {
		help_all();
		return 0;
	}
	for(i = 0; i < sizeof(g_sCmdTable) / sizeof(tCmdLineEntry); i ++) {
		if(strcmp(g_sCmdTable[i].pcCmd, argv[1]) == 0) {
			cout(g_sCmdTable[i].pcHelp);
			return 0;
		}
	}
	return -1;
#endif
	help_all();
	return 0;
}

static int bdr(int argc, char **argv) {
	unsigned long baudrate;
	
	if(argc < 2) {
		return -1;
	}
	sscanf(argv[1], "%ld", &baudrate);
	if(baudrate < MIN_USART_BDR || baudrate > MAX_USART_BDR) {
		baudrate = 115200;
	}
	
	UART_OPEN(baudrate);
	return 0;
}

static int dadr(int argc, char **argv) {
	unsigned int dev_addr;
	
	if(argc < 2) {
		return -1;
	}
	sscanf(argv[1], "%x", &dev_addr);
	
	if(dev_addr > 0xFF) {
		return -1;
	}
	
	sccb_set_dev_addr(dev_addr);
	return 0;
}

static int wreg(int argc, char **argv) {
	unsigned int addr;
	unsigned int value;
	
	if(argc < 3) {
		return -1;
	}
	
	sscanf(argv[1], "%x", &addr);
	sscanf(argv[2], "%x", &value);
	
	if(addr > 0xFF || value > 0xFF) {
		return -1;
	}
	
	if(sccb_writeb(addr, value) != 0) {
		return -1;
	} else {
		return 0;
	}
}

static int rreg(int argc, char **argv) {
	unsigned int addr;
	
	if(argc < 2) {
		return -1;
	}
	
	sscanf(argv[1], "%x", &addr);
	
	if(addr > 0xFF) {
		return -1;
	}
	
	if(USART_MODE_ENABLE) {
		usart_output_str("[0x");
		usart_output_hex(sccb_readb(addr) & 0xFF);
		usart_output_str("]");
	} else {
		uret(sccb_readb(addr) & 0xFF);
	}
	return 0;
}

static int cap(int argc, char **argv) {
	cam_pick_image();
	return 0;
}

int read_hook(int argc, char **argv) {
	return read(argc, argv);
}

static int read(int argc, char **argv) {
	unsigned long start;
	unsigned long len;
	unsigned long i;

	
	if(argc < 3) {
		return -1;
	}	
	
	print("[%s][%s]\r\n", argv[1], argv[2]);
	
	sscanf(argv[1], "%ld", &start);
	sscanf(argv[2], "%ld", &len);

	//x = 0;
	//y = 0;
	//len = 76800;
	
	print("%ld %ld\r\n", start, len);
	
	if(len > MAX_FIFO_LEN) {
		return -1;
	}
	
	usart_output_str_pgm(str0);
		
	cam_read(start, len);
	
	usart_output_str_pgm(str1);
	return 0;
}

static int wait(char c) {
	unsigned int timeout = WAIT_TIMEOUT_MS;
	char c2;
	
	while( (UCSR0A & (1 << 7)) == 0 ) {
		-- timeout;
		if(timeout == 0) {
			usart_output_str_pgm(str4);
			return -1;
		}
		_delay_ms(1);
	}
	c2 = UDR0 & 0xff;
	if(c == c2)
		return 0;
	else
		return c2;
}

#if (1)
static int write_block(char *buf, int index) {
	int i;
	unsigned int crc;
	
	crc = _crc16(0, buf, 128);
	
	UART_WRITE(SOH);
	UART_WRITE(index);
	UART_WRITE(~index);
	for(i = 0; i < 128; i ++) {
		UART_WRITE(buf[i]);
	}
	UART_WRITE(crc >> 8);
	UART_WRITE(crc & 0xFF);
	return 0;
}
#endif

static void write_start_frame(char *buffer, char *fname, unsigned long fsize) {
	int count;
	
	count = strlen(fname);
	memset(buffer, 0, 128);
	strcpy(buffer, fname);
	sprintf(buffer + count + 1, "%ld", fsize); 
	write_block(buffer, 0);
}

#if (0)
#define PACKAGE_LENGTH	128
static int modem_write(char *buf, int len, int flush, int reset_index) {
	static int index = 0;
	static unsigned long count = 0;
	//static unsigned long check_sum = 0;
	static unsigned long crc;
	char temp;
	int i;
		
	/* modem_write(NULL, 0, 0, TRUE);
	   call before one transfer */
	if(reset_index) {
		index = 0;
		count = 0;
		//check_sum = 0;
		crc = 0;
		return 0;
	}
	
	/* modem_write(first_package, 32, FALSE, FALSE);
	   modem_write(NULL, 0, TRUE, FALSE); */
	if(flush) {
		if(count) {
			temp = 0x0;
			for(i = 0; i < (PACKAGE_LENGTH - count); i ++) {
				UART_WRITE(0x0);
				//check_sum += 0x0;
				crc = crc16(crc, &temp, 1);
			}
			//UART_WRITE((check_sum % 256) & 0xFF);
			UART_WRITE(crc >> 8);
			UART_WRITE(crc & 0xFF);
			//check_sum = 0;
			crc = 0;
			/* wait for ACK */
			temp = wait(ACK);
			if(temp != 0) {
				while (1) {
					usart_output_str("\r\n[MARK1]:");
					usart_output_hex(temp);
					_delay_ms(1000);
				}
			}
		}
		count = 0;
		return 0;
	}
	
	if(buf == NULL || len == 0)
		return 0;
	
	while (1) {
		/* output package header */
		if (count == 0) {
			UART_WRITE(SOH);
			UART_WRITE(index);
			UART_WRITE(~index);
			index ++;
		} else if(count == PACKAGE_LENGTH) {
			//UART_WRITE((check_sum % 256) & 0xFF);
			//check_sum = 0;
			UART_WRITE(crc >> 8);
			UART_WRITE(crc & 0xFF);
			
			usart_output_str("CRC1=");
			usart_output_hex(crc);
			usart_output_str("\r\n");
			
			while(1);
			//crc = 0;
			count = 0;
			/* wait for ACK */
			temp = wait(ACK);
			if(temp != 0) {
				while (1) {
					usart_output_str("\r\n[MARK2]:");
					usart_output_hex(temp);
					_delay_ms(1000);
				}
			}
		}
		
		if (len == 0)
			break;
		UART_WRITE(*buf);
		//check_sum += *buf;
		crc = crc16(crc, buf, 1);
		buf ++;
		count ++;
		len --;	
	}
	return 0;
}
#endif

#define MAX_FILE_SIZE		(unsigned long)(640 * 480)
static int mrd(int argc, char **argv) {
	unsigned long fsize;
	unsigned long i;
	char pgm_header[16];
	unsigned long height;
	unsigned long width; 

	if(argc < 4)
		return -1;
	memset(pgm_header, 0, sizeof(pgm_header));

	if(strcmp(argv[1], "rgb") == 0) {
		strcpy(pgm_header, "P6");
	} else if(strcmp(argv[1], "bw") == 0) {
		strcpy(pgm_header, "P5");
	} else {
		return -1;
	}
	
	sscanf(argv[2], "%ld", &width);
	sscanf(argv[3], "%ld", &height);
	
	/* check the image resolution */
	if(width > 640 || height > 480) {
		return -1;
	} else {
		fsize = width * height;
		
		/* check the file size */
		if(fsize > MAX_FILE_SIZE)
			return -1;
			
		/* config the PGM header */
		i = sprintf(&pgm_header[2], " %ld %ld 255\n",
			width, height);
	}
	
	/* the image mark */
	usart_output_str_pgm(str0);
	
	/* output image header */
	usart_output_str(pgm_header);
	
	/* output binary code */
	FIFO_OE_LOW(); // #RE low too
	FIFO_RCLK_LOW();
	FIFO_RRST_LOW();
	
	// write some dummy read clocks
	for(i = 0; i < 32; i ++) {
		FIFO_RCLK_HIGH();
		FIFO_RCLK_LOW();
	}
	
	FIFO_RRST_HIGH();
	/* read data from FIFO and sent to uart */
	for(i = 0; i < fsize; i ++) {
		FIFO_RCLK_HIGH();
		//if(USART_MODE_ENABLE) {
		//usart_output_dec(FIFO_READ() & 0xFF);
		//usart_output_str(" ");
		//} else {
		UART_WRITE(FIFO_READ());
		//}
		FIFO_RCLK_LOW();
	}
	FIFO_OE_HIGH(); //#RE high too
	
	usart_output_str_pgm(str1);
	return 0;
}

#define IMAGE_FILE_NAME		"IMAGE.PGM"
#define MAX_FILE_SIZE		(unsigned long)(640 * 480)

//int xshot(int argc, char **argv) {
int xshot(image_t type, unsigned long width, unsigned long height) {
	unsigned long fsize;
	unsigned long i, j;
	char buffer[128];
	char pgm_header[16];
	char buf[] = IMAGE_FILE_NAME;
	// image_t type;
	// unsigned long height;
	// unsigned long width; 
	int temp;
	int index;
	char c;

#if (0)	
	if(argc < 4) {
		return -1;
	}	
#endif	
	memset(pgm_header, 0, sizeof(pgm_header));
#if (0)
	if(strcmp(argv[1], "rgb") == 0) {
		type = RGB;
		strcpy(pgm_header, "P6");
	} else if(strcmp(argv[1], "bw") == 0) {
		type = BW;
		strcpy(pgm_header, "P5");
	} else {
		return -1;
	}
	
	sscanf(argv[2], "%ld", &width);
	sscanf(argv[3], "%ld", &height);
#else
	switch(type) {
	case RGB:
		strcpy(pgm_header, "P6");
	break;
	case BW:
		strcpy(pgm_header, "P5");
	break;
	default:
		return -1;
	}


#endif	
	/* check the image resolution */
	if(width > 640 || height > 480) {
		return -1;
	} else {
		fsize = width * height;
		
		/* check the file size */
		if(fsize > MAX_FILE_SIZE)
			return -1;
			
		/* config the PGM header */
		i = sprintf(&pgm_header[2], " %ld %ld 255\n",
			width, height);
	}
	
	//usart_output_str(pgm_header);
	//usart_output_dec(fsize);
	//usart_output_str(" ");
	//usart_output_hex(fsize);
	//usart_output_str("\r\n sizeof(unsigned long)=");
	//usart_output_hex(sizeof(unsigned long));
	//return 0;
	
	/* ready to sent */
	usart_output_str_pgm(str3);
	
	/* this will reset modem package index */
	// modem_write(NULL, 0, 0, TRUE);

#if (0)	
	/* wait for 'C' */
	if(wait(MARK) != 0)
		return 1;
#else
	do {
		c = UART_GETCH();
		if(c == '\n' || c == '\r')
			return 0;
	} while(c != 'C'); 
#endif
	
	/* start package contents file name, file size, etc 
	SOH 00FF 123.123 78Bytes patch128 CRC16:
	01 00FF 3132332E313233 00 3738 [00]*118 CRC16 */

	//c = 0;
	/* file name */
	//modem_write(buf, strlen(buf), FALSE, FALSE);
	/* NULL */
	//modem_write(&c, 1, FALSE, FALSE);
	/* file size */
	//sprintf(buf, "%ld", fsize);
	//modem_write(buf, strlen(buf), FALSE, FALSE);
	/* flush out */
	//modem_write(NULL, 0, TRUE, FALSE);
	
	//return 0;
	write_start_frame(buffer, buf, fsize);

	/* wait for ACK */
	if(wait(ACK) != 0)
		return 2;
		
	/* wait for 'C' */
	if(wait(MARK) != 0) 
		return 3;

	/* ready for tansfer image data, take shot */
	//for(i = 0; i < 3; i ++) 
	cam_pick_image();
	
	/* make the FIFO ready*/
	/* output binary code */
	FIFO_OE_LOW(); // #RE low too
	FIFO_RCLK_LOW();
	FIFO_RRST_LOW();
	
	// write some dummy read clocks
	for(i = 0; i < 32; i ++) {
		FIFO_RCLK_HIGH();
		FIFO_RCLK_LOW();
	}
	
	FIFO_RRST_HIGH();
	
	/* output image header */
#if (0)
	if(pgm_header[0]) {
		modem_write(pgm_header, strlen(pgm_header), FALSE, FALSE);
		pgm_header[0] = 0;
	}
#endif
	/* read data from FIFO and sent to uart */
	index = 1;
	while (fsize) {
		if(fsize > 128) {
			j = 128;
		} else {
			j = fsize;
		}
		for(i = 0; i < j; i ++) {
			FIFO_RCLK_HIGH();
			buffer[i] = FIFO_READ();
			FIFO_RCLK_LOW();
		}
		// modem_write(&c, 1, FALSE, FALSE);
		
		do {
			write_block(buffer, index);
			temp = wait(ACK);
		} while(temp != 0);
		index ++;
		fsize -= j;
	}
	
	FIFO_OE_HIGH(); //#RE high too

	/* flush out */
	//modem_write(NULL, 0, TRUE, FALSE);

	/* STOP transfer wiht EOT */
	UART_WRITE(EOT);
	
	/* wait for NACK, often get ACK */
	wait(NACK);
	
	/* write EOT again */
	UART_WRITE(EOT);
	
	/* wait for ACK, but get MARK */
	wait(ACK);
	
	/* empty package to end this transfer
	   SOH 00FF [00]*128 CRC16 */
	//modem_write(NULL, 0, FALSE, TRUE);
	//c = 0;
	//for(i = 0; i < PACKAGE_LENGTH; i ++)
	//	modem_write(&c, 1, FALSE, FALSE);
	memset(buffer, 0, sizeof(buffer));
	write_block(buffer, 0);
	
	/* wait for ACK */
	if(wait(ACK) != 0)
		return 7;
		
	/* wait for 'C' */
	if(wait(MARK) != 0)
		return 8;
		
	/* all jobs done */
	return 0;
}

#if (0)
static int modem(int argc, char **argv) {
	xshot(BW, 320, 240);
	return 0;
}
#endif


static int test(int argc, char **argv) {
	return 0;
#if (0)
	unsigned long fsize;
	unsigned long i, j;
	char buffer[128];
	char pgm_header[16];
	char buf[] = IMAGE_FILE_NAME;
	// image_t type;
	// unsigned long height;
	// unsigned long width; 
	int temp;
	int index;
	char c;
	int frame_index = 0;
	
	/* this will keep the image stable */
	for(i = 0; i < 3; i ++) 
		cam_pick_image();
	
	/* wait for 'C' */
	if(wait(MARK) != 0)
		return 1;
	
	/* SOH 00FF 123.123 78Bytes patch128 CRC16:
		01 00FF 3132332E313233 00 3738 [00]*118 CRC16 */
	memset(buffer, 0, sizeof(buffer));
	memcpy(buffer, "image.pgm", 9);
	sprintf(&buffer[10], "%ld", len + strlen(header));
	write_block(buffer);
	
	/* wait for ACK */
	if(wait(ACK) != 0)
		return 2;
		
	/* wait for 'C' */
	if(wait(MARK) != 0) 
		return 3;
	
	for(j = 0; j < 256; j ++) {
		/* STX 01FE Data[1024] CRC16 */
		for(i = 0; i < 128; i ++) {
			buffer[i] = i;
		}
		write_block(buffer);
		
		/* wait for ACK */
		if(wait(ACK) != 0)
			return 4;
	}
	
	/* STX 02FD Data[1024] CRC16 */
	memset(buffer, 0x1A, 128);
	buffer[0] = 0x32;
	write_block(buffer);
	/* wait for ACK */
	if(wait(ACK) != 0)
		return 5;
	
	/* EOT */
	UART_WRITE(EOT);
	
	/* wait for NACK, but get ACK */
	//c = wait(NACK);
	c = wait(ACK);
	if(c != 0)
		return c;
	
	/* EOT */
	UART_WRITE(EOT);
	/* wait for ACK, but get MARK */
	c = wait(MARK);
	if(c != 0)
		return c;

#if (0)	
	/* wait for 'C' */
	if(wait(MARK) != 0)
		return 8;
#endif
	
	/* SOH 00FF [00]*128 CRC16 */
	frame_index = 0;
	memset(buffer, 0x00, sizeof(buffer));
	write_block(buffer);
	
	if(wait(ACK) != 0)
		return 9;
		
		/* wait for 'C' */
	if(wait(MARK) != 0)
		return 8;
	return 0;
#endif


#if (0)	
	int i;
	char buffer[128] = "file.txt 8970";
	unsigned int crc;
	
	buffer[8] = 0;
	//crc[0] = crc16(0, buffer, sizeof(buffer));
	//crc[1] = crc16(0, buffer, sizeof(buffer));
	//crc[1] = _crc16(0, buffer, sizeof(buffer));
	//crc[1] = crc_path(crc[1]);
	
	//usart_output_str("\r\n");
	//usart_output_hex(crc[0] & 0xFFFF);
	//usart_output_str(" ");
	//usart_output_hex(crc[1] & 0xFFFF);
	wait(MARK);
	
	write_block(buffer, 0);
	crc = crc16(0, buffer, 128);
	while(1) {
		usart_output_hex(crc);
		usart_output_str("\r\n");
		//crc16_hook(1);
		//usart_output_str(" ");
		//crc16_hook(1);
		//usart_output_str(" ");
		_delay_ms(1000);
	}

	char buffer[128];
	int i;
	unsigned int crc[2];
	
	memset(buffer, 0x5a, sizeof(buffer));
	crc[0] = crc16(0, buffer, sizeof(buffer));
	usart_output_str("CRC0=");
	usart_output_hex(crc[0]);
	usart_output_str("\r\n");
	
	buffer[0] = 0x5a;
	for(i = 0; i < 128; i ++) {
		modem_write(buffer, 1, FALSE, FALSE);
	}
	return 0;
#endif
}


#endif


#if (0)
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

unsigned char read_fifo_byte() {
	unsigned char buf;
	
	FIFO_RCLK_LOW();
	buf = FIFO_READ();
	FIFO_RCLK_HIGH();
	return buf;
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

#if (0)
/*
** @ open camera device
**/
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

#if (0)
int _usart_read(int timeout_ms, char c) {
	int buf = -1;
	
	while( (UCSR0A & (1 << 7)) == 0 ) {
		-- timeout_ms;
		if(timeout_ms == 0) {
			return -1;
		}
		_delay_ms(1);
	}
	buf = UDR0 & 0xFF;
	if(c == buf)
		return 0;
	return -1;
}
#endif

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

int ISR(USART_RX_vect) {  
	char data;   
		  
	data = UDR0;  
	UART_WRITE(data);
	return 0;
}  

#endif
