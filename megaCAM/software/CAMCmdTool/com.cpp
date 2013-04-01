#include "stdafx.h"


static usart_t * usart[NUM_OF_COM];

#define cir_plus(n, len)	 n = (n + 1) % len


cir_buf * cir_buf_malloc(unsigned int len) {
	cir_buf * cbuf = (cir_buf *)malloc(sizeof(cir_buf));
	
	if(cbuf == NULL) {
		//TRACE_ERROR("cir_buf pointer malloc failed.");
		return NULL;
	}
	cbuf->buf_handle = (char *)malloc(len);
	if(cbuf->buf_handle) {
		cbuf->buf_len = len;
		cbuf->rp = 0;
		cbuf->wp = 0;
		cbuf->ndat = 0;
		return cbuf;
	} else {
		//TRACE_ERROR("cir-buffer handler pointer malloc failed.");
		return NULL;
	}
	// cout << "buffer length = " << (buf_len - 1) << endl;
}



void cir_buf_free(cir_buf * cbuf) {
	if(cbuf) {
		if(cbuf->buf_handle) {
			free(cbuf->buf_handle);
		}
		free(cbuf);
	}
}


void cir_buf_store(cir_buf * cbuf, char * buf, unsigned int nbytes) {
	while(nbytes --) {
		cbuf->buf_handle[cbuf->wp] = *(buf ++);
		// cout << "------> write data at: " << wp;
		cir_plus(cbuf->wp, cbuf->buf_len);
		// buffer is full, plus rp by one
		if(cbuf->wp == cbuf->rp) {
			cir_plus(cbuf->rp, cbuf->buf_len);
		} else {
			// buffer is filled
			cbuf->ndat ++;
		}
		// cout << ", leaving at rp = " << rp << " wp = " << wp << endl;	
	}
}


unsigned int cir_buf_extract(cir_buf * cbuf, char * buf, unsigned int nbytes) {
	unsigned int count = 0;
	
	while(nbytes --) {
		// buffer is empty now
		if(cbuf->wp == cbuf->rp) {
			return count;
		}
		*(buf ++) = cbuf->buf_handle[cbuf->rp];
		// cout << "------> read data from: " << rp;
		count ++;
		cir_plus(cbuf->rp, cbuf->buf_len);
		cbuf->ndat --;
		// cout << ", leaving at rp = " << rp << " wp = " << wp << endl;
	}
	return count;
}

// read thread for COM reading
DWORD WINAPI usart_read_thread(LPVOID lpThreadParameter) {
	DWORD nbytes;
	OVERLAPPED lapped;
	char buffer;
	unsigned int ch = *(unsigned int *)lpThreadParameter;
	usart[ch]->rtlp = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	//TRACE_INFO("read thread is alive");
	while(1) {
		// create lapped for COM readding
		memset(&lapped, 0, sizeof(lapped));
		ResetEvent(usart[ch]->rtlp);
		lapped.hEvent = usart[ch]->rtlp;
		ReadFile(usart[ch]->handle, &buffer, 1, NULL, &lapped);
		// one byte one time, wait forever
		//TRACE_DEBUG("read thread waitting for data......");
		WaitForSingleObject(usart[ch]->rtlp, INFINITE);
		nbytes = 0;
		if(GetOverlappedResult(usart[ch]->handle, &lapped, &nbytes, TRUE) == TRUE) {
			// put in ciraular-buffer
			if(nbytes > 0) {
				WaitForSingleObject(usart[ch]->cir_r_mutex, INFINITE);
				cir_buf_store(usart[ch]->r_buf, &buffer, nbytes);
				//TRACE_DEBUG("read_thread: store %d bytes, set READ event", nbytes);
				ReleaseMutex(usart[ch]->cir_r_mutex);
				SetEvent(usart[ch]->r_event);
			} else {
				//TRACE_ERROR("read thread design error.");
			}
		} else {
			// something is wrong, clear error
			//TRACE_ERROR("clear COM error");
			ClearCommError(usart[ch]->handle, NULL, NULL);
		}
	}
}

// write thread for COM writting
DWORD WINAPI usart_write_thread(LPVOID lpThreadParameter) {
	DWORD nbytes;
	OVERLAPPED lapped;
	unsigned int ch = *(unsigned int *)lpThreadParameter;
	// handle for WRITE event
	usart[ch]->wtlp = CreateEvent(NULL, FALSE, FALSE, NULL);
	
	usart[ch]->wt_buf = (char *)malloc(usart[ch]->w_buf->buf_len);
	//TRACE_INFO("write thread is alive");
	while(1) {
		// wait for WRITE event forever
		//TRACE_DEBUG("write thread waitting for data......");
		WaitForSingleObject(usart[ch]->w_event, INFINITE);
		// wait for write circle buffer free
		WaitForSingleObject(usart[ch]->cir_w_mutex, 1000);
		// extract all datas in write circle buffer
		nbytes = cir_buf_extract(usart[ch]->w_buf, usart[ch]->wt_buf, usart[ch]->w_buf->buf_len);
		//TRACE_DEBUG("write_thread: extract %d bytes", nbytes);
		// clear WRITE event
		ResetEvent(usart[ch]->w_event);
		// free write circle buffer
		ReleaseMutex(usart[ch]->cir_w_mutex);
		if(nbytes) {
			// write datas to COM
			memset(&lapped, 0, sizeof(lapped));
			ResetEvent(usart[ch]->wtlp);
			lapped.hEvent = usart[ch]->wtlp;
			WriteFile(usart[ch]->handle, usart[ch]->wt_buf, nbytes, NULL, &lapped);
			// wait for end of writting
			WaitForSingleObject(usart[ch]->wtlp, INFINITE);
		}
	}
}


int COM::usart_open(unsigned int ch) {
	COMMTIMEOUTS com_timeout;
	wchar_t com_name[] = L"COMn";

	if(ch > (NUM_OF_COM - 1)) {
		//TRACE_ERROR("COM%d do not exsit", ch);
		return -1;
	}
	usart[ch] = (usart_t *)malloc(sizeof(usart_t));
	usart[ch]->ch = ch;
	// open COM as OVERLAPPED file
	com_name[3] = '0' + ch;
	usart[ch]->handle = CreateFile( com_name,
		GENERIC_READ | GENERIC_WRITE,
		0,
		NULL,
		OPEN_EXISTING,
		FILE_FLAG_OVERLAPPED,
		NULL);
	if(usart[ch]->handle == INVALID_HANDLE_VALUE) {
		//TRACE_ERROR("failed to open COM%d", ch);
		return -1;
	} else {
		// create mutex for two circle buffers
		usart[ch]->cir_r_mutex = CreateMutex(NULL, FALSE, NULL);
		usart[ch]->cir_w_mutex = CreateMutex(NULL, FALSE, NULL);
		// WRITE event to wake up writting thread
		usart[ch]->w_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		usart[ch]->r_event = CreateEvent(NULL, FALSE, FALSE, NULL);
		// malloc a dcb
		usart[ch]->dcb = (DCB *)malloc(sizeof(DCB));
		// init DCB struct with default settings "9600,n,8,1"
		SecureZeroMemory(usart[ch]->dcb, sizeof(DCB));
		usart[ch]->dcb->DCBlength = sizeof(usart[ch]->dcb);
		// load current DCB settings
		if(!GetCommState(usart[ch]->handle, usart[ch]->dcb)) {
			return -1;
		}
		// make our default settings
		usart[ch]->dcb->BaudRate = CBR_9600;
		usart[ch]->dcb->ByteSize = 8;
		usart[ch]->dcb->Parity   = NOPARITY;
		usart[ch]->dcb->StopBits = ONESTOPBIT;
		usart[ch]->dcb->fBinary = TRUE;
		usart[ch]->dcb->fParity = FALSE;
		usart[ch]->dcb->fTXContinueOnXoff = FALSE;
		usart[ch]->dcb->fInX = FALSE;
		usart[ch]->dcb->fOutX = FALSE;
		usart[ch]->dcb->fDtrControl = DTR_CONTROL_DISABLE;
		usart[ch]->dcb->fRtsControl = RTS_CONTROL_DISABLE;
		// set read and write timeout
		memset(&com_timeout, 0, sizeof(com_timeout));
		com_timeout.ReadIntervalTimeout = MAXDWORD;
		com_timeout.ReadTotalTimeoutConstant = MAXDWORD - 10;
		com_timeout.ReadTotalTimeoutMultiplier =  MAXDWORD;
		com_timeout.WriteTotalTimeoutConstant =  MAXDWORD;
		com_timeout.WriteTotalTimeoutMultiplier =  MAXDWORD;
		SetCommTimeouts(usart[ch]->handle, &com_timeout);
		// set COM event
		SetCommMask(usart[ch]->handle, EV_RXCHAR | EV_TXEMPTY);
		//TRACE_INFO("default settings: 9600,n,8,1");
		return 0;
	}
}

int COM::usart_close(unsigned int ch) {
	if(ch > (NUM_OF_COM - 1)) {
		return -1;
	}
	// stop the reading and writting thread
	TerminateThread(usart[ch]->r_thread, 0);
	TerminateThread(usart[ch]->w_thread, 0);
	// close handles
	CloseHandle(usart[ch]->handle);
	CloseHandle(usart[ch]->cir_r_mutex);
	CloseHandle(usart[ch]->cir_w_mutex);
	CloseHandle(usart[ch]->w_thread);
	CloseHandle(usart[ch]->r_thread);
	CloseHandle(usart[ch]->w_event);
	CloseHandle(usart[ch]->r_event);
	CloseHandle(usart[ch]->rtlp);
	CloseHandle(usart[ch]->wtlp);
	// free the writting thread buffer
	free(usart[ch]->wt_buf);
	// free DCB struct
	free(usart[ch]->dcb);
	// delte circle buffers
	cir_buf_free(usart[ch]->r_buf);
	cir_buf_free(usart[ch]->w_buf);
	// free the usart handle
	free(usart[ch]);
	return 0;
}

unsigned int COM::usart_read(unsigned int ch, char * buf, unsigned int addr, unsigned int size) {
	unsigned int count;
	
	// wait for read circle buffer free, NO TIMEOUT
	WaitForSingleObject(usart[ch]->cir_r_mutex, INFINITE);
	count = cir_buf_extract(usart[ch]->r_buf, buf, size);
	// free the read circle buffer
	//TRACE_INFO("usart_read: extract %d bytes", count);
	ReleaseMutex(usart[ch]->cir_r_mutex);
	return count;
}

unsigned int COM::usart_block_read(unsigned int ch, char * buf, unsigned int addr, unsigned int size) {
	unsigned int count = 0;
	unsigned int total = 0;
	
	while(size) {
		// wait for read thread set READ event
		//TRACE_DEBUG("wait for READ event");
		WaitForSingleObject(usart[ch]->r_event, INFINITE);
		// wait for read circle buffer free forever
		//TRACE_DEBUG("wait for circle buffer");
		WaitForSingleObject(usart[ch]->cir_r_mutex, INFINITE);
		count = cir_buf_extract(usart[ch]->r_buf, buf, size);
		//TRACE_DEBUG("total bytes in cirbuf: %d", usart[ch]->r_buf->ndat);
		if(usart[ch]->r_buf->ndat == 0) {
			//TRACE_DEBUG("usart_bloking_read: reset READ event.");
			ResetEvent(usart[ch]->r_event);
		} else {
			//TRACE_DEBUG("usart_bloking_read: set READ event");
			SetEvent(usart[ch]->r_event);
		}
		// free the read circle buffer
		ReleaseMutex(usart[ch]->cir_r_mutex);
		//TRACE_DEBUG("usart_bloking_read: extract %d bytes, free cirbuf.", count);
		size -= count;
		total += count;
		buf += count;
	}
	return total;
}

unsigned int COM::usart_write(unsigned int ch, const char *buf, unsigned int addr, unsigned int size) {
	// wait for write circle buffer free
	WaitForSingleObject(usart[ch]->cir_w_mutex, INFINITE);
	// store datas to circle buffer
	cir_buf_store(usart[ch]->w_buf, (char *)buf, size);
	// set WRITE event to wake up writting thread
	SetEvent(usart[ch]->w_event);
	//TRACE_INFO("usart_write: store %d bytes, set WRITE event", size);
	// free write circle buffer
	ReleaseMutex(usart[ch]->cir_w_mutex);
	return size;
}

int COM::usart_ioctrl(unsigned int ch, unsigned int cmd, unsigned int arg) {
	switch(cmd)	{
		// mode settings
		case USART_CMD_MODE:
			//TRACE_INFO("USART_CMD_MODE:");
			// stop bit: 1, 1.5 or 2
			switch(arg & USART_STOP_MASK) {
				case USART_STOP_1BIT:
					//TRACE_INFO_WP(" 1BIT stop");
					usart[ch]->dcb->StopBits = 0;
					break;
				case USART_STOP_1BIT5:
					//TRACE_INFO_WP(" 1BIT5 stop");
					usart[ch]->dcb->StopBits = 1;
					break;
				case USART_STOP_2BIT:
					//TRACE_INFO_WP(" 2BIT stop");
					usart[ch]->dcb->StopBits = 2;
					break;
			}
			// data length
			switch(arg & USART_DATA_MASK) {
				case USART_DATA_5BIT:
				case USART_DATA_6BIT:
					//TRACE_INFO_WP(" 5 and 6 BIT data is not supported");
					return -1;
				case USART_DATA_7BIT:
					//TRACE_INFO_WP(" 7BIT data");
					usart[ch]->dcb->ByteSize = 7;
					break;
				case USART_DATA_8BIT:
					//TRACE_INFO_WP(" 8BIT data");
					usart[ch]->dcb->ByteSize = 8;
					break;
			}
			// data parity
			switch(arg & USART_PARITY_MASK) {
				case USART_PARITY_NONE:
					//TRACE_INFO_WP(" parity none");
					usart[ch]->dcb->Parity = NOPARITY;
					break;
				case USART_PARITY_ODD:
					//TRACE_INFO_WP(" parity odd");
					usart[ch]->dcb->Parity = ODDPARITY;
					break;
				case USART_PARITY_EVEN:
					//TRACE_INFO_WP(" parity even");
					usart[ch]->dcb->Parity = EVENPARITY;
					break;
			}
		break;
		// baudrate
		case USART_CMD_BAUDRATE:
			if((arg > CBR_110) && (arg < CBR_256000)) {
				//TRACE_INFO("USART_CMD_BAUDRATE: %d", arg);
				usart[ch]->dcb->BaudRate = arg;
			} else {
				return -1;
			}
			break;
		// flush, dummy in this driver
		case USART_CMD_FLUSH:
			//TRACE_INFO("USART_CMD_FLUSH: dummy command here");
			break;
		// buffer length
		case USART_CMD_BUFLEN:
			//TRACE_INFO("USART_CMD_BUFLEN: %d", arg);
			// setup COM
			if(!SetupComm(usart[ch]->handle, arg, arg)) {
				return -1;
			}
			// malloc circle buffer for reading and writting
			usart[ch]->r_buf = cir_buf_malloc(arg);
			usart[ch]->w_buf = cir_buf_malloc(arg);
			// clear COM buffer
			PurgeComm(usart[ch]->handle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);
			// set COM event mask
			SetCommMask(usart[ch]->handle, EV_RXCHAR | EV_TXEMPTY);
			// lunch the reading and writting threads
			usart[ch]->r_thread = CreateThread(NULL, 512, usart_read_thread, (LPVOID)&usart[ch]->ch, 0, NULL);
			usart[ch]->w_thread = CreateThread(NULL, 512, usart_write_thread, (LPVOID)&usart[ch]->ch, 0, NULL);
			return 0;
	}
	// config COM state
	if(SetCommState(usart[ch]->handle, usart[ch]->dcb)) {
		return 0;
	} else {
		return -1;
	}
}