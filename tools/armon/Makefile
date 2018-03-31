#################################################################
#	Windows用です.
#
#	コンパイラは MinGw gccを使用します。
#	試してはいませんがcygwinでも大丈夫なはずです。
#	(cygwinでは、コンパイルオプション -mno-cygwinを付けます)
#	DEBUGの時には、-gをつけ、-sを外します。
#################################################################
# REMOVE DEFAULT MAKE RULE
MAKEFLAGS = -r



#============================
# DOSかどうかチェック.
 ifdef ComSpec
MSDOS=1
 endif

 ifdef COMSPEC
MSDOS=1
 endif
#============================

 ifdef MSDOS
USBFLAGS = -D_MSDOS_
EXE_SUFFIX = .exe
ICON_O	= hidasp.o icon.o
WIN32LIB= -lkernel32 -luser32 -lgdi32 -lsetupapi 
 else
USBFLAGS = -D_LINUX_
EXE_SUFFIX =
ICON_O	= hidasp_unix.o info.o
WIN32LIB= -lusb
 endif

# LPC1343のときのみ有効に.
#BOARDFLAG= -DLPC1343
#BOARDFLAG= -DLPC2388

#============================
CC = gcc
RC = windres

CFLAGS	= $(USBFLAGS) $(BOARDFLAG) -O2 -Wall
LIBS	= $(USBLIBS)

TARG1 = armon
TARG2 = armboot

#################################################################
#	armon :
OBJS1 = monit.o analog.o util.o gr.o portlist.o infra.o arm.o armdis.o $(ICON_O)


#################################################################
#	armboot :
OBJS2 = picboot.o util.o $(ICON_O)

#
PROGRAM1 = $(TARG1)$(EXE_SUFFIX)
PROGRAM2 = $(TARG2)$(EXE_SUFFIX)

#################################################################
#
all: $(PROGRAM1) $(PROGRAM2)

.c.o:
	$(CC) $(CFLAGS) -c $<

icon.o:	icon.rc
	$(RC) -i icon.rc -o icon.o

#################################################################
# Debug時は -s を指定しない
#
$(PROGRAM1): $(OBJS1)
	$(CC) -s -o $(PROGRAM1) $(OBJS1) $(WIN32LIB) -lm $(LIBS)

$(PROGRAM2): $(OBJS2)
	$(CC) -s -o $(PROGRAM2) $(OBJS2) $(WIN32LIB) -lm $(LIBS)


#################################################################
clean:
	-rm -f *.o

cleanall:
	-rm -f *.o $(PROGRAM1) $(PROGRAM2)

