/*============================================================
	名称: sccb.h
	描述: SCCB 总线驱动
	作者: L.H.R		jk36125@gmail.com
	时间: 2009-12-10 9:43:59
	版本: 1.0
	备注: 无
============================================================
*/


#ifndef _SCCB_H_
#define _SCCB_H_

#include <types.h>

void sccb_set_dev_addr(uint8 device_address);
int sccb_writeb(uint8 addr, uint8 dat);
int sccb_readb(uint8 addr);
				   
#endif  // sccb.h
