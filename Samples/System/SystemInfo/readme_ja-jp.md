![](./media/image1.png)

# SystemInfo サンプル

*このサンプルは Microsoft Game Development Kit Preview (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、システムの情報とハードウェアの機能を照会するための多数の API を示します。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

Windows 10 May 2019 Updateで PC を使用している場合 (バージョン 1903;ビルド 18362) リリース以降、アクティブなソリューション プラットフォームを Gaming.Deskop.x64 に設定します。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

このサンプルでは、技術情報を含む一連のテキスト ページを表示します。

![C:\\temp\\xbox_screenshot.png](./media/image3.png)

ゲームパッド コントローラーでページを切り替えるには、A または DPad Right/B または DPad Left を使用します。

キーボードの場合は、左、Enter、右、または BackSpace を使用します。

# 実装メモ

重要なコードは、**Render** 関数内の switch ケースに含まれます。

# 更新履歴

2018 年 10 月: GDK の初期リリース

2020 年 4 月 -- Gaming.Desktop.x64 をサポートするように更新

2020 年 6 月 -- GetLogicalProcessorInformation/Get LogicalProcessorInformationEx の使用が追加されました

2021 年 10 月 -- Windows 11 DirectX 12 更新プログラム (22000) の更新プログラム。

2022 年 9 月 -- Windows 11 Version 22H2 (22621) の更新プログラム

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


