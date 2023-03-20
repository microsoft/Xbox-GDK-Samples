# 音声のストリーミング再生の簡単なサンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 説明

このサンプルでは、Xbox one で XAudio2 を使用して wav
ファイルをストリーミング再生する方法を実演します。

![](./media/image1.png)

# サンプルのビルド

Xbox One の devkit を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Project Scarlett を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

*詳細については、GDK
ドキュメント*の「サンプルの実行」を*参照してください*。

# サンプルの使用

このサンプルでは、ビュー ボタンを押して終了する以外の操作はできません。

# 実装に関する注意事項

このサンプルでは、付属の WAV ファイル パーサーを使用して wav
ファイルをストリーミング再生する方法を実演します。

XAudio2 を使用したストリーミング再生のその他の例は GitHub
[でご覧になれます。](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)

-   **XAudio2AsyncStream** は、ディスク上に .WAV データを作成して Win32
    非バッファ重複 I/O をサポートします。

-   **XAudio2MFStream** は、Media Foundation Source Reader を使用して
    WMA ファイルからデータを展開します。

# 既知の問題

このサンプルは、xWMA .wav ファイルをサポートしていません。

# プライバシーに関する声明

サンプルをコンパイルして実行すると、サンプルの使用状況を追跡するため、サンプル実行可能ファイルのファイル名が
Microsoft に送信されます。このデータ収集を無効にするには、「Sample Usage
Telemetry」とラベル付けされた Main.cpp
内のコードのブロックを削除します。

Microsoft のプライバシー方針の詳細については、「[Microsoft
プライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
