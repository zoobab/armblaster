/********************************************************************
 *	ユーザー定義関数の実行
 ********************************************************************
 */
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <malloc.h>

#include "platform_config.h"
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "stm32_eval.h"
#include "hidcmd.h"
#include "monit.h"

#define	PRINTF_TEST	(0)		// printf()の動作テスト.



#if	APPLICATION_MODE

//	#undef	putc
//	#undef	fputs
//	#define	putc(c,fp)  _user_putc(c)
//	#define	fputs(s,fp) _user_puts(s)


int _user_putc(char c);

int _user_puts(char *s)
{
	while(*s) {
		_user_putc(*s++);
	}
	return 0;
}


/********************************************************************
 *	定義
 ********************************************************************
 */
/********************************************************************
 *	データ
 ********************************************************************
 */
/********************************************************************
 *	
 ********************************************************************
 */
void memdump(char *mesg,void *src , int len )
{
	char buf[16];
	uchar *s=(uchar*)src;
	int i;

	_user_puts(mesg);

	for(i=0;i<len;i++) {
		sprintf(buf,"%02x ",*s);s++;
		_user_puts(buf);
	}
	_user_puts("\n");
}


#if	PRINTF_TEST		// printf()の動作テスト.
/********************************************************************
 *	
 ********************************************************************
 */
void user_cmd(int arg)
{
	char buf[64];

#if	0
	//
	//	malloc()テストの実行.
	//
	int *t;
	t=malloc(1);
	sprintf(buf,"t=%x\n",(int) t );
	fputs(buf,stdout);

	t=malloc(16);
	sprintf(buf,"t=%x\n",(int) t );
	fputs(buf,stdout);

	t=malloc(16);
	sprintf(buf,"t=%x\n",(int) t );
	fputs(buf,stdout);
#endif

	//
	//	printf()テストの実行.
	//
	printf("Hello World\n");
	printf("Pi=%16.10g\n",atan(1.0)*4.0 );

#if	1
	//
	//	fputs()テストの実行.
	//
	ushort i,j;
	for(j=0;j<16;j++) {
//		printf("Hello :");
		fputs("Hello :",stdout);
		for(i=0;i<50;i++) {
			putc( i + ' ' ,stdout );
//			USBtask();
//			LED1_blink();
		}
//		printf("\n");
		fputs("\n",stdout);
	}
#endif

#if	0
	//
	//	double 型のメモリーダンプテスト.
	//
	double a = 3.14;
	double b = 2.0;
	double c;
	double d;

	c = a*b;
	d = atan(1.0)*4.0;
	memdump( "a=" , &a , 8 );
	memdump( "b=" , &b , 8 );
	memdump( "c=" , &c , 8 );
	memdump( "d=" , &d , 8 );
#endif
}

#else	//PRINTF_TEST	printf()の動作テスト.


extern char g_pfnVectors;
extern char _etext;
extern char _sdata;
extern char _edata;

extern int firm_top;
extern int firm_end;

void _user_puthex1(int h1)
{
	int c = h1 & 0x0f;
	if(c>=10) c += ('a' - ('9'+1));
	_user_putc('0'+c);
}

void _user_puthex2(int h2)
{
	_user_puthex1(h2>>4);
	_user_puthex1(h2);
}

void _user_puthex4(int h4)
{
	_user_puthex2(h4>>8);
	_user_puthex2(h4);
}

void _user_puthex8(int h8)
{
	_user_puthex4(h8>>16);
	_user_puthex4(h8);
}

void hex_print(char *s,int hex)
{
	_user_puts(s);
	_user_puthex8(hex);
	_user_puts("\n");
}

/********************************************************************
 *	_user_putc()  _user_puts()テストの実行.
 ********************************************************************
 */
void user_cmd(int arg)
{
	int rom_end;
	_user_puts("Hello World\n");
	ushort i,j;
	for(j=0;j<16;j++) {
		_user_puts("Hello :");
		for(i=0;i<50;i++) {
			_user_putc( i + ' ' );
		}
		_user_puts("\n");
	}

	rom_end = (int) &_etext + (&_edata - &_sdata);

//	printf("g_pfnVectors=%x\n",&g_pfnVectors);
//	printf("rom_end=%x\n",rom_end);
//	hex_print("g_pfnVectors=",&g_pfnVectors);
	hex_print("rom_end="     ,rom_end);

	hex_print("top=",firm_top);
	hex_print("end=",firm_end);
}


#endif	//PRINTF_TEST	printf()の動作テスト.
/********************************************************************
 *	
 ********************************************************************
 */
#endif
