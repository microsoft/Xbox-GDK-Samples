# シンプルなデバイスとスワップチェーンのサンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

このサンプルは Direct3D 12 デバイスと Xbox タイトルの PresentX スワップ チェーンを作成する方法を示します。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox One X|S 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」 を参照してください。

# サンプルの使用方法

このサンプルには、終了以外のコントロールはありません。

# 実装メモ

Xbox タイトルの Direct3D セットアップは他の Microsoft プラットフォームと非常によく似ていますが、このサンプルでは主な違いをいくつか示します。

- 標準 D3D12CreateDevice の代わりに **D3D12XboxCreateDevice** を使用します

- Xbox Series X / Xbox One X には 4k、Xbox Series S には 1440p、Xbox One には 1080p を使用します

- プレゼンテーションに DXGI を使用する代わりに、これは新しい **PresentX** API を使用します

Direct3D 12 デバイスの作成とスワップチェーンのベスト プラクティスの詳細については、「[Direct3D 12 Create Device の構造](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)」および「[最新のスワップ チェーンの管理とフィード](https://walbourn.github.io/care-and-feeding-of-modern-swapchains/)」を参照してください。

ループ タイマーの使用方法の詳細については、「[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)」 を参照してください。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。

# 更新履歴

2018 年 10 月 -- Microsoft GDK の初期バージョン

2019 年 10 月 -- 本体検出のために XSystemGetDeviceType に切り替え

2021 年 10 月 -- Xbox Series S で 1440p を使用するように更新

2022 年 8 月 -- 配信元イベントを待機する場所に関する PresentX のベスト プラクティスを改善。


