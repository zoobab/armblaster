�� �T�v

   ����́AOpenOCD �� DLL �n�b�N�i�����j�ł��B
   �����A�O���� DLL ��u���āAJTAG�A�_�v�^�[�̃h���C�o�[�̕������������݂Ă��܂��B

�� ���݂̃X�e�[�^�X

   �Ƃ肠����PIC18F14K50���g�p����JTAG�A�_�v�^�[�������Ă��܂��B


�� ������

   WindowsXP��p���܂��B

   ���LPIC18spx�t�@�[�����Ă���PIC18F14K50(��������PIC18F2550)��ƁA
   JTAG���g�p�ł���K����ARM����ȉ��̂悤�ȑΉ���JTAG�ڑ����Ă����܂��B

   pic18blaster/firmware/picmon-18F14K50.hex

 �����́A
   (PIC���C�^�[�@�\��)         ARM��� JTAG�[�q
           MCLR  ------------- TMS
           PGM   ------------- TDI
           PGD   ------------- TDO
           PGC   ------------- TCK
 �ƂȂ�܂��B����ȊO��nTRST�s���Ȃǂ͂܂��T�|�[�g���Ă��܂���B(USB-Blaster�Ɠ��l)

 ARM����3.3V�Ȃ̂ŁAPIC���̓d���ɒ��ӂ��Ă��������B(5V�͊댯�ł�)
 PIC����PIC18F2550��p����Ƃ��́A�d���ϊ��̕K�v�������܂��B
 �Q�l��Ƃ��ẮAUSB-Blaster�i���ǂ��j�̎����ɂ�����A��R�����𗘗p������@�ƁA
 ���܂肨���߂��܂��񂪁APIC18F2550��3.3V�쓮������@�Ȃǂ�����܂��B(18LF2550���g���΋K�i���ł�)

   hidblast/ �f�B���N�g���� ocd.bat ���邢�́A ocd2.bat ���N�����āAopenocd.exe�̓f���o�����b�Z�[�W
   ���m�F���邱�Ƃ��o���܂��B

   ����ɐڑ��o���Ă���悤�ł�����Atelnet�� localhost:4444 �Ԃɐڑ����āAOpenOCD�R�}���h�����s����
   �݂Ă��������B

   STM8S-Discovery�p�̃t�@�[�����Ă��Ă݂�e�X�g�p�� a.bat ��main-0000.hex ���܂߂Ă��܂��B


�� �f�B���N�g���\��

 pic18blast-+- �\�[�X.
            |
            +--helper\   �w�b�_�[�t�@�C��.
            +--jtag\     �w�b�_�[�t�@�C��.
            |
            +--openocd_patch\  openocd�{�̑��쐬�p�̉����_
            |
            +--firmware\       PIC18F14K50�p�t�@�[���E�F�A.
                (���萫����̂���pic18spx�t�@�[������A���荞�݊֘A��Disable�������̂ł�)



�� �v���O�����̍ăr���h���@

   WindowsXP���MinGW-gcc�R���p�C����p����make���Ă��������B
   make����ƁAhidblast.dll ���쐬����܂��B

   openocd.exe�{�̂��ăr���h������@�́A�ȉ���URL���Q�Ƃ��Ă��������B

-http://hp.vector.co.jp/authors/VA000177/html/2010-09.html
   
   ����̉��������\�[�X��openocd_patch/ �f�B���N�g���ɒu���Ă��܂��B

   Linux��ł̃r���h�I�v�V�����́A����Ȋ����ł��B
   $ ./configure \
       --build=i686-pc-linux-gnu \
       --host=i586-mingw32msvc \
       --enable-dummy

   �o���オ���� openocd.exe �{�̂́A�h���C�o�[�Ƃ��āA����f�B���N�g���ɑ��݂��� hidblast.dll ��
   �N�����ɌĂяo���܂��B(���݂��Ȃ���΁Adummy�h���C�o�[�݂̂��g�ݍ��܂�܂�)


�� ����̖��_

   �����s����ł��B--->�t�@�[�����X�V���邱�Ƃň��艻���܂����B
   PIC18F14K50�ŕs����ȏꍇ�́APIC18F2550�p�����������������B

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


