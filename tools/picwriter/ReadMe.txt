  -------------------------------------------------------------------
         HIDaspx ���g�p���� PIC18F�V���[�Y��Flash��ǂݏ���.

                               PICspx
  -------------------------------------------------------------------

�� �T�v

   ����� USB �ڑ��� AVR ���C�^�[ HIDaspx �̃n�[�h�E�F�A�����̂܂ܗ��p���āAPIC18F
   �ւ̏������݂��s�����Ƃ������ł��B

�� ���Ɏg���̂ł����H

   PIC 18F2550/18F4550/18F14K50 �Ȃ� USB I/F �����̗����� PIC �V���[�Y�̃}�C�R��
   �Ƀu�[�g���[�_�[���������ނ̂Ɏg�p�ł����炢���ȂƂ������ƂŐ�����n�߂܂����B


�� �p�ӂ���n�[�h�E�F�A�́H

   �i�P�jHIDaspx  AVR���C�^�[.

   AVR�}�C�R����HIDaspx�Ɋւ��Ă� ��H�[�~����̃T�C�g�ɖc��ȏ�񂪒~�ς���Ă��܂��B
   http://www-ice.yamagata-cit.ac.jp/ken/senshu/sitedev/index.php?AVR%2FHIDaspx00


   �i�Q�jPIC 18F2550/18F4550/18F14K50 �i�ǂꂩ�j

    LVP �������݂�OFF�ɂȂ��Ă���Ɠǂݏo���܂���B(���g�p�̃`�b�v��ON�ɂȂ��Ă��܂�)

    �������ݍς݂̃`�b�v�� LVP=OFF �̏ꍇ�́AHVP �������݉\�ȃ��C�^�[�ň�x����
    ���Ă��� LVP=ON �ŏ�������ł�������.

    �uFENG3's HOME PAGE�v��PIC18F14k50�Ɋւ���L���̒��ɁA006P���d�r��LVP�v���O���}
    ���g���āA������������@���Љ��Ă���APICspx���g���ď��������邱�Ƃ��\�ł��B

     LVP�r�b�g(ON�ɂ�����@)�� http://wind.ap.teacup.com/feng3/452.html

    �܂� LVP=ON ��Ԃ̃`�b�v�����ۂɎg�p����Ƃ��́A�펞 PGM �[�q�� PullDown (5�`
    10k �����x�� OK �ł�) ���Ȃ���΂Ȃ�܂���B

�� �ڑ�:

 AVR�pISP 6PIN                  PIC18F2550/14K50

	1 MISO    ------------------   PGD
	2 Vcc     ------------------   Vcc
	3 SCK     ------------------   PGC
	4 MOSI    ------------------   PGM
	5 RESET   ------------------   MCLR
	6 GND     ------------------   GND


�� PIC �������݃c�[��

   src/ �f�B���N�g���ɂ���܂��B

   PICspx.exe
-------------------------------------------------------
* PICspx Ver 0.2 (Aug 18 2009)
Usage is
  PICspx.exe [Options] hexfile.hex
Options
  -p[:XXXX]   ...  Select serial number (default=0000). HIDaspx�t�@�[���̃V���A���ԍ��w��.
  -r          ...  Read Info.    HIDaspx�Ɍq�����Ă���f�o�C�X�̏�������.
  -v          ...  Verify.       �������݂͍s�킸�A�x���t�@�C�̂ݎ��s����.
  -e          ...  Erase.        �����̂ݍs��.
  -rp         ...  Read program area.  FlashROM�̈��ǂݏo���� hexfile.hex �ɏo�͂���.
  -rf         ...  Read fuse(config).  Fuse�f�[�^��ǂݏo���ăR���\�[���ɕ\������.
  -d          ...  Dump ROM.           FlashROM�̈��ǂݏo���� �R���\�[���ɕ\������.
  -nv         ...  No-Verify.    �������݌�̃x���t�@�C���ȗ�����.

-------------------------------------------------------

�� PIC �������݃c�[���̎g����

  WindowsXP ���� DOS �����J����
  C:> picspx  -r
      ~~~~~~~~~~
  �̂悤�ɓ��͂���ƁA�ڑ����Ă���PIC18F �̕i���\�����܂��B


  C:> picspx  bootloader-0000.hex
      ~~~~~~~~~~~~~~~~~~~~~~~~~~~
  �̂悤�ɓ��͂���ƁA�ubootloader-0000.hex�v��PIC�ɏ������ނ��Ƃ��o���܂��B


  C:> picspx  -rf
      ~~~~~~~~~~~
  �̂悤�ɓ��͂���ƁAFuse(config)�̓��e��ǂݏo���� �R���\�[���\�����܂��B


  C:> picspx  -d read.hex
      ~~~~~~~~~~~~~~~~~~~
  �̂悤�ɓ��͂���ƁAFlashROM�̓��e��ǂݏo���� �R���\�[���Ƀ_���v�\�����܂��B

  C:> picspx  -rp read.hex
      ~~~~~~~~~~~~~~~~~~~~
  �̂悤�ɓ��͂���ƁAFlashROM �̓��e��ǂݏo���� �J�����g�f�B���N�g���Ɂuread.hex�v
  �̃t�@�C�����ŏo�͂��܂��B�i���Ɏ��Ԃ��|����܂��j



�� �e�X�g�c�[��( ���̃c�[�����f�o�b�O����ꍇ�݂̂Ɏg�p���Ă������� )

   test/ �f�B���N�g���ɂ���܂��BHIDmon ���������� pgm �R�}���h��ǉ��������̂ł��B
   a.bat ���N������� PIC �� flash ROM �̓��e (�擪 64byte) �� fuse(config),ID ��
   �ǂݏo����16�i�\�����܂��B


�� ���݂̃X�e�[�^�X

   �����ς݂�PIC 18F14K50�ɑ΂��āA
   HIDmon-14k �̃t�@�[���E�F�A bootloader-0000.hex ���ȉ��̎菇
   C:> picspx  bootloader-0000.hex
       ~~~~~~~~~~~~~~~~~~~~~~~~~~~
   �ŏ�������ŁAHIDmon/HIDboot �Ƃ��ē��삷�邱�Ƃ��m�F���Ă��܂��B



   �|�[�g�̏グ������ USB �� 1 �t���[�� (1mS �������� 4mS(UHCI)) �̎��Ԃ��|�����
   ���̂œǂݏo���⏑�����݂͔��ɒx���ł��B

   �u�[�g���[�_�[���������ނ̂��ړI�Ȃ̂ŁA���x�ɂ��Ă͖ڂ��Ԃ邱�Ƃɂ��܂��B

   �p�\�R������ USB �� 2.0(480Mbps) ���T�|�[�g���Ă���ꍇ�́A�p�\�R���� HIDaspx
   ���C�^�[�̊Ԃ� USB 2.0 Hub(480Mbps) �����ނ��Ƃŏ������݂����������邱�Ƃ��o��
   �܂��B

   ���邢�́AIntel/VIA �� USB Host �R���g���[�� (UHCI) ���g������ɁASiS/AMD/NEC
   ���� USB Host �R���g���[�� (OHCI) �i�}�U�[�{�[�h�A�������� PCI �J�[�h�j���g�p
   ���邱�Ƃł��������݂����������邱�Ƃ��o���܂��B

   ����������闝�R�́AUSB 2.0(480Mbps) �K�i�Ŋg�����ꂽ�}�C�N���t���[�� (1mS ��
   1/8 �̎��Ԃł� USB �����T�C�N��) ���g�p�ł��邽�߂ł��B�i�ʏ�� USB �̃t���[��
   ���Ԃ� 1mS �ł���AUHCI �z�X�g�ł̓R���g���[���]���� 4 �t���[�� (4mS) �����
   �̂ŁA���b 250 �񂵂��|�[�g�R���g���[���o���Ȃ��̂��x���Ȃ闝�R�ł��j


----------------
2009/09/02 8:54:18

senshu�̍��

1. 18F14k50�ŗ��p����EraseCommand�̃R�}���h�l���ȉ��̂悤�ɕύX

	EraseCmd(0x0f0f,0x8787);break;
		��
	EraseCmd(0x0f0f,0x8f8f);break;	// mod by senshu

2. PICspx�̎��s�t�@�C���ɃA�C�R����ǉ�

3. bin�f�B���N�g����ǉ����A���s�t�@�C���������ɔz�u

4. Sleep�֐��̐��x�����コ����C����ǉ�

5. �A�C�R���̃f�U�C������C��

6. PCtest�t�H���_��ǉ��iSleep�֐��̐��x���m�F����v���O�����j
   test-pc.exe �́APICspx�p�ɓ������A10m�b�ȉ��̑҂���500��J��Ԃ��A
   Sleep�֐������ۂɗv�������ԁi�ŏ��A�ő�A���ρj��\������B

   ���̔ł�PICspx�ł́Await_ms�֐��o�R�ŁA�ȉ��̒l��Sleep�֐����Ăяo��
   �Ă���B

   1mSec   ... PGM, PGC�̑���
   2mSec   ... Flash�̏������݊����҂��i1mSec����ύX�A�}�[�W���m�ۂ̂��߁j
   100mSec ... Config�������݊����҂�
   300mSec ... �`�b�v�������҂�

   test-pc���R�}���h�v�����v�g��Ŏ��s����ƁA��20�b��Ɉȉ��̂悤��
   ���ʂ�\������B�i���s���ɂ��A�\�����鐔�l�ɂ͈Ⴂ������j

   ���ʂ̌����́ASleep function�ɒ��ڂ��Aavg�̒l�������̐��l�ɋ߂����Ƃ�
   �m�F����B�i���̌��ʒ��x�̂���͋��e�͈́j

-----
 delay_ms function:
  10 -> min =   9, max =  29, avg = 10.17 [mSec]
   5 -> min =   5, max =  24, avg =  5.11 [mSec]
   2 -> min =   2, max =  19, avg =  2.03 [mSec] <--- ???
   1 -> min =   1, max =   1, avg =  1.00 [mSec]
   0 -> min =   0, max =   0, avg =  0.00 [mSec]

 Sleep function:
  10 -> min =   9, max =  22, avg = 10.07 [mSec]
   5 -> min =   4, max =  23, avg =  5.10 [mSec]
   2 -> min =   2, max =  20, avg =  2.04 [mSec] <--- ???
   1 -> min =   1, max =  19, avg =  1.04 [mSec] <--- ???
   0 -> min =   0, max =   0, avg =  0.00 [mSec]


7. LVP=ON �ɏ�����������@�i�����N�j��ǉ�

8. test�t�H���_���� hidmon.exe �ɂ��Await_ms�֐��̐��x����p�b�`��K�p

9. bin�t�H���_��PIC18F14k50�p��Bootloader�𓯍�


http://hp.vector.co.jp/authors/VA000177/html/hidmon-14kA4CEBBC8A4A4CAFD.html

�������ł���\�[�X�ɁA

�ȉ��̏C���������ăr���h����HEX�t�@�C��(bootloader-mod-0000.hex)��ǉ�

	CONFIG  PWRTEN = ON			; (OFF) PowerUp Timer

�ڍׂ́A����URL���������������B

�C���̗��R�́A���Z�b�g�@�\�̈��艻�̂��߁i���`�F�b�N�ł��j�B

