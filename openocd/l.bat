:L1
rem 
rem �J�����gDIR�ɂ��� main-0000.hex��STM8S-D��STM32CPU�ɏ�������Ŏ����I�ɏI������.
rem
rem 
openocd.exe -f blaster.cfg -f stm32.cfg -f batch.cfg

rem openocd.exe -f blaster.cfg -f lpc2378.cfg -f batch.cfg

sleep 10
goto L1

rem pause