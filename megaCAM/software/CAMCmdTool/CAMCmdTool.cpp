// CAMCmdTool.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "CAMCmdTool.h"
#include "wincom.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

CWinApp theApp;
char buffer[1024 * 768 * 2];

using namespace std;

#define BUF_SIZE	(320 * 240)
#define TIME_OUTMS	(10 * 1000)

#define CMD_WRITE_OVREG		0x80
#define CMD_READ_OVREG		0x81
#define CMD_CONF_OVADR		0x82
#define CMD_CONF_BDR		0x83
#define CMD_CAPTURE			0x84
#define CMD_ECHO			0x85
#define CMD_READ			0x86
#define CMD_PASS			0x00
#define CMD_FAILED			0x01

typedef struct {
	unsigned int cmd;
	unsigned int para1;
	unsigned int para2;
	unsigned int check_sum;
} cam_cmd_t;

typedef enum {
	IMAGE_FORMAT_GRAY8,
	IMAGE_FORMAT_RGB565,
	IMAGE_FORMAT_RGB555,
	IMAGE_FORMAT_RAW,
} image_t;

class Camera {
public:
	int port;
	int baudrate;
	int timeout;

private:
int uart_timeout_read(int port, unsigned int timeout_ms) {
	char c = -1;
	int nbytes;
	int tmark;

	tmark = GetTickCount();
	do {
		nbytes = COM::read(port, &c, 1, 1);
		if(nbytes == 0) {
			if((GetTickCount() - tmark) > timeout_ms)
				return -1;
		}
	} while (nbytes == 0);
	return c & 0xFF;
}

int write_cmd(int uport, unsigned int cmd, unsigned int para1, unsigned int para2, int timeout) {
	unsigned int check_sum = cmd + para1 + para2;
	int res = -1;
	cam_cmd_t ctrl;

	ctrl.cmd = cmd;
	ctrl.para1 = para1;
	ctrl.para2 = para2;
	ctrl.check_sum = check_sum;
	COM::write(uport, (char *)&ctrl, 1, sizeof(cam_cmd_t));
	res = uart_timeout_read(uport, timeout);
	if(res == -1)
		return -1;
	if(res == CMD_PASS)
		return 0;
	else
		return -1;
}

public:
int init_com(int port, int baudrate, int timeout) {
	int limit = 6;
	int ret;
	
	do {
		ret = COM::open(port);
		if(ret != 0)
			Sleep(1000);
	} while (ret != 0 && -- limit);

	if(limit == 0) {
		//printf("failed to open COM%d", port);
		return -1;
	}
	COM::ioctrl(port, USART_CMD_MODE, USART_STOP_1BIT | USART_DATA_8BIT | USART_PARITY_NONE);
	COM::ioctrl(port, USART_CMD_BAUDRATE, baudrate);
	COM::ioctrl(port, USART_CMD_BUFLEN, 10240);

	this->port = port;
	this->baudrate = baudrate;
	this->timeout = timeout;
	return 0;
}

int echo(void) {
	int echo;

	write_cmd(this->port, CMD_ECHO, NULL, NULL, this->timeout);
	echo = uart_timeout_read(this->port, this->timeout);
	if(echo == 'E') {
		return 0;
	} else {
		return -1;
	}
}

int open(int port, int baudrate, int timeout) {
	if(init_com(port, baudrate, timeout) != 0)
		return -1;
	if(this->echo() != 0)
		return -1;
	return 0;
}

int close(void) {
	COM::close(this->port);
	return 0;
}

int reconfig_usart_bandrate(unsigned int baudrate) {
	return write_cmd(this->port, CMD_CONF_BDR, baudrate, NULL, this->timeout);
}

int read_ovreg(int addr) {	
	write_cmd(this->port, CMD_READ_OVREG, addr, NULL, this->timeout);
	return uart_timeout_read(this->port, this->timeout);
}

int write_ovreg(int addr, int value) {
	return write_cmd(this->port, CMD_WRITE_OVREG, addr, value, this->timeout);
}

int config_ovaddr(int dev_addr) {
	return write_cmd(this->port, CMD_CONF_OVADR, dev_addr, NULL, this->timeout);
}

int capture(void) {
	return write_cmd(this->port, CMD_CAPTURE, NULL, NULL, this->timeout);
}

int read(char *buffer, int start_pixel, int length) {
	int nbytes;
	int count;
	unsigned int time_mark;
	int pre_cache;
	
	if(write_cmd(this->port, CMD_READ, start_pixel, length, this->timeout) != 0) {
		return -1;
	}
	
	printf("waitting for data(%d) ...\n", length);
	time_mark = GetTickCount();
	count = 0;
	pre_cache = 0;
	while(1) {
		nbytes = COM::read(this->port, buffer + count, 0, 10);
		if(nbytes) {
			count += nbytes;
			if(count * 100 / length > pre_cache) {
				pre_cache = count * 100 / length;
				printf("\r");
				printf("rcv %d bytes(%d%%)", count, pre_cache);
			}
			time_mark = GetTickCount();
		} else {
			if((GetTickCount() - time_mark) > timeout) {
				printf("\n-> timeout! <-\n");
				COM::close(port);
				getchar();
				return -1;
			}
		}
		if(count >= length) {
			break;
		}		
	}
	printf("\r\n");
	return 0;
}
};


Camera myCamera;


#if (0)
int test(int port, char *buffer) {
	int value;
	//char buffer[320 * 240];
	//const unsigned char pgm_header[] = "P5 320 240 255\r";
	
	if(init_com(3, 230400, 1000) != 0)
		return -1;

	if(echo() == -1) {
		printf("echo test failed\r\n");
		return -1;
	} else {
		printf("camera is ready\r\n");
	}

	if(config_ovaddr(0x42) !=- 0) {
		printf("config ov-device-address failed\r\n");
		return -1;
	} else {
		printf("config OV-DEVICE-ADDRESS 0x42\r\n");
	}

	value = read_ovreg(0x0a);
	if(value == -1) {
		printf("read addr 0x0A failed");
		return -1;
	} else {
		printf("address[0x0A]=0x%x\r\n", value);
	}

	value = read_ovreg(port, 0x0b, 1000);
	if(value == -1) {
		printf("read addr 0x0B failed");
		return -1;
	} else {
		printf("address[0x0B]=0x%x\r\n", value);
	}
	
	if(write_ovreg(port, 0x14, 0x04, 1000) != 0) {
		printf("write addr 0x14 failed\r\n");
		return -1;
	} else {
		printf("write address 0x14 with 0x04\r\n");
	}

	value = read_ovreg(port, 0x14, 1000);
	if(value == -1) {
		printf("read addr 0x14 failed");
		return -1;
	} else {
		printf("address[0x14]=0x%x\r\n", value);
	}

	if(write_ovreg(port, 0x14, 0x24, 1000) != 0) {
		printf("write addr 0x24 failed\r\n");
		return -1;
	} else {
		printf("write address 0x14 with 0x24\r\n");
	}

	value = read_ovreg(port, 0x14, 1000);
	if(value == -1) {
		printf("read addr 0x14 failed");
		return -1;
	} else {
		printf("address[0x14]=0x%x\r\n", value);
	}

	if(capture(port, 100) != 0) {
		printf("capture image faile\r\n");
		return -1;
	} else {
		printf("capture image pass\r\n");
	}

	if(read(port, buffer, 0, 320 * 240, 1000) != 0) {
		printf("read image data failed\r\n");
		return -1;
	} else {
		//printf("saving image ...");
	}
	
	//usart_close(3);
#if (0)
	fh = fopen("default.pgm", "wb");
	if (fh == NULL) {
		printf("call to open default.pgm\n");
		getchar();
		return -1;
	}
	fwrite(pgm_header, 1, strlen(pgm_header), fh);
	fwrite(buffer, 1, 320 * 240, fh);
	fclose(fh);
	printf("OK\n");
#endif
	return 0;
}
#endif


class image {

public:
	static int save_image(wchar_t *fname, char *buf, int width, int height, image_t color_type, GUID ftype) {
		CImage image;
		int i, j;
		unsigned int image_size = width * height;
		BYTE R, G, B;
		unsigned char temp[2];

		if(image.Create(width, height, 24, 0) == 0) {
			return -1;
		}
		
		for(i = 0; i < height; i ++) {
			for(j = 0; j < width; j ++) {
				switch(color_type) {
				case IMAGE_FORMAT_RGB565:
					temp[0] = *buf ++;
					temp[1] = *buf ++;
					R = temp[0] & 0xF8;
					G = ((temp[0] << 5) & 0xE0) | ((temp[1] >> 3) & 0x1C);
					B = (temp[1] << 3) & 0xF8;
					/* rgb turn out bgr ... */
					image.SetPixelRGB(j, i, B, G, R);
				break;
				case IMAGE_FORMAT_RGB555:
				break;
				case IMAGE_FORMAT_GRAY8:
					image.SetPixelRGB(j, i, *buf, *buf, *buf);
					buf ++;
				break;
				case IMAGE_FORMAT_RAW:
					// the first line
				
					if( (i % 2) == 0) {
						if( (j % 2) == 0) { /* RED */
							R = *buf;
							G = *(buf + 1);
							B = *(buf + 1 + width);
							image.SetPixelRGB(j, i, B, G, R);
						} else { /* GREEN */
							G = *buf; 
							R = *(buf - 1);
							B = *(buf + width);
							image.SetPixelRGB(j, i, B, G, R);
						}
					} else {
					// the second line
						if( (j % 2) == 0) { /* GREEN */
							G = *buf; 
							R = *(buf - width);
							B = *(buf + 1);
							image.SetPixelRGB(j, i, B, G, R);
						} else { /* BLUE */
							B = *buf;
							R = *(buf - width - 1);
							G = *(buf - 1);
							image.SetPixelRGB(j, i, B, G, R);
						}
					}
					buf ++;
				break;
				default:
					image.SetPixelRGB(j, i, 255, 0, 0);
				}
			}
		}
		image.Save(fname, ftype);
		image.Destroy();
		return 0;
	}
};


static int regBitSet(int address, int mask) {
	int buf;

	buf = myCamera.read_ovreg(address);
	if(buf != -1) {
		buf |= mask;
		return myCamera.write_ovreg(address, buf);
	}
	return -1;
}

static int regBitClear(int address, int mask) {
	int buf;

	buf = myCamera.read_ovreg(address);
	if(buf != -1) {
		buf &= ~mask;
		return myCamera.write_ovreg(address, buf);
	}
	return -1;
}

#if (0)
static int configQVGA(int port) {
	return regBitSet(port, 0x14, (1 << 5));
}

static int configVGA(int port) {
	return regBitClear(port, 0x14, (1 << 5));
}

static int config2OV7141(int port) {
	return regBitSet(port, 0x28, (1 << 6));
}

static int config2OV7640(int port) {
	return regBitClear(port, 0x28, (1 << 6));
}

static int configRGB565(int port){
	if(regBitSet(port, 0x12, (1 << 3)) != 0)
		return -1;
	if(regBitClear(port, 0x1F, (1 << 2)) != 0)
		return -1;
	if(regBitSet(port, 0x1F, (1 << 4)) != 0)
		return -1;
	if(regBitClear(port, 0x28, (1 << 7)) != 0)
		return -1;
	return 0;
}

static int configRAW(int port) {
	if(regBitSet(port, 0x12, (1 << 3)) != 0)
		return -1;
	if(regBitClear(port, 0x1F, ((1 << 2) | (1 << 4))) != 0)
		return -1;
	if(regBitSet(port, 0x28, (1 << 7)) != 0)
		return -1;
	return 0;
}
#endif

static void help(const char *cmd) {
	return;
}

static int sccb_addr(int argc, char **argv) {
	int addr;

	if(argc < 2) {
		help("sccb-addr");
		return -1;
	} else {
		sscanf(argv[1], "%x", &addr);
		
		printf("set sccb device address=[0x%x] ... ", addr);
		if(myCamera.config_ovaddr(addr)!= 0) {
			printf("[error]\n");
			return -1;
		} else {
			printf("[ok]\n");
			return 0;
		}
	}
}

static int open(int argc, char **argv) {
	int port;
	int baudrate;
	int timeout;

	if(argc < 4) {
		help("open");
		return -1;
	}

	sscanf(argv[1], "%d", &port);
	sscanf(argv[2], "%d", &baudrate);
	sscanf(argv[3], "%d", &timeout);

	printf("open camera on com%d[%d], timeout[%d]ms ... ", port, baudrate, timeout);
	if(myCamera.open(port, baudrate, timeout) != 0) {
		printf("[error]\n");
		return -1;
	} else {
		printf("[ok]\n");
		return 0;
	}
}

static int wreg(int argc, char **argv) {
	int addr;
	int value;

	if(argc < 3) {
		help("wreg");
		return -1;
	} else {
		sscanf(argv[1], "%x", &addr);
		sscanf(argv[2], "%x", &value);
		
		printf("write value[0x%x] to register[0x%x] ... ", value, addr);
		if(myCamera.write_ovreg(addr, value) != 0) {
			printf("[error]\r\n");
			return -1;
		} else {
			printf("[ok]\r\n");
			return 0;
		}
	}
}

static int rreg(int argc, char **argv) {
	int addr;
	int value;

	if(argc < 3) {
		help("rreg");
		return -1;
	} else {
		sscanf(argv[1], "%x", &addr);
					
		printf("read register[0x%x] ... ", addr);
		value = myCamera.read_ovreg(addr);
		if(value == -1) {
			printf("[error]\n");
			return -1;
		} else {
			printf("[ok][reg=0x%x]\r\n", value); 
			return 0;
		}
	}
}

static int clrbit(int argc, char **argv) {
	int addr;
	int bitNum;

	if(argc < 3) {
		help("clrbit");
		return -1;
	} else {
		sscanf(argv[1], "%x", &addr);
		sscanf(argv[2], "%d", &bitNum);
		
		printf("clear register[0x%x]-bit[%d] ... ", addr, bitNum);
		if(regBitClear(addr, (1 << bitNum)) != 0) {
			printf("[error]\n");
			return -1;
		} else {
			printf("[ok][reg=0x%x]\n", myCamera.read_ovreg(addr));
			return 0;
		}
	}
}

static int setbit(int argc, char **argv) {
	int addr;
	int bitNum;

	if(argc < 3) {
		help("setbit");
		return -1;
	} else {
		sscanf(argv[1], "%x", &addr);
		sscanf(argv[2], "%d", &bitNum);
		
		printf("set register[0x%x]-bit[%d] ... ", addr, bitNum);
		if(regBitSet(addr, (1 << bitNum)) != 0) {
			printf("[error]\n");
			return -1;
		} else {
			printf("[ok][reg=0x%x]\n", myCamera.read_ovreg(addr));
			return 0;
		}
	}
	return 0;
}

static int bd(int argc, char **argv) {
	int bd;

	if(argc < 2) {
		help("bd");
		return -1;
	} else {
		sscanf(argv[1], "%d", &bd);
		
		printf("set camera baudrate=[%d] ... ", bd);
		if(myCamera.reconfig_usart_bandrate(bd) != 0) {
			printf("[error]\n");
			return -1;
		} else {
			printf("[ok]\n");
			myCamera.close();
			Sleep(500);
			printf("reopen com%d[%d] ... ", myCamera.port, myCamera.baudrate);
			if(myCamera.open(myCamera.port, bd, myCamera.timeout) != 0) {
				printf("[error]\n");
				return -1;
			} else {
				printf("[ok]\n");
				return 0;
			}
		}
	}
}

static int shot(int argc, char **argv) {
	image_t type;
	int width;
	int height;
	GUID sformat;
	int fsize;
	
	if(argc < 5) {
		help("shot");
		return -1;
	} else {
		sscanf(argv[1], "%d", &width);
		sscanf(argv[2], "%d", &height);
		
		if(strcmp(argv[3], "rgb565") == 0) {
			type = IMAGE_FORMAT_RGB565;
			fsize = width * height * 2;
		} else if(strcmp(argv[3], "rgb555") == 0) {
			type = IMAGE_FORMAT_RGB555;
			fsize = width * height * 2;
		} else if(strcmp(argv[3], "raw") == 0) {
			type = IMAGE_FORMAT_RAW;
			fsize = width * height;
		} else if(strcmp(argv[3], "gray") == 0) {
			type = IMAGE_FORMAT_GRAY8;
			fsize = width * height;
		} else {
			printf("unknown image format\n");
			return -1;
		}
		
		time_t rawtime;
		struct tm * timeinfo;
		wchar_t wbuffer[128];

		time(&rawtime);
		timeinfo = localtime (&rawtime);

		if(strcmp(argv[4], "jpg") == 0) {
			sformat = Gdiplus::ImageFormatJPEG;
			wcsftime(wbuffer, 128, L"%Y-%m-%d-%H%M%S.jpg", timeinfo);
		} else if(strcmp(argv[4], "png") == 0) {
			sformat = Gdiplus::ImageFormatPNG;
			wcsftime(wbuffer, 128, L"%Y-%m-%d-%H%M%S.png", timeinfo);
		} else {
			printf("unknown image format\n");
			return -1;
		}
		printf("capture image ... ");
		if(myCamera.capture() != 0) {
			printf("[error]\n");
			return -1;
		} else {
			printf("[ok]\n");
		}
		
		printf("read image ... ");
		if(myCamera.read(buffer, 0, fsize) != 0) {
			printf("[failed]\n");
			return -1;
		} else {
			printf("[ok]\n");
		}
		wprintf(L"save image file %s ... ", wbuffer);
		image::save_image(wbuffer, buffer, width, height, type, sformat);
		printf("[ok]\n");
		return 0;
	}
}


tCmdLineEntry g_sCmdTable[] = {
	{"open", open, ""},
	{"sccb-addr", sccb_addr, ""},
	{"wreg", wreg, ""},
	{"rreg", rreg, ""},
	{"clrbit", clrbit, ""},
	{"setbit", setbit, ""},
	{"bd", bd, ""},
	{"shot", shot, ""},
	{0, 0, 0}
};


#define MINIUM_ARGC	4
#define TIMEOUT		2000
int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	int nRetCode = 0;
	CImage image;
	TCHAR *initFname;
	char lineBuffer[256];

	// 初始化 MFC 并在失败时显示错误
	if (!AfxWinInit(::GetModuleHandle(NULL), NULL, ::GetCommandLine(), 0))
	{
		// TODO: 更改错误代码以符合您的需要
		_tprintf(_T("错误: MFC 初始化失败\n"));
		nRetCode = 1;
	}
	else
	{
		if(argc < 2) {
			int c;
			printf("Warning: Init file is not specified\n");
			printf("Press ESC to give up, ENTER to continue\n");
			while (1) {
				c = _getch();
				if(c == 0x1b)
					return 0;
				if(c == 0x0d)
					break;
			}
		} else {
			FILE *fh;

			initFname = argv[1];
			fh = _wfopen(initFname, L"r");
			if(fh == NULL) {
				printf("Init file open error\n");
				return -1;
			} else {
				wprintf(L"Init file = %s\n", initFname); 
			}
			while(fgets(lineBuffer, sizeof(lineBuffer), fh) != NULL) {
				CmdLineProcess(lineBuffer);
			}
			fclose(fh);

			COM::close(3);
		}

#if (0)
		if(argc < MINIUM_ARGC) {
			help(NULL);
			return -1;
		} else {
			swscanf(argv[1], L"%d", &port);
			swscanf(argv[2], L"%d", &bd);
			inputCmd = argv[3];
		}

		if(camera.open(port, bd) != 0) {
			printf("open camera failed on COM%d[%d]\r\n", port, bd);
			return -1;
		}
		
		int count = 3;
		
		do {
			inputCmd = argv[count ++];
			if(wcscmp(inputCmd, L"wreg") == 0) {
				if(argc < (count + 2)) {
					help("wreg");
					COM::close(port);
					return -1;
				} else {
					swscanf(argv[count ++], L"%x", &addr);
					swscanf(argv[count ++], L"%x", &value);
					
					printf("write value[0x%x] to register[0x%x] ... ", value, addr);
					if(camera.write_ovreg(port, addr, value, TIMEOUT) != 0) {
						printf("[error]\r\n");
					} else {
						printf("[ok]\r\n");
					}
				}
			} else if(wcscmp(inputCmd, L"rreg") == 0) {
				if(argc < (count + 1)) {
					help("rreg");
					COM::close(port);
					return -1;
				} else {
					swscanf(argv[count ++], L"%x", &addr);
					
					printf("read register[0x%x] ... ", addr);
					value = camera.read_ovreg(port, addr, TIMEOUT);
					if(value == -1) {
						printf("[error]\r\n");
					} else {
						printf("[ok][reg=0x%x]\r\n", value); 
					}
				}
			} else if(wcscmp(inputCmd, L"clrbit") == 0) {
				if(argc < (count + 2)) {
					help("clrbit");
					COM::close(port);
					return -1;
				} else {
					swscanf(argv[count ++], L"%x", &addr);
					swscanf(argv[count ++], L"%d", &value);
					
					printf("clear register[0x%x] bit[%d] ... ", addr, value);
					if(regBitClear(port, addr, (1 << value)) != 0) {
						printf("[error]\r\n");
					} else {
						printf("[ok][reg=0x%x]\r\n", camera.read_ovreg(port, addr, TIMEOUT));
					}
				}
			} else if(wcscmp(inputCmd, L"setbit") == 0) {
				if(argc < (count + 2)) {
					help("setbit");
					COM::close(port);
					return -1;
				} else {
					swscanf(argv[count ++], L"%x", &addr);
					swscanf(argv[count ++], L"%d", &value);
					
					printf("set register[0x%x] bit[%d] ... ", addr, value);
					if(regBitSet(port, addr, (1 << value)) != 0) {
						printf("[error]\r\n");
					} else {
						printf("[ok][reg=0x%x]\r\n", camera.read_ovreg(port, addr, TIMEOUT));
					}
				}
			} else if(wcscmp(inputCmd, L"bd") == 0) {
				if(argc < (count + 1)) {
					help("bd");
					COM::close(port);
					return -1;
				} else {
					swscanf(argv[count ++], L"%d", &bd);
					
					printf("set camera baudrate=[%d] ... ", bd);
					if(camera.reconfig_usart_bandrate(port, bd, TIMEOUT) != 0) {
						printf("[error]\r\n");
					} else {
						printf("[ok]\r\n");
						COM::close(port);
						Sleep(500);
						if(camera.open(port, bd) != 0) {
							printf("reopen COM%d[%d] failed", port, bd);
							COM::close(port);
							return -1;
						} else {
							printf("reopen COM%d with [%d]\r\n", port, bd);
						}
					}
				}
			} else if(wcscmp(inputCmd, L"sccb-addr") == 0) {
				if(argc < (count + 1)) {
					help("sccb-addr");
					COM::close(port);
					return -1;
				} else {
					swscanf(argv[count ++], L"%x", &addr);
					
					printf("set SCCB device address=[0x%x] ... ", addr);
					if(camera.config_ovaddr(port, addr, 100)!= 0) {
						printf("[error]\r\n");
					} else {
						printf("[ok]\r\n");
					}
				}
			} else if(wcscmp(inputCmd, L"shot") == 0) {
				if(argc < (count + 4)) {
					help("shot");
					COM::close(port);
					return -1;
				} else {
					image_t type;
					int width;
					int height;
					GUID sformat;
					int fsize;

					swscanf(argv[count ++], L"%d", &width);
					swscanf(argv[count ++], L"%d", &height);
					
					if(wcscmp(argv[count], L"rgb565") == 0) {
						type = IMAGE_FORMAT_RGB565;
						fsize = width * height * 2;
					} else if(wcscmp(argv[count], L"rgb555") == 0) {
						type = IMAGE_FORMAT_RGB555;
						fsize = width * height * 2;
					} else if(wcscmp(argv[count], L"raw") == 0) {
						type = IMAGE_FORMAT_RAW;
						fsize = width * height;
					} else if(wcscmp(argv[count], L"gray") == 0) {
						type = IMAGE_FORMAT_GRAY8;
						fsize = width * height;
					} else {
						printf("unknown image format\r\n");
						continue;
					}
					
					count ++;
					time_t rawtime;
					struct tm * timeinfo;
					wchar_t wbuffer[128];

					time(&rawtime);
					timeinfo = localtime (&rawtime);

					if(wcscmp(argv[count], L"jpg") == 0) {
						sformat = Gdiplus::ImageFormatJPEG;
						wcsftime(wbuffer, 128, L"%Y-%m-%d-%H%M%S.jpg", timeinfo);
					} else if(wcscmp(argv[count], L"png") == 0) {
						sformat = Gdiplus::ImageFormatPNG;
						wcsftime(wbuffer, 128, L"%Y-%m-%d-%H%M%S.png", timeinfo);
					} else {
						printf("unknown image format\r\n");
						continue;
					}
					count ++;

					if(camera.capture(port, 100) != 0) {
						printf("capture image failed\r\n");
						continue;
					}

					if(camera.read(port, buffer, 0, fsize, TIMEOUT) != 0) {
						printf("read image failed\r\n");
						continue;
					}
					wprintf(L"save image file %s\n", wbuffer);
					image::save_image(wbuffer, buffer, width, height, type, sformat);
				}
			}
		} while (count < argc);
		
		COM::close(port);
#endif
	}
		
#if (0)		
		//camera.test(3, buffer);
		if(camera.open(3, 230400) != 0)
			return -1;
		
		if(camera.config_ovaddr(3, 0x42, 100))
			return -1;
		
		configQVGA(3);
		config2OV7141(3);
		//configRAW(3);
		//configRGB565(3);

		// 0x24 = QVGA, 0x04 = VGA
		if(camera.write_ovreg(3, 0x14, 0x24, 100) != 0)
			return -1;

		// GRAY255 = 0x60, RAW = 0xA0
		//if(camera.write_ovreg(3, 0x28, 0xA0, 100) != 0)
		//	return -1;

		// RAW-RGB = 0x1C
		if(camera.write_ovreg(3, 0x12, 0x1C, 100) != 0)
			return -1;
		
		// RGB:565 Enable = 0x11, RGB:555 Enable = 0x05
		if(camera.write_ovreg(3, 0x1F, 0x11, 100) != 0)
			return -1;
		
		Sleep(1000);

		if(camera.capture(3, 100) != 0)
			return -1;
		if(camera.read(3, buffer, 0, 320 * 240, 1000) != 0)
			return -1;
		image::save_image(L"test.png", buffer, 320, 240, IMAGE_FORMAT_GRAY8);
		// TODO: 在此处为应用程序的行为编写代码。
	}
#endif
	return nRetCode;
}