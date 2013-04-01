#ifndef _COM_H_
#define _COM_H_

// circullar buffer struct
typedef struct {
	char * buf_handle; // buffer handle
	unsigned int buf_len; // buffer length
	unsigned int rp; // read pointer
	unsigned int wp; // write pointer
	unsigned int ndat; // number of datas
} cir_buf;


#define NUM_OF_COM	32
// USART struct
typedef struct {
	unsigned int ch;
	HANDLE handle;		// COM file handle
	DCB * dcb;			// config struct
	cir_buf * r_buf;	// circle buffer for reading
	cir_buf * w_buf;	// circle buffer for writting
	char * wt_buf;		// buffer for write thread
	HANDLE cir_r_mutex; // mutex handle for read circle buffer
	HANDLE cir_w_mutex; // mutex handle for write circle buffer
	HANDLE w_thread;	// write thread handle 
	HANDLE r_thread;	// read thread handle
	HANDLE rtlp;		// read thread lpped handle
	HANDLE wtlp;		// write thread lpped handle
	HANDLE w_event;		// write event to wake up write thread
	HANDLE r_event;		// read event to wake up usart_bloking_read
} usart_t;

//usart ioctrl cmd-bit
typedef enum {
	USART_CMD_MODE,			//don't change the order
	USART_CMD_PARITY,
	USART_CMD_BAUDRATE,
	USART_CMD_STOPBIT,
	USART_CMD_HWFLOW,
	USART_CMD_WORDLEN,
	USART_CMD_FLUSH,		//清空缓冲数据
	USART_CMD_BUFLEN,		//环形缓冲深度
} usart_enum_t;

#define USART_STOP_MASK		(3 << 0)	//stop bit
#define USART_STOP_0BIT5	(0 << 0)
#define USART_STOP_1BIT		(1 << 0)
#define USART_STOP_1BIT5	(2 << 0)
#define USART_STOP_2BIT		(3 << 0)

#define USART_DATA_MASK		(3 << 2)	//data bit
#define USART_DATA_5BIT		(0 << 2)
#define USART_DATA_6BIT		(1 << 2)
#define USART_DATA_7BIT		(2 << 2)
#define USART_DATA_8BIT		(3 << 2)

#define USART_PARITY_MASK	(3 << 4)	//parity
#define USART_PARITY_NONE	(0 << 4)
#define USART_PARITY_ODD	(1 << 4)
#define USART_PARITY_EVEN	(2 << 4)

#define USART_FLOW_MASK		(1 << 6)	//flow
#define USART_FLOW_HARD		(0 << 6)
#define USART_FLOW_SOFT		(1 << 6)


class COM {
public:
	int usart_open(unsigned int ch);
	int usart_close(unsigned int ch);
	unsigned int usart_read(unsigned int ch, char * buf, unsigned int addr, unsigned int size);
	unsigned int usart_block_read(unsigned int ch, char * buf, unsigned int addr, unsigned int size);
	unsigned int usart_write(unsigned int ch, const char *buf, unsigned int addr, unsigned int size);
	int usart_ioctrl(unsigned int ch, unsigned int cmd, unsigned int arg);

};

#endif
