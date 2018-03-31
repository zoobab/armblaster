/*********************************************************************
 *	HID_Monit
 *********************************************************************
 *
 * �R�}���h�w���v�� 'h' �������� '?' �ł�.

�A�h���X�͈͎w��ɂāA
 ADDRESS < ADDRESS2 �𖞂����Ȃ��Ƃ��� ADDRESS2 ���o�C�g���Ƃ݂Ȃ��܂�.
�R�}���h��A�h���X�̋�؂�͋󔒂������� ',' �̂ǂ���ł���.

 ADDRESS ��16�i���A�������̓|�[�g������.

 *********************************************************************
 */

#define VERSION	"0.2"


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

#ifndef	_LINUX_
#include <stdlib.h>
#include <conio.h>		// getch() kbhit()
#endif

#define MAIN_DEF	// for monit.h
#include "monit.h"
#include "hidasp.h"
#include "util.h"
#include "gr.h"
#include "analog.h"



#define	VCC_IS_5VOLT		1	// 0:Vcc=3.3V   1:Vcc = 5V
#define	ANALOG_CALC_DEBUG	0	// A/D �ϊ��̌v�Z���f�o�b�O.

#if	VCC_IS_5VOLT

#define	VOLT_VCC	5.00	// 4.76

#else
//#define	VOLT_VCC	5.00
#define	VOLT_VCC	(3.30-0.02)

#endif


#if	0	//������1�ɂ��āA���炩���ߕ������Ă��� Tref �̒l��
		//���ɏ����Ă����ƁA����̂΂���������Ԍ���܂�.

#define	ANALOG_TREF_CONST	304	// A/D �ϊ���Vref�v�����ȗ�.

#endif


//#define	VOLT_REF	1.23
#define	VOLT_REF	0.970	//1.02	�`�b�v���Ƃɂ΂��������܂�.
							//AVR>ain 100000 �����s����,Tin=Tref�ɂȂ�����Ԃ�
							//DMM�ɂ�AIN0�̓d�ʂ𑪒肵�Č��߂Ă�������.
							//���̒l��1.23V�̂͂��ł����A�Q��v���̌�̒l��
							//���肵�Ă���悤�ł�.


#define	REFERENCE_K_OHM	2.200	// �Q�ƒ�R�q �̒�R�lK��.
#define	INTERNAL_K_OHM	0.000	//       AVR�̓�����RK��.

void cmdPortPrintOne(char *name,int adrs,int val);

#define	KEY_ESC		0x1b
#define	KEY_CTRL_C	0x03

//int	  kbhit();

#include "portlist.h"

extern PortList *portList;
void ChangePortList14K(void);
int	arm_init(int isthumb);

#define DEFAULT_SERIAL	"*"

int hidmon_mode = 1;	/* picmon �����s�� */

char HELP_USAGE[]={
	"* HID_Monit Ver " VERSION " (" __DATE__ ")\n"
	"Usage is\n"
	"  monit [Options]\n"
	"Options\n"
	"  -p[:XXXX]   ...  select Serial number (default=" DEFAULT_SERIAL ").\n"
	"  -l          ...  command echo to Log file.\n"
	"  -i<file>    ...  input script file to execute.\n"
	"  -v          ...  print verbose.\n"
};

char HELP_MESSAGES[]={
	"* HID_Monit Ver 0.1\n"
	"Command List\n"
	" d  <ADDRESS1> <ADDRESS2>    Dump Memory(RAM)\n"
#if 0
	" dr <ADDRESS1> <ADDRESS2>    Dump Memory(EEPROM)\n"
	" dp <ADDRESS1> <ADDRESS2>    Dump Memory(PGMEM)\n"
	" ain   <CNT>                 Analog input\n"
	" aingraph                    Analog input (Graphical)\n"
	" aindebug <mode>             Analog input debug\n"
	" reg   <CNT>                 Registance Meter\n"
	" reggraph                    Registance Meter (Graphical)\n"
	" p <PortName>.<bit> <DATA>   Write Data to PortName.bit\n"
	" poll  *  <CNT>              continuous polling port A,B,D\n"
	" poll <portName>             Continuous polling port\n"
#endif
	" e  <ADDRESS1> <DATA>        Edit Memory\n"
	" f  <ADDRESS1> <ADDRESS2> <DATA> Fill Memory\n"
	" l  <ADDRESS1> <ADDRESS2>    List (Disassemble) PGMEM\n"
	" p ?                         Print PortName-List\n"
	" p .                         Print All Port (column format)\n"
	" p *                         Print All Port (dump format)\n"
	" p <PortName>                Print PortAddress and data\n"
	" p <PortName> <DATA>         Write Data to PortName\n"
	" sleep <n>                   sleep <n> mSec\n"
	" label <LABEL>               set loop label.\n"
	" :<LABEL>                    set loop label.\n"
	" loop  <LABEL> <CNT>         loop execution <CNT> times.\n"
	" bench <CNT>                 HID Write Speed Test\n"
	" boot [<address>]            Start user program\n"
	" run  <address>              Run user program at <address>\n"
	" user <arg>                  Run user defined function (usercmd.c)\n"
	" poll <portName> [<CNT>]     continuous polling port\n"
	" graph <portName>            Graphic display\n"
	" q                           Quit to DOS\n"
};

//
//	�R�}���h��͗p���[�N.
//
#define	MAX_CMDBUF	4096
static	char cmdbuf[MAX_CMDBUF];
static  char *arg_ptr[128];
static  int  arg_val[128];
static  int  arg_hex[128];
static  int  arg_cnt;
static	char isExit = 0;		//1:�R�}���h���̓��[�v�𔲂���.

static	int	 adrs = 0;
static	int	 adrs2= 0;
static	int	 memcnt=0;
static	int	 loopcnt=0;
static	int  arena= 0;
static	int  poll_mode=POLL_PORT;	// 0xc0
static	int  trig_mode=0;			// ���W�A�i�̃g���K�[�����ɂȂ����bit�}�X�N
									// �t���[�����̏ꍇ��0 �Sbit�Ď��Ȃ�0xff


static	char usb_serial[128];	/* �g�p����HIDaspx�̃V���A���ԍ����i�[ */

static	uchar infra_sample[1280];

//
//	�I�v�V�����������[�N.
//
static	char verbose_mode = 0;	//1:�f�o�C�X�����_���v.
static	FILE *script_fp = NULL;	//�O���t�@�C������R�}���h��ǂݍ���.
static  int log_mode = 0;		//�[�����͂��G�R�[����.
static  int echo_mode = 1;


/**********************************************************************
 *		������ p �� ���� c �ŕ������Aargs[] �z��ɓ����B
 *		������͕��f����邱�Ƃɒ��ӁB
 *		�󔒂��X�v���b�^�[�ɂȂ�悤�ɂ��Ă���̂Œ���
 **********************************************************************
 */
static	int	splitchr=' ';
static	int is_spc(int c)
{
	if(c==splitchr) return 1;
	if(c==' ') 		return 1;
	if(c=='\t') 	return 1;
	return(0);
}

/**********************************************************************
 *		������ *p ���f���~�^���� c ���邢�͋󔒂ŕ�����
 *             *args[] �|�C���^�ɓ����
 *		������ n ��Ԃ�.
 **********************************************************************
 */
static	int split_str(char *p,int c,char **args)
{
	int argc=0;
	int qq=0;
	splitchr=c;

	while(1) {
		while( is_spc(*p) ) p++;
		if(*p==0) break;

		if(*p != 0x22) {
			args[argc++]=p;		/* ���ڂ̐擪���� */

			while( *p ) {		/* ��؂蕶���܂œǂݐi�߂� */
				if(is_spc(*p))break;
				p++;
			}

			if(is_spc(*p)) {	/* ��؂蕶���ł���� */
				*p=0;p++;		/* NULL�Ő؂� */
			}
		}else{
			qq=*p++;
			args[argc++]=p;		/* ���ڂ̐擪���� */

			while( *p ) {		/* ��؂蕶���܂œǂݐi�߂� */
				if(*p==qq)break;
				p++;
			}
			if(*p==qq) {	/* ��؂蕶���ł���� */
				*p=0;p++;		/* NULL�Ő؂� */
			}
		}
	}
	return argc;
}


/*********************************************************************
 *	���� c ���󔒕����Ȃ�1��Ԃ�.
 *********************************************************************
 */
int is_space(int c)
{
	if((c==' ') || (c=='\t')) return 1;
	return 0;
}
/*********************************************************************
 *	������ *buf �̐擪�]����ǂݔ�΂�.
 *********************************************************************
 */
char *sp_skip(char *buf)
{
	while(*buf) {
		if(is_space(*buf)==0) return buf;
		buf++;
	}
	return buf;
}
/*********************************************************************
 *	������ *buf ��2�i�l '0101_1111' �ƌ��Ȃ��邩�ǂ����`�F�b�N����.
 *********************************************************************
 */
int radix2scanf(char *s,int *p)
{
	int rc = 0;
	int c;

	// 2�i�l�������ꍇ�̏��� = 10�����ȉ��A���A _ ���܂�ł��邱��.
	// 0b �ł͂��߂���@�����邪�A16�i�l�Ƌ�ʂ����Ȃ��̂�,�o�����_������.

	if( (strlen(s)<=10) && (strchr(s,'_')) ) {
		while(1) {
			c = *s++;
			if( c==0 ) break;		//������I�[.
			if((c=='0')||(c=='1')) {
				rc = rc << 1;		// 2�{.
				rc |= (c-'0');		// '1' �Ȃ�LSB��On.
			}else
			if(c != '_') {
				return (-1);	//���e�ł��Ȃ������������̂Ŏ��s.
			}
		}
		*p = rc;	//2�i�l��Ԃ�.
		return 0;	// OK.
	}
	return (-1);
}

/*********************************************************************
 *	arg_ptr[] �̕������HEX���l�Ƃ݂Ȃ��Ēl�ɕϊ��� arg_hex[]�ɓ����
 *********************************************************************
 */
void scan_args(int arg_cnt)
{
	int i,v;
	for(i=0;i<arg_cnt;i++) {
		v = portAddress(arg_ptr[i]);	//�V���{�����ɂ�����.
		if(v) {
			arg_hex[i] = v;
			arg_val[i] = v;
		}else{
			v=(-1);
			sscanf(arg_ptr[i],"%x",&v);
			arg_hex[i] = v;

			radix2scanf(arg_ptr[i],&arg_hex[i]);

			v=(-1);
			sscanf(arg_ptr[i],"%d",&v);
			arg_val[i] = v;
		}
	}
}
/*********************************************************************
 *	������ *buf ����A�p�����[�^��ǂݎ��A( adrs , memcnt ) �����߂�
 *********************************************************************
 */
int	get_arg(char *buf)
{
#if	0
	arena = AREA_RAM;
	if(*buf == 'p') {
		buf++; arena = AREA_PGMEM;
	}else
	if(*buf == 'r') {
		buf++; arena = AREA_EEPROM;
	}
#endif
	arena=MEM_BYTE;
	if(*buf == 'b') {
		buf++; 
	}else
	if(*buf == 'h') {
		buf++; arena = MEM_HALF;
	}else
	if(*buf == 'w') {
		buf++; arena = MEM_WORD;
	}
	

//	memcnt = 64;
	memcnt = 256;
	adrs2 = (-1);

	buf = sp_skip(buf);
	if(*buf==0) return 0;

	arg_cnt = split_str(buf,',',arg_ptr);
	scan_args(arg_cnt);

	if(arg_cnt>=1) {
		adrs = arg_hex[0];
	}
	if(arg_cnt>=2) {
		adrs2 = arg_hex[1];
		if(adrs2 != (-1) ) {
			memcnt = adrs2 - adrs + 1;
		}
		if(	memcnt < 0) {
			memcnt = adrs2;
		}
	}
	return arg_cnt;
}
/*********************************************************************
 *	������ *buf ���R�}���h�� *cmd ���܂�ł��邩�ǂ������ׂ�.
 *********************************************************************
 */
int	str_comp(char *buf,char *cmd)
{
	while(*cmd) {
		if(*buf != *cmd) {return 1;}	// �s��v.
		cmd++;
		buf++;
	}
	return 0;
}
/*********************************************************************
 *
 *********************************************************************
 */
void usage(void)
{
	fputs( HELP_USAGE, stdout );
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdHelp(char *buf)
{
	fputs( HELP_MESSAGES, stdout );
}


/*********************************************************************
 *	1bit����On�̃}�X�N�p�^������Abit�ԍ�(0..7)�ɕϊ�.
 *********************************************************************
 */
static	int get_bpos(int mask)
{
	int i;
	for(i=0;i<8;i++) {
		if(mask & 1) return i;
		mask >>= 1;
	}
	return (-1);
}

/*********************************************************************
 *	�d��������@
 *********************************************************************
Vref(1.23V)�̂Ƃ���Tref�ƁA
Vin(xxx V)�̂Ƃ���Tin ��USB�o�R�Ŏ擾���āA
1.23V=5.0V * (1-exp(-Tref/RC)) �ɂāARC�̐ς̒l�𓾂�
Vin  =5.0V * (1-exp(-Tin/RC)) ���v�Z����΂n�j�B
������5V�̐��x���e�����Ă���Ǝv����B

RC�̋t�Z���� exp(-Tref/RC))=1-(1.23/5.0)=0.754
RC = Tref/0.28236
 *********************************************************************
 */

double AnalogCalcVoltage(int Tref,int Tin)
{
	double Vin,RC;
	if((Tref == 0xffff)) {
		return 0.0;			//�Q�Ƒ��̌��ʂ��Ȃ��̂Ōv���o����.
	}
	if((Tin == 0xffff)) {
		return VOLT_VCC;	//TIMER1�ߊl�o����. VCC���킸���ɒ����Ă���\������.
	}
	RC  = (double) Tref / log( 1.0 - (VOLT_REF/VOLT_VCC) ) * -1.0;
	Vin = VOLT_VCC * (1.0 - exp( -1.0 * ((double) Tin / RC )));

#if	ANALOG_CALC_DEBUG
	{	double R,C,RC1;
		RC1 = RC / 12000000.0;	// �N���b�N��.
		R = 2.2 * 1000.0;	// R=2.2k���Ƃ���.
		C = RC1 / R;
		printf("\n   RC=%12g , C=%12g uF\n",RC1,C * 1000000.0);
	}
#endif

	return Vin;
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdQuestion(char *buf)
{
	int v;
	if( get_arg(buf) < 1) {
		cmdHelp("");
	}else{
		v = portAddress(arg_ptr[0]);
		if(v) printf("%s=0x%x\n",arg_ptr[0],v);
	}
}

void cmdPortPrintOne(char *name,int adrs,int val)
{
	char tmp[128];
	if(val & 0xffff0000) {
		printf("%16s(0x%08x) 0x%08x %s\n",name,adrs,val,radix2str(tmp,val));
	}else{
		printf("%16s(0x%08x)     0x%04x %s\n",name,adrs,val,radix2str(tmp,val));
	}
}

void cmdPortPrintOneCr(int count, char *name,int adrs,int val)
{
	char tmp[128];
	printf("%5d: %8s(%02x) %02x %s\r",count,name,adrs,val,radix2str(tmp,val));
}

void cmdPortPrintAllCr(int count, unsigned char *pinbuf)
{
#if	_AVR_PORT_
	char tmp1[32];
	char tmp2[32];
	char tmp3[32];
	int pina = pinbuf[9];
	int pinb = pinbuf[6];
	int pind = pinbuf[0];

	printf("%5d: PINA %02x %s | PINB %02x %s | PIND %02x %s \r"
		, count
		,pina,radix2str(tmp1,pina)
		,pinb,radix2str(tmp2,pinb)
		,pind,radix2str(tmp3,pind)
	);
#else
	char tmp1[128];
	char tmp2[128];
	char tmp3[128];
	int pina = pinbuf[0];
	int pinb = pinbuf[1];
	int pind = pinbuf[2];

	printf("%5d: PORTA %02x %s | PORTB %02x %s | PORTC %02x %s \r"
		, count
		,pina,radix2str(tmp1,pina)
		,pinb,radix2str(tmp2,pinb)
		,pind,radix2str(tmp3,pind)
	);
#endif
}

void cmdPortPrintOne_b(char *name,int adrs,int val,int mask)
{
	int bpos = 0;
	char bitlist[16] = "____ ____";
	if( mask==0) {
		cmdPortPrintOne(name,adrs,val);
	}else{
		bpos = get_bpos(mask);
		if(bpos<0) {
			cmdPortPrintOne(name,adrs,val);
		}else{
			bpos = 7 - bpos;
			if(bpos >= 4) bpos ++;

			if( val & mask ) bitlist[bpos]='1';
			else			 bitlist[bpos]='0';
			printf("%8s(%02x) %02x %s\n",name,adrs, val , bitlist);
		}
	}
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdPort(char *buf)
{
	int v,n,mask=0;
	int arena=MEM_WORD;	//�ǂݏo����.
	char *portname="(?)";
	n = get_arg(buf);	//�����̐�.
	if(n != 0) {
		v = portAddress_b(arg_ptr[0],&mask);	//�|�[�g���̂���A�h���X���Z�o.
		if(v) {		// �A�h���X�����݂���.
			portname=realPortName(arg_ptr[0]);
			arena   =type2size(getPortAttrib(arg_ptr[0]));	//�ǂݏo����.
			if(n < 2) {					// 1�̃|�[�g��ǂݎ��.
				int val = UsbPeek(v,arena);
				cmdPortPrintOne_b(portname,v,val,mask);
			}else{						// p <adrs> <data>
				int val;
				//UsbPoke_b(v,0,arg_hex[1],mask);				//��������.
				mask=0;
				UsbPoke(adrs,arena,adrs2,mask);		// adrs2 �͏������݃f�[�^.
				//printf("UsbPoke(%x,%x,%x,%x)\n",adrs,arena,adrs2,mask);
				val = UsbPeek(v,arena);
				cmdPortPrintOne_b(portname,v,val,mask);	//�ēǂݍ���.
			}
			return;
		}else if (strcmp(arg_ptr[0],"*")==0) {//�_���v�`���Ń|�[�g�\������.
			PrintPortAll(0);
			return;
		}else if (strcmp(arg_ptr[0],".")==0) {//�i�g�`���Ń|�[�g�\������.
			PrintPortColumn();
			return;
		}else if (strcmp(arg_ptr[0],"?")==0) {//�|�[�g���̂̈ꗗ���o��.
			PrintPortNameList();
			return;
		}else{
			suggestPortName(arg_ptr[0]);
			return;
		}
	}
	PrintPortAll(1);	//�悭�Q�Ƃ���|�[�g�����\������.
}
/*********************************************************************
 *	�|�[�g���e���|�[�����O����.
 *********************************************************************
 */
void cmdPoll(char *buf)
{
	char sample[256];
	int v,n,count,i;
	n = get_arg(buf);	//�����̐�.
	if(n != 0) {
		if(n < 2) {					// 1�̃|�[�g��ǂݎ��.
			count = 1000;
		}else{
			count = arg_hex[1];
		}

		v = portAddress(arg_ptr[0]);	//�|�[�g���̂���A�h���X���Z�o.
		if(v) {		// �A�h���X�����݂���.
			UsbSetPoll(v,POLL_PORT);
			for(i=0;i<count;i++) {
				int val = UsbPoll(sample);
				cmdPortPrintOneCr(count - i, arg_ptr[0],v,val);
				if(kbhit()) break;
			}
		}else{
			//	A,B,D�S�|�[�g�ǂ�.
			unsigned char portbuf[16];
#if	_AVR_PORT_
			int adr = 0x30;	// PORTD
			int size = 10;	// 0x30�`0x39
#else
			int adr = portAddress( "PORTA" );	// default PORTC
			int size = 10;	// 0x30�`0x39
#endif
			for(i=0;i<count;i++) {
				int rc = UsbRead(adr,arena,portbuf,size);
				if( rc != size) {
					printf("poll error %d.\n",rc);
					return;
				}
				cmdPortPrintAllCr(count - i, portbuf);
				if(kbhit()) break;
			}
		}
		UsbSetPoll(0,0);
	}
	printf("\n\n");
}

/*********************************************************************
 *
 *********************************************************************
 */
int	cmdExecTerminal(void)
{
	char buf[1024];
//	int addr = 0;
	int mode = 'p';
	int rc,len,key;

//	UsbSetPoll(addr,mode);

	while(1) {
		rc = UsbReadString(buf);
		if(rc == 0) {
		#if	1
			fprintf(stderr, "hidRead error\n");
			exit(EXIT_FAILURE);
		#else
			return -1;
		#endif
		}
		if(	buf[1] == mode) {
			len = buf[2] & 0xff;
			if(len) {
				buf[3+len] = 0;
				printf("%s",buf+3);
			}
		}
		if(	buf[1] == 'q') {
			len = buf[2] & 0xff;
			if(len) {
				buf[3+len] = 0;
				printf("%s",buf+3);
			}
			printf("\n* Program Terminated.\n");
			break;
		}
		if(kbhit()) {
			key = getch();
			if((key==KEY_ESC)||(key==KEY_CTRL_C)) {
				printf("\n* User Break. \n");
				break;
			}else{
				UsbPutKeyInput(key);
			}
		}
	}
	return 0;
}
/*********************************************************************
 *
 *********************************************************************
 */
static	int get_vector(int adr)
{
#ifdef	LPC2388
	return adr;
#else
	int buf[16/4];
	int rc = UsbRead(adr,0,(uchar*) buf,16);
	(void)rc;
	return buf[1];	// initial PC
#endif
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdBoot(char *buf)
{
	int n = get_arg(buf);	//�����̐�.
	int adr = 0;
	int vect= 0;
	if(n != 0) {
		adr = arg_hex[0];
	}
	vect = get_vector(adr);

	UsbBootTarget(vect,1);
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdRun(char *buf)
{
	int n = get_arg(buf);	//�����̐�.

	if(n) {
		printf("* Start Program at 0x%x (Hit 'ESC' to quit)\n",arg_hex[0]);
	}else{
		printf("* Start Program (Hit 'ESC' to quit)\n");
	}

//	UsbSetPoll(addr,mode);
//	hidReadPoll(buf , REPORT_LENGTH4 ,REPORT_ID4);	//�_�~�[���[�h.

	if(n != 0) {
		UsbBootTarget(arg_hex[0],0);
	}
	Sleep(10);
	cmdExecTerminal();
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdUser(char *buf)
{
	int adr=0;
	int n = get_arg(buf);	//�����̐�.
	if( n != 0 ) {
		adr = arg_val[0];
	}
	if(UsbGetDevCaps(NULL,NULL) != 0) {	// BOOTLOADER
		printf("Can't Start User Program(BOOTLOADER)\n");
		return;
	}

	printf("* Start Program (Hit 'ESC' to quit)\n");
	UsbExecUser(adr);
	Sleep(10);
	cmdExecTerminal();
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdSync(char *buf)
{
	int adr=1;
	int n = get_arg(buf);	//�����̐�.
	if( n != 0 ) {
		adr = arg_val[0];
	}
	UsbSyncCmd(adr);
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdErase(char *buf)
{
	int n = get_arg(buf);	//�����̐�.
	if(n != 0) {
		UsbEraseTargetROM(adrs,memcnt);
	}
}

/*********************************************************************
 *
 *********************************************************************
 */
void cmdFlash(char *buf)
{
	unsigned char data[128];
	int size = 32;
	int n = get_arg(buf);	//�����̐�.
	if(n != 0) {
		int i;
		for(i=0;i<64;i++) data[i]=i;

		UsbFlash(adrs,AREA_PGMEM,data,size);

	}
}

/*********************************************************************
 *	��r��Ƃq�b���萔���g�p�����A�A�i���O����(for deBUG)
 *********************************************************************
 */
void cmdAinDebug(char *buf)
{
	int n,mode=0xa0;
	unsigned char abuf[256];
	n = get_arg(buf);	//�����̐�.
	if(n != 0) {
		mode = 0xa0 + (arg_hex[0] & 0x0f);
	}

	UsbAnalogPoll(mode,abuf);
	printf("%02x %02x %02x %02x \n",abuf[0],abuf[1],abuf[2],abuf[3]);
}

/*********************************************************************
 *	��r��Ƃq�b���萔���g�p�����A�A�i���O����
 *********************************************************************
 */
int	getAnalogVolt(double *val,int f)
{
	int mode,Tref,Tin;
	unsigned char abuf[256];
	double volt;

#if	ANALOG_TREF_CONST		// A/D �ϊ���Vref�v�����ȗ�.
	Tref = ANALOG_TREF_CONST;
#else
//for(i=0;i<2;i++)
  {
	mode = AIN_MODE_VREF ;
	UsbAnalogPoll(mode,abuf);
	if( abuf[1] != mode ) {
		printf("analog input error. unimplemented protocol.\n");
		printf("%02x %02x %02x %02x \n",abuf[0],abuf[1],abuf[2],abuf[3]);
		return -1;
	}
	Tref = abuf[2] + (abuf[3]<<8);
  }
#endif

  {
	mode = AIN_MODE_AIN0 ;
	UsbAnalogPoll(mode,abuf);
	if( abuf[1] != mode ) {
		printf("analog input error. unimplemented protocol.\n");
		printf("%02x %02x %02x %02x \n",abuf[0],abuf[1],abuf[2],abuf[3]);
		return -1;
	}
	Tin  = abuf[2] + (abuf[3]<<8);
  }

	volt = AnalogCalcVoltage(Tref,Tin);

	if(f) {
		// ���ʕ\��printf ����.
		printf("ain0 = %12.3f (Tin=%5d/Tref=%5d) \r",volt,Tin,Tref);
//		printf("ain0 = %12.3g\r",volt);
	}
	if(val) {*val = volt;}
	return 0;
}

/*********************************************************************
 *	��r��Ƃq�b���萔���g�p�����A��R����
 *********************************************************************
 */
int	getRegistance(double *val,int f)
{
	int mode,Tref,Tin;
	unsigned char abuf[256];
	double regval;

//  for(i=0;i<2;i++) {
	mode = AIN_MODE_VREF ;
	UsbAnalogPoll(mode,abuf);
	if( abuf[1] != mode ) {
		printf("analog input error. unimplemented protocol.\n");
		printf("%02x %02x %02x %02x \n",abuf[0],abuf[1],abuf[2],abuf[3]);
		return -1;
	}
	Tref = abuf[2] + (abuf[3]<<8);
 // }

	mode = AIN_MODE_REGISTANCE ;
	UsbAnalogPoll(mode,abuf);
	if( abuf[1] != mode ) {
		printf("analog input error. unimplemented protocol.\n");
		printf("%02x %02x %02x %02x \n",abuf[0],abuf[1],abuf[2],abuf[3]);
		return -1;
	}
	Tin  = abuf[2] + (abuf[3]<<8);

	if( (Tref == 0xffff) ) {
		//��R �v�Z�o���Ȃ�.
		return -1;
	}
	if( (Tin == 0xffff) ) {
		regval = -1;	//�v���ł���.
	}else{
		regval = (double) REFERENCE_K_OHM * Tin / Tref;
		regval -= INTERNAL_K_OHM;
		if(regval<0) regval = 0;
	}

	if(f) {
		// ���ʕ\��printf ����.
		if( regval >= 0) {
			printf(" R = %10.3f K (Treg=%5d/Tref=%5d) \r",regval,Tin,Tref);
		}else{
			printf(" R = Open         (Treg=%5d/Tref=%5d) \r",Tin,Tref);
		}
	}
	if(val) {*val = regval;}
	return 0;
}


/*********************************************************************
 *	��r��Ƃq�b���萔���g�p�����A�A�i���O����
 *********************************************************************
 */
void cmdAin(char *buf)
{
	int n,cnt=1,i,mode;
	unsigned char abuf[256];
	double volt;
	n = get_arg(buf);	//�����̐�.
	if(n != 0) { cnt = arg_val[0]; }	// �ϊ���.

	for(i=0;i<cnt;i++) {
		getAnalogVolt(&volt,1);
		if(kbhit()) break;
	}
	mode = AIN_MODE_NONE;
	UsbAnalogPoll(mode,abuf);
	printf("\n");
}

/*********************************************************************
 *	��r��Ƃq�b���萔���g�p�����A��R����
 *********************************************************************
 */
void cmdReg(char *buf)
{
	int n,cnt=1,i,mode;
	unsigned char abuf[256];
	double registance;
	n = get_arg(buf);	//�����̐�.
	if(n != 0) { cnt = arg_val[0]; }	// �ϊ���.

	mode = AIN_MODE_AIN0 ;
	UsbAnalogPoll(mode,abuf);
	if( abuf[1] != mode ) {
		printf("analog input error. unimplemented protocol.\n");
		printf("%02x %02x %02x %02x \n",abuf[0],abuf[1],abuf[2],abuf[3]);
		return;
	}

	for(i=0;i<cnt;i++) {
		getRegistance(&registance,1);
		if(kbhit()) break;
	}
	mode = AIN_MODE_NONE;
	UsbAnalogPoll(mode,abuf);
	printf("\n");
}



void analize_infra(uchar *buf,int cnt);

/*********************************************************************
 *	�|�[�g���e���|�[�����O����.
 *********************************************************************
 */
#define	SCREEN_W	768
#define	SCREEN_X0	32
#define	SCREEN_H	480

#define	SCREEN_H2	16		//�J�[�\�����C���̒���.
//#define	H_HEIGHT	20		// L��H�̍��̍���.
#define	H_HEIGHT	10		// L��H�̍��̍���.

#define	SIG_COLOR	0x00f000
#define	SIG_COLOR1	0x008000
#define	SIG_COLOR2	0x004000
#define	BACK_COLOR	0x000000
#define	FRAME_COLOR	0x504000
#define	FRAME_COLOR2	0x304000

int	calc_ypos(int i)
{
	int off;
	int y1;

#if	0
	if(i>=4) off = 24;
	else off = 0;
	y1 = 60 + i*48 + off;
#else
	if(i>=8) off = 16;
	else off = 0;
	y1 = 60 + i*24 + off;
#endif
	return y1;
}

void vline2(int x0,int y0,int x1,int y1,int col)
{
	int yc;
/*
	int ytmp;
	if(y0>y1) {
		ytmp=y0;y0=y1;y1=ytmp;
	}
 */
	yc = (y0+y1)/2;
	gr_vline(x0,y0	,x0,yc,col);		// (����)�c�̐�1.
	gr_vline(x1,yc+1,x1,y1,col);		// (����)�c�̐�2.
}
#define	ANALOG_CENTER	(480/2)
#define	ANALOG_BASE		(ANALOG_CENTER-128)
/*********************************************************************
 *	1�T���v����1�r�b�g����`��
 *********************************************************************
 */
void plot_analog(int x,int y,int val,int old_val)
{
	int diff = old_val - val;
	int yc = 128+ANALOG_BASE;

	int y0 = 256+ANALOG_BASE - old_val;
	int y1 = 256+ANALOG_BASE - val;

	if(x==SCREEN_X0) {	//���[.
		gr_vline(x-1,ANALOG_BASE,x-1,256+ANALOG_BASE,0);
	}

	gr_pset(x,yc 	,FRAME_COLOR);			// �x�[�X���C����.
	gr_pset(x,yc-129,FRAME_COLOR2);			// H���x�����C����.
	gr_pset(x,yc+129,FRAME_COLOR2);			// L���x�����C����.

	if(diff) vline2(x-1,y0,x,y1,SIG_COLOR1);	// (����)�c�̐�.
	else	       gr_pset(x,y1,SIG_COLOR1);	// �M���̐F.

}

/*********************************************************************
 *	1�T���v����1�r�b�g����`��
 *********************************************************************
 *	v    : 0 �� ��0   �M�����x��.
 *  diff : 0 �� ��0   �M�����]���N����.
 */
void plot_signal(int x,int y,int v,int diff)
{
	int col0,col1,col2;

	if(v) {	col0 = FRAME_COLOR;	col1 = SIG_COLOR ; }
	else  {	col0 = SIG_COLOR;	col1 = BACK_COLOR ; }

	if(diff) {	col2 = SIG_COLOR2;}
	else	 {	col2 = BACK_COLOR;}

	if(diff) gr_vline(x,y-1,x,y- H_HEIGHT,col2);	//�c�̐�.
	if(v) {
		gr_pset(x,y - H_HEIGHT,col1);				// H���x���̐F.
	}else{
		gr_pset(x,y - 2 ,col0);						// L�̐F.
		gr_pset(x,y - 1 ,col0);						// L�̐F.
	}
	gr_pset(x,y  ,FRAME_COLOR);						// �x�[�X���C����.
	gr_pset(x,y- H_HEIGHT-1,FRAME_COLOR2);			// H���x�����C����.
}

/*********************************************************************
 *	1�T���v����`��.
 *********************************************************************
 */
void draw_sample(int x,int y,int val)
{
	static int old_val;

	if( poll_mode == POLL_ANALOG) {		// 0xa0
		plot_analog(x,y,val,old_val);
	}else{
		int i,y1;
		int m=1;
		int diff = val ^ old_val;	//�O��ƒl���ς����?
		for(i=0;i<16;i++,m<<=1) {
			y1 = calc_ypos(i);
			plot_signal(x,y+y1,val & m,diff & m);
		}
	}

	old_val = val;
}

/*********************************************************************
 *	�O���t�\�����̕���������.
 *********************************************************************
 */
void draw_TimeScale()
{
//	int i,y1=20,ms=0;
	int i,ms=0;
	char string[128];
//	int color = 0x80ff00;
	int x1=SCREEN_X0;
	for(i=0;i<8;i++) {
		sprintf(string,"%dmS",ms);
//		gr_puts(x1,y1,string,color,0,0);
		ms+=10;
		x1+=100;
	}
}

/*********************************************************************
 *	�O���t�\�����̕���������.
 *********************************************************************
 */
void draw_PortName(int adr,int poll_mode)
{
	int i,y1;
	char string[128];
	int color = 0x80ff00;

#if	_AVR_PORT_
	char *port="-";
	if(adr == 0x39) port = "A";
	if(adr == 0x36) port = "B";
	if(adr == 0x30) port = "D";
#else
	char port[16]="-";
	int port_a = portAddress( "PORTA" );
	if((adr - port_a)>=0) {
		if((adr - port_a)< 5) {
			port[0] = 'A' + (adr - port_a);
		}
	}
#endif

	if( poll_mode == POLL_ANALOG) {	// 0xa0
		int yc = 128+ANALOG_BASE-8;
		int y0 =     ANALOG_BASE-8;
		int y1 = 256+ANALOG_BASE-8;
		gr_puts(4,y0,"100",color,0,0);
		gr_puts(4,yc,"50" ,color,0,0);
		gr_puts(4,y1,"0"  ,color,0,0);
	}else
	for(i=0;i<16;i++) {
//		sprintf(string,"P%s%d",port,i);
		sprintf(string,"P%d" ,i);
		y1 = calc_ypos(i);
		gr_puts(4,y1-20,string,color,0,0);
	}
}

void analog_conv(uchar *sample,int poll_mode)
{
	// �O���t�������₷���悤�ɃA�i���O�T���v����10bit����8bit�ɗ��Ƃ�.
	int i,n,lo,hi,c;
	uchar *p,*t;
	if( poll_mode == POLL_ANALOG) {		// 0xa0
		n = sample[2]/2;
		sample[2]=n;
		p = &sample[3];
		t = &sample[3];
		for(i=0;i<n;i++) {
			lo = *p++;
			hi = *p++;
#if	0
			c = (hi<<8) | lo;
			c >>= 2;			//ADC���ʂ�10bit���E�l�̏ꍇ.
#else
			c = hi;				//ADC���ʂ�10bit�����l�̏ꍇ.
#endif
			*t++ = c;
		}
	}
}
#define 	ANALOG_TRIG_THR	16
int	analog_chk_trig(int val,int oldval,int trig_mode)
{
	int diff;
	if(trig_mode) {
		if(	val>oldval) {
			diff = val-oldval;
		}else{
			diff = oldval-val;
		}
		if(diff >= ANALOG_TRIG_THR) return 1;
	}
	return 0;
}
/*********************************************************************
 *	�O���t�\��.(��)
 *********************************************************************
 */
void cmdGraph(char *buf)
{
	uchar sample[256];
	int adr, i, n, oldval=0,infra=0;
	int freerun=1;			// �t���[�����A�������̓g���K�[������1,��~=�O.
	int x=SCREEN_X0;
	poll_mode=POLL_PORT;	// 0xc0
	trig_mode = 0;	// FreeRun


#if	_AVR_PORT_
	adr = 0x36;	// default PINB
#else
//	adr = portAddress( "PORTC" );	// default PORTC
	adr = portAddress( "gpioa.idr" );	// default PORTC
#endif

	n = get_arg(buf);	//�����̐�.
	if( n ) {
		adr = portAddress(arg_ptr[0]);	//�|�[�g���̂���A�h���X���Z�o.
		if(	adr==0) {
			adr = arg_hex[0];
		}
		if( strcmp(arg_ptr[0],"analog")==0) {
			poll_mode = POLL_ANALOG;	// 0xa0
		}
		if(n>=2) {
			if( strcmp(arg_ptr[1],"trig")==0) {
				trig_mode = 0xff;
				freerun=0;
				if(n==3) trig_mode = 1<<(arg_hex[2]);
			}
			if( strcmp(arg_ptr[1],"infra")==0) {
				trig_mode = 0x01;	//
				freerun=0;
				infra=1;
			}
		}
	}


	gr_init(SCREEN_W,SCREEN_H,32,0);
	draw_PortName(adr,poll_mode);
	draw_TimeScale();
	UsbSetPoll(adr,poll_mode);
	do {
		int cnt;
		int vcol, val;

		val = UsbPoll((char*)sample);
		analog_conv(sample,poll_mode);
		cnt = sample[2];

		for(i=0;i<cnt;i++) {
//			val = sample[3+i];
			if( poll_mode == POLL_ANALOG) {	// 0xa0
				if(analog_chk_trig(val,oldval,trig_mode)) freerun=1;
			}else{
			if((val ^ oldval) & trig_mode) freerun=1;
			}
			if(freerun) {
				if((x-SCREEN_X0) % 50) {
					vcol = 0;
				}else{
					vcol = FRAME_COLOR2;
				}
//				gr_vline(x,0,x,SCREEN_H-1,vcol);		//�c�̐�.
				gr_vline(x,40,x,SCREEN_H-1,vcol);		//�c�̐�.
				draw_sample(x,0,val);
				infra_sample[x-SCREEN_X0]=val;
				x++;
				if(x>=SCREEN_W) {
					x=SCREEN_X0;
					if(trig_mode) {
						freerun=0;
						if(infra) {
							analize_infra(infra_sample,SCREEN_W-SCREEN_X0);
						}
					}
				}
//				gr_vline(x,0,x,SCREEN_H-1,0xffff00);	//�c�̐�.
//				gr_vline(x,0,x,SCREEN_H2 ,0xffff00);	//�c�̐�.
				gr_vline(x,SCREEN_H-1-SCREEN_H2,x,SCREEN_H-1,0xffff00);	//�c�̐�.
			}
			oldval=val;
		}
	}while(gr_break()==0);
	UsbSetPoll(0,0);
	gr_close();
}
/*********************************************************************
 *	�O���t�\�����̕���������.
 *********************************************************************
 */
void draw_AnalogFrame(void);
void draw_AnalogInput(void);
/*********************************************************************
 *	�A�i���O���̓O���t�\��.(��)
 *********************************************************************
 */
void cmdAinGraph(char *buf);
void cmdRegGraph(char *buf);
/*********************************************************************
 *
 *********************************************************************
 */
void cmdQuit(char *buf)
{
	printf("Bye.\n");
	isExit = 1;
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdDump(char *buf)
{
	get_arg(buf);
	if(adrs==(-1)) {
		adrs = 0;
		return;
	}
	UsbDump(adrs,arena,(memcnt+7)&(-8));
	adrs += memcnt;
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdList(char *buf)
{
	int n,bytes;
	n = get_arg(buf);
	if(n<2) {
		memcnt = 40;	//�X�e�b�v�����ȗ����ꂽ��40�ɂ���.
	}

#ifdef	LPC2388
	int thumb = adrs & 1;
	arm_init(thumb);	//��A�h���X���^����ꂽ�ꍇ�AThumb���[�h�ؑ�.
	if(n<2) {
		if(adrs & 1) {
			memcnt = 40;	//�X�e�b�v�����ȗ����ꂽ��40�ɂ���.
		}else{
			memcnt = 80;	//�X�e�b�v�����ȗ����ꂽ��80�ɂ���.
		}
	}
#endif
	adrs &= (-2);		//�ꉞ�J�n�A�h���X�͋����ɂ���.

	bytes = UsbDisasm(adrs,arena,(memcnt+7)&(-8));
	adrs += bytes;

#ifdef	LPC2388
	adrs |= thumb;		//��A�h���X���^����ꂽ�ꍇ�AThumb���[�h��ێ�.
#endif
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdEdit(char *buf)
{
	int mask = 0;
	int argc = get_arg(buf);

	if( argc >= 2) {
		UsbPoke(adrs,arena,adrs2,mask);		// adrs2 �͏������݃f�[�^.
	}else
	if( argc >= 1) {
		int val = UsbPeek(adrs,arena);
		printf("%06x %02x\n",adrs,val);
	}
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdFill(char *buf)
{
	int i;
	if( get_arg(buf) >= 3) {
		for(i=adrs;i<=adrs2;i++) {
			UsbPoke(i,arena,arg_hex[2],0);
			if(arena) {
				Sleep(25);	// EEPROM�ɑ΂���Fill �͑҂������Ȃ��ƃn���O����.
			}
		}
	}
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdBench(char *buf)
{
	int cnt = 1000;
	int packet = 8;
	int argc = get_arg(buf);

	if(argc >= 1) {
		cnt = arg_val[0];
	}
	if(argc >= 2) {
		packet = arg_val[1];
	}
//	printf("%d,%d\n",cnt,packet);
	UsbBench(cnt,packet);
}
/*********************************************************************
 *
 *********************************************************************
// ATtiny USART Baud Rate Register High UBBRH[11:8]
#define UBRRH   _SFR_IO8(0x02)

// ATtiny USART Control and Status Register C UCSRC
#define UCSRC   _SFR_IO8(0x03)

#define UMSEL   6
#define UPM1    5
#define UPM0    4
#define USBS    3
#define UCSZ1   2
#define UCSZ0   1
#define UCPOL   0

	UCSRC = (1<<USBS)|(3<<UCSZ0);     �t���[���`���ݒ�(8�r�b�g,2�X�g�b�v �r�b�g)

#define BAUD 9600             	// �ړIUSART�{�[���[�g���x
#define MYUBRR FOSC/16/BAUD-1 	// �ړIUBRR�l

 */

#define FOSC 12000000         	// MCU�N���b�N���g��

/*********************************************************************
 *
 *********************************************************************
 */
void cmdSleep(char *buf)
{
	int cnt = 1000;
	int argc = get_arg(buf);
	if(argc >= 1) {
		cnt = arg_val[0];
	}
	Sleep(cnt);
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdEcho(char *buf)
{
	int argc = get_arg(buf);
	if(argc >= 1) {
		if(str_comp(arg_ptr[0],"on")==0)  {echo_mode = 1;}
		if(str_comp(arg_ptr[0],"off")==0) {echo_mode = 0;}
	}
}
/*********************************************************************
 *
 *********************************************************************
 */
void cmdLabel(char *buf)
{
	/* none */
}
static void get_line(char *buf,int echo);

/*********************************************************************
 *	�X�N���v�g���s���̃��x����(�擪����)�T��.
 *********************************************************************
 *�߂�l:
 *	0	��������.
 *	-1	������Ȃ�.
 *	-2	script���s���łȂ��̂�LABEL�����ł��Ȃ�.
 */
int	find_script_label(char *label)
{
	char *t;
	int argc;
	if( script_fp != NULL) {
		fseek(script_fp,0,SEEK_SET);		//�擪�܂Ŋ����߂�.

		while(1) {
			get_line(cmdbuf,0);	// label ��T��.

			t = NULL;
			if( str_comp(cmdbuf,"label")==0 ) {t = cmdbuf+strlen("label");}
			if( str_comp(cmdbuf,":")==0 )     {t = cmdbuf+strlen(":");}
			if( t ) {
				argc = get_arg(t);
				if(argc >= 1) {
					if(strcmp(arg_ptr[0],label)==0) {
						return 0;		// OK.
					}
				}
			}
		}
		return -1;
	}
	return -2;
}

/*********************************************************************
 *	loop LABEL [count];   �蔲�������ł�.
 *********************************************************************
 */
void cmdLoop(char *buf)
{
	int argc = get_arg(buf);
	int cnt = 1000000;
	int rc  = 0;
	char label[256]="";
	
	if(argc >= 1) {
		strcpy(label,arg_ptr[0]);	//�R�s�[���Ă�������.
	}
	if(argc >= 2) {
		cnt = arg_val[1];
	}
	if(loopcnt==0) {
//		printf("*loopcnt=%d\n",cnt);
		loopcnt = cnt-1;	//	�������:���[�v�J�E���^��ݒ肷��.
	}else{
		loopcnt--;
//		printf("*--loopcnt=%d\n",loopcnt);
		if(loopcnt <= 0) return;	// loop �I��!!.	
	}

	// goto LABEL �̎��s.
	rc = find_script_label(label);
	if(rc== (-1)) {
		printf("label '%s' not found.",label);
	}
	if(rc== (-2)) {
		printf("error:Can't exec 'loop' command.\n");
	}
}
/*********************************************************************
 *
 *********************************************************************
 */


CmdList cmdlist[]={
	{	"h"	,	cmdHelp },
	{	"?"	,	cmdQuestion},
	{	"bench",cmdBench},
	{	"sleep",cmdSleep},
	{	"poll", cmdPoll },
	{	"graph",cmdGraph },
	{	"reggraph",cmdRegGraph },
	{	"aingraph",cmdAinGraph },
	{	"aindebug",cmdAinDebug },
	{	"ain",  cmdAin },
	{	"reg",  cmdReg },
	{	"exit",	cmdQuit },
	{	"boot",	cmdBoot },
	{	"run",	cmdRun  },
	{	"user",	cmdUser },
	{	"sync",	cmdSync },
	{	"erase",cmdErase},
	{	"flash",cmdFlash},
	{	":"    ,cmdLabel},
	{	"label",cmdLabel},
	{	"loop" ,cmdLoop},
	{	"echo" ,cmdEcho},

	{	"q"	,	cmdQuit },
	{	"d"	,	cmdDump },
	{	"m"	,	cmdEdit },
	{	"e"	,	cmdEdit },
	{	"f"	,	cmdFill },
	{	"l"	,	cmdList },
	{	"p"	,	cmdPort },
	{	NULL,	cmdQuit },
};



/*********************************************************************
 *	���j�^�R�}���h��͂Ǝ��s.
 *********************************************************************
 */
void cmdMonit(char *buf)
{
	CmdList *t;
	buf = sp_skip(buf);
	if(*buf==0) return;
	for(t = cmdlist; t->name != NULL ; t++) {
		if( str_comp(buf,t->name)==0 ) {
			buf += strlen(t->name);
			t->func(buf);
			return;
		}
	}
	/*error*/
}
/*********************************************************************
 *	�s����CR/LF ���폜����.
 *********************************************************************
 */
void chop_crlf(char *p)
{
	while(*p) {
		if((*p==0x0d)||(*p==0x0a)) { *p = 0; return; }
		p++;
	}
}

/*********************************************************************
 *	1�s����. �t�@�C���������̓R���\�[���̗��������e.
 *********************************************************************
 */
static void get_line(char *buf,int echo)
{
	char *rc;
	if( script_fp != NULL) {
		rc = fgets(buf,MAX_CMDBUF,script_fp);		//�t�@�C���������.
		if(rc == NULL) {		//�t�@�C�����������̓G���[.
			buf[0]=0;			//��s��Ԃ�.
			fclose(script_fp);
			if(echo) printf("\n");		//�R�}���h�G�R�[����.
			script_fp = NULL;
			echo_mode = 1;
		}else{
			chop_crlf(buf);		//�s����CR/LF ���폜����.
			if(echo) printf("%s\n",buf);	//�R�}���h�G�R�[����.
		}
	}else{
		rc = fgets(buf,MAX_CMDBUF,stdin);		//�R���\�[���������.
		chop_crlf(buf);		//�s����CR/LF ���폜����.
		if (log_mode) {
			if(echo) printf("%s\n",buf);	//�R�}���h�G�R�[����.
		}
	}
}



/*********************************************************************
 *	���C��
 *********************************************************************
 */
int main(int argc,char **argv)
{
	//�I�v�V�������.
	Getopt(argc,argv,"i");
	if(IsOpt('h') || IsOpt('?') || IsOpt('/')) {
		usage();
		exit(EXIT_SUCCESS);
	}
	if(IsOpt('i')) {
		script_fp = fopen( Opt('i') , "rb" );
	}
	if(IsOpt('l')) {
		log_mode = 1;
	}
	if(IsOpt('v')) {	//�f�o�C�X�����_���v����.
		verbose_mode = 1;
	}
	strcpy(usb_serial, DEFAULT_SERIAL);		// �ȗ���
	if(IsOpt('p')) {
		char *s;
		s = Opt('p');
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

#if 1	/* @@@ by senshu (for tee cmommand) */
      setvbuf( stdout, NULL, _IONBF, 0 );
#endif

#ifdef	LPC2388
	arm_init(0);
#else
	arm_init(1);
#endif
	//������.
#if 1
	{	int retry;
		for(retry=3;retry>=0;retry--) {
			//������.
			if( UsbInit(verbose_mode, 0, usb_serial) == 0) {
				if (verbose_mode) {
					fprintf(stderr, "Try, UsbInit(\"%s\").\n", usb_serial);
				}
				if(retry==0) {
					fprintf(stderr, "%s: UsbInit failure.\n", "picmon");
					exit(EXIT_FAILURE);
				}
			}else{
				break;
			}
			Sleep(2500);
		}
	}
#else
	if( UsbInit(verbose_mode, 0, usb_serial) == 0) {
		if (!verbose_mode) {
			fprintf(stderr, "picmon: UsbInit(\"%s\") failure.\n", usb_serial);
		}
		exit(EXIT_FAILURE);
	}
#endif
#if 0	//_AVR_PORT_	// add by senshu
#define PIND	0x30
	{
		int pind;
		pind =  hidPeekMem(PIND);
		if (pind & (1<<2)){		// PD2 check
			printf("HIDaspx is Programmer mode.\n");
		} else {
			printf("HIDaspx is USB-IO mode.\n");
		}
	}
#else

#if	0
	// PIC18F�p
	{
		if( UsbGetDevID() == DEV_ID_PIC18F14K) {
			ChangePortList14K();
			port_p4550 = 0;
		}else{
			port_p4550 = InitPortList();	// 2550 / 4550 �̑I��.
		}
	}
#endif

#endif

	do {					//���j�^�[���[�h.
		if(echo_mode) {
			printf("ARM> ");
		}
		get_line(cmdbuf,echo_mode);
		cmdMonit(cmdbuf);
	} while(isExit == 0);

	UsbExit();

	if(script_fp!=NULL) fclose(script_fp);

	return EXIT_SUCCESS;
}
/*********************************************************************
 *
 *********************************************************************
 */

