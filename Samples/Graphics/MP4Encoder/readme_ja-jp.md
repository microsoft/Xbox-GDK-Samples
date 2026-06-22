  ![](./media/image1.png)

#   MP4Encoder サンプル

*このサンプルは Microsoft Game Development Kit (2020 年 8 月)
と互換性があります*

# 

# 説明

このサンプルでは、Microsoft メディア ファンデーション API
を使用して、H264 コーデックを使用してゲームのバック バッファーを MP4
ビデオ ファイルにエンコードする方法を示します。

![A picture containing chart Description automatically generated](./media/image3.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合、アクティブ ソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Project Scarlett を使用している場合は、アクティブ ソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

*詳細については、GDK
のドキュメントの*サンプルの実行*を参照してください。*

# サンプルの使用方法

このサンプルは、次のコントロールを使用します。

| 操作                                         |  ゲームパッド          |
|----------------------------------------------|-----------------------|
| エンコードの停止                             |  ボタン                |
| 終了                                         |  ビュー ボタン         |

# 実装上の注意

このサンプルでは、コンピューティング シェーダーを使用して、バック
バッファーを NV12 テクスチャに変換します。これはビデオ
エンコードに使用できます。NV12 テクスチャには、輝度データ (Y)
を含む完全な解像度プレーン0 と、彩度データ (UV) を含む半解像度プレーン 1
の 2
つのパーツが含まれています。コンピューティングシェーダーは、単純なバイリニア
低解像度処理を実行して彩度を計算します。

Microsoft メディア ファンデーション API は、Xbox ハードウェア
エンコーダーを利用して NV12 テクスチャを H264 ビデオ
ストリームを使用して MP4 ビデオ
ファイルにエンコードするために使用されます。2
つのファイルが開発キットのシステム スクラッチ
ドライブに書き込まれます。1 つは未加工の H264 ビデオ ストリームと、H264
ビデオ ストリームを含む MP4
ファイルです。これらのファイルは、以下のように 「xbdir
xd:\\\\MP4EncoderSampleOutput.mp4」 を使用して見つけることができます。

ゲーム録画のシステム レベルでもエンコーダーが使用されるため、タイトルは
Xbox ハードウェア エンコーダーを使用して最大 1080p @ 30Hz
でエンコードできます。30Hz でのみエンコードできますが、サンプルは 60Hz
でレンダリングされます。そのため、サンプルでは、レンダリングのストールを回避するために、エンコード用に別のスレッドを作成します。

MP4 ファイルを簡単に表示するには、xbcp
を使用してファイルをコピーするか、Xbox Manager
ツールからエクスプローラー機能を使用できます。

![Graphical user interface, application Description automatically generated](./media/image4.png)

![Graphical user interface Description automatically generated](./media/image5.png)

![Graphical user interface, text, application Description automatically generated](./media/image6.png)

# 既知の問題

なし

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
