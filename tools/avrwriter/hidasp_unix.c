//----------------------------------------------------------------------------
//	Linux�ł͖�����.
//----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
//--------------------------------------
#include <usb.h>

#include "../firmware/hidcmd.h"
#include "hidasp.h"
#include "monit.h"


//----------------------------------------------------------------------------
//	Switches
//----------------------------------------------------------------------------

#define DEBUG 		   	0		// for DEBUG

#define DEBUG_PKTDUMP  	0		// HID Report�p�P�b�g���_���v����.
#define DUMP_PRODUCT   	0		// ������,���i����\��.

#define CHECK_COUNT		4		// 4: Connect���� Ping test �̉�.
#define BUFF_SIZE		256


#define	PAGE_WRITE_LENGTH	32

#define	ASYNC_RW		1
#define	ASYNC_DEBUG		1

#define	TIMEOUT_MS 		4000


//----------------------------------------------------------------------------
//	Defines
//----------------------------------------------------------------------------

#if	0
#ifdef	LPC2388
static int MY_VID = 0x0483;	// MicroChip
static int MY_PID = 0x5750;	// HIDbootloader
#define	MY_Product		"ARM32spx"
#else

#ifdef	LPC1343
static int MY_VID = 0x1fc9;	// NXP
static int MY_PID = 0x0003;	// HID
#define	MY_Product		"ARM32spx"
#else
static int MY_VID = 0x0483;	// MicroChip
static int MY_PID = 0x5750;	// HIDbootloader
#define	MY_Product		"ARM32spx"
#endif

#endif
#endif


//----------------------------------------------------------------------------
//	Defines
//----------------------------------------------------------------------------
static int MY_VID = 0x04D8;	// MicroChip
static int MY_PID = 0x003C;	// HIDbootloader

//#define	MY_Manufacturer	"YCIT"
#define	MY_Manufacturer	"AVRetc"
#define	MY_Product		"PIC18spx"

//	MY_Manufacturer,MY_Product ��define ���O���ƁAVID,PID�݂̂̏ƍ��ɂȂ�.
//	�ǂ��炩���͂����ƁA���̏ƍ����ȗ�����悤�ɂȂ�.
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//typedef	cmdBuf RxBuf;

static	int dev_id       = 0;	// �^�[�Q�b�gID: 0x25 �������� 0x14 ���������e.
static	int dev_version  = 0;	// �^�[�Q�b�g�o�[�W���� hh.ll
static	int dev_bootloader= 0;	// �^�[�Q�b�g��bootloader�@�\������.

static	int have_isp_cmd = 0;	// ISP����̗L��.
static	int	flash_size     = 0;	// FLASH_ROM�̍ŏI�A�h���X+1
static	int	flash_end_addr = 0;	// FLASH_ROM�̎g�p�ςݍŏI�A�h���X(4�̔{���؎�)
static	int	hidmon_mode =  1;
#define	REPORT_ID1			0
#define	REPORT_LENGTH1		65

void dump_info(  struct usb_device *dev );


//#define	MY_Manufacturer	"A"
//#define	MY_Product		"M"


/* the device's endpoints */
#define EP_IN  				0x81
#define EP_OUT 				0x01
#define PACKET_SIZE 		64
#define	VERBOSE				1
#define	REPORT_MATCH_DEVICE	0
#define	REPORT_ALL_DEVICES	0

#define	USE_BULK_TRANSFER	1		//1:�o���N�]���g�p���f�t�H���g�ɂ���.



#define	if_V	if(VERBOSE)

static  usb_dev_handle *usb_dev = NULL; /* the device handle */
static	char verbose_mode = 0;
#if	0
/****************************************************************************
 *	�������[���e���_���v.
 ****************************************************************************
 */
void memdump(void *ptr,int len,int off)
{
	unsigned char *p = (unsigned char *)ptr;
	int i,j,c;

	for(i=0; i<len; i++) {
		if( (i & 15) == 0 ) printf("%06x",(int)p - (int)ptr + off);
		printf(" %02x",*p);
		p++;
		if( (i & 15) == 15 ) {
#if	1	// ASCII DUMP
			printf("  ");
			for(j=0; j<16; j++) {
				c=p[j-16];
				if(c<' ') c='.';
				if(c>=0x7f) c='.';
				printf("%c",c);
			}
#endif
			printf("\n");
		}
	}
	printf("\n");
}
#endif
/*********************************************************************
 *
 *********************************************************************
 */
#if	0
int QueryAVR(usb_dev_handle *dev,RxBuf *cmd,uchar *buf)
{
	int nBytes  = 0;
	int nLength = 8;

	if((nBytes = usb_bulk_write(dev, EP_OUT, (void*)cmd, nLength, 5000) )
	        != nLength) {
		printf("error: bulk write failed\n");
		return 0;
	}
	if( (cmd->cmd & CMD_MASK) == CMD_PEEK ) {
		if((nBytes = usb_bulk_read(dev, EP_IN, (char*)buf , nLength, 5000) )
		        != nLength) {
			printf("error: bulk read failed\n");
			return 0;
		}
	}
	return 8;
}
#endif
static int  usbGetStringAscii(usb_dev_handle *dev, int index, int langid, char *buf, int buflen)
{
	char    buffer[256];
	int     rval, i;

	if((rval = usb_control_msg(dev, USB_ENDPOINT_IN, USB_REQ_GET_DESCRIPTOR, (USB_DT_STRING << 8) + index, langid, buffer, sizeof(buffer), 1000)) < 0)
		return rval;
	if(buffer[1] != USB_DT_STRING)
		return 0;
	if((unsigned char)buffer[0] < rval)
		rval = (unsigned char)buffer[0];
	rval /= 2;
	/* lossy conversion to ISO Latin1 */
	for(i=1; i<rval; i++) {
		if(i > buflen)  /* destination buffer overflow */
			break;
		buf[i-1] = buffer[2 * i];
		if(buffer[2 * i + 1] != 0)  /* outside of ISO Latin1 range */
			buf[i-1] = '?';
	}
	buf[i-1] = 0;
	return i-1;
}

int	open_dev_check_string(struct usb_device *dev,usb_dev_handle *handle)
{
	int len1,len2;
	char string1[256];
	char string2[256];

	len1 = usbGetStringAscii(handle, dev->descriptor.iManufacturer,
	                         0x0409, string1, sizeof(string1));
	len2 = usbGetStringAscii(handle, dev->descriptor.iProduct,
	                         0x0409, string2, sizeof(string2));
//	return 1;

	if((len1<0)||(len2<0)) {
		return 0;
	}
#if	1
	printf("iManufacturer:%s\n",string1);
	printf("iProduct:%s\n",string2);
#endif

#ifdef	MY_Manufacturer
	if(strcmp(string1, MY_Manufacturer) != 0) return 0;
#endif

#ifdef	MY_Product
	if(strcmp(string2, MY_Product) != 0)      return 0;
#endif

	return 1;	//���v����.
}


usb_dev_handle *open_dev(void)
{
	struct usb_bus *bus;
	struct usb_device *dev;
	usb_dev_handle *handle;
	int rc;

	for(bus = usb_get_busses(); bus; bus = bus->next) {
		for(dev = bus->devices; dev; dev = dev->next) {

#if	REPORT_ALL_DEVICES
			dump_info(dev);
#endif

			//
			//	VendorID,ProductID �̈�v�`�F�b�N.
			//
			if(dev->descriptor.idVendor  == MY_VID &&
			        dev->descriptor.idProduct == MY_PID) {
#if	REPORT_MATCH_DEVICE
				if(verbose_mode) {
					dump_info(dev);
				}
#endif

				handle = usb_open(dev);
				int cf=dev->config->bConfigurationValue;
				if((rc = usb_set_configuration(handle, cf))<0) {
					printf("usb_set_configuration error(1) cf=%d\n",cf);
					int fn=dev->config->interface->altsetting->bInterfaceNumber;
					if((rc = usb_detach_kernel_driver_np(handle,fn)) <0 ) {
						printf("usb_detach_kernel_driver_np(%d) error\n",fn);
						usb_close(handle);
						return NULL;
					}

					if((rc = usb_set_configuration(handle, dev->config->bConfigurationValue))<0) {
					printf("usb_set_configuration error(2)\n");
						usb_close(handle);
						return NULL;

					}
				}

				if(usb_claim_interface(handle, 0) < 0) {
					printf("error: claiming interface 0 failed\n");
					usb_close(usb_dev);
					return NULL;
				}


				if( open_dev_check_string(dev,handle) == 1) {
					return handle;	//��v.
				}
				usb_close(handle);
				handle = NULL;
			}
		}
	}
	return NULL;
}

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
static int hidRead(char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_interrupt_read(usb_dev, EP_IN , buf+1 , Length-1, 5000);
//	rc = HidD_GetFeature(h, buf, Length);
#if	DEBUG_PKTDUMP
	memdump("RD", buf, Length);
	printf("id=%d Length=%d rc=%d\n",id,Length,rc);
#endif
	return rc;
}

int	hidReadPoll(char *buf,int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_interrupt_read(usb_dev, EP_IN , buf+1 , Length, 5000);
//	rc = HidD_GetFeature(hHID, buf, Length);
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
//static 
int hidWrite(char *buf, int Length, int id)
{
	int rc;
	buf[0] = id;
	rc = usb_interrupt_write(usb_dev, EP_OUT , buf+1 , Length -1 , 5000);
//	rc = HidD_SetFeature(h, buf, Length);

#if	DEBUG_PKTDUMP
	memdump("WR", buf, Length);
#endif
	return rc;
}

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
	return hidWrite(buf, len , 0);
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
	return hidRead(buf, len, 0);
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

	return hidWrite((char*)buf, 65, 0);
}
/*********************************************************************
 *
 *********************************************************************
 */
int hidasp_init (int type, const char *serial)
{
	unsigned char rd_data[BUFF_SIZE];
	int *fl_size;
	int i, r, result;

	result = 0;

	verbose_mode = VERBOSE;

	usb_init(); /* initialize the library */

#if	0
	// USB�̃f�o�b�O������ꍇ�́A������L���ɂ���.
	usb_set_debug(3);
#endif

	usb_find_busses(); /* find all busses */
	usb_find_devices(); /* find all connected devices */

	if(!(usb_dev = open_dev())) {
		printf("error: device not found!\n");
		return 0;
	}
	printf("open device ok.\n");


	for (i = 0; i < CHECK_COUNT; i++) {
//		hidCommand(HIDASP_TEST,(i),0,0);	// Connection test
		hidCommand(HIDASP_TEST,(i),(i+1),(i+2));	// Connection test
		r = hidRead((char*)rd_data ,REPORT_LENGTH1, REPORT_ID1);

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
	r = hidRead((char*)rd_data ,REPORT_LENGTH1, REPORT_ID1);
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

	return 1;	// OK.
}



/*********************************************************************
 *
 *********************************************************************
 */

void hidasp_close(void)
{
	if(	usb_dev ) {
		usb_release_interface(usb_dev, 0);
		usb_close(usb_dev);
	}
}
/*********************************************************************
 *
 *********************************************************************
 */




//----------------------------------------------------------------------------
//  �y�[�W���[�h.
//----------------------------------------------------------------------------
int hidasp_page_read(long addr, unsigned char *wd, int pagesize)
{
	return 0;
}
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
void Sleep(int n)
{
	usleep(n * 1000);
//	printf("Sleep(%d)\n",n);
}
//----------------------------------------------------------------------------
void wait_ms(int ms)
{
	Sleep(ms);
}
//----------------------------------------------------------------------------
void hidasp_delay_ms(int dly)
{
	Sleep(dly);
}
int hidasp_list (char *string)
{
	(void) string;
	return 0;
}
//----------------------------------------------------------------------------
int gr_box(void)
{
	return 0;
}
int gr_boxfill(void)
{
	return 0;
}
int gr_break(void)
{
	return 0;
}
int gr_circle_arc(void)
{
	return 0;
}
int gr_close(void)
{
	return 0;
}
int gr_hline(void)
{
	return 0;
}
int gr_init(void)
{
	return 0;
}
int gr_line(void)
{
	return 0;
}
int gr_pset(void)
{
	return 0;
}
int gr_puts(void)
{
	return 0;
}
int gr_vline(void)
{
	return 0;
}
//----------------------------------------------------------------------------
int kbhit(void)
{
	return 0;
}
int getch(void)
{
	return 0;
}
//----------------------------------------------------------------------------
int	stricmp(char *s1,char *s2)
{
	return strcasecmp(s1,s2);
}
//----------------------------------------------------------------------------
void strupr(char *s)
{
	while(*s) {
		*s = toupper(*s);
		s++;
	}
}



