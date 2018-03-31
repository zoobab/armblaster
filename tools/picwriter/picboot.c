/*********************************************************************
 *	P I C b o o t
 *********************************************************************
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#ifndef	_LINUX_
#include <windows.h>
#endif


#define MAIN_DEF
#include "monit.h"
#include "hidasp.h"
#include "util.h"

#include "picdevice.h"
#include "picpgm.h"


#define DEFAULT_SERIAL	"*"

char HELP_USAGE[]={
	"* PICwrite Ver 0.2 (" __DATE__ ")\n"
	"Usage is\n"
	"  PICwrite.exe [Options] hexfile.hex\n"
	"Options\n"
	"  -p[:XXXX]   ...  Select serial number (default=" DEFAULT_SERIAL ").\n"
	"  -r          ...  Read Info.\n"
	"  -v          ...  Verify.\n"
	"  -e          ...  Erase.\n"
	"  -rp         ...  Read program area.\n"
	"  -rf         ...  Read fuse(config).\n"
	"  -wf         ...  Write fuse(config).\n"
	"  -d          ...  Dump ROM.\n"
	"  -nv         ...  No-Verify.\n"
};

#define	MAX_CMDBUF	4096

static	DeviceSpec  *picdev=NULL;
static	int   dev_id=0;

static	char lnbuf[1024];
static	char usb_serial[128]= DEFAULT_SERIAL ;	/* �g�p����HIDaspx�̃V���A���ԍ����i�[ */
static	char verbose_mode = 1;	//1:�f�o�C�X�����_���v.
		int  hidmon_mode = 1;	/* picmon �����s�� */
static	uchar 	databuf[256];
static	int		dataidx;
static	int		adr_u = 0;		// hexrec mode 4


#define	FLASH_START	0			//0x0800
#define	FLASH_SIZE	0x10000
#define	FLASH_STEP	32
#define	ERASE_STEP	256

#define	FUSE_SIZE	0x10
#define	FUSE_ADDR	0x300000
#define	IDLOC_SIZE	0x8
#define	IDLOC_ADDR	0x200000


static	uchar flash_buf[FLASH_SIZE];
static	int	  flash_start = FLASH_START;
static	int	  flash_end   = FLASH_SIZE;
static	int	  flash_end_for_read = 0x8000;
static	int	  flash_step  = FLASH_STEP;

static	uchar  config_buf[16];
static	uchar *config_value;
static	uchar *config_mask;

static	uchar fuse_buf[FUSE_SIZE];
static	uchar idloc_buf[IDLOC_SIZE];

#define	CACHE_SIZE	64
static	uchar cache_buf[CACHE_SIZE];
static	int   cache_addr=(-1);


static	char  opt_r  = 0;		//	'-r '
static	char  opt_rp = 0;		//	'-rp'
static	char  opt_re = 0;		//	'-re'
static	char  opt_rf = 0;		//	'-rf'
static	char  opt_d  = 0;		//	'-d'

static	char  opt_e  = 0;		//	erase
static	char  opt_v  = 0;		//	'-v'
static	char  opt_nv = 0;		//	'-nv'
static	char  opt_w  = 0;		//	'-w '
static	char  opt_wf = 0;		//	'-wf'

//	���[�U�[�A�v���P�[�V�����J�n�Ԓn.
static	int   opt_s  = 0x800;	//	'-s1000 �Ȃ�'

#define	CHECKSUM_CALC	(1)


#ifdef	_LINUX_
void	timeBeginPeriod(int n)	{}
void	timeEndPeriod(int n)	{}
#endif

/*********************************************************************
 *	�g�p�@��\��
 *********************************************************************
 */
void usage(void)
{
	fputs( HELP_USAGE, stdout );
}
//----------------------------------------------------------------------------
//  �������[�_���v.
static void memdump(char *msg, uchar *buf, int len)
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
/*********************************************************************
 *	��s���̓��[�e�B���e�B
 *********************************************************************
 */
static	int getln(char *buf,FILE *fp)
{
	int c;
	int l;
	l=0;
	while(1) {
		c=getc(fp);
		if(c==EOF)  {
			*buf = 0;
			return(EOF);/* EOF */
		}
		if(c==0x0d) continue;
		if(c==0x0a) {
			*buf = 0;	/* EOL */
			return(0);	/* OK  */
		}
		*buf++ = c;l++;
		if(l>=255) {
			*buf = 0;
			return(1);	/* Too long line */
		}
	}
}
/**********************************************************************
 *	INTEL HEX���R�[�h �`��01 ���o�͂���.
 **********************************************************************
 */
static	int	out_ihex01(unsigned char *data,int adrs,int cnt,FILE *ofp)
{
	unsigned char buf[1024];
	buf[0] = cnt;
	buf[1] = adrs >> 8;
	buf[2] = adrs & 0xff;
	buf[3] = 0;
	int length = cnt+5;
	int i;
	int sum=0;

	memcpy(buf+4,data,cnt);
	for(i=0;i<length-1;i++) {
		sum += buf[i];
	}
	sum = 256 - (sum&0xff);
	buf[length-1] = sum;

	fprintf(ofp,":");
	for(i=0;i<length;i++) {
		fprintf(ofp,"%02X",buf[i]);
	}
	fprintf(ofp,"\n");
	return 0;
}
/**********************************************************************
 *	INTEL HEX���R�[�h �`��01 (�o�C�i���[���ς�) �����߂���.
 **********************************************************************
 */
static	int	parse_ihex01(unsigned char *buf,int length)
{

	int adrs = (buf[1] << 8) | buf[2];
	int cnt  =  buf[0];
	int i;
	int sum=0;

#if	CHECKSUM_CALC
	for(i=0;i<length;i++) {
		sum += buf[i];
	}
//	printf("checksum=%x\n",sum);
	if( (sum & 0xff) != 0 ) {
		fprintf(stderr,"HEX RECORD Checksum Error!\n");
		exit(EXIT_FAILURE);
	}
#endif

	buf += 4;

#if	HEX_DUMP_TEST
	printf("%04x",adrs|adr_u);
	for(i=0;i<cnt;i++) {
		printf(" %02x",buf[i]);
	}
	printf("\n");
#endif


	if(adr_u != 0) {	//��ʃA�h���X����[��(64kB�ȊO)
		for(i=0;i<cnt;i++) {
			int adr24 = ( (adrs+i) | adr_u );
			if( ( adr24 & ~(FUSE_SIZE-1) ) == FUSE_ADDR ) {	// 0x30000X �Ȃ�� fuse_buf[X]�ɒl������.
				fuse_buf[ adr24 & (FUSE_SIZE-1) ] = buf[i] ;
			}
			if( ( adr24 & ~(IDLOC_SIZE-1) ) == IDLOC_ADDR ) {	// 0x20000X �Ȃ�� idloc_buf[X]�ɒl������.
				idloc_buf[ adr24 & (IDLOC_SIZE-1) ] = buf[i] ;
			}
		}
		return 1;
	}

	for(i=0;i<cnt;i++) {
		flash_buf[adrs+i] = buf[i];
	}
	return 0;
}
/**********************************************************************
 *	INTEL HEX���R�[�h(�P�s:�o�C�i���[���ς�) �����߂���.
 **********************************************************************
 */
int	parse_ihex(int scmd,unsigned char *buf,int length)
{
	switch(scmd) {
		case 0x00:	//�f�[�^���R�[�h:
					//�f�[�^�t�B�[���h�͏������܂��ׂ��f�[�^�ł���B
			return parse_ihex01(buf,length);

		case 0x01:	//�G���h���R�[�h:
					//HEX�t�@�C���̏I��������.�t����񖳂�.
			return scmd;
			break;

		case 0x02:	//�Z�O�����g�A�h���X���R�[�h:
					//�f�[�^�t�B�[���h�ɂ̓Z�O�����g�A�h���X������B ���Ƃ��΁A
					//:02000002E0100C
					//		   ~~~~
					//
			break;

		case 0x03:	//�X�^�[�g�Z�O�����g�A�h���X���R�[�h.
			break;

		case 0x04:	//�g�����j�A�A�h���X���R�[�h.
					//���̃��R�[�h�ł�32�r�b�g�A�h���X�̂������16�r�b�g
					//�i�r�b�g32�`�r�b�g16�́j��^����B
				adr_u = (buf[4]<<24)|(buf[5]<<16);
//				printf("adr_u=%x\n",adr_u);
				return scmd;
			break;

		case 0x05:	//�X�^�[�g���j�A�A�h���X�F
					//�Ⴆ��
					//:04000005FF000123D4
					//         ~~~~~~~~
					//�ƂȂ��Ă���΁AFF000123h�Ԓn���X�^�[�g�A�h���X�ɂȂ�B

			break;

	}
	fprintf(stderr,"Unsupported ihex cmd=%x\n",scmd);
	return 0;
}

/**********************************************************************
 *	16�i2���̕�������o�C�i���[�ɕϊ�����.
 **********************************************************************
 */
int read_hex_byte(char *s)
{
	char buf[16];
	int rc = -1;

	buf[0] = s[0];
	buf[1] = s[1];
	buf[2] = 0;
	sscanf(buf,"%x",&rc);
	return rc;
}

/**********************************************************************
 *	16�i�ŏ����ꂽ��������o�C�i���[�ɕϊ�����.
 **********************************************************************
 */
static	int read_hex_string(char *s)
{
	int rc;

	dataidx = 0;
	while(*s) {
		rc = read_hex_byte(s);s+=2;
		if(rc<0) return rc;
		databuf[dataidx++]=rc;
	}
	return 0;
}

/**********************************************************************
 *
 **********************************************************************
 0 1 2 3 4
:1000100084C083C082C081C080C07FC07EC07DC0DC
 ~~ �f�[�^��
   ~~~~ �A�h���X
       ~~ ���R�[�h�^�C�v
         ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~�f�[�^
                                         ~~�`�F�b�N�T��.
 */
int	read_ihex(char *s)
{
	int c;
	if(s[0] == ':') {
		read_hex_string(s+1);
		c = databuf[3];
		parse_ihex(c,databuf,dataidx);
	}
	return 0;
}
/*********************************************************************
 *	HEX file ��ǂݍ���� flash_buf[] �Ɋi�[����.
 *********************************************************************
 */
int	read_hexfile(char *fname)
{
	FILE *ifp;
	int rc;
	Ropen(fname);
	while(1) {
		rc = getln(lnbuf,ifp);
		read_ihex(lnbuf);
		if(rc == EOF) break;
	}
	Rclose();
	return 0;
}

/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	write_block(int addr,int size)
{
	int i,f=0;
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
		WriteFlash(addr,flash_buf+addr,size/2);
		fprintf(stderr,"Writing   ... %04x\r",addr);
	}
	return 0;
}
/*********************************************************************
 *	 �^�[�Q�b�g PIC �̓��e�� read_buf[] �ɓǂݍ���.
 *********************************************************************
 */
int	read_block(int addr,uchar *read_buf,int size)
{
	int errcnt=0;
#if	0
	ReadFlash(addr,read_buf,size,opt_d);
#else
	// Cached.
	if((size==32)&&(addr == (cache_addr+32))) {
		memcpy(read_buf,cache_buf+32,size);
	}else{
		// Uncached.
		ReadFlash( addr,cache_buf,CACHE_SIZE,opt_d);
		cache_addr = addr;
		memcpy(read_buf,cache_buf,size);
	}
#endif
	return errcnt;
}
/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	verify_block(int addr,int size)
{
	int i,f=0;
	int errcnt=0;
	uchar 	verify_buf[256];
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
//		ReadFlash(addr,verify_buf,size,0);
		read_block(addr,verify_buf,size);
		for(i=0;i<size;i++) {
			if(flash_buf[addr+i] != verify_buf[i]) {
				errcnt++;
				fprintf(stderr,"Verify Error in %x : write %02x != read %02x\n"
					,addr+i,flash_buf[addr+i],verify_buf[i]);
			}
		}
		fprintf(stderr,"Verifying ... %04x\r",addr);
	}
	return errcnt;
}
/*********************************************************************
 *	FLash�̏���.
 *********************************************************************
 */
int	erase_flash(void)
{
	fprintf(stderr,"Erase ... \n");
	BulkErase(picdev->EraseCommand);
	return 0;
}
/*********************************************************************
 *	�_�~�[.
 *********************************************************************
 */
int disasm_print(unsigned char *buf,int size,int adr)
{
	unsigned short *inst = (unsigned short *)buf;
	printf("%04x %04x\n",adr,inst[0]);
	return 2;
}
/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	write_hexdata(void)
{
	int i;
	for(i=flash_start;i<flash_end;i+= flash_step) {
		write_block(i,flash_step);
	}
	return 0;
}
/*********************************************************************
 *	fuse_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	write_fusedata(void)
{
	int i,f=0;

	fprintf(stderr,"Writing Fuse ...\n");

	for(i=0;i<14;i++) {
		if(fuse_buf[i] != 0xff) f = 1;
	}
	if(f) {
		if(((dev_id & 0xff00) == 0x4700 )||		// 18F14K / 13K
		   ((dev_id & 0xff00) == 0x1200 )  ) {	// 18F2550/4550
			if((fuse_buf[6] &  0x04)==0) {
				fuse_buf[6] |= 0x04;
				fprintf(stderr,"WARNING: config LVP=OFF -> ON .\n");
			}
		}
		WriteFuse(fuse_buf);
	}
	return 0;
}

void read_fusedata(void)
{
	ReadFlash(PIC_CONFIG_ADR,config_buf,14,0);
}
/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	verify_hexdata(void)
{
	int i;
	int errcnt = 0;
	cache_addr = (-1);

	fprintf(stderr,"\n");
	for(i=flash_start;i<flash_end;i+= FLASH_STEP) {
		errcnt += verify_block(i,FLASH_STEP);
	}
	return errcnt;
}
/*********************************************************************
 *	ROM�ǂݏo�����ʂ�HEX�ŏo�͂���.
 *********************************************************************
 */
#define	HEX_DUMP_STEP	16

void print_hex_block(FILE *ofp,int addr,int size)
{
	int i,j,f;
	for(i=0;i<size;i+=HEX_DUMP_STEP,addr+=HEX_DUMP_STEP) {
		f = 0;
		for(j=0;j<HEX_DUMP_STEP;j++) {
			if( flash_buf[addr+j] != 0xff ) {
				f = 1;
			}
		}
		if(f) {

#if	0
			printf(":%04x",addr);
			for(j=0;j<HEX_DUMP_STEP;j++) {
				printf(" %02x",flash_buf[addr+j]);
			}
			printf("\n");
#endif
			out_ihex01(&flash_buf[addr],addr,HEX_DUMP_STEP,ofp);


		}
	}


}
/*********************************************************************
 *	Config�ǂݏo�����ʂ�HEX�ŏo�͂���.
 *********************************************************************
 */
#define	HEX_DUMP_STEP	16

void print_hex_byte(FILE *ofp,int addr,uchar *data)
{
	out_ihex01(data,addr,1,ofp);
}
/*********************************************************************
 *	ROM�ǂݏo��.
 *********************************************************************
 */
void read_from_pic(char *file)
{
	int i;
	FILE *ofp;

	cache_addr = (-1);

//	fprintf(stderr, "Reading...\n");
	Wopen(file);
	fprintf(ofp,":020000040000FA\n");
	for(i=flash_start;i<flash_end_for_read;i+= FLASH_STEP) {
		fprintf(stderr,"Reading ... %04x\r",i);
		read_block(i,&flash_buf[i],FLASH_STEP);
		print_hex_block(ofp,i,FLASH_STEP);
	}

	read_fusedata();
	fprintf(ofp,":020000040030CA\n");
	for(i=0;i<14;i++) {
		if((config_mask[i]!=0)||(config_buf[i]!=0)) {
			print_hex_byte(ofp,i,&config_buf[i]);
		}
	}
	fprintf(ofp,":00000001FF\n");
	Wclose();
}
/*********************************************************************
 *	���C��
 *********************************************************************
 */
void getopt_p(char *s)
{
		if (s) {
			if (*s == ':') s++;
			if (*s == '?') {
				hidasp_list("picmon");
				exit(EXIT_SUCCESS);
			} else if (isdigit(*s)) {
				if (s) {
					int n, l;
					l = strlen(s);
					if (l < 4 && isdigit(*s)) {
						n = atoi(s);
						if ((0 <= n) && (n <= 999)) {
							sprintf(usb_serial, "%04d", n);
						} else {
							if (l == 1) {
								usb_serial[3] = s[0];
							} else if  (l == 2) {
								usb_serial[2] = s[0];
								usb_serial[3] = s[1];
							} else if  (l == 3) {
								usb_serial[1] = s[0];
								usb_serial[2] = s[1];
								usb_serial[3] = s[2];
							}
						}
					} else {
						strncpy(usb_serial, s, 4);
					}
				}
			} else if (*s == '\0'){
				strcpy(usb_serial, DEFAULT_SERIAL);		// -p�݂̂̎�
			} else {
				strncpy(usb_serial, s, 4);
			}
			strupr(usb_serial);
		}
}
/*********************************************************************
 *	���C��
 *********************************************************************
 */
int main(int argc,char **argv)
{
	int errcnt, ret_val=0;
	uchar devid_buf[256];
	extern void wait_ms(int ms);

	//�I�v�V�������.
	Getopt(argc,argv,"i");
	if(IsOpt('h') || IsOpt('?') || IsOpt('/')) {
		usage();
		exit(EXIT_SUCCESS);
	}
	if((argc<2)&& (!IsOpt('r')) && (!IsOpt('e')) && (!IsOpt('p')) ) {
		usage();
		exit(EXIT_SUCCESS);
	}


	if(IsOpt('e')) {
		opt_e = 1;				//������������.
	}
	if(IsOpt('p')) {
		getopt_p(Opt('p'));		//�V���A���ԍ��w��.
	}
	if(IsOpt('s')) {
		sscanf(Opt('s'),"%x",&opt_s);	//�A�v���P�[�V�����̊J�n�Ԓn�w��.
	}
	if(IsOpt('r')) {
		char *r = Opt('r');
		if(r[0]==0  ) opt_r = 1;		//'-r'
		if(r[0]=='p') opt_rp = 1;		//'-rp' flash�̈�̓ǂݍ���.
		if(r[0]=='e') opt_re = 1;		//'-re'
		if(r[0]=='f') opt_rf = 1;		//'-rf'
	}
	if(IsOpt('w')) {
		char *w = Opt('w');
		if(w[0]==0  ) opt_w = 1;		//'-w'
		if(w[0]=='f') opt_wf = 1;		//'-wf'
	}
	if(IsOpt('n')) {
		char *n = Opt('n');
		if(n[0]=='v') opt_nv = 1;		//'-nv'
	}
	if(IsOpt('v')) {
		opt_v = 1;						//'-v'
	}
	if(IsOpt('d')) {
		opt_d = 1;
	}

	//������.
	if( UsbInit(verbose_mode, 0, usb_serial) == 0) {
		exit(EXIT_FAILURE);
	}

#if 1
	timeBeginPeriod(1);	// @@@
	wait_ms(10);		// �Ȃ����A�ŏ��̈��ڂ͐��x���ǂ��Ȃ�(dummy)
#endif
	PicInit();		// portb , pinb , ddrb �̃A�h���X������.
	PicPgm(1);		// PGM,MCLR�̃Z�b�g�A�b�v.

	//�f�o�C�X���o.
	{
		ReadFlash(PIC_DEVICE_ADR,devid_buf,2,0);
		dev_id = devid_buf[0] | (devid_buf[1]<<8);
		picdev = FindDevice(dev_id);
		if(	picdev==NULL) {
			printf("ERROR: unknown device id 0x%04x\n",dev_id);
			ret_val = EXIT_FAILURE;
			goto Terminate;
		}

		printf("*Device:%s  DEV_ID:0x%04x\n",picdev->DeviceName,dev_id);

		flash_end_for_read = picdev->ProgramMemorySize;
		flash_end  = picdev->ProgramMemorySize;
		flash_step = picdev->WriteBufferSize;
		config_value = picdev->ConfigValue;
		config_mask  = picdev->ConfigMask;
	}

	// Fuse �ǂݏo��(�̂�)
	if(opt_rf) {
		read_fusedata();
		memdump("CONFIG:"		,config_buf,14);
		goto Terminate;
	}

	// ���������̎��{.
	if((opt_e) && (argc < 2)) {					// ���������̎��s.
		erase_flash();							//  Flash����.
	}
	if(opt_wf) {opt_v=1;opt_nv=1;}				// -wf�̂Ƃ���Flash�͏����Ȃ�.

	memset(flash_buf,0xff,FLASH_SIZE);
	memset(fuse_buf ,0xff,FUSE_SIZE);

	ret_val = EXIT_SUCCESS;

	if(argc>=2) {
		if(opt_rp||opt_re||opt_rf||opt_d) {	// ROM�ǂݏo��.
			read_from_pic(argv[1]);
		}else{
			read_hexfile(argv[1]);		//	HEX�t�@�C���̓ǂݍ���.
			if(opt_v == 0) {			// �x���t�@�C�̂Ƃ��͏������݂����Ȃ�.
				erase_flash();				//  Flash����.
				write_hexdata();			//  ��������.
			}

		  if(opt_nv==0) {
			errcnt = verify_hexdata();	//  �x���t�@�C.
			if(errcnt==0) {
				fprintf(stderr,"Verify Ok.\n");
			}else{
				fprintf(stderr,"Verify Error. errors=%d\n",errcnt);
				ret_val = EXIT_FAILURE;
				goto Terminate;
			}
		  }


			if((opt_v == 0)||(opt_wf == 1)) {	// �x���t�@�C�̂Ƃ��͏������݂����Ȃ�.
				write_fusedata();		// Fuse ��������.
				read_fusedata();
				memdump("CONFIG:"		,config_buf,14);
			}
		}
	}

Terminate:
#if 1
	timeEndPeriod(1);	// @@@
#endif
	PicPgm(0);		// PGM,MCLR�̃N���[�Y.
	UsbExit();
	return ret_val;
}
/*********************************************************************
 *
 *********************************************************************
 */

