# シンプルな三角形のサンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、画面に三角形をレンダリングするために静的 Direct3D 12 頂点バッファーを作成する方法を示します。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S 開発キットを使用している場合は、アクティブ ソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

このサンプルには、終了以外のコントロールはありません。

# 実装メモ

このサンプルの主な目的は、ATG サンプル テンプレート構造を読者に理解し、Direct3D 12 API を使用する簡単なデモンストレーションを提供することです。

> **CreateDeviceDependentResources**: ここでコンパイルされた頂点
> およびピクセル シェーダー BLOB が読み込まれ、さまざまな Direct3D レンダリング
> リソースが作成されます。 *シェーダーは、Visual Studioによってコンパイルされます。*
>
> **レンダリング:** 三角形がレンダリングされ、表示される場所です
> 画面に。

デバイスの作成とプレゼンテーションの処理の詳細については、「[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)」 を参照してください。

ループ タイマーの使用方法の詳細については、「[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)」 を参照してください。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


