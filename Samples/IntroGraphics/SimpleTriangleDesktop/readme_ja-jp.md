# シンプルな三角形のサンプル (PC)

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

このサンプルでは、画面に三角形をレンダリングするために静的 Direct3D 12 頂点バッファーを作成する方法を示します。

![](./media/image1.png)

# サンプルの使用方法

このサンプルには、終了以外のコントロールはありません。

このサンプルは、DirectX 12 対応ビデオカードが搭載されているすべての Windows 10 システムで実行できます。 DirectX 12 対応ビデオ カードが見つからない場合は、デバッグ構成で、使用可能な場合は WARP12 を使用します (*グラフィックス ツール* のオプションの Windows コンポーネントが必要です)。

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


