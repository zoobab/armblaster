/********************************************************************
 *	������
 ********************************************************************
#BASEBOARD = STM8S_D
#BASEBOARD = CQ_STARM
#BASEBOARD = STBEE
#BASEBOARD = STBEE_MINI
 */
#include "stm32f10x.h"
#include "platform_config.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "stm32_eval.h"
#include "monit.h"


#ifdef	STM8S_D
#define	LED_PORT		GPIOA
#define	LED_PIN			GPIO_Pin_8

#define	HWTYPE	0			// 0:STM8S 1:STBEE 2:CQ-STARM 3:STBEE-MINI
#define	ROMSIZE	(64/32)		// 32kB�̔{��.
#define	RAMSIZE	(20/4)		//  4kB�̔{��.
#define	SIGNATURESTRING	"STM8S-D"

#endif

#ifdef	STBEE
#define	LED_PORT		GPIOD
#define	LED_PIN			GPIO_Pin_4
#define	USB_PULLUP_PORT	GPIOD
#define	USB_PULLUP_BIT  GPIO_Pin_3

#define	HWTYPE	1
#define	ROMSIZE	(512/32)	// 32kB�̔{��.
#define	RAMSIZE	(20/4)		//  4kB�̔{��.
#define	SIGNATURESTRING	"STBEE"

#endif

#ifdef	CQ_STARM
#define	LED_PORT		GPIOC
#define	LED_PIN			GPIO_Pin_6

#define	HWTYPE	2
#define	ROMSIZE	(128/32)	// 32kB�̔{��.
#define	RAMSIZE	(20/4)		//  4kB�̔{��.
#define	SIGNATURESTRING	"CQ-STARM"

#endif


#ifdef	STBEE_MINI
#define	LED_PORT		GPIOA
#define	LED_PIN			GPIO_Pin_15
#define	USB_PULLUP_PORT	GPIOA
#define	USB_PULLUP_BIT  GPIO_Pin_14

#define	HWTYPE	3
#define	ROMSIZE	(128/32)	// 32kB�̔{��.
#define	RAMSIZE	(20/4)		//  4kB�̔{��.
#define	SIGNATURESTRING	"STBEE-MINI"

#endif

#define	_BV(n)		( 1 << (n) )

//
//	16byte �� ����:	�@�픻�ʗp
//
typedef	struct {
	uchar mark[4];	// "ARM"
	uchar signid;	// 0xa3 : ARM Cortex-M3
	uchar hwtype;	// �n�[�h�E�F�A�^�C�v.
	uchar romsize;	// ROM�e��.
	uchar ramsize;	// RAM�e��.
	uchar name[12];	// ���̕�����.
} SIGNATURE;


__attribute__ ((section(".isr_vector")))
const SIGNATURE HW_Signature=
{
	"ARM",
	0xa3,
	HWTYPE,
	ROMSIZE,
	RAMSIZE,
	SIGNATURESTRING
//	"01234567890"
};

register char *stack_ptr asm ("sp");

/********************************************************************
 *
 ********************************************************************
 */
void led_on(void)
{
	GPIO_SetBits(LED_PORT,LED_PIN);
}

/********************************************************************
 *
 ********************************************************************
 */
void led_off(void)
{
	GPIO_ResetBits(LED_PORT,LED_PIN);
}

/********************************************************************
 *
 ********************************************************************
 */
void led_blink(int interval)
{
	static int led_cnt=0;
	int mask = 1<<interval;

	led_cnt++;
	if(led_cnt & mask) {
		led_on();
	} else {
		led_off();
	}
}


extern	char _estack[];

#if	0
/********************************************************************
 *	hot start (boot�R�}���h)���ꂽ���ǂ�����STACK LEVEL�Ŕ��肷��.
 ********************************************************************
 *	�߂�l:
 *		0: HOT START
 *		1: COLD START
 */
static int check_cold(int sp)
{
	int sp0 = _estack;
	int level = sp0-sp;
	
//	int *p = (int*) 0x20002008;
//	p[0]=level;

	if(level < 0x20 ) return 1;	// COLD START
	return 0;					// HOT START

//	�g���b�N: 
//		COLD START�����ꍇ�́ACPU�̃��Z�b�g�V�[�P���X�ɂ��A
//		sp = _estack(0�Ԓn) �Ƃ��������������s�����.
//
//		HOT START �����ꍇ(boot�R�}���h���s��) ��sp�̏����l���
//		���s���Ȃ�(�蔲�����Ă���)���߁ACOLD START�����X�^�b�N���[���Ȃ�.
//
//		���̈Ⴂ�𗘗p���āAHOT START����BOOT JUMPER�𖳎������邱�Ƃ��\.
}
#endif

#ifdef	STM8S_D
/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(int sp)
{
	GPIO_InitTypeDef GPIO_InitStructureA;
	GPIO_InitTypeDef GPIO_InitStructureB;
	GPIO_InitTypeDef GPIO_InitStructureD;

	// GPIO A,B,C,D �ɃN���b�N��^����.
	RCC_APB2PeriphClockCmd(
	    RCC_APB2Periph_GPIOA |
	    RCC_APB2Periph_GPIOB |
	    RCC_APB2Periph_GPIOC | /* == RCC_APB2Periph_GPIO_IOAIN      */
	    RCC_APB2Periph_GPIOD   /* == RCC_APB2Periph_GPIO_DISCONNECT */
	    , ENABLE);

	// GPIO_B.5 (SWIM RESET#) pull-up
	GPIO_InitStructureB.GPIO_Pin   = GPIO_Pin_5;
	GPIO_InitStructureB.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureB.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructureB);

#if	BOOTLOADER_MODE
	GPIO_SetBits(GPIOB , GPIO_Pin_5);
#endif

	/* PD.09 used as USB pull-up */
	GPIO_InitStructureD.GPIO_Pin   = GPIO_Pin_9;			// PD9 �� USB PullUp�炵��.
	GPIO_InitStructureD.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureD.GPIO_Mode  = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOD, &GPIO_InitStructureD);

	// GPIO_A.8 = LED
	GPIO_InitStructureA.GPIO_Pin   = GPIO_Pin_8;
	GPIO_InitStructureA.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureA.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructureA);

	GPIO_SetBits(GPIOA , GPIO_Pin_8);	// LED�_��.

	//
	// 0x0800_2000 �N���Ńr���h�����ꍇ�̓��[�U�[�t�@�[���E�F�A�ւ�JUMP�������s��Ȃ�.
	//
#if	BOOTLOADER_MODE
	// GND <=> RESET#(SWIM=PB6)�Ԃ̃W�����p�[�N���[�Y������.
	int boot_jumper_pin = GPIO_ReadInputData(GPIOB) & 0x40;

	GPIO_ResetBits(GPIOB , GPIO_Pin_5);	// pull-up��߂�.
	if(check_cold(sp))
	if(boot_jumper_pin) {	//�W�����p�[�I�[�v��.
		int *resvec = (int *) 0x08002000;	// reset vector
		int adrs = resvec[1];
		if((adrs & 0xfff00000) == 0x08000000) {	// 0x0800_0000 �` 0x080f_ffff �܂ł̊Ԃ�jump.
			call_func(adrs);
		}
	}
#endif
}
#endif


#ifdef	CQ_STARM
/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(int sp)
{
	GPIO_InitTypeDef GPIO_InitStructureA;
	GPIO_InitTypeDef GPIO_InitStructureB;
	GPIO_InitTypeDef GPIO_InitStructureC;
	GPIO_InitTypeDef GPIO_InitStructureD;

	// GPIO A,B,C,D �ɃN���b�N��^����.
	RCC_APB2PeriphClockCmd(
	    RCC_APB2Periph_GPIOA |
	    RCC_APB2Periph_GPIOB |
	    RCC_APB2Periph_GPIOC | /* == RCC_APB2Periph_GPIO_IOAIN      */
	    RCC_APB2Periph_GPIOD   /* == RCC_APB2Periph_GPIO_DISCONNECT */
	    , ENABLE);

	// GPIO_B
	GPIO_InitStructureB.GPIO_Pin   = 0;
	GPIO_InitStructureB.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureB.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructureB);

	// GPIO_C.6 = LED
	GPIO_InitStructureC.GPIO_Pin   = GPIO_Pin_6;
	GPIO_InitStructureC.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureC.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructureC);

	GPIO_SetBits(GPIOC , GPIO_Pin_6);	// LED�_��.

	//
	// 0x0800_2000 �N���Ńr���h�����ꍇ�̓��[�U�[�t�@�[���E�F�A�ւ�JUMP�������s��Ȃ�.
	//
#if	BOOTLOADER_MODE
	int boot_jumper_pin ;
	boot_jumper_pin = GPIO_ReadInputDataBit(GPIOB, GPIO_Pin_9);
	if(check_cold(sp))
	if(boot_jumper_pin) {	//�W�����p�[�I�[�v��.
		int *resvec = (int *) 0x08002000;	// reset vector
		int adrs = resvec[1];
		if((adrs & 0xfff00000) == 0x08000000) {	// 0x0800_0000 �` 0x080f_ffff �܂ł̊Ԃ�jump.
			call_func(adrs);
		}
	}
#endif
}
#endif


#ifdef	STBEE
/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(int sp)
{
	GPIO_InitTypeDef GPIO_InitStructureA;
	GPIO_InitTypeDef GPIO_InitStructureB;
	GPIO_InitTypeDef GPIO_InitStructureC;
	GPIO_InitTypeDef GPIO_InitStructureD;

	//check_hot(sp);
	// GPIO A,B,C,D �ɃN���b�N��^����.
	RCC_APB2PeriphClockCmd(
	    RCC_APB2Periph_GPIOA |
	    RCC_APB2Periph_GPIOB |
	    RCC_APB2Periph_GPIOC | /* == RCC_APB2Periph_GPIO_IOAIN      */
	    RCC_APB2Periph_GPIOD   /* == RCC_APB2Periph_GPIO_DISCONNECT */
	    , ENABLE);

	// GPIO_B
	GPIO_InitStructureB.GPIO_Pin   = 0;
	GPIO_InitStructureB.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureB.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructureB);

	/* PD.3 used as USB pull-up (LOW=pull-up) */
	/* PD.4 = LED */
	GPIO_InitStructureD.GPIO_Pin   = GPIO_Pin_3|GPIO_Pin_4;	// PD3 �� USB PullUp�炵��.
	GPIO_InitStructureD.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureD.GPIO_Mode  = GPIO_Mode_Out_OD;
	GPIO_Init(GPIOD, &GPIO_InitStructureD);

	// GPIO_C.6 = LED
	GPIO_InitStructureC.GPIO_Pin   = GPIO_Pin_6;
	GPIO_InitStructureC.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureC.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructureC);

//	GPIO_SetBits(GPIOC , GPIO_Pin_6);	// LED�_��.


	//
	// 0x0800_2000 �N���Ńr���h�����ꍇ�̓��[�U�[�t�@�[���E�F�A�ւ�JUMP�������s��Ȃ�.
	//
#if	BOOTLOADER_MODE
	int boot_jumper_pin ;
	boot_jumper_pin = GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0);
	if(check_cold(sp))
	if(boot_jumper_pin==0) {	//USER SW �I�[�v��.
		int *resvec = (int *) 0x08002000;	// reset vector
		int adrs = resvec[1];
		if((adrs & 0xfff00000) == 0x08000000) {	// 0x0800_0000 �` 0x080f_ffff �܂ł̊Ԃ�jump.
			call_func(adrs);
		}
	}
#endif
}
#endif


#ifdef	STBEE_MINI
/*******************************************************************************
* Function Name  : GPIO_Configuration
* Description    : Configures the different GPIO ports.
* Input          : None
* Output         : None
* Return         : None
*******************************************************************************/
void GPIO_Configuration(int sp)
{
	GPIO_InitTypeDef GPIO_InitStructureA;
	GPIO_InitTypeDef GPIO_InitStructureB;
	GPIO_InitTypeDef GPIO_InitStructureC;
//	GPIO_InitTypeDef GPIO_InitStructureD;

	// JTAG�𖳌��ɂ��܂��B
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO , ENABLE);
	AFIO->MAPR = _BV(26);

	// GPIO A,B,C,D �ɃN���b�N��^����.
	RCC_APB2PeriphClockCmd(
	    RCC_APB2Periph_GPIOA |
	    RCC_APB2Periph_GPIOB |
	    RCC_APB2Periph_GPIOC | /* == RCC_APB2Periph_GPIO_IOAIN      */
	    RCC_APB2Periph_GPIOD   /* == RCC_APB2Periph_GPIO_DISCONNECT */
	    , ENABLE);

	// GPIO_A 13,15=LED 14=USB Pull-up
	GPIO_InitStructureA.GPIO_Pin   = GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15;
	GPIO_InitStructureA.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureA.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructureA);

	// GPIO_B
	GPIO_InitStructureB.GPIO_Pin   = 0;
	GPIO_InitStructureB.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureB.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructureB);

	// GPIO_C.6 = LED
	GPIO_InitStructureC.GPIO_Pin   = GPIO_Pin_6;
	GPIO_InitStructureC.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureC.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOC, &GPIO_InitStructureC);

//	GPIO_SetBits(GPIOC , GPIO_Pin_6);	// LED�_��.

	//
	// 0x0800_2000 �N���Ńr���h�����ꍇ�̓��[�U�[�t�@�[���E�F�A�ւ�JUMP�������s��Ȃ�.
	//
#if	BOOTLOADER_MODE
	int boot_jumper_pin ;
	boot_jumper_pin = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_13);
	if(check_cold(sp))
	if(boot_jumper_pin) {	//�W�����p�[�I�[�v��.
		int *resvec = (int *) 0x08002000;	// reset vector
		int adrs = resvec[1];
		if((adrs & 0xfff00000) == 0x08000000) {	// 0x0800_0000 �` 0x080f_ffff �܂ł̊Ԃ�jump.
			call_func(adrs);
		}
	}
#endif
}
#endif


/*******************************************************************************
* Function Name  : USB_Cable_Config.
* Description    : Software Connection/Disconnection of USB Cable.
* Input          : NewState: new state.
* Output         : None.
* Return         : None.
*******************************************************************************/
#ifdef	STBEE_MINI

void USB_Cable_Config (FunctionalState NewState)
{
#ifdef	USB_PULLUP_PORT
	if (NewState == DISABLE) {
		GPIO_ResetBits(USB_PULLUP_PORT, USB_PULLUP_BIT);
	} else {
		GPIO_SetBits(  USB_PULLUP_PORT, USB_PULLUP_BIT);
	}
#endif
}

#else
//	�ȉ���pullup����PIN�����_��(L�ŃA�N�e�B�u).
//		CQ_STARM
//		STBEE

//	�ȉ��́Apullup����PIN���g���Ă��Ȃ�.
//		STM8_D
//
void USB_Cable_Config (FunctionalState NewState)
{
#ifdef	USB_PULLUP_PORT
	if (NewState != DISABLE) {
		GPIO_ResetBits(USB_PULLUP_PORT, USB_PULLUP_BIT);
	} else {
		GPIO_SetBits(  USB_PULLUP_PORT, USB_PULLUP_BIT);
	}
#endif
}
#endif



#ifndef	USB_PULLUP_PORT

//
//	USB_PULLUP_PORT ������o���Ȃ��@��.
//

//	CQ_STARM
//	STM8S_D

void usbModuleDisable(void)
{
	PowerOff();			// Disconnect USB.
	GPIO_InitTypeDef GPIO_InitStructureA;

	// GPIO_A.8 = LED  11=USB D+ 12=USB D-
	GPIO_InitStructureA.GPIO_Pin   = GPIO_Pin_8|GPIO_Pin_11|GPIO_Pin_12; // LED,USB D+,D-
	GPIO_InitStructureA.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructureA.GPIO_Mode  = GPIO_Mode_Out_PP;
	GPIO_Init(GPIOA, &GPIO_InitStructureA);

	GPIOA->ODR = 0;		// USB D+ D- ��GND�ɂ���.

}

#else

//
//	USB_PULLUP_PORT �𐧌�o����@��.
//

//	STBEE
//	STBEE_MINI

void usbModuleDisable(void)
{
	PowerOff();					// Disconnect USB.
	USB_Cable_Config(DISABLE);
}

#endif


