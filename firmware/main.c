/********************************************************************
 *	メイン処理
 ********************************************************************
 */
#include <stdio.h>
#include <string.h>


#include "platform_config.h"
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "stm32_eval.h"
#include "hw_config.h"
#include "hidcmd.h"
#include "monit.h"



#if	APPLICATION_MODE

/* by default stdout issues to the USART */
//FILE *stdout = _H_USER;

/********************************************************************
 *	定義
 ********************************************************************
 */
/********************************************************************
 *	データ
 ********************************************************************
 */
//#pragma udata access accessram
//#pragma udata

#define	PUTBUF_SIZE	(64-4)

uchar puts_buf[PUTBUF_SIZE];
uchar puts_ptr;

//#pragma code

/********************************************************************
 *	
 ********************************************************************
 */
int _user_putc(char c)
{
	uchar flush = 0;
	if( c == 0x0a) { flush = 1; }
	if( puts_ptr >= PUTBUF_SIZE ) {flush = 1;}

	if( flush ) {
		while(puts_ptr) {
			wait_ms(1);
		}
	}

	if(	puts_ptr < PUTBUF_SIZE ) {
		puts_buf[puts_ptr++]=c;
	}

	return 1;
}
/********************************************************************
 *	
 ********************************************************************
 */
void *memcpy(void *dst,const void *src,size_t size)
{
	char *t = (char*)dst;
	char *s = (char*)src;
	while(size--) {*t++=*s++;}
	return t;
}

#endif	//APPLICATION_MODE

/********************************************************************
 *	( ms ) * 1mS 待つ.
 ********************************************************************
 */
void wait_ms(int ms)
{
	int i;
	for(i=0;i<ms;i++) {
		wait_u8s(1000*8);
	}
}

/********************************************************************
 *	( us ) * 1uS 待つ.
 ********************************************************************
 */
void wait_us(int us)
{
	wait_u8s(us*8);
}

/********************************************************************
 *	( us * 1/8 ) uS 待つ. clock=72MHzと仮定.
 ********************************************************************
 8002186:	bf00      	nop
 8002188:	bf00      	nop
 800218a:	bf00      	nop
 800218c:	bf00      	nop
 800218e:	bf00      	nop
 8002190:	3301      	adds	r3, #1
 8002192:	4283      	cmp	r3, r0
 8002194:	dbf7      	blt.n	8002186 <wait_u8s+0x4>
 */
void wait_u8s(int us)
{
	int i;
	for(i=0;i<us;i++) {
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
		asm("nop");
	}
}



/********************************************************************
 *	
 ********************************************************************
 */
int main(void)
{
	Set_System();
#if	1
	USB_Interrupts_Config();
	Set_USBClock();
	USB_Init();
#endif
	while(1) {
		USBtask();
#if	APPLICATION_MODE
		wait_ms(1);
		led_blink(8);
#endif
	}
}

/********************************************************************
 *	
 ********************************************************************
 */
