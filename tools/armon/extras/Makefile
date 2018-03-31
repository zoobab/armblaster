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

stm32:
	perl stm2port.pl -stm32 >a.cpp
	g++ a.cpp 
	./a.out >a.txt
	perl stm2port.pl -stm32 -a >a.h
	cp a.h ../portlist_stm32.h

lpc13xx:
	perl stm2port.pl -lpc13xx >a.cpp
	g++ a.cpp 
	./a.out >a.txt
	perl stm2port.pl -lpc13xx -a >a.h

#################################################################
clean:
	-rm -f *.o

cleanall:
	-rm -f *.o $(PROGRAM1) $(PROGRAM2)

