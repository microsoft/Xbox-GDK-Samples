# 簡単なデバイスとスワップチェーンのサンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 説明

このサンプルでは、Xbox One アプリで Direct3D 12 デバイスと PresentX
スワップチェーンを作成する方法を実演します。

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

Xbox One アプリの Direct3D のセットアップは、他の Microsoft
のプラットフォームとよく似ていますが、このサンプルではいくつかの主な違いを説明します。

-   標準の D3D12CreateDevice の代わりに、**D3D12XboxCreateDevice**
    を使用

-   4K ネイティブ対応のスワップチェーンと1080p
    スワップチェーンを選択可能

-   画面への表示に DXGI ではなく、新しく導入された **PresentX** API
    を使用します。

Direct3D 12 デバイス作成のベスト プラクティスの詳細は「[Direct3D 12
Create Device
の解説](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)」をご覧ください。

ループ
タイマーの使用方法の詳細は「[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)」をご覧ください。

# プライバシーに関する声明

サンプルをコンパイルして実行すると、サンプルの使用状況を追跡するため、サンプル実行可能ファイルのファイル名が
Microsoft に送信されます。このデータ収集を無効にするには、「Sample Usage
Telemetry」とラベル付けされた Main.cpp
内のコードのブロックを削除します。

Microsoft のプライバシー方針の詳細については、「[Microsoft
プライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
