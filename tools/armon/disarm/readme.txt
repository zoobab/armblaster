* ARM CPU DisAssembler Ver 0.1 *
------------------------------------------------------------------------------
概要：
    ARM/Thumb シリーズＣＰＵ用の逆アセンブラです。

------------------------------------------------------------------------------
使い方：
   DISARM -Option FILENAME.BIN

オプション:
         -s<start address>  先頭番地を１６進数で指定.
         -t                 全てThumb命令と見なして逆アセンブル.
         -m                 ビッグエンディアンコードを解析する.

------------------------------------------------------------------------------
特徴：
  ・Ｗｉｎ３２コンソールアプリです。Windows 95/98/NTのDOS(NT)窓で動作します。
  ・実行ファイルは、たったの１８ｋＢ弱です。
  
  ・入力はバイナリーイメージ（ヘッダー無しのベタファイル）です。
  ・開始アドレス指定は -sオプションに続けて、空白を空けずに１６進で指定します。
  ・入力ファイルのサイズに制限はありません。
  ・Thumb命令を解析する場合は -t オプションが必要です。１６進データ部の表示
    がロングワードのままですが、気にしないでください。

  ・結果はコンソールに表示されるので、ファイル出力する場合はリダイレクトして
    ください。

------------------------------------------------------------------------------
開発環境：

    今回は珍しくBorland-Cではなく、Cygwin-b20.1を使用しました。

    CygWinの入手先については、
        <a href="http://sourceware.cygnus.com/cygwin/">ここ</a>
    にアクセスしてみてください。

    Win32が嫌いな方は、MS-DOSのDJGPP v2でもＯＫです。DJGPP付属のCWSDPMI
    を使用すれば、DOS 6.2以前でも動作可能かと思われます。
    （が、CPU条件として、i386以上が必須となってしまいます。）
    DJGPP v2 を使用する場合は、makefileの -mno-cygwin というオプションを
    外してください。このオプションは、EXEファイル実行時に CygWin1.DLLを
    不要にする（代わりにcrtdll.DLLに依存する）オプションです。

  ・Ｃソースファイル名等は全て小文字にしないとwin32のmakeは通らないかもしれません。
    （アーカイブは古いPKZIPを使用しましたので解凍すると全部大文字になります）

------------------------------------------------------------------------------
転載等：
  ・このソースの核心部は、ＧＮＵ（ｇｄｂ等）から借用しております。
  ・オリジナルソースは Cygwin-b20.1の src/opcodes/あたりから持ってきています。
  ・汎用のＧＮＵ binutilsに含まれる、objdump では生のバイナリーデータに対する
    逆アセンブル表示が出来なかったので、作成してみました。

  ・ＧＰＬに従います。
  ・転載、改造は自由です。
------------------------------------------------------------------------------
免責：
  ・制作者は本プログラムの実行結果に対する責任を一切負いません。本プログラムの
    使用、変更等は、各自責任で行なってください。
------------------------------------------------------------------------------
履歴：
  ver 0.1: 
     from scratch.

