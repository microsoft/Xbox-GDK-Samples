# 簡単な三角形のサンプル (PC)

*このサンプルは、Microsoft ゲーム開発キット (2019 年 11 月)
に対応しています。*

# 説明

このサンプルでは、 Direct3D 12
の静的頂点バッファを作成して、画面に三角形を描画する方法を実演します。

![](./media/image1.png)

# サンプルの使用

このサンプルでは、終了する以外の操作はできません。

このサンプルは、DirectX 12 をサポートするビデオ
カードを搭載した、あらゆる Windows 10
システム上で実行できます。デバッグの構成では、DirectX 12
をサポートするビデオ カードが検出されない場合、WARP12
が使用可能ならこれが使用されます (Windows のオプション コンポーネント
*Graphics Tools* の追加が必要です)。

# 実装に関する注意事項

このサンプルの主な目的は、ATG
サンプルのテンプレートの構造に慣れていただくこと、Direct3D 12 API
の使用方法を実演することです。

> **CreateDeviceDependentResources:**コンパイルした頂点シェーダーとピクセル
> シェーダーの BLOB をロードし、各種 Direct3D レンダリング
> リソースを作成します。*シェーダーは Visual Studio
> によってコンパイルされます。*
>
> **Render:** 三角形を描画し、画面に表示します。

デバイスの作成と表示方法の詳細は「[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)」をご覧ください。

ループ
タイマーの使用方法の詳細は「[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)」をご覧ください。
