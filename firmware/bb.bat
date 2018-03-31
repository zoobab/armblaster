rem ブートローダーエリアを更新します.
rem LEDが点滅中であることを確認して、細心の注意を払ってください.


rem === UPDATE BOOTLOADER for STM32 (Discovery) ===
pause ARE YOU SURE ?

..\host\armboot -nv -B main-0000.hex

