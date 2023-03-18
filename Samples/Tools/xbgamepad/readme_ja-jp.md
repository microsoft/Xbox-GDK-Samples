# xbgamepad

*このツールは、Microsoft ゲーム開発キットのプレビュー (2020 年 2 月)
に対応しています。*

# 説明

これは、ローカルに接続された XINPUT デバイス (Xbox One
のコントローラーなど) から入力を受け取り、XDK および GDK に付属している
XTF (Xbox ツール フレームワーク) ライブラリを使用して、Xbox One または
Xbox シリーズ X devkit に送信する最小限の win32 コンソール アプリです。

# 実行

xtfdlls フォルダー内に dll が存在する場合はそれらを使用するため、GDK
がインストールされていないコンピューターで実行できます。このツールには、Windows
8 以降が必要です。

# 使用方法

*xbgamepad /x:\<devkit ipv4 address\> \[/r:\<update rate in hz - default
is 30\>\]*

TCP ポート 4211 および 4212 経由で devkit にアクセスする必要があります。

# ビルドの前提条件

-   Visual Studio 2017

-   Windows 10 SDK

-   XTF のヘッダーおよびライブラリ用の最近の GDK のインストール
    (環境変数 GameDK をインクルードおよびリンカー内で XDK
    バージョンに変更して、プロジェクトは、XDK
    を使うように修正できます)。

# 配布

GDK がインストールされていないコンピューターで実行するには、xbtp.dll
および xtfinput.dll のファイルを既存の GDK
インストールからコピーする必要があります。 %GameDK%\\bin
内にあります。GDK がインストールされていないコンピューターの
xbgamepad.exe の横に配置することができます。
