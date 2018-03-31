/********************************************************************
 *	�ȈՃ��j�^
 ********************************************************************
 */

#include "stm32f10x.h"
#include "platform_config.h"
#include "hw_config.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_pwr.h"
#include "stm32_eval.h"
#include "monit.h"

#if	0
#include "usb/typedefs.h"
#include "usb/usb.h"
#include "io_cfg.h"
#include "monit.h"
#endif

#include "hidcmd.h"
#include "picwrt.h"
#include "gpio.h"
/********************************************************************
 *	
 ********************************************************************
	// PICwriter��p.
	struct{
		uchar  piccmd;
		uchar  picsize;
		uchar  picadrl;
		uchar  picadrh;
		uchar  picadru;
		uchar  piccmd4;
		uchar  picms;
		uchar  picdata[32];
	};
 */
enum CMD_4 {
	b_0000 = 0x00,
	b_0001 = 0x01,
	b_1000 = 0x08,
	b_1001 = 0x09,
	b_1100 = 0x0c,	//
	b_1101 = 0x0d,	// TBLPTR+=2
	b_1111 = 0x0f,	// Write!
};


// JTAG =================================
#define	TDI		PGM	 
#define	TCK		PGC	 
#define	TDO		PGD	 
#define	TMS		MCLR 
//	JTAG �|�[�g�ǂݍ���.
#define	inTDO	inPGD	

#define	JCMD_BITCOUNT_MASK	0x3f
#define	JCMD_LAST_TMS		0x40
#define	JCMD_INIT_TMS		0x80


/********************************************************************
 *	�f�[�^
 ********************************************************************
 */
//#pragma udata access accessram

//	�R�}���h�̕ԐM���K�v�Ȃ�1���Z�b�g����.
extern	uchar ToPcRdy;

//#pragma udata SomeSectionName1
extern	Packet PacketFromPC;			//���̓p�P�b�g 64byte

#if defined(__18F14K50)
extern	near Packet PacketToPC;				//�o�̓p�P�b�g 64byte
#else
//#pragma udata SomeSectionName2
extern	Packet PacketToPC;				//�o�̓p�P�b�g 64byte
#endif

#define	Cmd0	PacketFromPC.cmd

//#define	set_bit(Port_,val_) 
//				Port_ = val_ 

#define	set_bit(Port_,val_) 	digitalWrite(Port_,val_)

void set_dir(int Port,int val)
{
	if(val) {
		pinMode(Port,INPUT);
	}else{
		pinMode(Port,OUTPUT);
	}
}
/********************************************************************
 *	24bit�A�h���X��8bit �R�ɕ�������union.
 ********************************************************************
 */
typedef	union {
	dword adr24;
	uchar adr8[4];
} ADR24;

typedef	union {
	ushort adr16;
	uchar  adr8[2];
} ADR16;


/********************************************************************
 *	ISP���W���[�� �C���N���[�h.
 ********************************************************************
 */
#if	0
//#pragma code

/*********************************************************************
 *	�P�ʕb�P�ʂ̎��ԑ҂�.
 *********************************************************************
 *	Windows���ł�USB�t���[�����Ԃ̏����������̂ŁA���ۂɂ͕s�v.
 *	�}�C�R���t�@�[�����ɈڐA����Ƃ��̓}�C�R�����ł��ꂼ������̕K�v������.
 */
static	void wait_us(uchar us)
{
	do {
		_asm 
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		_endasm;
	} while(--us);
}
//static	
void wait_ms(uchar ms)
{
	do {
		wait_us(250);
		wait_us(250);
		wait_us(250);
		wait_us(250);
	}while(--ms);
}
#endif
//
//	0  �`200   : 0�`200�}�C�N���b�҂�.
//	201�`250   : 1�` 50�~���b�҂�.
//
//
static	void wait_ums(uchar us)
{
	if(us) {
		if(us>200) 	wait_ms(us-200);
		else 		wait_us(us);
	}else{
		wait_u8s(8);	// 1/8 uS �҂�.
	}
}

static	int  getword(uchar *t)
{
	return t[0] | (t[1]<<8);
}

static	void setword(uchar *t,int d)
{
	t[0] = d;
	t[1] = d>>8;
}
#if	0
static	long getlong(uchar *t)
{
	return 	t[0] | (t[1]<<8) | (t[2]<<16) | (t[3]<<24);
}
#endif
/*********************************************************************
 *	1bit��M.
 *********************************************************************
        +---+
  PGC --+   +---

           �� �����œǂ�.

  PGD [ ����    ]
 */
#if	0
inline uchar GetCmdb1(void)
{
	uchar b=0;
	set_bit(PGC,1);
	wait_us(1);
//	b = get_port(Pinb) & (1<<PGD);
	if(inPGD) b=1;
	set_bit(PGC,0);
	wait_us(1);
	return b;
}
#endif
/*********************************************************************
 *	n bit ���M.
 *********************************************************************
 */
void SetCmdN(ushort cmdn,uchar len)
{
	uchar i;
	for(i=0;i<len;i++) {
		if(cmdn&1)	set_bit(PGD,1);
		else		set_bit(PGD,0);
		set_bit(PGC,1);
		wait_us(1);
		cmdn >>=1;
		set_bit(PGC,0);
	}
}

/*********************************************************************
 *	4 bit ���M , �Ō�� PGC_H���Ԃ��w��.
 *********************************************************************
 */
void SetCmdNLong(uchar cmdn,uchar len,uchar ms)
{
	uchar i;	//,b;
	for(i=0;i<(len-1);i++) {
//		b = cmdn & 1;
//		SetCmdb1(b);
//		cmdn >>=1;
		{
			if(cmdn&1)	set_bit(PGD,1);
			else		set_bit(PGD,0);
			set_bit(PGC,1);
			wait_us(1);
			cmdn >>=1;
			set_bit(PGC,0);
		}
	}
	{
//		SetCmdb1Long(cmdn & 1,ms);
		{
			if(cmdn&1)	set_bit(PGD,1);
			else		set_bit(PGD,0);
			set_bit(PGC,1);
			if(ms) wait_ms(ms);
			wait_us(1);
			cmdn >>=1;
			set_bit(PGC,0);
		}
	}
}

/*********************************************************************
 *	PGD�r�b�g�̓��o�͕�����؂�ւ���.(0=����)
 *********************************************************************
 *	f=0 ... PGD=in
 *	f=1 ... PGD=out
 */

void SetPGDDir(uchar f)
{
	if(f) {
//		dirPGD=0;	// out
		pinMode(dirPGD,OUTPUT);
	}else{
//		dirPGD=1;	// in
		pinMode(dirPGD,INPUT);
	}
}


/*********************************************************************
 *	4bit�̃R�}���h �� 16bit�̃f�[�^�𑗐M.
 *********************************************************************
 */
void SetCmd16(uchar cmd4,ushort data16,uchar ms)
{
	SetCmdNLong(cmd4,4,ms);
	SetCmdN(data16,16);
}

/*********************************************************************
 *	TBLPTR[U,H,L] �̂ǂꂩ��8bit�l���Z�b�g����.
 *********************************************************************
 */
static	void SetAddress8x(uchar adr,uchar inst)
{
	SetCmd16(b_0000,0x0e00 |  adr,0);		// 0E xx  : MOVLW xx
	SetCmd16(b_0000,0x6e00 | inst,0);		// 6E Fx  : MOVWF TBLPTR[U,H,L]
}
/*********************************************************************
 *	TBLPTR ��24bit�A�h���X���Z�b�g����.
 *********************************************************************
 */
void setaddress24(void)
{
	SetAddress8x(PacketFromPC.picadru,0xf8);	// TBLPTRU
	SetAddress8x(PacketFromPC.picadrh,0xf7);	// TBLPTRH
	SetAddress8x(PacketFromPC.picadrl,0xf6);	// TBLPTRL
}

/*********************************************************************
 *	TBLPTR ����P�o�C�g�ǂݏo��. TBLPTR�̓|�X�g�E�C���N�������g�����.
 *********************************************************************
 */
static	uchar GetData8(void)
{
	uchar i,data8=0;

	SetCmdN(b_1001,4);
	SetCmdN(0 , 8);

//	return pic18_GetData8b();
/*********************************************************************
 *	8 bit ��M.	 LSB�t�@�[�X�g.
 *********************************************************************
static	uchar pic18_GetData8b(void)
{
 */

	SetPGDDir(0);		// PGD=in
//	for(i=0;i<8;i++,mask<<=1) {
//		if( GetCmdb1() ) {
//			data8 |= mask;
//		}
	i=8;do {
		set_bit(PGC,1);
		data8>>=1;
		wait_us(1);
//		if(inPGD) {
		if(digitalRead(inPGD)) {
			data8 |= 0x80;
		}
		set_bit(PGC,0);
		wait_us(1);
	}while(--i);
	SetPGDDir(1);		// PGD=out
	return data8;
}


#if	0
void GetData8L(int adr,uchar *buf,int len)
{
	int i,c;
	SetAddress24(adr);
	for(i=0;i<len;i++) {
		c = GetData8();
		*buf++ = c;
//		if(dumpf)printf(" %02x",c);
	}
}
#endif


#if	0
/*********************************************************************
 *	4bit�̃R�}���h �� 16bit�̃f�[�^�𑗐M.
 *********************************************************************
 */
void SetCmd16L(uchar cmd4,ushort *buf,uchar len,uchar ms)
{
	uchar i;
	ushort data16;
	for(i=0;i<len;i++) {
		data16 = *buf++;
		SetCmdNLong(cmd4,4,ms);
		SetCmdN(data16,16);
	}
}
#endif
/********************************************************************
 *	16bit�f�[�^���(size)���[�h����������.
 ********************************************************************
 *	cmd4 �́A�e16bit�f�[�^���������ޑO�ɗ^����4bit�R�}���h.
 *	ms   �́A4bit�R�}���h�̍ŏIbit���o��ɉ��~���b�҂ׂ����̒l.
 *	ms=0 �̂Ƃ��� 1uS �҂�.
 */
void pic_setcmd16L(void)
{
	uchar len = PacketFromPC.picsize;
	uchar cmd4= PacketFromPC.piccmd4;
	uchar ms  = PacketFromPC.picms;
	uchar i;
//	ushort *s = (ushort *) PacketFromPC.picdata;
	uchar *s = PacketFromPC.picdata;
	ushort data16;
	for(i=0;i<len;i++) {
//		data16 = *s++;
		data16 = getword(s);s+=2;
		SetCmdNLong(cmd4,4,ms);
		SetCmdN(data16,16);
	}
}
/********************************************************************
 *	24bit�A�h���X���Z�b�g���āA 8bit�f�[�^���(size)�o�C�g���擾����.
 ********************************************************************
 */
void pic_getdata8L(void)
{
	uchar i,len;

	setaddress24();

	len =PacketFromPC.picsize;
	for(i=0;i<len;i++) {
		PacketToPC.raw[i] = GetData8();
	}
}
/********************************************************************
 *	PIC�v���O�������[�h
 ********************************************************************
 */
#if	0
void pic_setpgm(void)
{
	uchar f=PacketFromPC.picsize;

	if(f) {
		//�S�Ă̐M����L�ɂ���.
		set_bit(PGM,0);
		set_bit(PGD,0);
		set_bit(PGC,0);
		set_bit(MCLR,0);

	dirPGM=0;
	dirPGC=0;
	dirMCLR=0;

	SetPGDDir(1);		// PGD=out

		wait_ms(1);
		set_bit(PGM,1);	// PGM=H
		wait_ms(1);
		set_bit(MCLR,1);// MCLR=H
		wait_ms(1);
	}else{
		set_bit(PGD,0);// MCLR=L
		set_bit(PGC,0);// MCLR=L
		wait_ms(1);
		set_bit(MCLR,0);// MCLR=L
		wait_ms(1);
		set_bit(PGM,0);	// PGM=L
		wait_ms(1);
		//�S�Ă̐M����L�ɂ���.
		set_bit(PGM,0);
		set_bit(PGD,0);
		set_bit(PGC,0);
		set_bit(MCLR,0);

	dirPGM=1;
	dirPGC=1;
	dirMCLR=1;

	SetPGDDir(0);		// PGD=out
	}
}
#endif
/********************************************************************
 *	PIC24�v���O�������[�h
 ********************************************************************
 */
void pic_bitbang(void)
{
	uchar cnt =PacketFromPC.raw[1];
	uchar wait=PacketFromPC.raw[2];
	uchar *p =&PacketFromPC.raw[3];
	uchar *t =&PacketToPC.raw[0];
	uchar c;
	while(cnt) {
		c = *p++;
//	�|�[�g�o�̓f�[�^.    
		set_bit(MCLR,c & 0x08);
		set_bit(PGM ,c & 0x04);
		set_bit(PGD ,c & 0x02);
		set_bit(PGC ,c & 0x01);
//	�������W�X�^ (0=�o��)
		set_dir(dirMCLR,c & 0x80);
		set_dir(dirPGM ,c & 0x40);
		set_dir(dirPGD ,c & 0x20);
		set_dir(dirPGC ,c & 0x10);
		if(Cmd0==PICSPX_BITREAD) {
			c = 0;
			if(digitalRead(inMCLR)) c|=0x08;
			if(digitalRead(inPGM))  c|=0x04;
			if(digitalRead(inPGD))  c|=0x02;
			if(digitalRead(inPGC))  c|=0x01;
			*t++ = c;
		}
#if	0
//	�|�[�g�o�̓f�[�^.    
		if( (c & 0x08)) MCLR = 1;
		if(!(c & 0x08)) MCLR = 0;
		if( (c & 0x04)) PGM = 1;
		if(!(c & 0x04)) PGM = 0;
		if( (c & 0x02)) PGD = 1;
		if(!(c & 0x02)) PGD = 0;
		if( (c & 0x01)) PGC = 1;
		if(!(c & 0x01)) PGC = 0;

//	�������W�X�^ (0=�o��)
		if( (c & 0x80)) dirMCLR = 1;
		if(!(c & 0x80)) dirMCLR = 0;
		if( (c & 0x40)) dirPGM = 1;
		if(!(c & 0x40)) dirPGM = 0;
		if( (c & 0x20)) dirPGD = 1;
		if(!(c & 0x20)) dirPGD = 0;
		if( (c & 0x10)) dirPGC = 1;
		if(!(c & 0x10)) dirPGC = 0;
		
		if(Cmd0==PICSPX_BITREAD) {
			c = 0;
			if(inMCLR)c|=0x08;
			if(inPGM) c|=0x04;
			if(inPGD) c|=0x02;
			if(inPGC) c|=0x01;
			*t++ = c;
		}
#endif
		wait_ums(wait);
		wait_us(1);
		cnt--;
	}
}

#if	INCLUDE_JTAG_CMD
static void set_tms(int tms)
{
	set_bit(TMS ,tms);
#if	0
	if(tms) {
		TMS=1;
	}else{
		TMS=0;
	}
#endif
}

//	LSB first.
void jtag_command()
{
	uchar *p = &PacketFromPC.raw[1];	// jtag �X�g���[��(for WRITE)
	uchar *q = &PacketToPC.raw[0];		// jtag �X�g���[��(for READ)

	uchar   cnt,c; 						// ��������r�b�g��.
	uchar 	bitcnt=0;					// cnt ��mod 7 ������.
	uchar 	tdi_byte=0;
	uchar 	tdo_byte=0;

	uchar 	jcmd;						// jtag command
	uchar 	maxcnt;
	uchar   mask_bit=1;					// �����l=1 �A���� ���V�t�g�A8bit���������� 1�ɖ߂�.

  while(1) {
	jcmd =*p++;						// jtag command
	if(jcmd==0) return;
	maxcnt=jcmd & JCMD_BITCOUNT_MASK;
	if(jcmd & JCMD_INIT_TMS) {
		for(cnt=0;cnt<maxcnt;cnt++) {
			c = *p++;

			set_bit(MCLR,c & 0x80);
			set_bit(PGM ,c & 0x40);
			set_bit(PGC ,c & 0x10);
			wait_ums(0);
	
			set_bit(MCLR,c & 0x08);
			set_bit(PGM ,c & 0x04);
			set_bit(PGC ,c & 0x01);
			wait_ums(0);
	
#if	0
//	�|�[�g�o�̓f�[�^. 
//			ISP_OUT = tdi_byte & 0xf0;
			if( (c & 0x80)) MCLR = 1;
			if(!(c & 0x80)) MCLR = 0;
			if( (c & 0x40)) PGM = 1;
			if(!(c & 0x40)) PGM = 0;
			if( (c & 0x10)) PGC = 1;
			if(!(c & 0x10)) PGC = 0;

			wait_ums(0);

//			tdi_byte <<=4;
//			ISP_OUT = tdi_byte;
			if( (c & 0x08)) MCLR = 1;
			if(!(c & 0x08)) MCLR = 0;
			if( (c & 0x04)) PGM = 1;
			if(!(c & 0x04)) PGM = 0;
			if( (c & 0x01)) PGC = 1;
			if(!(c & 0x01)) PGC = 0;
			wait_ums(0);
#endif

		}
	}else{

	bitcnt=0;					// cnt ��mod 7 ������.
	tdi_byte=0;
	tdo_byte=0;
	mask_bit=1;				// �����l=1 �A���� ���V�t�g�A8bit���������� 1�ɖ߂�.

	set_tms(0);

	for(cnt=0;cnt<maxcnt;cnt++) {

//	==== TCK = LOW ====
//		ISP_OUT &= ~(1 << TCK);		/* TCK low */
//		TCK=0;
		set_bit(TCK ,0);

		if( bitcnt ==0 ) {		// fetch data.
			tdi_byte = *p++;
			tdo_byte = 0;
			mask_bit  = 1;
		}

		set_bit(TDI ,tdi_byte & mask_bit);

#if	0
		if(tdi_byte & mask_bit) {
//			ISP_OUT |=  (1 << TDI);	/* TDI High */
			TDI=1;
		}else{
//			ISP_OUT &= ~(1 << TDI);	/* TDI low */
			TDI=0;
		}
#endif
		if(cnt == (maxcnt-1)) {
			set_tms(jcmd & JCMD_LAST_TMS);
		}
//		if( inTDO != 0) {
		if(digitalRead(inTDO)) {
			tdo_byte |= mask_bit;
		}

//	==== TCK = HIGH ====
//		ISP_OUT |=  (1 << TCK);		/* TCK High */
//		TCK=1;
		set_bit(TCK ,1);

		mask_bit <<=1;
		bitcnt++;
		if( bitcnt == 8 ) {			// store data.
			*q++ = tdo_byte;
			bitcnt=0;
		}
	}
	*q = tdo_byte;
  	}
  }
}
#endif


#if	SUPPORT_PIC24F	

/*********************************************************************
 *	�P�N���b�N���� PGD ���M
 *********************************************************************

   PGD <�f�[�^1bit>
   
   PGC _____|~~~~~~
 */
/********************************************************************
 *	PIC24F:�C��bit(�ő�8bit)��PGD�f�[�^�𑗂�.
 ********************************************************************
 */
void add_data(uchar c,uchar bits)
{
	uchar b;
	for(b=0;b<bits;b++) {	// LSB���� bits �r�b�g����.
		set_bit(PGC ,0);
//		PGC = 0;

		set_bit(PGD ,c & 1);
//		if( (c & 1)) PGD = 1;
//		if(!(c & 1)) PGD = 0;

		wait_us(1);
		c>>=1;
		set_bit(PGC ,1);
//		PGC = 1;
		wait_us(1);
	}
}
/********************************************************************
 *	PIC24F: 4bit����PGD�f�[�^�𑗂�.
 ********************************************************************
 */
void add_data4(uchar c)
{
	add_data(c,4);
}
/********************************************************************
 *	PIC24F: 8bit����PGD�f�[�^�𑗂�.
 ********************************************************************
 */
void add_data8(uchar c)
{
	add_data(c,8);
}
#if	0
/********************************************************************
 *	PIC24F: 3byte����PGD�f�[�^�𑗂�.
 ********************************************************************
 */
void add_data24(uchar u,uchar h,uchar l)
{
	add_data4(b_0000);	// send SIX command '0000'
	add_data8(l);
	add_data8(h);
	add_data8(u);
	set_bit(PGC ,0);
	//PGC = 0;
}
#endif
/********************************************************************
 *	PIC24F: 24bit����PGD�f�[�^�𑗂�.
 ********************************************************************
 */
void sendData24(dword c)
{
	ADR24 adr;
	adr.adr24 = c;
	add_data4(b_0000);	// send SIX command '0000'
	add_data8(adr.adr8[0]);	// l
	add_data8(adr.adr8[1]);	// h
	add_data8(adr.adr8[2]);	// u
	set_bit(PGC ,0);
//	PGC = 0;
}
void send_nop1()
{
	sendData24(0x000000);				// NOP
}
void send_nop2()
{
	send_nop1();
	send_nop1();
}
void send_goto200()
{
	sendData24(0x040200);				// GOTO 0x200
	send_nop1();
}

#define	W6	6
#define	W7	7

/********************************************************************
 *	PIC24F: ���W�X�^�y�A (W0,reg) �Ƀz�X�gPC�w��̃A�h���X24bit��ݒ�
 ********************************************************************
 *	�z�X�gPC���ŁA���炩���߃A�h���X�� 1/2�ɂ��Ă�������(device adr��).
 */
void send_addr24(uchar reg)
{
	ADR24 adr;

	adr.adr8[0] = PacketFromPC.picadru;
	adr.adr8[1] = 0;
	adr.adr8[2] = 0;
	adr.adr8[3] = 0;

	sendData24(0x200000| ( adr.adr24 <<4) );	// MOV  #adr_h, W0
	sendData24(0x880190);				// MOV  W0, TBLPAG

	adr.adr8[0] = PacketFromPC.picadrl;
	adr.adr8[1] = PacketFromPC.picadrh;

	sendData24(0x200000|reg| ( adr.adr24 <<4) );	// MOV  #adr_l, W6
}

/*********************************************************************
 *	���ʃ��[�`���E�A�h���X�Z�b�g�A�b�v
 *********************************************************************
 */
static	void setup_addr24()		//int adr)
{
	send_nop1();
	send_goto200();
	send_addr24(W6);
	sendData24(0x207847);				// MOV  #VLSI, W7
	send_nop1();
}
static	void setup_addr24write(void)	//int adr,int nvmcon)
{
	send_nop1();
	send_goto200();
	sendData24(0x24001A);				// MOV  #0x4001, W10
	sendData24(0x883B0A);				// MOV  W10, NVMCON
	send_addr24(W7);
}
static	void bset_nvmcon(void)
{
	send_nop2();
	sendData24(0xA8E761);					// BSET NVMCON
	send_nop2();
}
/*********************************************************************
 *	8bit�f�[�^�̎�M
 *********************************************************************
   
   PGC _____|~~~~~~

   PGD <�f�[�^1bit>
			 ^
			 |
		�T���v�����Opoint
 */
static	uchar pic24_getData8(void)
{
	uchar c=0;
	uchar b;

//	PGC = 0;
	set_bit(PGC ,0);
	for(b=0;b<8;b++) {	// LSB ���� 8 �r�b�g�󂯎��.
		
		//PGC = 1;
		set_bit(PGC ,1);
		c>>=1;

		wait_us(1);
//		if(inPGD) {
		if(digitalRead(inPGD)) {
			c|=0x80;
		}
		//PGC = 0;
		set_bit(PGC ,0);
		wait_us(1);
	}
	return c;
}
/*********************************************************************
 *	16bit�f�[�^�̎�M
 *********************************************************************
 */
static	int	pic24_getData16(void)
{
	ADR16 adr;
	adr.adr8[0]=pic24_getData8();
	adr.adr8[1]=pic24_getData8();
	return adr.adr16;
}
/*********************************************************************
 *	16bit�f�[�^�̎�M
 *********************************************************************
 */
int	recvData16(void)
{
	int rc;
	add_data4(b_0001);	// send SIX command '0000'
	add_data8(0x00  );	// send 1 byte '0000_0000'

//	PGC=0;
	set_bit(PGC ,0);

//	dirPGD=1;			// INPUT
	set_dir(dirPGD ,1);
	
	wait_us(1);
	rc = pic24_getData16();
	
//	dirPGD=0;			// OUTPUT
	set_dir(dirPGD ,0);
	wait_us(1);
	send_nop1();
	return rc;
}


static	void WaitComplete(void)
{
	int r;
	bset_nvmcon();
	do {
		send_goto200();
		sendData24(0x803B00);	// MOV NVMCON, W0
		sendData24(0x883C20);	// MOV W0, VLSI
		send_nop1();
		r = recvData16();
	} while (r & 0x8000);
}
/********************************************************************
 *	PIC24F:Flash��������
 ********************************************************************
 */
void pic_write24f(void)
{
	uchar i;
	uchar cmd4=PacketFromPC.piccmd4;	// 0=address setup����. 1=�p���������� 2=complete��������.
	uchar len =PacketFromPC.size;		// ���[�v��. ���f�[�^byte�� / 6 �Ɠ���.
//	ushort *wordp=(ushort *)PacketFromPC.picdata;
	uchar *wordp = PacketFromPC.picdata;

	if(cmd4==0) {
		setup_addr24write();	//(adr>>1,0x4001);
	}

	for(i=0;i<len;i++) {
		sendData24(0xEB0300);					// CLR W6
		sendData24(0x200000| (getword(wordp+0)<<4) );	// MOV  #w0, W0
		sendData24(0x200001| (getword(wordp+2)<<4) );	// MOV  #w1, W1
		sendData24(0x200002| (getword(wordp+4)<<4) );	// MOV  #w2, W2
		sendData24(0xBB0BB6);					// TBLWTL [W6++], [W7]
		send_nop2();
		sendData24(0xBBDBB6);					// TBLWTH.B [W6++], [W7++]
		send_nop2();
		sendData24(0xBBEBB6);					// TBLWTH.B [W6++], [++W7]
		send_nop2();
		sendData24(0xBB1BB6);					// TBLWTL [W6++], [W7++]
		send_nop2();
//		wordp+=3;
		wordp+=3*2;
	}
	
	if(cmd4==2) {
		WaitComplete();
		send_goto200();
	}
}
/********************************************************************
 *	PIC24F:Flash�ǂݏo��
 ********************************************************************
 */
void pic_read24f(void)
{
	uchar i;
	uchar len=PacketFromPC.size;
//	ushort *wordp=(ushort *)PacketToPC.raw;
	uchar *wordp=PacketToPC.raw;

	setup_addr24();			//(adr>>1);

	for(i=0;i<len;i++) {
		sendData24(0xBA0B96);				// TBLRDL [W6], [W7]
		send_nop2();
//		wordp[0] = recvData16();
		setword(wordp , recvData16());

		sendData24(0xBADBB6);				// TBLRDH.B [W6++], [W7++]
		send_nop2();
		sendData24(0xBAD3D6);				// TBLRDH.B [++W6], [W7--]
		send_nop2();
//		wordp[1] = recvData16();
		setword(wordp+2 , recvData16());

		sendData24(0xBA0BB6);				// TBLRDL [W6++], [W7]
		send_nop2();
//		wordp[2] = recvData16();
		setword(wordp+4 , recvData16());

//		wordp +=  3;
		wordp +=  3*2;
	}
}

#endif


/********************************************************************
 *	PIC���C�^�[�n�R�}���h��M�Ǝ��s.
 ********************************************************************
 *	Cmd0 : 0x10�`0x1F
 */
void cmd_picspx(void)
{
	if(		Cmd0==PICSPX_SETCMD16L) {pic_setcmd16L();}
	else if(Cmd0==PICSPX_SETADRS24) {setaddress24();}
	else if(Cmd0==PICSPX_GETDATA8L) {pic_getdata8L();}
	else if(Cmd0==PICSPX_BITBANG) 	{pic_bitbang();}
	else if(Cmd0==PICSPX_BITREAD) 	{pic_bitbang();}
#if	SUPPORT_PIC24F
	else if(Cmd0==PICSPX_WRITE24F) 	{pic_write24f();}
	else if(Cmd0==PICSPX_READ24F) 	{pic_read24f();}
#endif

#if	INCLUDE_JTAG_CMD
	else if(Cmd0==HIDASP_JTAG_WRITE){jtag_command();}
	else if(Cmd0==HIDASP_JTAG_READ) {jtag_command();}
#endif
//	else if(Cmd0==PICSPX_SETPGM) 	{pic_setpgm();}

	ToPcRdy = Cmd0 & 1;	// LSB��On�Ȃ�ԓ����K�v.
}


