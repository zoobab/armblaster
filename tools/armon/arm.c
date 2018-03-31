/**********************************************************************
 *	ARM DISASM
 **********************************************************************
 */

#include "std.h"
#include <stdarg.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include "ansidecl.h"
#include "dis-asm.h"
#include "symbol.h"

typedef	unsigned char uchar;

int		set_force_thumb(int f);
int		is_crlf(void);
void 	reg_symbol(SYMBOL *symbol);
void 	init_symbol(void);

long	lpeek(long adr);
//int	print_insn_little_arm (long memaddr,struct disassemble_info *info);

struct 	disassemble_info info;
long	filesize;
struct  stat statbuf;
long	startaddr=0;
long	endaddr=0;
int		bigarm=0;
int		is_thumb =0;


char	 hbuf[256];		// 00000000 00000000
unsigned int		 hbuf_adr;
unsigned int		 hbuf_dat;

char	 lbuf[1024];		//					 mov	ax,3
char	 remarkbuf[1024];	//									;ÉRÉÅÉìÉg.
char	*lbufp;

SYMBOL  lsymbol;
char    label[80];

#define	INS_BUFSIZE	16
static	uchar	ins_buf[INS_BUFSIZE];
static	long	ins_adr=0;

void pr_rem(char *fmt , ... )
{
    va_list args;
    va_start(args,fmt);

    vsprintf(remarkbuf , fmt, args);

    va_end(args);
}

static void prfun(FILE *stream,char *fmt, ...)
{
    va_list args;
    va_start(args,fmt);

    vsprintf(lbufp,fmt,args);

    lbufp+=strlen(lbufp);

    va_end(args);
}

static	unsigned long	chkbig(unsigned long data)
{
	if(bigarm) {
		return	(((data <<24L) & 0xff000000L) |
				 ((data << 8L) & 0x00ff0000L) |
				 ((data >> 8L) & 0x0000ff00L) |
				 ((data >>24L) & 0x000000ffL) );
	}
	return data;
}


static	int	memfun(long adr,long *buf)
{
	int  off;
	if(	adr >= ins_adr) {
		off = adr - ins_adr;
		if(	off < 16 ) {
			memcpy(buf,ins_buf+off,sizeof(long));
			hbuf_adr =  adr;
			hbuf_dat = *buf;
			return 0;
		}
	}
	return 1;
}

#if	0
static	long	seek_oldpos = (-1);
static	void	OptSeek(pos)
{
	if(	seek_oldpos != pos) {
		seek_oldpos = pos;
		Seek(ifp,pos);
	}
}
static	int	memfun(long adr,long *buf)
{
	long data;

	if( (adr>=startaddr) && (adr<endaddr) ) {
		OptSeek(adr - startaddr);
		Read(&data,sizeof(long));
		seek_oldpos += sizeof(long);

		*buf = data;
//		sprintf(lbufp,"%08x %08x\t",adr,chkbig(data));lbufp+=strlen(lbufp);
		hbuf_adr = adr;
		hbuf_dat = data;
		return 0;
	}else{
		return 1;
	}
}
#endif

static	void	errfun()
{
	//printf("errfun()\n");
}

static	int	pr_addr(long addr)
{
#if	0
	if(find_symbol_by_adrs(&lsymbol,addr)) {
		char *name=lsymbol.name;
		char *p = label;
		while(*name) *p++ = *name++;
					 *p++ = 0;
		sprintf(lbufp," ;%s\n",label);lbufp+=strlen(lbufp);
	}
#endif
	return 0;
}

void set_ea(char *buf,long addr)
{
#if	0
	if(find_symbol_by_adrs(&lsymbol,addr)) {
		char *name=lsymbol.name;
		char *p = label;
		while(*name) *p++ = *name++;
					 *p++ = 0;
		sprintf(buf,"%s",label);
	}else
#endif
	{
		sprintf(buf,"0x%08lx",addr);
	}
}


static	void	prafun(long addr)
{
	sprintf(lbufp,"$%08lx",addr);lbufp+=strlen(lbufp);
	pr_addr(addr);
}


static	void	dislarm_init()
{
	info.stream				= NULL;
	info.fprintf_func       = (fprintf_ftype) prfun;
	info.read_memory_func   = (int (*)()) memfun;
	info.memory_error_func  = errfun;
	info.print_address_func = (void (*)())prafun;
}

int	dislarm(long memaddr)
{
	int oplen;
	lbufp=lbuf;
	remarkbuf[0]=0;

#if	0
	if(find_symbol_by_adrs(&lsymbol,memaddr)) {
		char *name=lsymbol.name;
		char *p = label;
		while(*name) *p++ = *name++;
					 *p++ = 0;
		printf("                  %s:\n",label);
	}
#endif

	if(bigarm) {
		oplen = print_insn_big_arm (memaddr,&info);
	}else{
		oplen = print_insn_little_arm (memaddr,&info);
	}
	
	if(is_thumb) {
		if(oplen==2) {
			sprintf(hbuf ,"%08lx %04x     "	,memaddr,hbuf_dat & 0xffff);
		}else{
			sprintf(hbuf ,"%08lx %04x %04x"	,memaddr,hbuf_dat & 0xffff,hbuf_dat>>16);
		}
	}else{
			sprintf(hbuf ,"%08lx %08lx "	,memaddr,chkbig(hbuf_dat));
	}
	
	if(remarkbuf[0]) {
		printf("%s\t%-48s %s\n"	,hbuf,lbuf,remarkbuf);
	}else{
		printf("%s\t%s\n"		,hbuf,lbuf);
	}
	if(is_crlf()) {
		printf("\n");
	}
//	oplen = info.bytes_per_line;
	return oplen;
}

bfd_vma		bfd_getl32(const unsigned char *buffer)
{
	unsigned long *l=(unsigned long *) buffer;
	return *l;
}

bfd_vma		bfd_getb32(const unsigned char *buffer)
{
	return (((unsigned long )buffer[0]<<24L) |
			((unsigned long )buffer[1]<<16L) |
			((unsigned long )buffer[2]<< 8L) |
			((unsigned long )buffer[3]     ) )
	;
}

int	arm_init(int isthumb)
{
	is_thumb=isthumb;
	dislarm_init();
	set_force_thumb(is_thumb);
	return 0;
}

int disasm_print(uchar *buf,int size,int adr)
{
	if(	size >=INS_BUFSIZE) {
		size = INS_BUFSIZE;
	}
	memcpy(ins_buf,buf,size);
	ins_adr = adr;
//	printf("adr=%x ",adr);
	return dislarm(adr);
}
