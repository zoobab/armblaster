#
#  Makefile for MinGW
#
#
#COPT = -g -O2
COPT = -s -O2 -Os
#
#files	=	disarm.o	symbol.o	arm-dis.o
files	=	disarm.o	symbol.o	armdis.o

TARGET = disarm.exe
#
#
$(TARGET) :  $(files)
	gcc $(COPT) -mno-cygwin -o disarm.exe $(files)
#
test:
	./$(TARGET)	main.bin >test.dis
#
testt:
	./$(TARGET) -t -s8003000 main-3000.bin >test.dis
#
.c.o:
	gcc $(COPT) -Wall -c $*.c
#

clean:
	-rm *.o
	-rm *.bak
	-rm test.dis
#
