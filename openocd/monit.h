#ifndef	_include_monit_h_
#define	_include_monit_h_

//#include "../firmware/hidcmd.h"
#include "hidcmd.h"

//  BitBang�f�[�^�r�b�g.

#define	bitMCLR	0x08	// TMS 
#define	bitPGM	0x04	// TDI MOSI
#define	bitPGD	0x02	// TDO MISO
#define	bitPGC	0x01	// TCK

#define	bitTMS	0x08	// TMS 
#define	bitTDI	0x04	// TDI MOSI
#define	bitTDO	0x02	// TDO MISO
#define	bitTCK	0x01	// TCK

//	�������W�X�^ (0=�o��)

#define	dirTMS	0x80
#define	dirTDI	0x40
#define	dirTDO	0x20	// ����PIN�̂ݓ���(==1)
#define	dirTCK	0x10

typedef	unsigned short ushort;
typedef	unsigned char uchar;

typedef	struct {
	uchar report_id;
	uchar cmd;
	uchar size;		// bit7:6 ��arena�Ɏg�p����\��.
	uchar adr[2];
	uchar data[48];
} cmdBuf;

//
//	cmd.
//
enum {
	CMD_PING 	= HIDASP_TEST,
	CMD_SETPAGE = HIDASP_SET_MODE,
	CMD_POKE = HIDASP_POKE,
	CMD_PEEK = HIDASP_PEEK,
};

//
//	area.
//
enum {
	AREA_RAM    = 0   ,
	AREA_EEPROM = 0x40,
	AREA_PGMEM  = 0x80,
//	reserved      0xc0
	AREA_MASK	= 0xc0,
	SIZE_MASK	= 0x3f,
};


typedef	struct {
	char *name;
	void (*func)();
} CmdList;


/*
 *	�I�v�V����������`�F�b�N
 */
#ifdef MAIN_DEF
char  *opt[128];	/* �I�v�V�����������w�肳��Ă�����A
						���̕����ɑ�����������i�[�A
						�w�肪�Ȃ����NULL�|�C���^���i�[	*/
#else
extern char  *opt[128];
#endif

/*
 *	�I�v�V����������`�F�b�N
 *		optstring �Ɋ܂܂��I�v�V���������́A
 *				  �㑱�p�����[�^�K�{�Ƃ݂Ȃ��B
 */
#define Getopt(argc,argv,optstring)           		\
 {int i;int c;for(i=0;i<128;i++) opt[i]=NULL; 		\
   while( ( argc>1 )&&( *argv[1]=='-') ) {    		\
	 c = argv[1][1] & 0x7f;   						\
       opt[c] = &argv[1][2] ; 						\
       if(( *opt[c] ==0 )&&(strchr(optstring,c))) {	\
         opt[c] = argv[2] ;argc--;argv++;          	\
       }                        					\
     argc--;argv++;           						\
 } }

#if	0
#define Getopt(argc,argv)  \
 {int i;for(i=0;i<128;i++) opt[i]=NULL; \
   while( ( argc>1 )&&( *argv[1]=='-') ) \
    { opt[ argv[1][1] & 0x7f ] = &argv[1][2] ; argc--;argv++; } \
 }
#endif

#define IsOpt(c) ((opt[ c & 0x7f ])!=NULL)
#define   Opt(c)   opt[ c & 0x7f ]

#define Ropen(name) {ifp=fopen(name,"rb");if(ifp==NULL) \
{ printf("Fatal: can't open file:%s\n",name);exit(1);}  \
}

#define Wopen(name) {if(strcmp(name, "con")==0) ofp=stdout; else {ofp=fopen(name,"wb");if(ofp==NULL) \
{ printf("Fatal: can't create file:%s\n",name);exit(1);}}  \
}

#if 0
#define Read(buf,siz)   fread (buf,1,siz,ifp)
#define Write(buf,siz)  fwrite(buf,1,siz,ofp)
#endif
#define Rclose()  fclose(ifp)
#define Wclose()  fclose(ofp)


#define	ZZ	printf("%s:%d: ZZ\n",__FILE__,__LINE__);

/* monit.c */
char *sp_skip (char *buf);
double AnalogCalcVoltage (int Tref, int Tin);
int analog_chk_trig (int val, int oldval, int trig_mode);

int calc_ypos (int i);
int cmdExecTerminal (void);
int getAnalogVolt (double *val, int f);
int getRegistance (double *val, int f);
int get_arg (char *buf);
int is_space (int c);
int main (int argc, char **argv);
int radix2scanf (char *s, int *p);
int str_comp (char *buf, char *cmd);

void analog_conv (uchar *sample, int poll_mode);
void chop_crlf (char *p);
void cmdAin (char *buf);
void cmdAinDebug (char *buf);
void cmdBench (char *buf);
void cmdBoot (char *buf);
void cmdDump (char *buf);
void cmdEdit (char *buf);
void cmdErase (char *buf);
void cmdFill (char *buf);
void cmdFlash (char *buf);
void cmdGraph (char *buf);
void cmdHelp (char *buf);
void cmdList (char *buf);
void cmdMonit (char *buf);
void cmdPoll (char *buf);
void cmdPort (char *buf);
void cmdPortPrintAllCr (int count, unsigned char *pinbuf);
void cmdPortPrintOne (char *name, int adrs, int val);
void cmdPortPrintOneCr (int count, char *name, int adrs, int val);
void cmdPortPrintOne_b (char *name, int adrs, int val, int mask);
void cmdQuestion (char *buf);
void cmdQuit (char *buf);
void cmdReg (char *buf);
void cmdRun (char *buf);
void cmdSleep (char *buf);
void cmdSync (char *buf);
void cmdUser (char *buf);
void draw_PortName (int adr, int poll_mode);
void draw_sample (int x, int y, int val);
void plot_analog (int x, int y, int val, int old_val);
void plot_signal (int x, int y, int v, int diff);
void scan_args (int arg_cnt);
void usage (void);
void vline2 (int x0, int y0, int x1, int y1, int col);

#ifdef	_LINUX_
//	MSDOS�ɑ��݂���֐��̃_�~�[�ł��v���g�^�C�v�錾����:
void Sleep(int mSec);			// wait mSec.
void strupr(char *s);			// �������S���啶����.
int	stricmp(char *s1,char *s2);	// �啶���������̋�ʂ����Ȃ��������r.
int getch(void);
int kbhit(void);

#endif


#endif	//_include_monit_h_

