■ 概要

   これは、OpenOCD の DLL ハック（実験）です。
   即ち、外部に DLL を置いて、JTAGアダプターのドライバーの分離実装を試みています。

■ 現在のステータス

   STBEE MINI を使用したJTAGアダプターが動いています。


■ 試し方

   WindowsXPを用います。

   下記armblasterファームを焼いたSTBEE基板と、
   JTAGが使用できる適当なARM基板を以下のような対応でJTAG接続しておきます。

   armblaster/firmware/main-2000.hex

 結線は、
     STBEE MINI      ターゲットARM基板 JTAG端子
           PB11  ------------- TCK
           PB10  ------------- TDI
           PB9   ------------- TDO
           PB8   ------------- TMS
 となります。これ以外のnTRSTピンなどはまだサポートしていません。(USB-Blasterと同様)
 また、上記結線は直結ではなく１００Ω程度の抵抗を介して接続するようにしてください。

   armblaster/openocd/ ディレクトリの ocd.bat あるいは、 ocd2.bat を起動して、openocd.exeの吐き出す
   メッセージを確認することが出来ます。

   正常に接続出来ているようでしたら、telnetで localhost:4444 番に接続して、OpenOCDコマンドを実行して
   みてください。


■ ディレクトリ構成

 armblaster-+
            |
            +- firmware\       STBEE MINI用ファームウェア.
            +- HW\             STBEE MINI用ファームウェアのビルドに必要なライブラリ
            +- inc\            STBEE MINI用ファームウェアのビルドに必要なヘッダー
            |
            +- openocd -+      STBEE MINI用 openocd.exe と hidblast.dllソース.
            |           |
            |           +--helper\   ヘッダーファイル.
            |           +--jtag\     ヘッダーファイル.
            |           |
            |           +--openocd_patch\  openocd本体側作成用の改造点
            |
            |
            |
            +- tools ---+--armon\      armon (動作確認用モニタ)
                        |
                        +--avrwriter\  AVRライター (HIDaspx互換)
                        |
                        +--picwriter\  PICライター (pic18spx互換)
                        |
                        +--picwriter24\ PIC24Fライター
                        |
                        +--bin\        上記toolsの実行ファイルのみを収録


■ プログラムの再ビルド方法

   WindowsXP上のMinGW-gccコンパイラを用いて openocd/ ディレクトリにて makeしてください。
   makeすると、hidblast.dll が作成されます。

   openocd.exe本体を再ビルドする方法は、以下のURLを参照してください。

-http://hp.vector.co.jp/authors/VA000177/html/2010-09.html
   
   今回の改造部分ソースはopenocd_patch/ ディレクトリに置いています。

   Linux上でのビルドオプションは、こんな感じです。
   $ ./configure \
       --build=i686-pc-linux-gnu \
       --host=i586-mingw32msvc \
       --enable-maintainer-mode \
       --enable-dummy

   出来上がった openocd.exe 本体は、ドライバーとして、同一ディレクトリに存在する hidblast.dll を
   起動時に呼び出します。(存在しなければ、dummyドライバーのみが組み込まれます)


■ 現状の問題点

   HIDデバイスなので遅いです。
   Flashの書き込みがやや遅いです。

   USBデバイスの文字列名称が "ARM32spx" --> "ARMblast" に変更になっていますので、
   armonを使用する場合は、この配布ファイル内のarmon.exe を使用してください。

   (名称変更の理由は、書き込みターゲットのファームウェアがarmon/armbootの場合に
    そちらに接続されてしまい誤動作するので変更しました。)


■ ライセンス

   OpenOCDの配布ライセンスに準じます。


■ 展望

   hidblast.dll ファイルを(自力で)差し替えるだけで、自作デバイスがサポート可能になります。
   （たとえばATtiny2313を使用したJTAGアダプターなどをサポート出来る可能性があります）

   hidblast.dll のエントリーポイントは、
      DLL_int get_if_spec(struct jtag_command **q);
   だけです。引数のstruct jtag_command **qのqには、openocd本体のjtag_command_queueという
   グローバル変数のアドレスを渡します。
   戻り値は、(intになっていますが) ドライバー記述構造体のアドレスになります。


■ ARM以外の書き込みターゲットについて
-現状のarmblasterは、[[pic18spx]]ベースの汎用ライターにJTAGコマンドを付け足しただけのものですので、
-多くのATMEL製AVRチップとMicrochip製PIC18Fシリーズ、そして一部のPIC24Fシリーズへの書き込み機能を備えています。
-コマンドライン版書き込みツールを用意しました。
 armblaster/tools/avrwriter/
                 |
                 /picwriter/
                 |
                 /picwriter24/

-AVR,PIC18Fを焼く場合の結線図
     STBEE MINI    ARMターゲットのJTAG端子    PIC18F   AVR 
           PB11  ------------- TCK ---------- PGC  --- SCK
           PB10  ------------- TDI ---------- PGM  --- MOSI
           PB9   ------------- TDO ---------- PGD  --- MISO
           PB8   ------------- TMS ---------- MCLR --- RESET
-上記結線は直結ではなくSTBEE MINIからは１００Ω程度の抵抗を介して接続するようにしてください。
-PIC18Fは低電圧書き込みのみをサポートします。高電圧（9V～12V デバイスによって異なります)書き込みは出来ません。

■ 参考
-AVR-ISP用:ピンヘッダーを基板上から見た配置
    5    3    1
  +----+----+----+
  |Rset|SCK |MISO|
  +----+----+----+
  |GND |MOSI|Vcc |
  +----+----+----+
    6    4    2
-PIC18FのISPにもそのまま使いまわし
    5    3    1
  +----+----+----+
  |MCLR|PGC |PGD |
  +----+----+----+
  |GND |PGM |Vcc |
  +----+----+----+
    6    4    2
-それをまたARM(JTAG)にも使いまわし
    5    3    1
  +----+----+----+
  |TMS |TCK |TDO |
  +----+----+----+
  |GND |TDI |Vcc |
  +----+----+----+
    6    4    2



■ STBEE MINI以外の基板への移植について。
-STM8S-D,CQ-STARM,STBEE,STBEE MINIの４機種の基板については全て[[armon/armboot>armon]]を移植済みです。
-なので、armblasterの移植も簡単に出来ると思います。
-JTAG,AVR-ISP,PIC18書き込み用の端子(STBEE MINIではPB8～PB11)をそれぞれの基板の都合の良い位置に割り付ける作業は、おそらく定義ファイルを書き換えるだけで済みます。(ポートの制御はArduino風にdigitalRead(pin)とdigitalWrite(pin,level)で行なっていますので
、pinの定義が変わるだけで対応可能です)
-Makefileにはすでに基板の種別定義が含まれていますので基板名のdefineを書き換えるだけですみます。

-ただし、''FRISKケースに入る基板''は今のところSTBEE MINIだけしか対応できません。



■ JTAGコマンドの追加とプロトコルについて

-hidcmd.h
 #define HIDASP_JTAG_WRITE	  0x18	//JTAG 書き込み.
 #define HIDASP_JTAG_READ	  0x19	//JTAG 読み書き.
-が追加されてます。

-HidReportの下りストリーム(PC->AVR) サイズは最大64バイトまでです.
 +------+------+-------------------+------+-------------------+------+-------------+-----+
 | 0x18 | jcmd |  data列           | jcmd |  data列           | jcmd |  data列     | 0x00|
 +------+------+-------------------+------+-------------------+------+-------------+-----+

--jcmdの1バイトは以下のように定義（その１）
 bit 7   6   5   4   3   2   1   0  
   +---+---+---+---+---+---+---+---+
   | 0 | b6| JTAG転送bit数(TDIの数)|   ＋ JTAG転送bit数分の TDIビット（LSBファースト）  
   +---+---+---+---+---+---+---+---+
--- b6はTDI送出の最終bitでTMSを1にするなら1 しないなら 0 ： TMSは最終bit以外は常時0

--jcmdの1バイトは以下のように定義（その２）
 bit 7   6   5   4   3   2   1   0  
   +---+---+---+---+---+---+---+---+
   | 1 |BITBANG転送回数(後続byte数)|   ＋ BITBANG転送回数分(byte数)のデータ
   +---+---+---+---+---+---+---+---+
--BITBANGデータの１回分は、TCK=LOWのサンプルとTCK=HIGHのサンプルをパックしたデータ。
 bit 7   6   5   4   3   2   1   0  
   +---+---+---+---+---+---+---+---+
   |TCK|TDI| - |TMS|TCK|TDI| - |TMS|  （上位４ビットが最初にセットされ、次に下位４ビットがセットされます）
   +---+---+---+---+---+---+---+---+
--TCKを変化させたくないときは、両方のTCKビットを同じ値にします。


-HidReportの登りストリーム(AVR->PC) サイズは最大64バイトまで. HIDASP_JTAG_READ発行時のみ折り返し返送されます.
 +----------------------------------------+
 | JTAG受信データ(TDOの読み取りビット列)  |  （最大64バイトまで）
 +----------------------------------------+
--ビット列はLSBファースト. 送信されたTDIビット列とそのまま対応しています.

----
その他補足
 #define HIDASP_JTAG_READ	  0x19	//JTAG 読み書き.
-を実行するときは、HidReportの下りストリーム(PC->AVR)を単純な形式（１コマンドのみ）にします。
 +------+------+-------------------+------+
 | 0x19 | jcmd |  data列           | 0x00 |
 +------+------+-------------------+------+
-jcmdがBitBangモードのときは、返答データはありません。JTAG(TDI列)のときのみ(TDO列)を返します。
-JTAGストリームが長い場合(56bit以上のTDIを送ってTDOを受け取る)は、56bit単位に分割転送します。
-その場合、最後のストリームの最終bitのみ、TMSを1にする処理が入ります。(b6=1のパケットを用意します)


-armblasterのファームを再ビルドするときは、CodeSourcery G++ Lite を使用してください。


■ OpenOCDの簡単な使い方 

-起動したらまず、localhost:4444番ポートにtelnetで(TeraTermなどを使って)繋いでください。

-TeraTermから、以下のようなコマンドをタイプすると、結果が表示されます。
|OpenOCDコマンド|意味|
|scan_chain		|接続されているＴＡＰのリストを見る。|
|reset halt		|ターゲットCPUをHALTさせる|
|reg			|ターゲットCPUのレジスタを見る|
|mdw アドレス カウント|display memory words <addr> [count]メモリーダンプ|
|step			|ＣＰＵをステップ実行させる|
|flash write_image erase main.hex	|main.hexをFLASH ROMに書き込む(そのまえにCPUをHALTにしておきます)|


■ 参考文献(LINK)

OpenOCDが動くまで (kimura Lab)
-http://www.kimura-lab.net/wiki/index.php/OpenOCD%E3%81%8C%E5%8B%95%E3%81%8F%E3%81%BE%E3%81%A7

OpenOCD (ベストテクノロジーさん)
-http://www.besttechnology.co.jp/modules/knowledge/?OpenOCD

OpenOCD本家
-http://openocd.berlios.de/web/
