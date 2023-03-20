  ![](./media/image1.png)

#   FrontPanelLogo サンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 

# 説明

このサンプルは、Xbox One X Devkit および Project Scarlett devkit
のフロント
パネルに表示される画像を、標準の画像形式を使用してレンダリングするのに役立つ開始コードを提供します。たとえば、博覧会や会議でゲームをデモンストレーションする場合は、ゲームのアートとスタイルに合ったグラフィックをフロント
パネルに配置するとよいでしょう。このサンプルは、メイン
ディスプレイに画像を表示するため、Xbox One S または Xbox One devkit
でも実行できますが、この方法でのサンプルの使用には制限があります。

# サンプルの作成

Xbox One の devkit を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Project Scarlett を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

*詳細については、GDK ドキュメントの*
「サンプルの実行」*を参照してください。*

既定では、サンプルは実行中に 2 つの画像を使用してフロント パネルとメイン
ディスプレイに表示します。FullScreenLogo.png がメイン
ディスプレイに表示されている間、FrontPanelLogo.png は FrontPanel
に表示されます。すばやく簡単にカスタマイズするには、FrontPanelLogo.png
と FullScreenLogo.png
を独自のアートワークに置き換えて再構築するだけです。

# サンプルの使用

## メイン ディスプレイのロゴ

![](./media/image3.png)

## フロント パネル ディスプレイのロゴ

![](./media/image4.png)

| 動作                                   |  ゲームパッド                |
|----------------------------------------|-----------------------------|
| 終了                                   |  \[ビュー\] ボタン           |

# 

# 

# 実装に関する注意事項

このサンプルは、FrontPanel
のバッファーを管理し、表示操作を簡略化するメソッドを提供する、ヘルパー
クラス FrontPanelDisplay を使用します。特に、サンプルは
FrontPanelDisplay::LoadWICFromFile を使用して標準の .png
画像ファイルを読み込みます。このメソッドは、PNG、JPG、および BMP
を含む多くの標準画像形式をサポートしています。また、この方法では、画像が自動的に拡大縮小され、フロント
パネルに必要なサイズとピクセル形式に変換されます。フロント
パネルの画質が気になる場合は、事前に画像を編集して、パネルのサイズとピクセル形式に最適にすることで、最良の結果を得ることができます。(パネルには
256x64 ピクセルがあり、ピクセルごとに 16 階調のグレーがあります。)

独自のゲームにカスタム フロント
パネル画像をすばやく追加するには、FrontPanelDisplay クラス
(およびサポート コード)
をコードベースに追加し、サンプルから数行のコードのみをコピーして表示を初期化し、画像を読み込みます。

if (XFrontPanelIsAvailable())

{

// FrontPanelDisplay オブジェクトを初期化する

m_frontPanelDisplay = std::make_unique\<FrontPanelDisplay\>();

// ロゴ画像を読み込む

m_frontPanelDisplay-\>LoadWICFromFile(L\"Assets\\\\FrontPanelLogo.png\");

}

初期化/更新コード パスのどこかに、FrontPanelDisplay::Present():
を少なくとも 1 回呼び出す必要があります。

if (XFrontPanelIsAvailable())

{

// 数フレーム待ってから、これを 1 回だけ呼び出す必要がある

if (m_timer.GetFrameCount() == 10)

{

m_frontPanelDisplay-\>Present();

}

}

# 更新履歴

2019 年 4 月に最初にリリースされました。

2019 年 11 月の Project Scarlett Devkit のサポート。

# プライバシーに関する声明

サンプルをコンパイルして実行すると、サンプルの使用状況を追跡するため、サンプル実行可能ファイルのファイル名が
Microsoft に送信されます。このデータ収集を無効にするには、「Sample Usage
Telemetry」とラベル付けされた Main.cpp
内のコードのブロックを削除します。

Microsoft のプライバシー ステートメントの詳細については、「[Microsoft
のプライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
