�� �T�v

   ����́AOpenOCD �� DLL �n�b�N�i�����j�ł��B
   �����A�O���� DLL ��u���āAJTAG�A�_�v�^�[�̃h���C�o�[�̕������������݂Ă��܂��B

�� ���݂̃X�e�[�^�X

   STBEE MINI ���g�p����JTAG�A�_�v�^�[�������Ă��܂��B


�� ������

   WindowsXP��p���܂��B

   ���Larmblaster�t�@�[�����Ă���STBEE��ƁA
   JTAG���g�p�ł���K����ARM����ȉ��̂悤�ȑΉ���JTAG�ڑ����Ă����܂��B

   armblaster/firmware/main-2000.hex

 �����́A
     STBEE MINI      �^�[�Q�b�gARM��� JTAG�[�q
           PB11  ------------- TCK
           PB10  ------------- TDI
           PB9   ------------- TDO
           PB8   ------------- TMS
 �ƂȂ�܂��B����ȊO��nTRST�s���Ȃǂ͂܂��T�|�[�g���Ă��܂���B(USB-Blaster�Ɠ��l)
 �܂��A��L�����͒����ł͂Ȃ��P�O�O�����x�̒�R����Đڑ�����悤�ɂ��Ă��������B

   armblaster/openocd/ �f�B���N�g���� ocd.bat ���邢�́A ocd2.bat ���N�����āAopenocd.exe�̓f���o��
   ���b�Z�[�W���m�F���邱�Ƃ��o���܂��B

   ����ɐڑ��o���Ă���悤�ł�����Atelnet�� localhost:4444 �Ԃɐڑ����āAOpenOCD�R�}���h�����s����
   �݂Ă��������B


�� �f�B���N�g���\��

 armblaster-+
            |
            +- firmware\       STBEE MINI�p�t�@�[���E�F�A.
            +- HW\             STBEE MINI�p�t�@�[���E�F�A�̃r���h�ɕK�v�ȃ��C�u����
            +- inc\            STBEE MINI�p�t�@�[���E�F�A�̃r���h�ɕK�v�ȃw�b�_�[
            |
            +- openocd -+      STBEE MINI�p openocd.exe �� hidblast.dll�\�[�X.
            |           |
            |           +--helper\   �w�b�_�[�t�@�C��.
            |           +--jtag\     �w�b�_�[�t�@�C��.
            |           |
            |           +--openocd_patch\  openocd�{�̑��쐬�p�̉����_
            |
            |
            |
            +- tools ---+--armon\      armon (����m�F�p���j�^)
                        |
                        +--avrwriter\  AVR���C�^�[ (HIDaspx�݊�)
                        |
                        +--picwriter\  PIC���C�^�[ (pic18spx�݊�)
                        |
                        +--picwriter24\ PIC24F���C�^�[
                        |
                        +--bin\        ��Ltools�̎��s�t�@�C���݂̂����^


�� �v���O�����̍ăr���h���@

   WindowsXP���MinGW-gcc�R���p�C����p���� openocd/ �f�B���N�g���ɂ� make���Ă��������B
   make����ƁAhidblast.dll ���쐬����܂��B

   openocd.exe�{�̂��ăr���h������@�́A�ȉ���URL���Q�Ƃ��Ă��������B

-http://hp.vector.co.jp/authors/VA000177/html/2010-09.html
   
   ����̉��������\�[�X��openocd_patch/ �f�B���N�g���ɒu���Ă��܂��B

   Linux��ł̃r���h�I�v�V�����́A����Ȋ����ł��B
   $ ./configure \
       --build=i686-pc-linux-gnu \
       --host=i586-mingw32msvc \
       --enable-maintainer-mode \
       --enable-dummy

   �o���オ���� openocd.exe �{�̂́A�h���C�o�[�Ƃ��āA����f�B���N�g���ɑ��݂��� hidblast.dll ��
   �N�����ɌĂяo���܂��B(���݂��Ȃ���΁Adummy�h���C�o�[�݂̂��g�ݍ��܂�܂�)


�� ����̖��_

   HID�f�o�C�X�Ȃ̂Œx���ł��B
   Flash�̏������݂����x���ł��B

   USB�f�o�C�X�̕����񖼏̂� "ARM32spx" --> "ARMblast" �ɕύX�ɂȂ��Ă��܂��̂ŁA
   armon���g�p����ꍇ�́A���̔z�z�t�@�C������armon.exe ���g�p���Ă��������B

   (���̕ύX�̗��R�́A�������݃^�[�Q�b�g�̃t�@�[���E�F�A��armon/armboot�̏ꍇ��
    ������ɐڑ�����Ă��܂��듮�삷��̂ŕύX���܂����B)


�� ���C�Z���X

   OpenOCD�̔z�z���C�Z���X�ɏ����܂��B


�� �W�]

   hidblast.dll �t�@�C����(���͂�)�����ւ��邾���ŁA����f�o�C�X���T�|�[�g�\�ɂȂ�܂��B
   �i���Ƃ���ATtiny2313���g�p����JTAG�A�_�v�^�[�Ȃǂ��T�|�[�g�o����\��������܂��j

   hidblast.dll �̃G���g���[�|�C���g�́A
      DLL_int get_if_spec(struct jtag_command **q);
   �����ł��B������struct jtag_command **q��q�ɂ́Aopenocd�{�̂�jtag_command_queue�Ƃ���
   �O���[�o���ϐ��̃A�h���X��n���܂��B
   �߂�l�́A(int�ɂȂ��Ă��܂���) �h���C�o�[�L�q�\���̂̃A�h���X�ɂȂ�܂��B


�� ARM�ȊO�̏������݃^�[�Q�b�g�ɂ���
-�����armblaster�́A[[pic18spx]]�x�[�X�̔ėp���C�^�[��JTAG�R�}���h��t�������������̂��̂ł��̂ŁA
-������ATMEL��AVR�`�b�v��Microchip��PIC18F�V���[�Y�A�����Ĉꕔ��PIC24F�V���[�Y�ւ̏������݋@�\������Ă��܂��B
-�R�}���h���C���ŏ������݃c�[����p�ӂ��܂����B
 armblaster/tools/avrwriter/
                 |
                 /picwriter/
                 |
                 /picwriter24/

-AVR,PIC18F���Ă��ꍇ�̌����}
     STBEE MINI    ARM�^�[�Q�b�g��JTAG�[�q    PIC18F   AVR 
           PB11  ------------- TCK ---------- PGC  --- SCK
           PB10  ------------- TDI ---------- PGM  --- MOSI
           PB9   ------------- TDO ---------- PGD  --- MISO
           PB8   ------------- TMS ---------- MCLR --- RESET
-��L�����͒����ł͂Ȃ�STBEE MINI����͂P�O�O�����x�̒�R����Đڑ�����悤�ɂ��Ă��������B
-PIC18F�͒�d���������݂݂̂��T�|�[�g���܂��B���d���i9V�`12V �f�o�C�X�ɂ���ĈقȂ�܂�)�������݂͏o���܂���B

�� �Q�l
-AVR-ISP�p:�s���w�b�_�[����ォ�猩���z�u
    5    3    1
  +----+----+----+
  |Rset|SCK |MISO|
  +----+----+----+
  |GND |MOSI|Vcc |
  +----+----+----+
    6    4    2
-PIC18F��ISP�ɂ����̂܂܎g���܂킵
    5    3    1
  +----+----+----+
  |MCLR|PGC |PGD |
  +----+----+----+
  |GND |PGM |Vcc |
  +----+----+----+
    6    4    2
-������܂�ARM(JTAG)�ɂ��g���܂킵
    5    3    1
  +----+----+----+
  |TMS |TCK |TDO |
  +----+----+----+
  |GND |TDI |Vcc |
  +----+----+----+
    6    4    2



�� STBEE MINI�ȊO�̊�ւ̈ڐA�ɂ��āB
-STM8S-D,CQ-STARM,STBEE,STBEE MINI�̂S�@��̊�ɂ��Ă͑S��[[armon/armboot>armon]]���ڐA�ς݂ł��B
-�Ȃ̂ŁAarmblaster�̈ڐA���ȒP�ɏo����Ǝv���܂��B
-JTAG,AVR-ISP,PIC18�������ݗp�̒[�q(STBEE MINI�ł�PB8�`PB11)�����ꂼ��̊�̓s���̗ǂ��ʒu�Ɋ���t�����Ƃ́A�����炭��`�t�@�C�������������邾���ōς݂܂��B(�|�[�g�̐����Arduino����digitalRead(pin)��digitalWrite(pin,level)�ōs�Ȃ��Ă��܂��̂�
�Apin�̒�`���ς�邾���őΉ��\�ł�)
-Makefile�ɂ͂��łɊ�̎�ʒ�`���܂܂�Ă��܂��̂Ŋ����define�����������邾���ł��݂܂��B

-�������A''FRISK�P�[�X�ɓ�����''�͍��̂Ƃ���STBEE MINI���������Ή��ł��܂���B



�� JTAG�R�}���h�̒ǉ��ƃv���g�R���ɂ���

-hidcmd.h
 #define HIDASP_JTAG_WRITE	  0x18	//JTAG ��������.
 #define HIDASP_JTAG_READ	  0x19	//JTAG �ǂݏ���.
-���ǉ�����Ă܂��B

-HidReport�̉���X�g���[��(PC->AVR) �T�C�Y�͍ő�64�o�C�g�܂łł�.
 +------+------+-------------------+------+-------------------+------+-------------+-----+
 | 0x18 | jcmd |  data��           | jcmd |  data��           | jcmd |  data��     | 0x00|
 +------+------+-------------------+------+-------------------+------+-------------+-----+

--jcmd��1�o�C�g�͈ȉ��̂悤�ɒ�`�i���̂P�j
 bit 7   6   5   4   3   2   1   0  
   +---+---+---+---+---+---+---+---+
   | 0 | b6| JTAG�]��bit��(TDI�̐�)|   �{ JTAG�]��bit������ TDI�r�b�g�iLSB�t�@�[�X�g�j  
   +---+---+---+---+---+---+---+---+
--- b6��TDI���o�̍ŏIbit��TMS��1�ɂ���Ȃ�1 ���Ȃ��Ȃ� 0 �F TMS�͍ŏIbit�ȊO�͏펞0

--jcmd��1�o�C�g�͈ȉ��̂悤�ɒ�`�i���̂Q�j
 bit 7   6   5   4   3   2   1   0  
   +---+---+---+---+---+---+---+---+
   | 1 |BITBANG�]����(�㑱byte��)|   �{ BITBANG�]���񐔕�(byte��)�̃f�[�^
   +---+---+---+---+---+---+---+---+
--BITBANG�f�[�^�̂P�񕪂́ATCK=LOW�̃T���v����TCK=HIGH�̃T���v�����p�b�N�����f�[�^�B
 bit 7   6   5   4   3   2   1   0  
   +---+---+---+---+---+---+---+---+
   |TCK|TDI| - |TMS|TCK|TDI| - |TMS|  �i��ʂS�r�b�g���ŏ��ɃZ�b�g����A���ɉ��ʂS�r�b�g���Z�b�g����܂��j
   +---+---+---+---+---+---+---+---+
--TCK��ω����������Ȃ��Ƃ��́A������TCK�r�b�g�𓯂��l�ɂ��܂��B


-HidReport�̓o��X�g���[��(AVR->PC) �T�C�Y�͍ő�64�o�C�g�܂�. HIDASP_JTAG_READ���s���̂ݐ܂�Ԃ��ԑ�����܂�.
 +----------------------------------------+
 | JTAG��M�f�[�^(TDO�̓ǂݎ��r�b�g��)  |  �i�ő�64�o�C�g�܂Łj
 +----------------------------------------+
--�r�b�g���LSB�t�@�[�X�g. ���M���ꂽTDI�r�b�g��Ƃ��̂܂ܑΉ����Ă��܂�.

----
���̑��⑫
 #define HIDASP_JTAG_READ	  0x19	//JTAG �ǂݏ���.
-�����s����Ƃ��́AHidReport�̉���X�g���[��(PC->AVR)��P���Ȍ`���i�P�R�}���h�̂݁j�ɂ��܂��B
 +------+------+-------------------+------+
 | 0x19 | jcmd |  data��           | 0x00 |
 +------+------+-------------------+------+
-jcmd��BitBang���[�h�̂Ƃ��́A�ԓ��f�[�^�͂���܂���BJTAG(TDI��)�̂Ƃ��̂�(TDO��)��Ԃ��܂��B
-JTAG�X�g���[���������ꍇ(56bit�ȏ��TDI�𑗂���TDO���󂯎��)�́A56bit�P�ʂɕ����]�����܂��B
-���̏ꍇ�A�Ō�̃X�g���[���̍ŏIbit�̂݁ATMS��1�ɂ��鏈��������܂��B(b6=1�̃p�P�b�g��p�ӂ��܂�)


-armblaster�̃t�@�[�����ăr���h����Ƃ��́ACodeSourcery G++ Lite ���g�p���Ă��������B


�� OpenOCD�̊ȒP�Ȏg���� 

-�N��������܂��Alocalhost:4444�ԃ|�[�g��telnet��(TeraTerm�Ȃǂ��g����)�q���ł��������B

-TeraTerm����A�ȉ��̂悤�ȃR�}���h���^�C�v����ƁA���ʂ��\������܂��B
|OpenOCD�R�}���h|�Ӗ�|
|scan_chain		|�ڑ�����Ă���s�`�o�̃��X�g������B|
|reset halt		|�^�[�Q�b�gCPU��HALT������|
|reg			|�^�[�Q�b�gCPU�̃��W�X�^������|
|mdw �A�h���X �J�E���g|display memory words <addr> [count]�������[�_���v|
|step			|�b�o�t���X�e�b�v���s������|
|flash write_image erase main.hex	|main.hex��FLASH ROM�ɏ�������(���̂܂���CPU��HALT�ɂ��Ă����܂�)|


�� �Q�l����(LINK)

OpenOCD�������܂� (kimura Lab)
-http://www.kimura-lab.net/wiki/index.php/OpenOCD%E3%81%8C%E5%8B%95%E3%81%8F%E3%81%BE%E3%81%A7

OpenOCD (�x�X�g�e�N�m���W�[����)
-http://www.besttechnology.co.jp/modules/knowledge/?OpenOCD

OpenOCD�{��
-http://openocd.berlios.de/web/
