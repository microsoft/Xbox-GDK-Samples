# シンプル プレイ サウンド ストリームのサンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、Xbox One で XAudio2 を使用して wav ファイルをストリーミングする方法を示します。

![](./media/image1.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Project Scarlett を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

サンプルには、[表示] ボタンを使用して終了する以外のコントロールはありません。

# 実装メモ

このサンプルでは、独自の WAV ファイル パーサーを使用して wav ファイルをストリーミングする方法を示します。

XAudio2 でストリーミングを行うその他の例については、[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2) を参照してください。

- **XAudio2AsyncStream** は Win32 非バッファー オーバーラップ I/O をサポートするためディスク上の WAV データを準備します

- **XAudio2MFStream** はWMA ファイルからデータの圧縮を解除するためソース リーダー メディア ファンデーションを使用します。

- *DirectX ツール キットの* **SoundStreamInstance** は、すべての XAaudio2 形式に対してバッファーされていないオーバーラップ I/O を実装します。

# 既知の問題

このサンプルでは、xWMA .wav ファイルのストリーミングはサポートされていません。

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


