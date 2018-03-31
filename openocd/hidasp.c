/* hidasp.c
 * original: binzume.net
 * modify: senshu , iruka
 * 2008-09-22 : for HIDaspx.
 * 2008-11-07 : for HIDaspx, hidmon, bootmon88
 */
#include <windows.h>
#include <stdio.h>
#include "usbhid.h"
#include "monit.h"
#include "hidasp.h"
//#include "avrspx.h"

//#include "../firmware/hidcmd.h"
#include "hidcmd.h"

#ifndef __GNUC__
#pragma comment(lib, "setupapi.lib")
#endif


//	for JTAG
#define	JTAG_DUMP		0		// JTAG COMMAND ���_���v.
#define	POKE_DUMP		0		// PEEK/POKE COMMAND ���_���v.
#define	DEBUG_STREAM	0		// 1: BitBang �f�[�^����f�o�b�O����.

//----------------------------------------------------------------------------
//	Switches
//----------------------------------------------------------------------------

#define DEBUG 		   	0		// for DEBUG

#define DEBUG_PKTDUMP   0		// HID Report�p�P�b�g���_���v����.
#define DUMP_PRODUCT   	0		// ������,���i����\��.

#define CHECK_COUNT		4		// 4: Connect���� Ping test �̉�.
#define BUFF_SIZE		256


#define	PAGE_WRITE_LENGTH	32

#define	ASYNC_RW		1
#define	ASYNC_DEBUG		1

#define	TIMEOUT_MS 		8000

OVERLAPPED g_ovl;                // �I�[�o�[���b�v�\����
HANDLE     hEvent;

//----------------------------------------------------------------------------
//	Defines
//----------------------------------------------------------------------------

#ifdef	LPC2388
static int MY_VID = 0x0483;	// MicroChip
static int MY_PID = 0x5750;	// HIDbootloader
#define	MY_Product		"ARMblast"
#else

#ifdef	LPC1343
static int MY_VID = 0x1fc9;	// NXP
static int MY_PID = 0x0003;	// HID
#define	MY_Product		"ARMblast"
#else
static int MY_VID = 0x0483;	// MicroChip
static int MY_PID = 0x5750;	// HIDbootloader
#define	MY_Product		"ARMblast"
#endif

#endif

//#define	MY_Manufacturer	"YCIT"
#define	MY_Manufacturer	"AVRetc"

//	MY_Manufacturer,MY_Product ��define ���O���ƁAVID,PID�݂̂̏ƍ��ɂȂ�.
//	�ǂ��炩���͂����ƁA���̏ƍ����ȗ�����悤�ɂȂ�.
static int found_hidaspx;
//extern 
int hidmon_mode=0;

// HID API (from w2k DDK)
_HidD_GetAttributes 		HidD_GetAttributes;
_HidD_GetHidGuid 			HidD_GetHidGuid;
_HidD_GetPreparsedData 		HidD_GetPreparsedData;
_HidD_FreePreparsedData 	HidD_FreePreparsedData;
_HidP_GetCaps 				HidP_GetCaps;
_HidP_GetValueCaps 			HidP_GetValueCaps;
_HidD_GetFeature 			HidD_GetFeature;
_HidD_SetFeature 			HidD_SetFeature;
_HidD_GetManufacturerString	HidD_GetManufacturerString;
_HidD_GetProductString 		HidD_GetProductString;
_HidD_GetSerialNumberString HidD_GetSerialNumberString;	// add by senshu

HINSTANCE hHID_DLL = NULL;		// hid.dll handle
HANDLE hHID = NULL;				// USB-IO dev handle
HIDP_CAPS Caps;

static	int dev_id       = 0;	// �^�[�Q�b�gID: 0x25 �������� 0x14 ���������e.
static	int dev_version  = 0;	// �^�[�Q�b�g�o�[�W���� hh.ll
static	int dev_bootloader= 0;	// �^�[�Q�b�g��bootloader�@�\������.

static	int have_isp_cmd = 0;	// ISP����̗L��.

static	int	flash_size     = 0;	// FLASH_ROM�̍ŏI�A�h���X+1
static	int	flash_end_addr = 0;	// FLASH_ROM�̎g�p�ςݍŏI�A�h���X(4�̔{���؎�)

//----------------------------------------------------------------------------
//--------------------------    Tables    ------------------------------------
//----------------------------------------------------------------------------
//  HID Report �̃p�P�b�g�̓T�C�Y���� 3��ޗp�ӂ���Ă���.
	#define	REPORT_ID1			0
	#define	REPORT_LENGTH1		65
	//	�ő�̒��������� HID ReportID,Length
//	#define	REPORT_IDMAX		0
//	#define	REPORT_LENGTHMAX	65


#if	DEBUG_PKTDUMP || ASYNC_DEBUG
static	void memdump(char *msg, char *buf, int len);
#endif


#if	1
/*
 *	HID�f�o�C�X�� HID Report �𑗐M����.
 *	���M�o�b�t�@�̐擪��1�o�C�g��ReportID �����鏈����
 *	���̊֐����ōs���̂ŁA�擪1�o�C�g��\�񂵂Ă�������.
 *
 *	id �� Length �̑g�̓f�o�C�X���Œ�`���ꂽ���̂łȂ���΂Ȃ�Ȃ�.
 *
 *	�߂�l��HidD_SetFeature�̖߂�l( 0 = ���s )
 *
 */
static int hidWrite(HANDLE h, char *buf, int Length, int id)
{
	int rc;
	DWORD sz;
	buf[0] = 0;		// ReportID�͏��0

#if	ASYNC_RW
	int err;
	// �C�x���g�I�u�W�F�N�g���쐬����
	hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	if( hEvent == NULL ) {
		return -1;	// �G���[
	}
	// �I�[�o�[���b�v�\���̂̏�����
	ZeroMemory( &g_ovl, sizeof(g_ovl) );
	g_ovl.Offset = 0;
	g_ovl.OffsetHigh = 0;
	g_ovl.hEvent = hEvent;

	rc = WriteFile(h, buf , Length , &sz, &g_ovl);
	if(rc==0) {	// Pending?
		if( (err=GetLastError()) != ERROR_IO_PENDING ) {
#if ASYNC_DEBUG
			memdump("WR", buf, Length);
			fprintf(stderr, "hidWrite() error %x\n" ,err);
#endif
			return -1;		//ERROR
		}
		if( WaitForSingleObject( hEvent, TIMEOUT_MS ) == WAIT_OBJECT_0 ) {
			rc = Length;	//�ꉞ����.
		}else{
#if ASYNC_DEBUG
			memdump("WR", buf, Length);
			fprintf(stderr, "hidWrite() timeout\n" );
#endif
			return -1;		//�^�C���A�E�g,�j��,�������͎��s.
		}
	}

#else
	rc = WriteFile(h, buf , Length , &sz, NULL);
#endif

#if	DEBUG_PKTDUMP
	memdump("WR", buf, Length);
#endif
	return rc;
}


static int hidRead(HANDLE h, char *buf, int Length, int id)
{
	int rc;
	DWORD sz;

#if	ASYNC_RW
	int err;
	
	// �C�x���g�I�u�W�F�N�g���쐬����
	hEvent = CreateEvent( NULL, TRUE, FALSE, NULL );
	if( hEvent == NULL ) {
		return -1;	// �G���[
	}
	// �I�[�o�[���b�v�\���̂̏�����
	ZeroMemory( &g_ovl, sizeof(g_ovl) );
	g_ovl.Offset = 0;
	g_ovl.OffsetHigh = 0;
	g_ovl.hEvent = hEvent;

	rc = ReadFile(h, buf, Length,&sz,&g_ovl);
	if(rc==0) {	// Pending?
		if( (err=GetLastError()) != ERROR_IO_PENDING ) {
#if ASYNC_DEBUG
			fprintf(stderr, "hidRead() error %x\n" ,err);
#endif
			return -1;		//ERROR
		}
		if( WaitForSingleObject( hEvent, TIMEOUT_MS  ) == WAIT_OBJECT_0 ) {
			rc = Length;	//�ꉞ����.
		}else{
#if ASYNC_DEBUG
			fprintf(stderr, "hidRead() timeout\n" );
#endif
			return -1;		//�^�C���A�E�g,�j��,�������͎��s.
		}
	}

#else
	rc = ReadFile(h, buf, Length,&sz,NULL);
#endif

#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("id=%d Length=%d rc=%d\n",buf[0],Length,rc);
#endif
	return rc;
}

int	hidReadPoll(char *buf, int Length, int id)
{
	return hidRead(hHID, buf, Length, id);
}

#else
/*
 * wrapper for HidD_GetFeature / HidD_SetFeature.
 */
//----------------------------------------------------------------------------
/*
 *	HID�f�o�C�X���� HID Report ���擾����.
 *	�󂯎�����o�b�t�@�͐擪��1�o�C�g�ɕK��ReportID�������Ă���.
 *
 *	id �� Length �̑g�̓f�o�C�X���Œ�`���ꂽ���̂łȂ���΂Ȃ�Ȃ�.
 *
 *	�߂�l��HidD_GetFeature�̖߂�l( 0 = ���s )
 *
 */
static int hidRead(HANDLE h, char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = HidD_GetFeature(h, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("id=%d Length=%d rc=%d\n",id,Length,rc);
#endif
	return rc;
}

int	hidReadPoll(char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = HidD_GetFeature(hHID, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
#endif
	return rc;
}


/*
 *	HID�f�o�C�X�� HID Report �𑗐M����.
 *	���M�o�b�t�@�̐擪��1�o�C�g��ReportID �����鏈����
 *	���̊֐����ōs���̂ŁA�擪1�o�C�g��\�񂵂Ă�������.
 *
 *	id �� Length �̑g�̓f�o�C�X���Œ�`���ꂽ���̂łȂ���΂Ȃ�Ȃ�.
 *
 *	�߂�l��HidD_SetFeature�̖߂�l( 0 = ���s )
 *
 */
static int hidWrite(HANDLE h, char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = HidD_SetFeature(h, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("WR", buf, Length);
#endif
	return rc;
}

#endif

/*
 *	hidWrite()���g�p���āA�f�o�C�X���� buf[] �f�[�^�� len �o�C�g����.
 *	�������������ʂ��āAReportID�������I������.
 *
 *	�߂�l��HidD_SetFeature�̖߂�l( 0 = ���s )
 *
 */
int	hidWriteBuffer(char *buf, int len)
{
	len = 65;
	return hidWrite(hHID, buf, len , 0);
}

/*
 *	hidRead()���g�p���āA�f�o�C�X������ buf[] �f�[�^�� len �o�C�g�擾����.
 *	�������������ʂ��āAReportID�������I������.
 *
 *	�߂�l��HidD_GetFeature�̖߂�l( 0 = ���s )
 *
 */
int	hidReadBuffer(char *buf, int len , int id)
{
	len = 65;
	return hidRead(hHID, buf, len, 0);
}
/*
 *	hidWrite()���g�p���āA�f�o�C�X����4�o�C�g�̏��𑗂�.
 *	4�o�C�g�̓���� cmd , arg1 , arg2 , arg 3 �ł���.
 *  ReportID��ID1���g�p����.
 *
 *	�߂�l��HidD_SetFeature�̖߂�l( 0 = ���s )
 *
 */
int hidCommand(int cmd,int arg1,int arg2,int arg3)
{
	unsigned char buf[BUFF_SIZE];

	memset(buf , 0, sizeof(buf) );

	buf[1] = cmd;
	buf[2] = arg1;
	buf[3] = arg2;
	buf[4] = arg3;

	return hidWrite(hHID, buf, REPORT_LENGTH1, REPORT_ID1);
}

//
//	mask ��   0 �̏ꍇ�́A addr �� data0 ��1�o�C�g��������.
//	mask �� ��0 �̏ꍇ�́A addr �� data0 �� mask �̘_���ς���������.
//		�A���A���̏ꍇ�� mask bit�� 0 �ɂȂ��Ă��镔���ɉe����^���Ȃ��悤�ɂ���.
//
//	��:	PORTB �� 8bit �� data����������.
//		hidPokeMem( PORTB , data , 0 );
//	��:	PORTB �� bit2 ������ on
//		hidPokeMem( PORTB , 1<<2 , 1<<2 );
//	��:	PORTB �� bit2 ������ off
//		hidPokeMem( PORTB ,    0 , 1<<2 );
//
int hidPokeMem(int addr,int data0,int mask)
{
	unsigned char buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_POKE;
	buf[2] = 0;
	buf[3] = addr;
	buf[4] =(addr>>8);
	if( mask ) {
		buf[5] = data0 & mask;
		buf[6] = ~mask;
	}else{
		buf[5] = data0;
		buf[6] = 0;
	}
	return hidWrite(hHID, buf, REPORT_LENGTH1, REPORT_ID1);
}

int hidPeekMem(int addr)
{
	unsigned char buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_PEEK;
	buf[2] = 1;
	buf[3] = addr;
	buf[4] =(addr>>8);

	hidWrite(hHID, buf, REPORT_LENGTH1, REPORT_ID1);
	hidReadBuffer( buf, REPORT_LENGTH1,0 );
	return buf[1];
}

#if HIDMON88
static void hidSetDevCaps(void)
{
	hidCommand(HIDASP_GET_CAPS,0,0,0);	// bootloadHID���擾���ׂ������o�b�t�@�ɒu��.
}
#endif

#define	USICR			0x2d	//
#define	DDRB			0x37	// PB4=RST PB3=LED
#define	DDRB_WR_MASK	0xf0	// ����\bit = 1111_0000
#define	PORTB			0x38	// PB4=RST PB3=LED
#define	PORTB_WR_MASK	0		// 0 �̏ꍇ��MASK���Z�͏ȗ�����A������.

#define HIDASP_RST		0x10	// RST bit

/*
 *	LED�̐���. (hidasp.h)
#define HIDASP_RST_H_GREEN	0x18	// RST����,LED OFF
#define HIDASP_RST_L_BOTH	0x00	// RST���s,LED ON
#define HIDASP_SCK_PULSE 	0x80	// RST-L SLK-pulse ,LED ON	@@kuga
 */


static void hidSetStatus(int ledstat)
{
	unsigned char rd_data[BUFF_SIZE];
	static int once = 0;
	int ddrb;
	if (have_isp_cmd) {
		hidCommand(HIDASP_SET_STATUS,0,ledstat,0);	// cmd,portd(&0000_0011),portb(&0001_1111),0
		// ��PIC18F�t�@�[���̏ꍇ�A�n���h�V�F�[�N�p�P�b�g�͕K���󂯎��K�v������.
		hidRead(hHID, rd_data ,REPORT_LENGTH1, REPORT_ID1);
		//   �����łȂ��ƁA���̃R�}���h�̉����p�P�b�g�ɁA���̃S�~�����M����A�Ȍ�P�������.
	}else{
		if (once == 0 && !hidmon_mode) {
//			fprintf(stderr, "Warnning: Please update HIDaspx firmware.\n");
			fprintf(stderr, "Warnning: Please check HIDaspx mode.\n");
			once++;
		}

		if (hidmon_mode) {
			ddrb = 0xff;			// DDRB = 0xff : 1���o�̓s�� ������bit3-0�͉e�����Ȃ�.
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);
			hidPokeMem(DDRB ,ddrb   ,DDRB_WR_MASK);
		} else if(ledstat & HIDASP_RST) {	// RST����.
			ddrb = 0x10;			// PORTB �S����.
			hidPokeMem(USICR,0      ,0);
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);
			hidPokeMem(DDRB ,ddrb   ,DDRB_WR_MASK);
		}else{
			// RST��L�ɂ���.
			ddrb = 0xd0;			// DDRB 1101_1100 : 1���o�̓s�� ������bit3-0�͉e�����Ȃ�.
			hidPokeMem(USICR,0      ,0);
			hidPokeMem(DDRB ,ddrb   ,DDRB_WR_MASK);
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);
			hidPokeMem(PORTB,ledstat|HIDASP_RST,PORTB_WR_MASK);	// RST�͂܂�Hi
			hidPokeMem(PORTB,ledstat,PORTB_WR_MASK);	// RST�͂��Ƃ�L��.
			hidPokeMem(USICR,0x1a   ,0);
		}
	}
}

////////////////////////////////////////////////////////////////////////
//             hid.dll �����[�h
static int LoadHidDLL()
{
	hHID_DLL = LoadLibrary("hid.dll");
	if (!hHID_DLL) {
#if 1
		fprintf(stderr, "Error at Load 'hid.dll'\n");
#else
		MessageBox(NULL, "Error at Load 'hid.dll'", "ERR", MB_OK);
#endif
		return 0;
	}
	HidD_GetAttributes = (_HidD_GetAttributes)GetProcAddress(hHID_DLL, "HidD_GetAttributes");
	if (!HidD_GetAttributes) {
#if 1
		fprintf(stderr, "Error at HidD_GetAttributes\n");
#else
		MessageBox(NULL, "Error at HidD_GetAttributes", "ERR", MB_OK);
#endif
		return 0;
	}
	HidD_GetHidGuid = (_HidD_GetHidGuid)GetProcAddress(hHID_DLL, "HidD_GetHidGuid");
	if (!HidD_GetHidGuid) {
#if 1
		fprintf(stderr, "Error at HidD_GetHidGuid\n");
#else
		MessageBox(NULL, "Error at HidD_GetHidGuid", "ERR", MB_OK);
#endif
		return 0;
	}
	HidD_GetPreparsedData =	(_HidD_GetPreparsedData)GetProcAddress(hHID_DLL, "HidD_GetPreparsedData");
	HidD_FreePreparsedData = (_HidD_FreePreparsedData)GetProcAddress(hHID_DLL, "HidD_FreePreparsedData");
	HidP_GetCaps = (_HidP_GetCaps)GetProcAddress(hHID_DLL, "HidP_GetCaps");
	HidP_GetValueCaps =	(_HidP_GetValueCaps) GetProcAddress(hHID_DLL, "HidP_GetValueCaps");

//
	HidD_GetFeature = (_HidD_GetFeature)GetProcAddress(hHID_DLL, "HidD_GetFeature");
	HidD_SetFeature = (_HidD_SetFeature)GetProcAddress(hHID_DLL, "HidD_SetFeature");
	HidD_GetManufacturerString = (_HidD_GetManufacturerString)GetProcAddress(hHID_DLL, "HidD_GetManufacturerString");
	HidD_GetProductString = (_HidD_GetProductString)GetProcAddress(hHID_DLL, "HidD_GetProductString");
	HidD_GetSerialNumberString = (_HidD_GetSerialNumberString)GetProcAddress(hHID_DLL, "HidD_GetSerialNumberString");

#if	DEBUG
	printf("_HidD_GetFeature= %x\n", (int) HidD_GetFeature);
	printf("_HidD_SetFeature= %x\n", (int) HidD_SetFeature);
#endif
	return 1;
}

////////////////////////////////////////////////////////////////////////
// �f�B�o�C�X�̏����擾
static void GetDevCaps()
{
	PHIDP_PREPARSED_DATA PreparsedData;
	HIDP_VALUE_CAPS *VCaps;
	char buf[1024];

	VCaps = (HIDP_VALUE_CAPS *) (&buf);

	HidD_GetPreparsedData(hHID, &PreparsedData);
	HidP_GetCaps(PreparsedData, &Caps);
	HidP_GetValueCaps(HidP_Input, VCaps, &Caps.NumberInputValueCaps,  PreparsedData);
	HidD_FreePreparsedData(PreparsedData);

#if DEBUG
	fprintf(stderr, " Caps.OutputReportByteLength = %d\n", Caps.OutputReportByteLength);
	fprintf(stderr, " Caps.InputReportByteLength = %d\n", Caps.InputReportByteLength);
#endif


#if	REPORT_LENGTH_OVERRIDE
	//������REPORT_COUNT�����݂���Ƃ��͉��LCaps�� 0 �̂܂܂Ȃ̂ŁA�㏑������.
	Caps.OutputReportByteLength = REPORT_LENGTH_OVERRIDE;
	Caps.InputReportByteLength = REPORT_LENGTH_OVERRIDE;
#endif
}

//----------------------------------------------------------------------------
/*
 * 	unicode �� ASCII�ɕϊ�.
 */
static char *uni_to_string(char *t, unsigned short *u)
{
	char *buf = t;
	int c;
	// short �� char �z��ɓ���Ȃ���.
	while (1) {
		c = *u++;
		if (c == 0)
			break;
		if (c & 0xff00)
			c = '?';
		*t++ = c;
	}

	*t = 0;
	return buf;
}

//----------------------------------------------------------------------------
/*  Manufacturer & Product name check.
 *  ���O�`�F�b�N : ����=1  ���s=0 �ǂݎ��s�\=(-1)
 */
static int check_product_string(HANDLE handle, const char *serial, int list_mode)
{
	static int first = 1;
	int i;
	unsigned short unicode[BUFF_SIZE*2];
	char string1[BUFF_SIZE];
	char string2[BUFF_SIZE];
	char string3[BUFF_SIZE];
	char tmp[2][BUFF_SIZE];

#if	DEBUG
	list_mode = 1;
#endif

	Sleep(20);
	if (!HidD_GetManufacturerString(handle, unicode, sizeof(unicode))) {
		return -1;
	}
	uni_to_string(string1, unicode);

	Sleep(20);
	if (!HidD_GetProductString(handle, unicode, sizeof(unicode))) {
		return -1;
	}
	uni_to_string(string2, unicode);

	// �V���A���ԍ��̃`�F�b�N�������� (2010/02/12 13:24:08)
	if (serial[0]=='*') {
#define f_auto_retry 3		/* for HIDaspx (Auto detect, Retry MAX) */

		for (i=0; i<f_auto_retry; i++) {
			Sleep(20);
			if (!HidD_GetSerialNumberString(handle, unicode, sizeof(unicode))) {
				return -1;
			}
			uni_to_string(tmp[i%2], unicode);
			if ((i>0) && ((i%2) == 1)) {
				if (strcmp(tmp[0], tmp[1])==0) {
					strcpy(string3, tmp[0]);	// OK
				} else {
					return -1;
				}
			}
		}
		if (list_mode) {
			if (first) {
				fprintf(stderr,
				"VID=%04x, PID=%04x, [%s], [%s], serial=[%s] (*)\n", MY_VID, MY_PID, string1,  string2, string3);
				first = 0;
			} else {
				fprintf(stderr,
				"VID=%04x, PID=%04x, [%s], [%s], serial=[%s]\n", MY_VID, MY_PID, string1,  string2, string3);
			}
		}
	} else {
		Sleep(20);
		if (!HidD_GetSerialNumberString(handle, unicode, sizeof(unicode))) {
			return -1;
		}
		uni_to_string(string3, unicode);
	}


#ifdef	MY_Manufacturer
	if (strcmp(string1, MY_Manufacturer) != 0)
		return 0;	// ��v����
#endif

#ifdef	MY_Product
	if (strcmp(string2, MY_Product) != 0)
		return 0;	// ��v����
#endif

	found_hidaspx++;

	/* Check serial number */
	if (found_hidaspx) {
		if (strcmp(string3, serial) == 0)
			return 1;		//���v����.
		else if (strcmp(serial ,"*") == 0)
			return 1;		//���v����.
	}

	return 0;	// ��v����

}

////////////////////////////////////////////////////////////////////////
// HID�f�B�o�C�X�ꗗ����USB-IO������
static int OpenTheHid(const char *serial, int list_mode)
{
	int f, i, rc;
	ULONG Needed, l;
	GUID HidGuid;
	HDEVINFO DeviceInfoSet;
	HIDD_ATTRIBUTES DeviceAttributes;
	SP_DEVICE_INTERFACE_DATA DevData;
	PSP_INTERFACE_DEVICE_DETAIL_DATA DevDetail;
	//SP_DEVICE_INTERFACE_DETAIL_DATA *MyDeviceInterfaceDetailData;

	DeviceAttributes.Size = sizeof(HIDD_ATTRIBUTES);
	DevData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

	HidD_GetHidGuid(&HidGuid);
#if 1							/* For vista */
	DeviceInfoSet =
		SetupDiGetClassDevs(&HidGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
#else
	DeviceInfoSet =
		SetupDiGetClassDevs(&HidGuid, "", NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
#endif

	f = i = 0;
	while ((rc = SetupDiEnumDeviceInterfaces(DeviceInfoSet, 0, &HidGuid, i++, &DevData))!=0) {
		SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DevData, NULL, 0, &Needed, 0);
		l = Needed;
		DevDetail = (SP_DEVICE_INTERFACE_DETAIL_DATA *) GlobalAlloc(GPTR, l + 4);
		DevDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
		SetupDiGetDeviceInterfaceDetail(DeviceInfoSet, &DevData, DevDetail, l, &Needed, 0);

		hHID = CreateFile(DevDetail->DevicePath,
						  GENERIC_READ | GENERIC_WRITE,
						  FILE_SHARE_READ | FILE_SHARE_WRITE,
						  NULL, OPEN_EXISTING,
#if	ASYNC_RW
						  FILE_FLAG_OVERLAPPED,
#else
						  0,
#endif
						  NULL);

		GlobalFree(DevDetail);

		if (hHID == INVALID_HANDLE_VALUE)	// Can't open a device
			continue;
		HidD_GetAttributes(hHID, &DeviceAttributes);

		// HIDasp���ǂ������ׂ�.
		if (DeviceAttributes.VendorID == MY_VID && DeviceAttributes.ProductID == MY_PID) {
			int rc;
			rc = check_product_string(hHID, serial, list_mode);
			if ( rc == 1) {
				f++;				// ��������
				// ���������X�g���� (2010/02/12 13:19:38)
				if (list_mode == 0) {
					break;			// �����F�����́A�ŏ��̌��𗘗p����
				}
			}
		} else {
			// ����������
			CloseHandle(hHID);
			hHID = NULL;
		}
	}
	SetupDiDestroyDeviceInfoList(DeviceInfoSet);
	return f;
}

#if	DEBUG_PKTDUMP || ASYNC_DEBUG
//----------------------------------------------------------------------------
//  �������[�_���v.
void memdump(char *msg, char *buf, int len)
{
	int j;
	fprintf(stderr, "%s", msg);
	for (j = 0; j < len; j++) {
		fprintf(stderr, " %02x", buf[j] & 0xff);
		if((j & 0x1f)== 31)
			fprintf(stderr, "\n +");
	}
	fprintf(stderr, "\n");
}
#endif


int hidasp_list(char * string)
{
	int r, rc;

	LoadHidDLL();
	found_hidaspx = 0;
	r = OpenTheHid("*", 1);
	if (r == 0) {
		rc = 1;
	} else {
		rc = 0;
	}
	if (found_hidaspx==0) {
		fprintf(stderr, "%s: HIDaspx(VID=%04x, PID=%04x) isn't found.\n", string, MY_VID, MY_PID);
	}
	if (hHID_DLL) {
		FreeLibrary(hHID_DLL);
	}
	return rc;
}

//----------------------------------------------------------------------------
//	������
//----------------------------------------------------------------------------
int hidasp_init(int type, const char *serial)
{
	unsigned char rd_data[BUFF_SIZE];
	int *fl_size;
	int i, r, result;

	result = 0;

	LoadHidDLL();
	if (OpenTheHid(serial, 0) == 0) {
#if DEBUG
		fprintf(stderr, "ERROR: fail to OpenTheHid(%s)\n", serial);
#endif
		return HIDASP_MODE_ERROR;
	}

	GetDevCaps();

#if DEBUG
	fprintf(stderr, "HIDaspx Connection check!\n");
#endif

	for (i = 0; i < CHECK_COUNT; i++) {
//		hidCommand(HIDASP_TEST,(i),0,0);	// Connection test
		hidCommand(HIDASP_TEST,(i),(i+1),(i+2));	// Connection test
		r = hidRead(hHID, rd_data ,REPORT_LENGTH1, REPORT_ID1);

#if DEBUG
		fprintf(stderr, "HIDasp Ping(%2d) = %d\n", i, rd_data[4]);
#endif
		if (r == 0) {
			fprintf(stderr, "Error: fail to Read().\n");
			return HIDASP_MODE_ERROR;
		}

		dev_id         = rd_data[2];
		dev_version    = rd_data[3] | (rd_data[4]<<8) ;
		dev_bootloader = rd_data[5];

		fl_size = (int*)&rd_data[9];
//		memdump("fl_size",fl_size,8);
		flash_size     = fl_size[0];
		flash_end_addr = fl_size[1];
#if	0
		if((dev_id != DEV_ID_ARM_M3)) {
			fprintf(stderr, "Error: fail to ping test. (id = %x)\n",dev_id);
			return HIDASP_MODE_ERROR;
		}

		if (hidmon_mode) {
//			fprintf(stderr, "TARGET DEV_ID=%x\n",dev_id);
		}

		if (rd_data[6] != i) {
			fprintf(stderr, "Error: fail to ping test. %d != %d\n", rd_data[6] , i);
			return HIDASP_MODE_ERROR;
		}
#endif
	}
	{	char *sBoot="(Application)";
		if(dev_bootloader==1) {sBoot="(Bootloader)";}
		fprintf(stderr, "TARGET DEV_ID=%x VER=%d.%d%s FLASH=%x,%x\n",dev_id
				,dev_version>>8,dev_version & 0xff,sBoot
				,flash_end_addr,flash_size
				);
	}

#if	1
	if (!hidmon_mode) {
		hidCommand(HIDASP_SET_STATUS,0,HIDASP_RST_H_GREEN,0);	// RESET HIGH
		// ��PIC18F�t�@�[���̏ꍇ�A�n���h�V�F�[�N�p�P�b�g�͕K���󂯎��K�v������.
	r = hidRead(hHID, rd_data ,REPORT_LENGTH1, REPORT_ID1);
	if( rd_data[1] == 0xaa ) {				// ISP�R�}���h(isp_enable)�����퓮�삵��.
		have_isp_cmd = HIDASP_ISP_MODE;		// ISP����OK.
		result |= HIDASP_ISP_MODE;
	} else if( rd_data[1] == DEV_ID_FUSION ) {
		result |= HIDASP_FUSION_OK;			// NEW firmware
	} else if( rd_data[1] == DEV_ID_STD ) {
		result &= ~HIDASP_FUSION_OK;		// OLD firmware
	} else if( rd_data[1] == DEV_ID_MEGA88 ) {		// USB-IO mode
		result |= HIDASP_USB_IO_MODE;		// ISP����NG.
	} else if( rd_data[1] == DEV_ID_MEGA88_USERMODE ) {		// USB-IO mode
		result |= HIDASP_USB_IO_MODE;		// ISP����NG.
	}

#if DEBUG
	if (result & HIDASP_ISP_MODE) {
		fprintf(stderr, "[ISP CMD] ");
	}
	if (result & HIDASP_FUSION_OK) {
		fprintf(stderr, "[FUSION] ");
	}
	if (result & HIDASP_USB_IO_MODE) {
		fprintf(stderr, "[USB-IO mode] ");
	}
	if (result == 0) {
		fprintf(stderr, "ISP CMD Not support.\n");
	} else {
		fprintf(stderr, "OK.\n");
	}
#endif
#endif
	}

	return result;
}

//----------------------------------------------------------------------------
//  �I��.
//----------------------------------------------------------------------------
void hidasp_close()
{
#define HIDASP_NOP 			  0	//����� ISP�̃R�}���h�Ǝv����?

	if (hHID) {
		if (!hidmon_mode) {
		/* ��������s����ƁAPORTB7(SCK) ����u ON�ɂȂ� */
			unsigned char buf[BUFF_SIZE];

			buf[0] = 0x00;
			buf[1] = HIDASP_NOP;
			buf[2] = 0x00;
			buf[3] = 0x00;
			hidasp_cmd(buf, NULL);					// AVOID BUG!

			hidSetStatus(HIDASP_RST_H_GREEN);		// RESET HIGH
		}
#if	HIDMON88
		hidSetDevCaps();
#endif
		CloseHandle(hHID);
	}
	if (hHID_DLL) {
		FreeLibrary(hHID_DLL);
	}
	hHID = NULL;
	hHID_DLL = NULL;
}

//----------------------------------------------------------------------------
//  ISP�R�}���h���s.
//----------------------------------------------------------------------------
int hidasp_cmd(const unsigned char cmd[4], unsigned char res[4])
{
	char buf[128];
	int r;

	memset(buf , 0, sizeof(buf) );
	if (res != NULL) {
		buf[1] = HIDASP_CMD_TX_1;
	} else {
		buf[1] = HIDASP_CMD_TX;
	}
	memcpy(buf + 2, cmd, 4);

	r = hidWrite(hHID, buf, REPORT_LENGTH1 , REPORT_ID1);
#if DEBUG
	fprintf(stderr, "hidasp_cmd %02X, cmd: %02X %02X %02X %02X ",
		buf[1], cmd[0], cmd[1], cmd[2], cmd[3]);
#endif

	if (res != NULL) {
		r = hidRead(hHID, buf, REPORT_LENGTH1 , REPORT_ID1);
		memcpy(res, buf + 1, 4);
#if DEBUG
		fprintf(stderr, " --> res: %02X %02X %02X %02X\n",
				res[0], res[1], res[2], res[3]);
#endif
	}

	return 1;
}


#if (HIDMON || HIDASPX)
//----------------------------------------------------------------------------
//  �^�[�Q�b�g�}�C�R�����v���O�������[�h�Ɉڍs����.
//----------------------------------------------------------------------------
int hidasp_program_enable(int delay)
{
	unsigned char buf[BUFF_SIZE];
	unsigned char res[4];
	int i, rc;
	int tried;	//AVRSP �Ɠ��l�̃v���g�R�����̗p

	rc = 1;
	hidSetStatus(HIDASP_RST_H_GREEN);			// RESET HIGH
	hidSetStatus(HIDASP_RST_L_BOTH);			// RESET LOW (H PULSE)
	hidCommand(HIDASP_SET_DELAY,delay,0,0);		// SET_DELAY
	Sleep(30);				// 30

	for(tried = 0; tried < 3; tried++) {
		for(i = 0; i < 32; i++) {

			buf[0] = 0xAC;
			buf[1] = 0x53;
			buf[2] = 0x00;
			buf[3] = 0x00;
			hidasp_cmd(buf, res);

			if (res[2] == 0x53) {
				rc = 0;					// AVR�}�C�R���Ɠ������m�F
				goto hidasp_program_enable_exit;
			}
			if (tried < 2) {			// 2��܂ł͒ʏ�̓������@
				break;
			}
			// AT90S�p�̓������@�œ��������
			hidSetStatus(HIDASP_SCK_PULSE);			// RESET LOW SCK H PULSE shift scan point
		}
	}

	hidasp_program_enable_exit:
#if DEBUG
	if (rc == 0) {
		fprintf(stderr, "hidasp_program_enable() == OK\n");
	} else  {
		fprintf(stderr, "hidasp_program_enable() == NG\n");
	}
#endif
	return rc;
}




static	void hid_transmit(BYTE cmd1, BYTE cmd2, BYTE cmd3, BYTE cmd4)
{
	unsigned char cmd[4];

	cmd[0] = cmd1;
	cmd[1] = cmd2;
	cmd[2] = cmd3;
	cmd[3] = cmd4;
	hidasp_cmd(cmd, NULL);
}

//----------------------------------------------------------------------------
//  �t���[�W�����T�|�[�g�̃y�[�W���C�g.
//----------------------------------------------------------------------------
int hidasp_page_write_fast(long addr, const unsigned char *wd, int pagesize)
{
	int n, l;
	char buf[BUFF_SIZE];
	int cmd = HIDASP_PAGE_TX_START;

	memset(buf , 0, sizeof(buf) );

	// Load the page data into page buffer
	n = 0;	// n ��PageBuffer�ւ̏������݊�_offset.
	while (n < pagesize) {
		l = PAGE_WRITE_LENGTH;		// 32�o�C�g���ő�f�[�^��.
		if (pagesize - n < l) {		// �c�ʂ�32�o�C�g�����̂Ƃ��� len ���c�ʂɒu��������.
			l = pagesize - n;
		}
		buf[1] = cmd;				// HIDASP_PAGE_TX_* �R�}���h.
		buf[2] = l;					// l       �������݃f�[�^��̒���.
		memcpy(buf + 3, wd + n, l);	// data[l] �������݃f�[�^��.

		if((pagesize - n) == l) {
			buf[1] |= HIDASP_PAGE_TX_FLUSH;		//�ŏIpage_write�ł�isp_command��t������.
			// ISP �R�}���h��.
			buf[3 + l + 0] = C_WR_PAGE;
			buf[3 + l + 1] = (BYTE)(addr >> 9);
			buf[3 + l + 2] = (BYTE)(addr >> 1);
			buf[3 + l + 3] = 0;
		}
		hidWriteBuffer(buf, REPORT_LENGTHMAX);

		n += l;
		cmd = HIDASP_PAGE_TX;		// cmd ���m�[�}����page_write�ɖ߂�.
	}


	return 0;
}

//----------------------------------------------------------------------------
//  �y�[�W���C�g.
//----------------------------------------------------------------------------
int hidasp_page_write(long addr, const unsigned char *wd, int pagesize,int flashsize)
{
	int n, l;
	char buf[BUFF_SIZE];

//	if(	(dev_id == DEV_ID_FUSION) && (flashsize <= (128*1024)) ) {
	if(	(flashsize <= (128*1024)) ) {
//		((addr & 0xFF) == 0 	) ) {
		// addres_set , page_write , isp_command ���Z�����ꂽ�����ł����s.
		return hidasp_page_write_fast(addr,wd,pagesize);
	}
#if	0
	fprintf(stderr,"dev_id=%x flashsize=%x addr=%x\n",
		dev_id,flashsize,(int)addr
	);
#endif
	// set page
	hidCommand(HIDASP_SET_PAGE,0x40,0,(addr & 0xFF));	// Set Page mode , FlashWrite

	// Load the page data into page buffer
	n = 0;	// n ��PageBuffer�ւ̏������݊�_offset.
	while (n < pagesize) {
		l = PAGE_WRITE_LENGTH;	// MAX
		if (pagesize - n < l) {
			l = pagesize - n;
		}
		buf[0] = 0x00;
		buf[1] = HIDASP_PAGE_TX;	// PageBuf
		buf[2] = l;				// Len
		memcpy(buf + 3, wd + n, l);
		hidWriteBuffer(buf,  3 + l);

#if DEBUG
		fprintf(stderr, "  p: %02x %02x %02x %02x\n",
				buf[1] & 0xff, buf[2] & 0xff, buf[3] & 0xff, buf[4] & 0xff);
#endif
		n += l;
	}

	/* Load extended address if needed */
	if(flashsize > (128*1024)) {
		hid_transmit(C_LD_ADRX,0,(BYTE)(addr >> 17),0);
	}

	/* Start page programming */
	hid_transmit(C_WR_PAGE,(BYTE)(addr >> 9),(BYTE)(addr >> 1),0);

	return 0;
}

//----------------------------------------------------------------------------
//  �y�[�W���[�h.
//----------------------------------------------------------------------------
int hidasp_page_read(long addr, unsigned char *wd, int pagesize)
{
	int n, l;
	char buf[BUFF_SIZE];

	// set page
	if (addr >= 0) {
		hidCommand(HIDASP_SET_PAGE,0x20,0,0);	// Set Page mode , FlashRead
	}

	// Load the page data into page buffer
	n = 0;	// n �͓ǂݍ��݊�_offset.
	while (n < pagesize) {
		l = REPORT_LENGTHMAX - 1;	// MAX
		if (pagesize - n < l) {
			l = pagesize - n;
		}
		hidCommand(HIDASP_PAGE_RD,l,0,0);	// PageRead , length

		memset(buf + 3, 0, l);
		hidRead(hHID, buf, REPORT_LENGTHMAX, REPORT_IDMAX);
		memcpy(wd + n, buf + 1, l);			// Copy result.

#if 1	// �i���󋵂𒀎��\������
		report_update(l);
#else	// �i���󋵂�128�o�C�g���ɕ\������
		if (n % 128 == 0) {
			report_update(128);
		}
#endif

#if DEBUG
		fprintf(stderr, "  p: %02x %02x %02x %02x\n",
				buf[1] & 0xff, buf[2] & 0xff, buf[3] & 0xff, buf[4] & 0xff);
#endif
		n += l;
	}

	return 0;
}
#endif
//----------------------------------------------------------------------------
int	UsbGetDevID(void)
{
	return dev_id;
}
//----------------------------------------------------------------------------
int	UsbGetDevVersion(void)
{
	return dev_version;
}
int	UsbGetDevCaps(void)
{
	return dev_bootloader;
}
//----------------------------------------------------------------------------
void wait_ms(int ms)
{
	Sleep(ms);
}
//----------------------------------------------------------------------------
void hidasp_delay_ms(int dly)
{
	// 10mS�ȏ�҂ꍇ�́A����.
	while(dly>10) {
		hidCommand(HIDASP_WAIT_MSEC,10,0,0);	// WAIT <n> mSec
		dly-=10;
	}

	// 10�̒[�������҂�. 0�̏ꍇ�� NOP�R�}���h�̂悤�ɂȂ�(����1mS�͑҂�)
	hidCommand(HIDASP_WAIT_MSEC,dly,0,0);	// WAIT <n> mSec
}


/*********************************************************************
 *********************************************************************





 *********************************************************************
 *********************************************************************
 */
/*********************************************************************

 *********************************************************************
 */
typedef	struct {
	uchar report_id;
	uchar cmd;
	uchar size;
	uchar wait;
	uchar data[61];
} bitBuf;

#define	REPORT_LENGTH3		65

static  int  HidLength1 = REPORT_LENGTH1;
/*********************************************************************
 *	������
 *********************************************************************
 */
int UsbInit(int verbose,int enable_bulk, char *serial)
{
	int type = 0;
//	verbose_mode = verbose;
	if(	hidasp_init(type,serial) & HIDASP_MODE_ERROR) {
		if (verbose) {
	    	fprintf(stderr, "error: [%s] device not found!\n", serial);
    	}
    	Sleep(1000);
    	return 0;
	} else {
//		local_init();
//		UsbCheckPollCmd();
		return 1;	// OK.
	}
}
/*********************************************************************
 *	�I��
 *********************************************************************
 */
int UsbExit(void)
{
	hidasp_close();
	return 0;
}


/*********************************************************************
 *	AVR�f�o�C�X�ɏ����R�}���h�𑗂��āA�K�v�Ȃ猋�ʂ��󂯎��.
 *********************************************************************
 *	cmdBuf   *cmd : �����R�}���h
 *	uchar    *buf : ���ʂ��󂯎��̈�.
 *  int reply_len : ���ʂ̕K�v�T�C�Y. (0�̏ꍇ�͌��ʂ��󂯎��Ȃ�)
 *�߂�l
 *		0	: ���s
 *	   !0   : ����
 */
int QueryAVR(cmdBuf *cmd,uchar *buf,int size,int reply_len)
{
	int rc = 0;
	int report_id = cmd->report_id;
	char *s;

	rc = hidWriteBuffer((char*) cmd , HidLength1 );
	if(rc == 0) {
		printf("hidWrite error\n");
		UsbExit();
		exit(1);
		return 0;
	}

	if(reply_len) {
		rc = hidReadBuffer((char *)cmd , reply_len , 0);
		if(rc == 0) {
//			if(report_id != 4) 
			{
				printf("hidRead error %d\n",report_id);
				UsbExit();
				exit(1);
				return 0;
			}
		}
		s = (char*) cmd;
		memcpy(buf, s+1 ,reply_len);
	}
    return rc;
}


/*********************************************************************
 *	JTAG�R�}���h���s
 *********************************************************************
 */
static	uchar j_buf[128];
static	int	  j_index=0;
static	int	  j_anchor=0;

//#define	MAX_JCMD_SIZE	28
#define	MAX_JCMD_SIZE	(REPORT_LENGTH3-2)	//
/*********************************************************************
 *	JTAG�R�}���h���s
 *********************************************************************
 */
int jcmd_write_nb(uchar *stream,int size,uchar *result)
{
    cmdBuf cmd;
	char *t;
	int  rsize=0;
	int  rc=0;
	if(result!=NULL) rsize = size;

#if	JTAG_DUMP
	{int i;
	printf("JTAG:");
	for(i=0;i<size;i++) {
		printf(" %02x",stream[i]);
	}
	printf("\n");
	}
#endif

	t = (char*)&cmd;
	memset(t,0,sizeof(cmdBuf));
	if(result==NULL) {
		cmd.cmd   = HIDASP_JTAG_WRITE;
	}else{
		cmd.cmd   = HIDASP_JTAG_READ;
	}
	memcpy(t+2,stream,size);

	rc = QueryAVR(&cmd,result,2+size,rsize);

#if	JTAG_DUMP
	if(result!=NULL) {int i;
	printf("RET :");
	for(i=0;i<rsize;i++) {
		printf(" %02x",result[i]);
	}
	printf("\n");
	}
#endif

	return rc;
}

/*********************************************************************
 *	USB���֑���JTAG�R�}���h���L���[�Ɏc���Ă�����S�����M����.
 *********************************************************************
 */
int jcmd_write_flash()
{
	if(	j_index ) {
		jcmd_write_nb(j_buf,j_index,NULL);
		j_index=0;
	}
	return 0;
}

/*********************************************************************
 *	JTAG�R�}���h��A�����鏈��.
 *********************************************************************
 */
int jcmd_add(uchar *stream,int size)
{
	int c1,c2;
	// ����A��: �����f�[�^����̏ꍇ.
	if(	j_index == 0) {
		j_anchor = 0;				//���O�̃R���g���[�����[�h�̏ꏊ.
		memcpy(j_buf,stream,size);	//�ۃR�s�[����.
		j_index = size;
		return size;
	}
	// ����łȂ��A��: �����f�[�^�����݂���ꍇ.
	c1 = j_buf[j_anchor];
	c2 = stream[0];
	if((c1 & 0x80) && (c2 & 0x80)) {	//�����Ƃ�bitbang�X�g���[���̏ꍇ�͂P�̃X�g���[���ɂ��Ă��܂�.
		memcpy(j_buf+j_index-1,stream+1,size-1);	// �����f�[�^��NUL�^�[�~�l�[�g�ʒu��bitbang���R�s�[.
		j_buf[j_anchor] = 0x80 | ((c1&0x7f)+(c2&0x7f));	//��������������.
		j_index += size-2;							// �ڑ�������NUL������bitbang����R�[�h���P�Â���̂�-2
		// anchor�̈ʒu�͕ς��Ȃ�.
		return j_index;
	}
	
	// ����łȂ��A��: bitbang���m�Ŗ����ꍇ.
	{
		j_anchor = j_index-1;
		memcpy(j_buf+j_anchor,stream,size);		// �����f�[�^��NUL�^�[�~�l�[�g�ʒu�ɃR�s�[.
		j_index += size-1;						// �ڑ�������NUL�������P����̂�-1
		return j_index;
	}
}

/*********************************************************************
 *	USB���֑���JTAG�R�}���h���L���[�C���O����.
 *********************************************************************
 */
int jcmd_write(uchar *stream,int size,uchar *result)
{
	//�ǂݏ����𔺂��ꍇ�̓o�b�t�@�s��.
	if(result) {
		jcmd_write_flash();
		return jcmd_write_nb(stream,size,result);
	}

	// �A����̃T�C�Y��MAX_JCMD_SIZE���z����̂��������Ă���ꍇ��
	//   ��������f���o��.
	if( (j_index+size)>=MAX_JCMD_SIZE ) {
		jcmd_write_flash();
	}

	// �A�������s����.
	jcmd_add(stream,size);
	return j_index;
}


/*********************************************************************
 *	USB�^�[�Q�b�g�ɑ΂��đ��M�����APIC�p��BitBang���߂��_���v����.
 *********************************************************************
 */
void dumpStream(uchar *p,int cnt,int wait)
{
	int i,c,m=0x80,b,t;
	char tbl[8]="rmdcRMDC";
	printf("stream: cnt=%d wait=%d\n",cnt,wait);
	printf("  ");
		for(i=0;i<cnt;i++) {
			c = i % 10;t = '0'+c;
			putchar(t);
		}
	printf("\n");

	for(b=0;b<8;b++) {
		printf("%c:",tbl[b]);
		for(i=0;i<cnt;i++) {
			c = p[i];
			if(c&m) t='1';else t='0';
			putchar(t);
		}
		printf("\n");
		m>>=1;
	}
}

//  BitBang�f�[�^�r�b�g.

#define	bitMCLR	0x08
#define	bitPGM	0x04
#define	bitPGD	0x02
#define	bitPGC	0x01

//	�������W�X�^ (0=�o��)

#define	dirMCLR	0x80
#define	dirPGM	0x40
#define	dirPGD	0x20
#define	dirPGC	0x10

/*********************************************************************
 *	USB�^�[�Q�b�g�ɑ΂��āAPIC�p��BitBang���߂𑗐M����.
 *********************************************************************
 */
void BitBang(uchar *stream,int cnt,int wait,uchar *result)
{
	bitBuf cmd;
	int	reply_len = 0;
	memset(&cmd,0,sizeof(bitBuf));
	memcpy(&cmd.data,stream,cnt);

	if(result) {
		cmd.cmd   = PICSPX_BITREAD ;
		reply_len = cnt;
	}else{
		cmd.cmd   = PICSPX_BITBANG ;
	}
	cmd.size= cnt;
	cmd.wait= wait;

#if	DEBUG_STREAM			// 1:�f�[�^����f�o�b�O����.
	dumpStream(cmd.data,cmd.size,cmd.wait);
#endif

	if( QueryAVR((cmdBuf *)&cmd,result,HidLength1,reply_len) == 0) {
		//���s.
	}

}

#if	1
//
//	�ȉ��̏����́A��{�|�[�gI/O�ɂ��ᑬ.
//	  �܂��Ajcmd�o�b�t�@�ɖ������̃f�[�^�����݂��Ă��Ă͂����Ȃ�.
//		(PORT�l�ݒ�̏������t�ɂȂ�)
//

static	int	bb_data=dirPGD;	// TDO�̂ݓ��͂ɂ���.
/*********************************************************************
 *	TCK ,TDI ,TDO ,TMS �|�[�g�̓��̓f�[�^��ǂݎ��.
 *********************************************************************
  ATtiny2313        ARM��� JTAG�[�q
         PB7  ------------- TCK
         PB6  ------------- TDI
         PB5  ------------- TDO
         PB4  ------------- TMS
 */
int get_portb(void)
{
	uchar stream[128];
	uchar result[128];
	int c;
	jcmd_write_flash();
	
	stream[0]=bb_data;
	stream[1]=bb_data;
	
	BitBang(stream,2,0,result);
	c = result[1];
#if	POKE_DUMP			// PEEK/POKE COMMAND ���_���v.
	if(c & bitPGD) 	printf("+");
	else 			printf("_");
#endif
	if(c & bitPGD) return 0xff;

//	return peekmem(pinb,AREA_RAM);
	return 0;
}
/*********************************************************************
 *	TCK ,TDI ,TDO ,TMS �|�[�g�̏o�̓f�[�^���Z�b�g����.
 *********************************************************************
  ATtiny2313        ARM��� JTAG�[�q
         PB7  ------------- TCK
         PB6  ------------- TDI
         PB5  ------------- TDO
         PB4  ------------- TMS
 */
#define	JCMD_bitTMS	0x80	// TMS 
#define	JCMD_bitTDI	0x40	// TDI MOSI
#define	JCMD_bitTDO	0x20	// TDO MISO
#define	JCMD_bitTCK	0x10	// TCK

void set_portb(int d)
{
	uchar stream[128];

	jcmd_write_flash();

	bb_data = dirPGD;
	if(d & JCMD_bitTCK) bb_data |= bitPGC ;	// TCK
	if(d & JCMD_bitTDI) bb_data |= bitPGM ;	// TDI
	if(d & JCMD_bitTMS) bb_data |= bitMCLR;	// TMS
	
	stream[0]=bb_data;
	
	BitBang(stream,1,0,NULL);

#if	POKE_DUMP			// PEEK/POKE COMMAND ���_���v.
	printf("poke(%02x)");
#endif
}
/*********************************************************************
 *	TCK ,TDI ,TDO ,TMS �|�[�g�̓��o�͕��������߂�.
 *********************************************************************
  ATtiny2313        ARM��� JTAG�[�q
         PB7  ------------- TCK
         PB6  ------------- TDI
         PB5  ------------- TDO
         PB4  ------------- TMS
 */
void set_ddrb(int d)
{
	uchar stream[128];
	bb_data = dirPGD;

	jcmd_write_flash();

	stream[0]=bb_data;

	BitBang(stream,1,0,NULL);
}
#endif
/*********************************************************************
 *	
 *********************************************************************
 */