/*********************************************************************
 *	USB�o�R�̃^�[�Q�b�g�������[�A�N�Z�X
 *********************************************************************
 *API:
int		UsbInit(int verbose,int enable_bulk, char *serial);			������.
int		UsbExit(void);												�I��.
void 	UsbBench(int cnt,int psize);					�x���`�}�[�N

void 	UsbPoke(int adr,int arena,int data,int mask);	��������
int 	UsbPeek(int adr,int arena);						1�o�C�g�ǂݏo��
int 	UsbRead(int adr,int arena,uchar *buf,int size);	�A���ǂݏo��
void	UsbDump(int adr,int arena,int size);			�������[�_���v

void 	UsbFlash(int adr,int arena,int data,int mask);	�A����������
 *
 *�����֐�:
void	dumpmem(int adr,int cnt);
void	pokemem(int adr,int data0,int data1);
void	memdump_print(void *ptr,int len,int off);
 */

#ifndef	_LINUX_
#include <windows.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <time.h>

#include "hidasp.h"
#include "monit.h"
#include "portlist.h"
#include "util.h"
#include "../../firmware/hidcmd.h"

#define	VERBOSE	0

#define	if_V	if(VERBOSE)

static	char verbose_mode = 0;
static  int  HidLength1 = REPORT_LENGTH1;
//static  int  HidLength2 = REPORT_LENGTH2;
//static  int  HidLength3 = REPORT_LENGTH3;
#define	ID1			REPORT_ID1
#define	ID4			REPORT_ID4
#define	LENGTH4		REPORT_LENGTH4

// ReportID:4  POLLING PORT�������ςł���.
static	int	pollcmd_implemented=1;

//	�ی�|�[�g
#define	PROTECTED_PORT_MAX	3
static	int	protected_ports[PROTECTED_PORT_MAX]={-1,-1,-1};
#define	PROTECTED_PORT_MASK	0x38	// 0011_1000

#define	READ_BENCH			1

int disasm_print(unsigned char *buf,int size,int adr);

//	exit()
#define	EXIT_SUCCESS		0
#define	EXIT_FAILURE		1

/****************************************************************************
 *	�������[���e���_���v.
 ****************************************************************************
 *	void *ptr : �_���v�������f�[�^.
 *	int   len : �_���v�������o�C�g��.
 *	int   off : �^�[�Q�b�g���̔Ԓn�\��.
 */
void memdump_print(void *ptr,int len,int off)
{
	unsigned char *p = (unsigned char *)ptr;
	int i,j,c;

	for(i=0;i<len;i++) {
		if( (i & 15) == 0 ) printf("%06lx", ((long)p - (long)ptr + off) & 0xffffff);
		printf(" %02x",*p);p++;
		if( (i & 15) == (len-1) )
		{
#if	1	// ASCII DUMP
			printf("  ");
			for(j=0;j<len;j++) {
				c=p[j-len];
				if(c<' ') c='.';
				if(c>=0x7f) c='.';
				printf("%c",c);
			}
#endif
		}
	}
	printf("\n");
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
static int queryUsb(cmdBuf *cmd)
{
	int rc = 0;
	int size = cmd->size & SIZE_MASK;
	if( size == 0 ) size = SIZE_MASK+1;

	rc = hidWriteBuffer((char*) cmd , HidLength1 );
	if(rc == 0) {
		printf("hidWrite error\n");
		exit(EXIT_FAILURE);
	}
	return rc;
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
int QueryAVR(cmdBuf *cmd,uchar *buf,int reply_len)
{
	int rc = 0;
	int report_id = cmd->report_id;
//	int size = cmd->size;
	char *s;
	rc = hidWriteBuffer((char*) cmd , HidLength1 );
	if(rc == 0) {
		printf("hidWrite error\n");
		exit(1);
	}
	if(reply_len) {
		rc = hidReadBuffer((char *)cmd , reply_len , 0);
		if(rc == 0) {
			if(report_id != 4) {
				printf("hidRead error %d\n",report_id);
				exit(1);
			}
		}
		s = (char*) cmd;
		memcpy(buf, s+1 ,reply_len);
	}
    return rc;
}

/*********************************************************************
 *	�^�[�Q�b�g���̃������[���e���擾����
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int           size :�ǂݎ��T�C�Y.
 *	unsigned char *buf :�󂯎��o�b�t�@.
 *	����: HID Report Length - 1 ��蒷���T�C�Y�͗v�����Ă͂Ȃ�Ȃ�.
 */
int dumpmem(int adr,int arena,int size,unsigned char *buf)
{
	char reply[256];
	cmdBuf cmd;
	int rc;

	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = CMD_PEEK;
	cmd.size  = (size & SIZE_MASK) | (arena & AREA_MASK);
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	if( queryUsb(&cmd) == 0) return 0;	//���s.

	rc = hidReadBuffer(reply , HidLength1 , ID1);
	if(rc == 0) {
		printf("hidRead error \n");
		exit(EXIT_FAILURE);
	}

	//�ǂݎ�����������[�u���b�N�����݂̂��R�s�[�ŕԂ�.
	memcpy(buf, &reply[1] ,size);

	return size;
}

/*********************************************************************
 *
 *********************************************************************
 */
int UsbBootTarget(int adr,int boot)
{
	cmdBuf cmd;

	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = HIDASP_JMP;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	cmd.data[1]= boot;		// 1�Ȃ�USB�o�X�����Z�b�g����.

#if	0
	if(boot) {
		cmd.cmd   = HIDASP_BOOT_EXIT;
	}else{
		cmd.cmd   = HIDASP_JMP;
		cmd.adr[0]= adr;
		cmd.adr[1]= adr>>8;
//		printf("HIDASP_JMP %x\n",adr);
	}
#endif

	if( queryUsb(&cmd) == 0) return 0;	//���s.
	return 1;
}

/*********************************************************************
 *
 *********************************************************************
 */
int UsbExecUser(int arg)
{
#if	1
	cmdBuf cmd;

	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = HIDASP_USER_CMD;
	cmd.adr[0]= arg;
	cmd.adr[1]= arg>>8;

	if( queryUsb(&cmd) == 0) return 0;	//���s.
#endif
	return 1;
}

/*********************************************************************
 *
 *********************************************************************
 */
int UsbReadString(char *buf)
{
	cmdBuf cmd;
	int rc;

	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = HIDASP_GET_STRING;

	buf[0]=0;
	if( queryUsb(&cmd) == 0) return 0;	//���s.


	rc = hidReadBuffer(buf , HidLength1 , ID1);
	if(rc == 0) {
		printf("hidRead error \n");
		exit(EXIT_FAILURE);
	}

	return 1;
}

/*********************************************************************
 *
 *********************************************************************
 */
int UsbPutKeyInput(int key)
{
	cmdBuf cmd;

	memset(&cmd,0,sizeof(cmdBuf));
		cmd.cmd   = HIDASP_KEYINPUT;
		cmd.size  = key;

	if( queryUsb(&cmd) == 0) return 0;	//���s.
	return 1;
}

/*********************************************************************
 *	�^�[�Q�b�g���̃������[���e(1�o�C�g)������������.
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int          data0 :�������݃f�[�^.      (OR)
 *	int          data1 :�������݃r�b�g�}�X�N.(AND)
 *����:
 *	�t�@�[�����̏������݃A���S���Y���͈ȉ��̂悤�ɂȂ��Ă���̂Œ���.

	if(data1) {	//�}�X�N�t�̏�������.
		*adr = (*adr & data1) | data0;
	}else{			//�ׂ���������.
		*adr = data0;
	}

 */
int	pokemem(int adr,int arena,int data0,int data1)
{
    cmdBuf cmd;

	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = CMD_POKE ;	//| arena;
	cmd.size  = 1 | arena;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	cmd.data[0] = data0;
	cmd.data[1] = data1;
	return queryUsb(&cmd);
}

void hid_read_mode(int mode,int adr)
{
	cmdBuf cmd;

	memset(&cmd,0,sizeof(cmdBuf));
	cmd.cmd   = HIDASP_SET_MODE;
	cmd.size  = mode;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;
	queryUsb(&cmd);

}

int hid_read(int cnt)
{
	char buf[128];	// dummy
	int rc;
	rc = hidReadBuffer(buf , 65 , ID1);
	if(rc == 0) {
		printf("hidRead error \n");
		exit(EXIT_FAILURE);
	}
	return rc;
}

/*********************************************************************
 *	�^�[�Q�b�g�Ƃ̐ڑ��`�F�b�N(�x���`�}�[�N�����˂�)
 *********************************************************************
 *	int i : 0�`255 �̐��l.
 *�߂�l
 *		�G�R�[�o�b�N���ꂽ i �̒l.
 */
int hid_ping(int i)
{
 	cmdBuf cmd;
	char buf[128];	// dummy
	int rc;

	memset(&cmd,i,sizeof(cmdBuf));
	cmd.cmd   = CMD_PING ;
	queryUsb(&cmd);

	rc = hidReadBuffer(buf , HidLength1, ID1 );
	if(rc == 0) {
		printf("hidRead error\n");
		exit(EXIT_FAILURE);
	}
	// +00        +01      +02     +03     +04     +05
	// [ReportID] [CMD]  [DEV_ID] [VER_L] [VER_H] [Echoback]
//	return buf[2] & 0xff;
	return buf[5] & 0xff;
}
/*********************************************************************
 *	�]�����x�x���`�}�[�N
 *********************************************************************
 *	int cnt  : PING��ł�.
 *	int psize: PING�p�P�b�g�̑傫��(���݂̓T�|�[�g����Ȃ�)
 */
void UsbBench(int cnt,int psize)
{
	int i,n,rc;
	int time1, rate;
#ifdef __GNUC__
	long long total=0;
#else
	long total=0;
#endif
	int nBytes;
	int time0;

#if		READ_BENCH
	nBytes  = HidLength1-1 ;				//�o��p�P�b�g�̂݌v��.
#else
	nBytes  = HidLength1-1 + HidLength1-1 ;	//���݌Œ�.
#endif
	// 1���Ping�� 63byte��Query��63�o�C�g�̃��^�[��.

   	printf("hid write start\n");
	time0 = clock();
#if		READ_BENCH
	hid_read_mode(1,0);
#endif

	for(i=0;i<cnt;i++) {
		n = i & 0xff;
#if		READ_BENCH
		rc = hid_read(i);
#else
		rc = hid_ping(n);
		if(rc != n) {
			printf("hid ping mismatch error (%x != %x)\n",rc,n);
			exit(EXIT_FAILURE);
		}
#endif
		total += nBytes;
	}

#if		READ_BENCH
	hid_read_mode(0,0);
#endif

	time1 = clock() - time0;
	if (time1 == 0) {
		time1 = 2;
	}
	rate = (int)((total * 1000) / time1);

	if (total > 1024*1024) {
	   	printf("hid write end, %8.3lf MB/%8.2lf s,  %6.3lf kB/s\n",
	   		(double)total/(1024*1024), (double)time1/1000, (double)rate/1024);
	} else 	if (total > 1024) {
	   	printf("hid write end, %8.3lf kB/%8.2lf s,  %6.3lf kB/s\n",
	   		 (double)total/1024, (double)time1/1000, (double)rate/1024);
	} else {
	   	printf("hid write end, %8d B/%8d ms,  %6.3lf kB/s\n",
	   		(int)total, time1, (double)rate/1024);
   	}
}
/*********************************************************************
 *	�^�[�Q�b�g���������[���_���v����.
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int            cnt :�ǂݎ��T�C�Y.
 */
void UsbDump(int adr,int arena,int cnt)
{
	unsigned char buf[16];
	int size;
	int rc;
	while(cnt) {
		size = cnt;
		if(size>=8) size = 8;
		rc = dumpmem(adr,arena,size,buf);	//�������[���e�̓ǂݏo��.

		if(rc !=size) return;
		memdump_print(buf,size,adr);		//���ʈ�.
		adr += size;
		cnt -= size;
	}
}

/*********************************************************************
 *	�^�[�Q�b�g���������[���t�A�Z���u������.
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int            cnt :�ǂݎ��T�C�Y.
 */
int UsbDisasm (int adr, int arena, int cnt)
{
	unsigned char buf[16];
	int size = 8;
	int done = 0;
	int endadr = adr + cnt;
	int rc,disbytes;
	while(adr < endadr) {
		rc = dumpmem(adr,AREA_PGMEM,size,buf);	//�������[���e�̓ǂݏo��.
		if(rc != size) return done;
		disbytes = disasm_print(buf,size,adr);	//���ʈ�.
		adr += disbytes;
		done+= disbytes;
	}
	return done;
}

/*********************************************************************
 *	�^�[�Q�b�g���������[�A���ǂݏo��
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	uchar         *buf :�ǂݎ��o�b�t�@.
 *	int           size :�ǂݎ��T�C�Y.
 *�߂�l
 *	-1    : ���s
 *	���̒l: ����.
 */
int UsbRead(int adr,int arena,uchar *buf,int size)
{
	int rc = size;
	int len;
	while(size) {
		int rc1;
		len = size;
		if(len >= 8) len = 8;
		rc1 = dumpmem(adr,arena,len,buf);
		if( rc1!= len) {
			return -1;
		}
		adr  += len;	// �^�[�Q�b�g�A�h���X��i�߂�.
		buf  += len;	// �ǂݍ��ݐ�o�b�t�@��i�߂�.
		size -= len; 	// �c��T�C�Y�����炷.
	}
	return rc;
}
/*********************************************************************
 *	�^�[�Q�b�g���������[1�o�C�g�ǂݏo��
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *�߂�l
 *	�������[���e�̒l.
 */
int UsbPeek(int adr,int arena)
{
	unsigned char buf[16];
	int size=1;
	int rc = UsbRead(adr,arena,buf,size);
	if( rc != size) {
		return -1;
	}
	return buf[0];
}

static	int	poll_port = 0;

/*********************************************************************
 *	�^�[�Q�b�g���������[1�o�C�g�A���ǂݏo���̃Z�b�g�A�b�v
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *�߂�l
 *	�������[���e�̒l.
 */
int UsbSetPoll_slow(int adr,int arena)
{
	poll_port = adr;
	return 1;
}
/*********************************************************************
 *	�^�[�Q�b�g���������[1�o�C�g�A���ǂݏo��
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *�߂�l
 *	�������[���e�̒l.
 */
int UsbPoll_slow()
{
	return UsbPeek(poll_port,0);
}

static	int	chkSyncCmd(uchar *buf,int i)
{
	if(buf[1]==CMD_PING) {
		if((buf[2]==0x14)||(buf[2]==0x25)) {
		// +00        +01      +02     +03     +04     +05       +06
		// [ReportID] [CMD]  [DEV_ID] [VER_L] [VER_H] [DEVCAPS] [Echoback]
			if(buf[6]==i) return 1;
		}
	}
	return 0;	// error
}

int	UsbSyncCmd(int cnt)
{
 	cmdBuf cmd;
	uchar buf[128];	// dummy
	int rc;
	int i,c,fail,retry;

	for(i=0x80;i<0x80+cnt;i++) {
		c = i & 0xff;
		memset(&cmd,c,sizeof(cmdBuf));
		cmd.cmd   = CMD_PING ;
		queryUsb(&cmd);

		fail=0;
		for(retry=0;retry<16;retry++) {
			rc = hidReadBuffer((char*)buf , HidLength1, ID1 );
			if(rc == 0) {
				printf("hidRead error\n");
				exit(EXIT_FAILURE);
				return -1;
			}
			if( chkSyncCmd(buf,c) ) {
				fail=0;				// OK.
				break;
			}else{
				fail=1;				// Retry!
			}
		}
		if(fail) {
			printf("hidRead Sync Error\n");
			exit(EXIT_FAILURE);
			return -1;
		}
	}
if_V	printf("hidRead Sync OK.\n");
	return 0;
}
/*********************************************************************
 *	�^�[�Q�b�g���������[1�o�C�g�A���ǂݏo���̃Z�b�g�A�b�v
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *�߂�l
 *	�������[���e�̒l.
 */
int UsbSetPoll(int adr,int mode)
{
//	char buf[128];
//	char rd_buf[128];

	if( pollcmd_implemented == 0 ) {
		return UsbSetPoll_slow(adr,0);
	}

	hid_read_mode(mode,adr);

//	SYNC����.
	if(mode==0) {
		UsbSyncCmd(1);
	}
	return 1;	// OK.
}
/*********************************************************************
 *	�^�[�Q�b�g���������[1�o�C�g�A���ǂݏo��
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *�߂�l
 *	�������[���e�̒l.
 */
int UsbPoll(char *buf)
{
	int rc;

	if( pollcmd_implemented == 0 ) {
		rc = UsbPoll_slow();
		buf[0]=0;		// poll_mode
		buf[1]=0xc0;	// poll_mode
		buf[2]=1;		// samples
		buf[3]=rc;
		return rc;
	}

	rc = hidReadPoll(buf , LENGTH4 ,ID4);
	if(rc == 0) {
#if	0
		printf("hidRead error\n");
		exit(EXIT_FAILURE);
#else
		return -1;
#endif
	}
	return buf[3] & 0xff;	// [ReportID] [ 0xaa ] [ poll_mode ] [ DATA ]
}


int UsbAnalogPoll(int mode,unsigned char *abuf)
{
	int rc;
	hidCommand(HIDASP_SET_PAGE,mode,0,0);	// Set Page mode
	if(mode == 0xa2) {
		Sleep(3);
	}
	rc = hidReadPoll((char*)abuf , LENGTH4 ,ID4);
//	Sleep(100);
	return rc;
}


/*********************************************************************
 *
 *********************************************************************
 */
int 	UsbEraseTargetROM(int adr,int size)
{
	cmdBuf cmd;
//	printf("adrs = %x size=%x\n",adr,size);

	cmd.cmd   = HIDASP_PAGE_ERASE;
	cmd.size  = size;
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;

	if( queryUsb(&cmd) == 0) return 0;	//���s.
	return size;
}

/*********************************************************************
 *	�^�[�Q�b�g����Flash���e(32�o�C�g�܂�)������������.
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int           data :�������݃f�[�^.
 *	int           mask :�������ݗL���r�b�g�}�X�N.
 *����
 *  mask  = 0 �̏ꍇ�͑S�r�b�g�L�� (����������)
 *	mask != 0 �̏ꍇ�́Amask�r�b�g1�ɑΉ�����data�̂ݍX�V���A���͕ύX���Ȃ�.
 */
int UsbFlash(int adr,int arena,uchar *buf,int size)
{
	cmdBuf cmd;

	memcpy(cmd.data,buf,size);

	cmd.cmd   = HIDASP_PAGE_WRITE;
	cmd.size  =(size & SIZE_MASK) | (arena & AREA_MASK);
	cmd.adr[0]= adr;
	cmd.adr[1]= adr>>8;

	if( queryUsb(&cmd) == 0) return 0;	//���s.
	return size;
}

/*********************************************************************
 *	�^�[�Q�b�g���̃������[���e(1�o�C�g)������������.
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int           data :�������݃f�[�^.
 *	int           mask :�������ݗL���r�b�g�}�X�N.
 *����
 *  mask  = 0 �̏ꍇ�͑S�r�b�g�L�� (����������)
 *	mask != 0 �̏ꍇ�́Amask�r�b�g1�ɑΉ�����data�̂ݍX�V���A���͕ύX���Ȃ�.
 */
int	usbPoke(int adr,int arena,int data,int mask)
{
	int data0 ,data1;
	if(mask == 0) {
		// �}�X�N�s�v�̒�����.
		data0 = data;
		data1 = 0;
	}else{
		// �}�X�N�������܂ޏ�������.
		// �������݃f�[�^�̗L�������� mask �̃r�b�g�� 1 �ɂȂ��Ă��镔���̂�.
		// mask �̃r�b�g�� 0 �ł��镔���́A���̏���ێ�����.

		data0 = data & mask;	// OR���.
		data1 = 0xff &(~mask);	// AND���.

		// �}�X�N�������݂̃��W�b�N�͈ȉ�.
		// *mem = (*mem & data1) | data0;
	}

	return pokemem(adr,arena,data0,data1);
}
/*********************************************************************
 *	�^�[�Q�b�g���̃������[���e(1�o�C�g)������������.
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int           data :�������݃f�[�^.
 *	int           mask :�������ݗL���r�b�g�}�X�N.
 *����
 *  mask  = 0 �̏ꍇ�͑S�r�b�g�L�� (����������)
 *	mask != 0 �̏ꍇ�́Amask�r�b�g1�ɑΉ�����data�̂ݍX�V���A���͕ύX���Ȃ�.
 */
int UsbPoke(int adr,int arena,int data,int mask)
{
#if	_AVR_PORT_
	int i;
	int f=0;

	for(i=0;i<PROTECTED_PORT_MAX;i++) {
		if(adr == protected_ports[i]) f=1;
	}
	if(f==0) return usbPoke(adr,arena,data,mask);

	if(mask==0) mask = 0xff;
	mask &= ~PROTECTED_PORT_MASK;	// USB D+,D-,PULLUP �r�b�g�̏����������}�X�N.
	data &= ~PROTECTED_PORT_MASK;

	if(adr == protected_ports[0]) { mask = 0;}	// pind ��mask write���Ă͂����Ȃ�.
#endif

	return usbPoke(adr,arena,data,mask);
}
/*********************************************************************
 *	1�r�b�g�̏�������
 *********************************************************************
 *	int            adr :�^�[�Q�b�g���̃������[�A�h���X
 *	int          arena :�^�[�Q�b�g���̃������[���(���ݖ��g�p)
 *	int            bit :�������݃f�[�^(1bit) 0 �������� 1
 *	int           mask :�������ݗL���r�b�g�}�X�N.
 */
void UsbPoke_b(int adr,int arena,int bit,int mask)
{
	int data=0;
	if(mask == 0) {
		//1�o�C�g�̏�������.
		UsbPoke(adr,arena,bit,0);
	}else{
		//1�r�b�g�̏�������.
		if(bit) data = 0xff;
		UsbPoke(adr,arena,data,mask);
	}
}

/*********************************************************************
 *
 *********************************************************************
 */
void local_init(void)
{
	protected_ports[0]=portAddress("pind");
	protected_ports[1]=portAddress("portd");
	protected_ports[2]=portAddress("ddrd");
}
/*********************************************************************
 *
 *********************************************************************
 */
void UsbCheckPollCmd(void)
{
#if	_AVR_PORT_
	pollcmd_implemented = 1;			// �Ƃ肠������������Ă���Ɖ���.
	{
		int gtccr = portAddress("gtccr");	// LSB�̂�1��0�̃|�[�g.
		int data;
		int rc = UsbSetPoll(gtccr,0);
		if(rc==0) {					// ���s.
			pollcmd_implemented = 0;		// ��������Ă��Ȃ��Ɣ���.
			return ;
		}

		data = UsbPoll();
		if((data == (-1)) || (data & 0xfe)) {
			pollcmd_implemented = 0;		// ��������Ă��Ȃ��Ɣ���.
		}
	}
#else
	if((UsbGetDevCaps(0,0)) == 1) {	// bootloader�ł���.
		pollcmd_implemented = 0;			// ��������Ă��Ȃ�.
	}else{
		pollcmd_implemented = 1;			// ��������Ă���.
	}
#endif
}

/*********************************************************************
 *	������
 *********************************************************************
 */
int UsbInit(int verbose,int enable_bulk, char *serial)
{
	int type = 0;
	verbose_mode = verbose;
	if(	hidasp_init(type,serial) & HIDASP_MODE_ERROR) {
		if (verbose) {
	    	fprintf(stderr, "error: [%s] device not found!\n", serial);
    	}
    	Sleep(1000);
    	return 0;
	} else {
		local_init();
		UsbCheckPollCmd();
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


#ifdef	_LINUX_

#define BUFF_SIZE		256
int hidWrite(char *buf, int Length, int id);
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
	return hidWrite( (char*)buf, REPORT_LENGTH1, REPORT_ID1);
}

int hidPeekMem(int addr)
{
	unsigned char buf[BUFF_SIZE];
	memset(buf , 0, sizeof(buf) );

	buf[1] = HIDASP_PEEK;
	buf[2] = 1;
	buf[3] = addr;
	buf[4] =(addr>>8);

	hidWrite( (char*)buf, REPORT_LENGTH1, REPORT_ID1);
	hidReadBuffer( (char*)buf, REPORT_LENGTH1,0 );
	return buf[1];
}

#endif
