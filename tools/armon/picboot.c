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


#define MAIN_DEF	// for monit.h
#include "monit.h"
#include "hidasp.h"
#include "util.h"

#define CMD_NAME "picboot.exe"
#define DEFAULT_SERIAL	"*"

char HELP_USAGE[]={
	"* PICboot Ver 0.3 (" __DATE__ ")\n"
	"Usage is\n"
	"  " CMD_NAME " [Options] <hexfile.hex>\n"
	"Options\n"
	"  -p[:XXXX]   ...  Select serial number (default=" DEFAULT_SERIAL ").\n"
	"  -r          ...  Run after write.\n"
	"  -v          ...  Verify.\n"
	"  -E          ...  Erase.\n"
	"  -rp         ...  Read program area.\n"
	"  -nv         ...  No-Verify.\n"
	"  -B          ...  Allow Bootloader Area to Write / Read !.\n"
	"  -sXXXX      ...  Set program start address. (default=2000)\n"
};

#define	MAX_CMDBUF	4096

static	char lnbuf[1024];
static	char usb_serial[128]=DEFAULT_SERIAL;	/* �g�p����HIDaspx�̃V���A���ԍ����i�[ */
static	char verbose_mode = 0;	//1:�f�o�C�X�����_���v.
		int  hidmon_mode = 1;	/* picmon �����s�� */
static	uchar 	databuf[256];
static	int		dataidx;
static	int		adr_u = 0;		// hexrec mode 4

#define	HIGH_MASK	0xffff0000

#define	READ_SIZE	64
#define	WRITE_SIZE	(64-8)

#ifdef	LPC2388
//==LPC2388===================
#define	FLASH_BASE	0x00000000
#define	FLASH_START	0x00002000
#define	FLASH_SIZE	0x0007e000

#define	FLASH_STEP	4096
#define	ERASE_STEP	4096

#else

#ifdef	LPC1343
//==LPC1343===================
#define	FLASH_BASE	0x00000000
#define	FLASH_START	0x00002000
#define	FLASH_SIZE	0x00008000

#define	FLASH_STEP	4096
#define	ERASE_STEP	4096

#else

//==STM32=====================
#define	FLASH_BASE	0x08000000
#define	FLASH_START	0x00002000
#define	FLASH_SIZE	0x00080000	// 512kB

#define	FLASH_STEP	1024
#define	ERASE_STEP	1024
#endif

#endif


static	uchar flash_buf[FLASH_SIZE];
static	int	  flash_start = FLASH_START;
static	int	  flash_end   = FLASH_SIZE;
static	int	  flash_end_for_read = 0x8000;	//0x20000;


static	char  opt_r  = 0;		//	'-r '
static	char  opt_rp = 0;		//	'-rp'
static	char  opt_re = 0;		//	'-re'
static	char  opt_rf = 0;		//	'-rf'

static	char  opt_E  = 0;		//	erase
static	char  opt_v  = 0;		//	'-v'
static	char  opt_nv = 0;		//	'-nv'

//	���[�U�[�A�v���P�[�V�����J�n�Ԓn.
static	int   opt_s  = 0x2000;	//	'-s1000 �Ȃ�'

#define	CHECKSUM_CALC	(1)


#define	DEVCAPS_BOOTLOADER	0x01

void hidasp_delay_ms(int dly);
/*********************************************************************
 *	�g�p�@��\��
 *********************************************************************
 */
void usage(void)
{
	fputs( HELP_USAGE, stdout );
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
static	int	out_ihex01(unsigned char *data,int adrs,int cnt,FILE *ofp,int rec_id)
{
	int length, i, sum;
	unsigned char buf[1024];

	buf[0] = cnt;
	buf[1] = adrs >> 8;
	buf[2] = adrs & 0xff;
	buf[3] = rec_id;

	length = cnt+5;
	sum=0;

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
static void put_flash_buf(int adrs,int data)
{
	adrs = adrs - FLASH_BASE;
	if( (adrs >= 0) && (adrs < FLASH_SIZE ) ) {
		flash_buf[adrs] = data;
	}
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
		fprintf(stderr,"%s: HEX RECORD Checksum Error!\n", CMD_NAME);
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

	adrs |= adr_u;
	for(i=0;i<cnt;i++) {
		put_flash_buf(adrs+i,buf[i]);
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
				//�Ƃ肠��������.
				return scmd;
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
				//�Ƃ肠��������.
				return scmd;
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
 *	flash_buf[]�̎w��G���A�� 0xff�Ŗ��܂��Ă��邱�Ƃ��m�F����.
 *********************************************************************
 *	���܂��Ă���� 0 ��Ԃ�.
 */
#if	0
static	int	check_prog_ff(int start,int size)
{
	int i;
	for(i=0;i<size;i++) {
		if(flash_buf[start+i]!=0xff) return 1;
	}
	return 0;
}
#endif
/*********************************************************************
 *	
 *********************************************************************
 *	
 */
static	void modify_vector( int addr , int dst )
{
	int i;
	for(i=0;i<0x180;i++) {
		flash_buf[addr+i] = flash_buf[dst+i];	
	}
}
/*********************************************************************
 *	�K�v�Ȃ�Aflash_buf[] �� 0x800,0x808,0x818�Ԓn������������.
 *********************************************************************
 */
void modify_start_addr(int start)
{
	if(start == 0x2000) return;	//�f�t�H���g�l�Ȃ̂ŉ������Ȃ�.

	if(start <  0x2000) {		// ���[�U�[�v���O�����J�n�Ԓn��0x800�ȑO�ɂ���̂͂�������.
		fprintf(stderr,"WARNING: start address %x < 0x2000 ?\n",start);
		return;
	}
	// ���[�U�[�t�@�[���̗�O�x�N�^�[�G���A(start��offset�ɂȂ��Ă���)
	// ��0x2000�Ԓn�ɃR�s�[����.

	// �������邱�ƂŁA�C�ӔԒn(������256byte�{���Ԓn)�J�n�̃t�@�[����
	// jumper����ɂ�肻�̂܂܋N����������ł���.
	modify_vector( 0x2000 , start );
}
int	read_block(int addr,uchar *read_buf,int size);

/*********************************************************************
 *	�^�[�Q�b�g�̍ċN��.
 *********************************************************************
 */
int reboot_target(int vect)
{
#ifdef	LPC2388
//	ARM7 �̃x�N�^�[��ARM�̕��򖽗߂��̂��̂��������܂�Ă���.
//	RESET��ROM�擪�̖��߂����s.

	UsbBootTarget(vect,1);

#else
//	Cortex-M3 �̃x�N�^�[�̓A�h���X�z�񂪏������܂�Ă���.
//	RESET �̓A�h���X�z���2�Ԗڂɏ����ꂽ�Ԓn�ւ̕��򂷂邱�ƂƓ���.
	int	buf[32];

	read_block(FLASH_BASE+vect,(uchar*)buf,32);
	int boot_pc = buf[1];	// PC

	if(( boot_pc >= FLASH_BASE)&&
	   ( boot_pc < (FLASH_BASE+FLASH_SIZE)) ){
		printf("reset vector: %08x\n",boot_pc);
		UsbBootTarget(boot_pc,1);
	}else{
		printf("Illegal reset vector: %08x\n",boot_pc);
	}
#endif

	return 0;
}

/*********************************************************************
 *	1024 byte������.
 *********************************************************************
 */
int	erase_block(int addr,int size)
{
	int i,f=0;
	int pages = size / 1024;

	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if((f) || (opt_E)) {
#if	0	// TEST
		printf("UsbEraseTargetROM(%x,%x)\n",FLASH_BASE+addr , pages);
#else
		UsbEraseTargetROM(FLASH_BASE+addr , pages);
#endif
		Sleep(20);
//		hidasp_delay_ms(20);
	}
	return 0;
}

/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	write_block_usb_packet(int addr,int size)
{
	int i,f=0;
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}
	if(f) {
		UsbFlash(addr ,AREA_PGMEM,flash_buf+addr,size);
	}
	return 0;
}

/*********************************************************************
 *	flash_buf[] �̓��e�� ��������( 1PAGE = 1kBytes )
 *********************************************************************
 */
int	write_block_page(int addr,int size)
{
#if	defined(LPC1343)||defined(LPC2388)
	int addr0=addr;
#endif

	int wsize;
	while(size) {
		wsize = size;
		if(	wsize>= WRITE_SIZE) {
			wsize = WRITE_SIZE;		//�P��̓ǂݏo���\�T�C�Y=64byte
		}
		write_block_usb_packet(addr,wsize);
		addr += wsize;
		size -= wsize;
	}

#if	defined(LPC1343)||defined(LPC2388)
	// FLASH 1�y�[�W���̍X�V���s.LPC�}�C�R���̂�.
	UsbFlash(addr0 ,AREA_PGMEM , flash_buf + addr0, 0);
#endif

	return 0;
}
/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 *	�����P�ʂ� FLASHROM �̃y�[�W�T�C�Y.(STM32=1K LPC1343=4K)
 */
int	write_block(int addr,int size)
{
	int i,f=0;
	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
		write_block_page(addr,size);		// 1�y�[�W����������.
		fprintf(stderr,"Writing ... %04x\r",addr);
	}
	return 0;
}
/*********************************************************************
 *	 �^�[�Q�b�g PIC �̓��e�� flash_buf[] �ɓǂݍ���.
 *********************************************************************
 */
int	read_block(int addr,uchar *read_buf,int size)
{
	int errcnt=0;
	int rsize;

	while(size) {
		rsize = size;
		if(	rsize>= READ_SIZE) {
			rsize = READ_SIZE;		//�P��̓ǂݏo���\�T�C�Y=64byte
		}
		UsbRead(addr ,AREA_PGMEM,read_buf,rsize);
		addr     += rsize;
		read_buf += rsize;
		size -= rsize;
	}
	return errcnt;
}


/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	verify_block(int addr,int size)
{
	uchar verify_buf[FLASH_STEP];
	int i,f=0;
	int errcnt=0;

	for(i=0;i<size;i++) {
		if(flash_buf[addr+i]!=0xff) f=1;
	}

	if(f) {
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
	int i;
	fprintf(stderr,"Erase ... %x-%x\n",flash_start,flash_end);
	for(i=flash_start;i<flash_end;i+= ERASE_STEP) {
		erase_block(i,ERASE_STEP);
	}
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
	for(i=flash_start;i<flash_end;i+= FLASH_STEP) {
		write_block(i,FLASH_STEP);
	}
	return 0;
}
/*********************************************************************
 *	flash_buf[] �̓��e�� �^�[�Q�b�g PIC �ɏ�������.
 *********************************************************************
 */
int	verify_hexdata(void)
{
	int i;
	int errcnt = 0;

	fprintf(stderr,"\n");
	for(i=flash_start;i<flash_end;i+= FLASH_STEP) {
		errcnt += verify_block(i,FLASH_STEP);
	}
	return errcnt;
}
/*********************************************************************
 *	�|�[�g���̂���A�h���X�����߂�.�i�_�~�[�����j
 *********************************************************************
 */
int	portAddress(char *s)
{
	return 0;
}

/*********************************************************************
 *	ROM�ǂݏo�����ʂ�HEX�ŏo�͂���.
 *********************************************************************
 */
#define	HEX_DUMP_STEP	16

void print_high_adrs(FILE *ofp,int addr)
{
								//":020000040000F2";
	static  uchar hi_record[64]={0x02,0x00,0x00,0x04,0x00,0x00,0xf2};
	int hi = (FLASH_BASE+addr) & HIGH_MASK;
	if(	adr_u != hi) {
		adr_u  = hi;
		hi_record[0]=adr_u >> 24;
		hi_record[1]=adr_u >> 16;
		out_ihex01(hi_record,0,2,ofp,0x04);
	}
}

void print_hex_block(FILE *ofp,int addr,int size)
{
	int i,j,f;

	print_high_adrs(ofp,addr);

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
			out_ihex01(&flash_buf[addr],addr,HEX_DUMP_STEP,ofp,0x00);


		}
	}


}
/*********************************************************************
 *	ROM�ǂݏo��.
 *********************************************************************
 */
void read_from_pic(char *file)
{
	int i, progress_on;
	FILE *ofp;
	int adr=0;
	uchar *read_buf;

//	fprintf(stderr, "Reading...\n");
	 progress_on = 1;
#if 1	/* by senshu */
	if(file != NULL && strcmp(file, "con")==0) {
		ofp=stdout;
		progress_on = 0;
	} else {
		if (file == NULL) {
			 file = "NUL";
		}
		ofp=fopen(file, "wb");
		if(ofp==NULL) {
			fprintf(stderr, "%s: Can't create file:%s\n", CMD_NAME, file);
			exit(1);
		}
	}
#else
	Wopen(file);
#endif
//	fprintf(ofp,":020000040000FA\n");
	adr_u=-1;

	for(i=flash_start;i<flash_end_for_read;i+= FLASH_STEP) {
		adr = FLASH_BASE + i;
		if (progress_on) {
			fprintf(stderr,"Reading ... %08x\r",adr);
		}
		read_buf = &flash_buf[i] ;
		read_block(adr,read_buf,FLASH_STEP);
		print_hex_block(ofp,i,FLASH_STEP);
		fflush(ofp);
	}
	fprintf(ofp,":00000001FF\n");
	if (progress_on) {
		fprintf(stderr,"\nRead end address = %08x\n", adr-1);
	}
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
void Flash_Unlock(void)
{
	UsbFlashLock(0);
}
void Flash_Lock(void)
{
	UsbFlashLock(1);
}
/*********************************************************************
 *	���C��
 *********************************************************************
 */
int main(int argc,char **argv)
{
	int errcnt, ret_val,retry;
	int dev_flash_size=0;
	int dev_flash_used=0;

	//�I�v�V�������.
	Getopt(argc,argv,"i");
	if(IsOpt('h') || IsOpt('?') || IsOpt('/')) {
		usage();
		exit(EXIT_SUCCESS);
	}
	if((argc<2)&& (!IsOpt('r')) && (!IsOpt('E')) && (!IsOpt('p')) ) {
		usage();
		exit(EXIT_SUCCESS);
	}


	if(IsOpt('B')) {					// Boot�G���A�̋����ǂݏ���.
		flash_start = 0;
		flash_end   = FLASH_START;
	}
	if(IsOpt('E')) {
		opt_E = 1;				//������������.
	}
	if(IsOpt('p')) {
		getopt_p(Opt('p'));		//�V���A���ԍ��w��.
	}
	if(IsOpt('s')) {
		sscanf(Opt('s'),"%x",&opt_s);	//�A�v���P�[�V�����̊J�n�Ԓn�w��.
	}
	if(IsOpt('r')) {
		char *r = Opt('r');
		if(r[0]==0  ) opt_r = 1;		//�������݌ナ�Z�b�g���삠��.
		if(r[0]=='p') opt_rp = 1;		//'-rp' flash�̈�̓ǂݍ���.
		if(r[0]=='e') opt_re = 1;		//'-re'
		if(r[0]=='f') opt_rf = 1;		//'-rf'
	}
	if(IsOpt('n')) {
		char *n = Opt('n');
		if(n[0]=='v') opt_nv = 1;		//'-nv' 
	}
	if(IsOpt('v')) {
		opt_v = 1;						//'-v' 
	}


  for(retry=2;retry>=0;retry--) {
	//������.
   if( UsbInit(verbose_mode, 0, usb_serial) == 0) {
		fprintf(stderr, "Try, UsbInit(\"%s\").\n", usb_serial);
		if(retry==0) {
			fprintf(stderr, "%s: UsbInit failure.\n", CMD_NAME);
			exit(EXIT_FAILURE);
		}
   }else{
	if(IsOpt('B')) {
		break;
	}

	if((UsbGetDevCaps(&dev_flash_size,&dev_flash_used) & DEVCAPS_BOOTLOADER)) {
		break;		// BOOTLOADER�@�\����.
	}else{
		fprintf(stderr, "Reboot firmware ...\n");
		reboot_target(0);
		Sleep(10);
		UsbExit();
	}
   }
   Sleep(2500);
  }

//	=================================
#define	END_MASK	0xfffff

	if(!IsOpt('B')) {
		flash_end = dev_flash_size & END_MASK;				// FLASH�̎��e��.
		flash_end_for_read = dev_flash_used & END_MASK;		// FLASH�̎g�p�ςݗe��.
#if	0
		printf("flash_end=%x\n",flash_end);
		printf("flash_end_for_read=%x\n",flash_end_for_read);
#endif
	}

	Flash_Unlock();


	if((opt_E) && (argc < 2)) {					// ���������̎��s.
		erase_flash();							//  Flash����.
	}

	memset(flash_buf,0xff,FLASH_SIZE);
	ret_val = EXIT_SUCCESS;

	if(argc>=2) {
		if(opt_rp||opt_re||opt_rf) {	// ROM�ǂݏo��.
			read_from_pic(argv[1]);
		}else{
			read_hexfile(argv[1]);		//	HEX�t�@�C���̓ǂݍ���.
			modify_start_addr( opt_s & 0x7ffff );

			if(IsOpt('v')==0) {			// �x���t�@�C�̂Ƃ��͏������݂����Ȃ�.
				erase_flash();				//  Flash����.
				write_hexdata();			//  ��������.
			}

		  if(opt_nv==0) {
			errcnt = verify_hexdata();	//  �x���t�@�C.
			if(errcnt==0) {
				fprintf(stderr,"\nVerify Ok.\n");
			}else{
				fprintf(stderr,"\nVerify Error. errors=%d\n", errcnt);
				ret_val = EXIT_FAILURE;
			}
		  }
		}
	}
#if 1
	else if (argc==1) {
		if(opt_rp||opt_re||opt_rf) {	// ROM�ǂݏo��.
			read_from_pic(NULL);
		}
	}
#endif

	Flash_Lock();

	if(opt_r) {
		reboot_target(FLASH_START);		//  �^�[�Q�b�g�ċN��.
	}
	UsbExit();
	return ret_val;
}
/*********************************************************************
 *
 *********************************************************************
 */

