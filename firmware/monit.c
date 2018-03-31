/********************************************************************
 *	�ȈՃ��j�^
 ********************************************************************
 */
#ifdef	LPC2388


//	NXP arm7tdmi-s
#include <string.h>
#include "hwlib/led.h"
#include "hwlib/LPC23xx.h"
#include "utype.h"
#include "stm32dmy.h"


#else

//	STM32 Cortex-M3
#include <string.h>
#include "platform_config.h"
#include "stm32f10x.h"
#include "usb_lib.h"
#include "usb_istr.h"
#include "stm32_eval.h"

#endif

#include "hidcmd.h"
#include "monit.h"
#include "picwrt.h"
#include "gpio.h"


/********************************************************************
 *	�X�C�b�`
 ********************************************************************
 */

/********************************************************************
 *	��`
 ********************************************************************
 */
#if defined(__18F14K50)
#define	SOFTWARE_SPI_MODE	1
#define	DEVID	DEV_ID_PIC18F14K	//0x14
#else
#define	SOFTWARE_SPI_MODE	1		//18F4550����SPI��M�f�[�^��������.
#define	DEVID	DEV_ID_ARM_M3		//0xa3
#endif

#define	VER_H	1
#define	VER_L	1

#if	APPLICATION_MODE
#define	DEV_LOADER	0
#else
#define	DEV_LOADER	1
#endif

#define	REPLY_REPORT	0xaa

extern void user_cmd(int arg);

/********************************************************************
 *	�f�[�^
 ********************************************************************
 */

//	AVR�������ݗp�R���e�N�X�g.
uchar  page_mode;
uchar  page_addr;
uchar  page_addr_h;

//	�f�[�^����胂�[�h�̐ݒ�.
uchar   poll_mode;	// 00=���ݒ�  0xa0=�A�i���O�T���v��  0xc0=�f�W�^���T���v��
						// 0xc9=run�R�}���h�^�[�~�l�[�g
uchar  *poll_addr;	//

//	�R�}���h�̕ԐM���K�v�Ȃ�1���Z�b�g����.
uchar  ToPcRdy;

uchar	puts_mode;

uchar 	puts_buf[64];
uchar 	puts_ptr;

uchar	poll_wptr;
uchar	poll_rptr;
uchar	poll_buf[256];

void   (*user_func)(int arg);
int		user_arg;
int		firm_top=0x8000000;
int		firm_end=0x8002000;

Packet PacketFromPC;			//���̓p�P�b�g 64byte
Packet PacketToPC;				//�o�̓p�P�b�g 64byte

#define	Cmd0	PacketFromPC.cmd

/********************************************************************
 *	
 ********************************************************************
 */

extern char g_pfnVectors;
extern char _etext;
extern char _sdata;
extern char _edata;

/********************************************************************
 *	
 ********************************************************************
 */
int	align_size(int adr,int align)
{
	return 	(adr + align - 1) & (-align) ;	// align byte���E�ɍ��킹��.
}

/********************************************************************
 *	
 ********************************************************************
 */
void Init_Monitor(void)
{
	firm_top = (int) &g_pfnVectors;
	firm_end = (int) &_etext + (&_edata - &_sdata);

	firm_end = align_size(firm_end,1024);
}

/********************************************************************
 *	
 ********************************************************************
 */
int check_flash_adrs(int adrs)
{
	if( (adrs >= firm_top)&&(adrs < firm_end) )	return 0;	// INVALID!
	return 1;	// VALID.
}
/********************************************************************
 *	ISP���W���[�� �C���N���[�h.
 ********************************************************************
 */
//#pragma code

#if	1

#if	1	//SOFTWARE_SPI_MODE
#include "usi_pic18s.h"
#else
#include "usi_pic18.h"
#endif


static void isp_command(uchar *data) {
	uchar i;
	for (i=0;i<4;i++) {
		PacketToPC.raw[i]=usi_trans(data[i]);
	}
}

#endif

#if 0
unsigned char ReadEE(unsigned char Address)
{
	EECON1 = 0x00;
	EEADR = Address;
	EECON1bits.RD = 1;
	return (EEDATA);
}

void WriteEE(unsigned char Address, unsigned char Data)
{
	EEADR = Address;
	EEDATA = Data;
	EECON1 = 0b00000100;	// Setup writes: EEPGD=0,WREN=1
	/* Start Write */
	EECON2 = 0x55;
	EECON2 = 0xAA;
	EECON1bits.WR = 1;
	while( EECON1bits.WR);	// Wait till WR bit is clear, hopefully not long enough to kill USB
}
#endif

#if RAM_SERIAL	/* Add by senshu */
/*
  EEPROM MAP

	0x00�i���[�U�̈�j
	�@:
	0xef�i���[�U�̈�j

	0xf0 �V���A���ԍ��̑��������i���i�K�ł́A4�Ɍ���j
	0xf1 �V���A���ԍ��i1�����ځj
	0xf2 �V���A���ԍ��i2�����ځj
	0xf3 �V���A���ԍ��i3�����ځj
	0xf4 �V���A���ԍ��i4�����ځj
	0xf5 �V���A���ԍ��i5�����ځj
	0xf6 �V���A���ԍ��i6�����ځj
	0xf7 �\��
	 :
	0xfc �\��
	0xfd USB�V���A���̏������{�[���[�g
	0xfe DTR/RTS�Ȃǂ̗L���E�����̎w��Ȃ�
	0xff JUMP�A�h���X(H) .... �A�v���̏��8�r�b�g�w��
	�����ʂ� 0x00��z��
 */
#define EE_SERIAL_LEN 0xf0
#define EE_SERIAL_TOP 0xf1


void set_serial_number()
{
	unsigned char ch;
	int i, j;
	if (ReadEE(EE_SERIAL_LEN) == 4) {
		/* �f�[�^�̑Ó��������� */
		j = 0;
		for (i = EE_SERIAL_TOP; i < EE_SERIAL_TOP + 4; i++) {
			ch = ReadEE(i);
			if (' ' <= ch && ch <= 'z') {
				j++;
			}
		}
		/* �f�[�^���Ó��ł���΁A�l���Z�b�g���� */
		if (j == 4) {
			for (i = 0; i < 4; i++) {
				sd003.string[i] = ReadEE (EE_SERIAL_TOP + i);
			}
		}
	}
}

#endif

#if	0
/********************************************************************
 *	������.
 ********************************************************************
 */
void UserInit(void)
{
    mInitAllLEDs();
	timer2_interval(5);	// '-d5'
	poll_mode = 0;
	poll_addr = 0;
	puts_ptr = 0;
	ToPcRdy = 0;

#if	TIMER2_INT_SAMPLE			// �^�C�}�[�Q���荞�݂�PORT�T���v��.
	poll_wptr=0;
	poll_rptr=0;
#endif
}
#endif


/********************************************************************
 *	�|�[�g�E�T���v��
 ********************************************************************
 */
#if	TIMER2_INT_SAMPLE			// �^�C�}�[�Q���荞�݂�PORT�T���v��.
void mon_int_handler(void)
{
	uchar c;

	if(	poll_mode == 0) return;

	if(	poll_mode == POLL_ANALOG) {
		// ANALOG
        while(ADCON0bits.NOT_DONE);     // Wait for conversion

		c = ADRESL;			//ADC �|�[�g�ǂݎ��,
		poll_buf[poll_wptr] = c;
		poll_wptr++;
		c = ADRESH;			//ADC �|�[�g�ǂݎ��,
		poll_buf[poll_wptr] = c;
		poll_wptr++;

        ADCON0bits.GO = 1;              // Start AD conversion
		return;
	}else{
		// DIGITAL
		c = *poll_addr;			//�|�[�g�ǂݎ��,
		poll_buf[poll_wptr] = c;
		poll_wptr++;
	}
}

int mon_read_sample(void)
{
	uchar c;
	if(	poll_rptr != poll_wptr) {
		c = poll_buf[poll_rptr];
		poll_rptr++;
		return c;
	}else{
		return -1;
	}
}

#endif

#ifdef	LPC2388
#define	FLASH_END_ADR	0x7e000		// 512k-8k
#endif

#ifdef	LPC1343
#define	FLASH_END_ADR	0x00008000	// 32k
#endif

#ifdef	STM8S_D
#define	FLASH_END_ADR	0x08010000	// 64k
#endif

#ifdef	STBEE
#define	FLASH_END_ADR	0x08080000	// 512k
#endif

#ifdef	CQ_STARM
#define	FLASH_END_ADR	0x08020000	// 128k
#endif

#ifdef	STBEE_MINI
#define	FLASH_END_ADR	0x08020000	// 128k
#endif

/********************************************************************
 *
 ********************************************************************
 */
int	search_flash_end(int ea)
{
	int *p = (int *)ea;

	while(1) {
		p--;
		if(*p != (-1)) break;
	}
	return (int)p;
}

/********************************************************************
 *	�ڑ��e�X�g�̕ԓ�
 ********************************************************************
 */
void cmd_echo(void)
{
	int *fl_stat = (int*) &PacketToPC.rawint[2];
//	led_set(1);
	PacketToPC.raw[1]=DEVID;				// PIC25/14K
	PacketToPC.raw[2]=VER_L;				// version.L
	PacketToPC.raw[3]=VER_H;				// version.H
	PacketToPC.raw[4]=DEV_LOADER;			// bootloader
	PacketToPC.raw[5]=PacketFromPC.raw[1];	// ECHOBACK

	fl_stat[0]=FLASH_END_ADR;
	fl_stat[1]=search_flash_end(FLASH_END_ADR);

	ToPcRdy = 1;
}

/********************************************************************
 *	�������[�ǂݏo��
 ********************************************************************
 */
void cmd_peek(void)
{
	Packet *t = &PacketFromPC;
	uchar  i,size,subcmd;

	uchar  *pb;
	ushort *ph;
	uint   *pw;

	uchar  *tb;
	ushort *th;
	uint   *tw;

 	size   = t->size;
	subcmd = t->subcmd;

	switch(subcmd) {
	 default:
	 case MEM_BYTE:
	 	pb = (uchar *)t->adrs;
		tb = (uchar *)PacketToPC.raw;
		for(i=0;i<size;i++) {
			*tb++ = *pb++;
		}
		break;
	 case MEM_HALF:
	 	ph = (ushort *)t->adrs;
		th = (ushort *)PacketToPC.rawint;
		for(i=0;i<size;i+=2) {
			*th++ = *ph++;
		}
		break;
	 case MEM_WORD:
	 	pw = (uint *)t->adrs;
		tw = (uint *)PacketToPC.rawint;
		for(i=0;i<size;i+=4) {
			*tw++ = *pw++;
		}
		break;
	}

	ToPcRdy = 1;
}
/********************************************************************
 *	�������[��������
 ********************************************************************
 */
void cmd_poke(void)
{
	Packet *t = &PacketFromPC;
	uchar i,size,subcmd;

	uchar  *pb;
	ushort *ph;
	uint   *pw;

	uchar  *sb;
	ushort *sh;
	uint   *sw;

 	size   = t->size;
	subcmd = t->subcmd;

	switch(subcmd) {
	 default:
	 case MEM_BYTE:
	 	pb = (uchar *)t->adrs;
		sb = (uchar *)t->data;
		for(i=0;i<size;i++) {
			*pb++ = *sb++;
		}
		break;
	 case MEM_HALF:
	 	ph = (ushort *)t->adrs;
		sh = (ushort *)t->data;
		for(i=0;i<size;i+=2) {
			*ph++ = *sh++;
		}
		break;
	 case MEM_WORD:
	 	pw = (uint *)t->adrs;
		sw = (uint *)t->data;
		for(i=0;i<size;i+=4) {
			*pw++ = *sw++;
		}
		break;
	}
}

/********************************************************************
 *	
 ********************************************************************
 */
void cmd_page_erase()
{
	if( check_flash_adrs(PacketFromPC.adrs)) {
		FLASH_ErasePage(PacketFromPC.adrs);
	}
}
/********************************************************************
 *	
 ********************************************************************
 */
void cmd_flash_lock()
{
 	if(PacketFromPC.size) {
		FLASH_Lock();
	}else{
		/* Unlock the internal flash */
		FLASH_Unlock();
	}
}
/********************************************************************
 *	
 ********************************************************************
 */
void cmd_page_write()
{
	int  *p = PacketFromPC.data;
	int	  adr;
	int   i;
	uchar size = PacketFromPC.size;
 	adr = PacketFromPC.adrs;
	size = (size+3) >> 2;

#if	defined(LPC1343)||defined(LPC2388)
	if(size==0) {
		FLASH_ProgramPage(adr);
		return ;
	}
#endif

	for(i=0;i<size;i++) {
		if( check_flash_adrs(adr) ) {
			FLASH_ProgramWord(adr, *p);
		}
		p++;
		adr+=4;
	}
}


/********************************************************************
 *	�Ԓn�w��̎��s
 ********************************************************************
 */
void call_func(int adrs)
{
	void (*func)(void);
	func = (void (*)()) adrs;

	func();
}
/********************************************************************
 *	�Ԓn�w��̎��s
 ********************************************************************
 */
void boot_func(int adrs)
{
	usbModuleDisable();		// USB����U�؂藣��.
//	__disable_irq();
	wait_ms(600);			// 0.6�b�҂�.
	call_func(adrs);
}
/********************************************************************
 *	�Ԓn�w��̎��s
 ********************************************************************
 */
void cmd_boot(int adrs)
{
	user_arg  = adrs;
	user_func = boot_func;
}
/********************************************************************
 *	�Ԓn�w��̎��s
 ********************************************************************
 */
void cmd_exec(int adrs,int bootflag)
{
	if(	bootflag ) {
		cmd_boot(adrs);
	}else{
		call_func(adrs);
	}
}



#if	APPLICATION_MODE
/********************************************************************
 *	puts ��������z�X�g�ɕԑ�����.
 ********************************************************************
 */
void cmd_get_string(void)
{
	PacketToPC.raw[0] =  puts_mode;	//'p';		/* �R�}���h���s������HOST�ɒm�点��. */
	PacketToPC.raw[1] =  puts_ptr;	// ������.
	memcpy( (void*)&PacketToPC.raw[2] , (void*)puts_buf , puts_ptr & 0x3f);	//������.
	puts_ptr = 0;
	ToPcRdy = 1;
}
/********************************************************************
 *	���[�U�[��`�֐��̎��s.
 ********************************************************************
 */
void func1(int arg)
{
	puts_ptr = 0;
	puts_mode = 'p';
	user_cmd(arg);
	puts_mode = 'q';
}
/********************************************************************
 *	���[�U�[��`�֐��̎��s.
 ********************************************************************
 */
void cmd_user_cmd(void)
{
	user_arg  = PacketFromPC.adrs;
	user_func = func1;
}
#endif
/********************************************************************
 *	1mS�P�ʂ̒x�����s��.
 ********************************************************************
 */
void cmd_wait_msec(void)
{
//	ushort ms = PacketFromPC.size;
	uchar ms = PacketFromPC.raw[1];	// '-dN'
	if(ms) {
		wait_ms(ms);
	}
}

/********************************************************************
 *	�f�[�^�̘A�����M���s.
 ********************************************************************
 */
void make_report(void)
{
#if	TIMER2_INT_SAMPLE			// �^�C�}�[�Q���荞�݂�PORT�T���v��.
	uchar i;
	uchar cnt=0;
	int c;
	//�T���v���l���ő�60�܂ŕԂ�����.
	PacketToPC.raw[0] =  REPLY_REPORT;		/* �R�}���h���s������HOST�ɒm�点��. */
	for(i=0;i<60;i++) {
		c = mon_read_sample();
		if(c<0) break;
		PacketToPC.raw[2+i]=c;
		cnt++;
	}
	PacketToPC.raw[1] =  cnt;
#else
	//�T���v���l���P�����Ԃ�����.
	PacketToPC.raw[0] =  REPLY_REPORT;		/* �R�}���h���s������HOST�ɒm�点��. */

	if(	poll_mode == POLL_ANALOG) {
		PacketToPC.raw[1] = 2;
//        while(ADCON0bits.NOT_DONE);     // Wait for conversion
//		PacketToPC.raw[2] = ADRESL;
//		PacketToPC.raw[3] = ADRESH;

	}else{
		PacketToPC.raw[1] = 1;
		PacketToPC.raw[2] = *poll_addr;
	}
#endif
	ToPcRdy = 1;
}
/********************************************************************
 *	�f�[�^����胂�[�h�̐ݒ�
 ********************************************************************
 */
void cmd_set_mode(void)
{
	poll_mode = PacketFromPC.size;
	poll_addr = (uchar  *) PacketFromPC.adrs;

	if(	poll_mode == POLL_ANALOG) {
//		mInitPOT();
//		ADCON0bits.GO = 1;              // Start AD conversion
	}

	make_report();
}

#if	1
/********************************************************************
 *	AVR���C�^�[�̐ݒ�
 ********************************************************************
 */
void cmd_set_status(void)
{
/* ISP�p�̃s����Hi-Z���� */
/* ISP�ڍs�̎菇���A�t�@�[�����Ŏ��� */
	if(PacketFromPC.raw[2] & 0x10) {// RST�����̏ꍇ
		ispDisconnect();
	}else{
		if(PacketFromPC.raw[2] & 0x80) {// RST��Ԃ�SCK H�� SCK�p���X�v��
			ispSckPulse();
		} else {
			ispConnect();
		}
	}
	PacketToPC.raw[0] = REPLY_REPORT;	/* �R�}���h���s������HOST�ɒm�点��. */
	ToPcRdy = 1;
}
/********************************************************************
 *	PORT�ւ̏o�͐���
 ********************************************************************
 */
void cmd_tx(void)
{
	isp_command( &PacketFromPC.raw[1]);
	ToPcRdy = Cmd0 & 1;	// LSB��On�Ȃ�ԓ����K�v.
}
/********************************************************************
 *	�y�[�W�A�h���X�̐ݒ�.
 ********************************************************************
 */
void cmd_set_page(void)
{
	page_mode = PacketFromPC.raw[1];
	page_addr = PacketFromPC.raw[2];
	page_addr_h = PacketFromPC.raw[3];
}
/********************************************************************
 *	ISP�������݃N���b�N�̐ݒ�.
 ********************************************************************
 */
void cmd_set_delay(void)
{
	usi_set_delay(PacketFromPC.raw[1]);	// '-dN'
}

/********************************************************************
 *	AVR��������(Fusion)�R�}���h�̎��s
 ********************************************************************
 *	Cmd0 : 0x20�`0x27
 */
void cmd_page_tx(void)
{
	uchar i;
	uchar size=PacketFromPC.raw[1];
	//
	//	page_write�J�n����page_addr��data[1]�ŏ�����.
	//
	if( Cmd0 & (HIDASP_PAGE_TX_START & MODE_MASK)) {
		page_mode = 0x40;
		page_addr = 0;
		page_addr_h = 0;
	}
	//
	//	page_write (�܂���page_read) �̎��s.
	//
	for(i=0;i<size;i++) {
		usi_trans(page_mode);
		usi_trans(page_addr_h);
		usi_trans(page_addr);
		PacketToPC.raw[i]=usi_trans(PacketFromPC.raw[i+2]);

		if (page_mode & 0x88) { // EEPROM or FlashH
			page_addr++;
			if(page_addr==0) {page_addr_h++;}
			page_mode&=~0x08;
		} else {
			page_mode|=0x08;
		}
	}
	//
	//	isp_command(Flash��������)�̎��s.
	//
	if( Cmd0 & (HIDASP_PAGE_TX_FLUSH & MODE_MASK)) {
		isp_command( &PacketFromPC.raw[size+2]);
	}
	ToPcRdy = Cmd0 & 1;	// LSB��On�Ȃ�ԓ����K�v.
}

/********************************************************************
 *	AVR���C�^�[�n�R�}���h��M�Ǝ��s.
 ********************************************************************
 *	Cmd0 : 0x20�`0x2F
 */
void cmd_avrspx(void)
{
	if(Cmd0 < (HIDASP_CMD_TX) ) 	{cmd_page_tx();}	// 0x20�`0x27

	// 0x28�`0x2F
	else if(Cmd0==HIDASP_SET_STATUS){cmd_set_status();}
	else if(Cmd0==HIDASP_SET_PAGE) 	{cmd_set_page();}
	else if(Cmd0==HIDASP_CMD_TX) 	{cmd_tx();}
	else if(Cmd0==HIDASP_CMD_TX_1) 	{cmd_tx();}
	else if(Cmd0==HIDASP_SET_DELAY) {cmd_set_delay();}
	else if(Cmd0==HIDASP_WAIT_MSEC) {cmd_wait_msec();}
}

#endif


#define	mHIDTxIsBusy()	(0)
#define	mHIDRxIsBusy()	(1)


/********************************************************************
 *	���j�^�R�}���h��M�Ǝ��s.
 ********************************************************************
 */
void ProcessIO(void)
{
	// �ԓ��p�P�b�g����ł��邱�ƁA���A
	// �����Ώۂ̎�M�f�[�^������.
	if((ToPcRdy == 0)) {
		//��M�f�[�^������΁A��M�f�[�^���󂯎��.
		PacketToPC.raw[0]=Cmd0;		// CMD ECHOBACK

		//�R�}���h�ɑΉ����鏈�����Ăяo��.
			 if(Cmd0 >= HIDASP_PAGE_TX)  {cmd_avrspx();}	// AVR���C�^�[�R�}���h.
		else if(Cmd0 >= PICSPX_SETADRS24){cmd_picspx();}	// PIC���C�^�[�R�}���h.
		else 
		switch(Cmd0) {
		 case HIDASP_PEEK: 		{cmd_peek();break;}	// �������[�ǂݏo��.
		 case HIDASP_POKE: 		{cmd_poke();break;}	// �������[��������.
		 case HIDASP_JMP: 		{cmd_exec(PacketFromPC.adrs,PacketFromPC.size);break;}	// ���s.
		 case HIDASP_PAGE_ERASE:{cmd_page_erase();break;}	//Flash����.
		 case HIDASP_PAGE_WRITE:{cmd_page_write();break;}	//Flash����.
		 case HIDASP_FLASH_LOCK:{cmd_flash_lock();break;}	//FlashLock.
		 case HIDASP_SET_MODE:  {cmd_set_mode();break;}
		 case HIDASP_TEST: 		{cmd_echo();break;}			// �ڑ��e�X�g.

#if	APPLICATION_MODE
		 case HIDASP_GET_STRING:{cmd_get_string();break;}
		 case HIDASP_USER_CMD:  {cmd_user_cmd();break;}
#endif
		 default: break;
		}
	}

	// �K�v�Ȃ�A�ԓ��p�P�b�g���C���^���v�g�]��(EP1)�Ńz�X�gPC�ɕԋp����.
	if( ToPcRdy ) {
		if(!mHIDTxIsBusy()) {
			UserToPMABufferCopy(PacketToPC.raw, GetEPTxAddr(ENDP1), 64);
			/* enable endpoint for transmission */
			SetEPTxValid(ENDP1);
			ToPcRdy = 0;

			if(poll_mode!=0) {
				if(mHIDRxIsBusy()) {	//�R�}���h�����Ȃ����著�葱����.
					make_report();
				}
			}
		}
	}
}
/********************************************************************
 *
 ********************************************************************
 */
void EP1_OUT_Callback(void)
{
//	led_off();
	USB_SIL_Read(EP1_OUT, PacketFromPC.raw);
	ProcessIO();

#ifndef STM32F10X_CL   
	SetEPRxStatus(ENDP1, EP_RX_VALID);	//���̎�M������.
#endif /* STM32F10X_CL */
}
/********************************************************************
 *
 ********************************************************************
 */
void USBtask(void)
{
	if( user_func ) {
		user_func(user_arg);
		user_func =NULL;
	}
}


/********************************************************************
 *
 ********************************************************************
 */
