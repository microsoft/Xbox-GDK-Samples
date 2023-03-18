# WASAPI による音声再生の簡単なサンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 説明

このサンプルでは、Xbox One で、再生の設定をしてから WASAPI
のレンダラのエンドポイントで簡単な音声 (正弦波音)
を再生する方法を実演します。

![](./media/image1.png)

# サンプルのビルド

Xbox One の devkit を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Project Scarlett を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

*詳細については、GDK
ドキュメント*の「サンプルの実行」を*参照してください*。

# サンプルの使用

キーボードの Space キーまたはコントローラーの A
ボタンで再生、停止します。 キーボードの Escape キーまたはビュー
ボタンでアプリを終了します。

# 実装に関する注意事項

WASAPI の詳細は、MSDN
[をご覧ください。](https://msdn.microsoft.com/en-us/library/windows/desktop/dd371455.aspx)

# プライバシーに関する声明

サンプルをコンパイルして実行すると、サンプルの使用状況を追跡するため、サンプル実行可能ファイルのファイル名が
Microsoft に送信されます。このデータ収集を無効にするには、「Sample Usage
Telemetry」とラベル付けされた Main.cpp
内のコードのブロックを削除します。

Microsoft のプライバシー方針の詳細については、「[Microsoft
プライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
