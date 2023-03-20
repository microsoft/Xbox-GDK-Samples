# 簡単な三角形のサンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 説明

このサンプルでは、 Direct3D 12
の静的頂点バッファを作成して、画面に三角形を描画する方法を実演します。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# サンプルのビルド

Xbox One の devkit を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Project Scarlett を使用している場合は、アクティブなソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

*詳細については、GDK
ドキュメント*の「サンプルの実行」を*参照してください*。

# サンプルの使用

このサンプルでは、終了する以外の操作はできません。

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

# プライバシーに関する声明

サンプルをコンパイルして実行すると、サンプルの使用状況を追跡するため、サンプル実行可能ファイルのファイル名が
Microsoft に送信されます。このデータ収集を無効にするには、「Sample Usage
Telemetry」とラベル付けされた Main.cpp
内のコードのブロックを削除します。

Microsoft のプライバシー方針の詳細については、「[Microsoft
プライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
