# xbgamepad

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

これは、ローカルに接続された XINPUT デバイス (例: Xbox ゲームパッド) から入力を受け取り、 それらの入力をXDK と GDK に付属するライブラリに送信する、
最小限の win32 コンソール アプリです。
| | |
|---|---|
|XTF (Xbox ツール フレームワーク)を使用した Xbox One または Xbox Series X|S 開発キット|

# Running

xtfdlls フォルダー内の DLL が存在する場合はそれを使用するため、GDK がインストールされていないマシンでも実行できます。 ツールには Windows 8 以降が必要です。

# 使用法

```
xbgamepad /x:<devkit ipv4 address> [/r:<update rate in hz - default is 30>]
```


TCP ポート 4211 および 4212 経由で開発キットにアクセスする必要があります。

# 前提条件を構築する

- Visual Studio 2019 (16.11) および Visual Studio 2022

- Windows 10 SDK

- XTF ヘッダーとライブラリ用の最近の GDK インストール (インクルードとリンカーの入力で環境変数 GameDK を XDK バージョンに変更することで、XDK を使用するようにプロジェクトを変更できます)

# 配布

GDK がインストールされていないコンピューターで実行するには、既存の GDK インストールから xbtp.dll ファイルと xtfinput.dll ファイルをコピーする必要があります。 これらは `%GameDK%\bin` に存在します。GDK がインストールされていないコンピューターで xbgamepad.exe の横に配置できます。


