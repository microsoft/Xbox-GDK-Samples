# 単純なテクスチャ サンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、Direct3D 12 を使い、単純なテクスチャ クワッドをレンダリングする方法について説明します。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S 開発キットを使用している場合は、アクティブ ソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

このサンプルには、終了以外のコントロールはありません。

# 実装メモ

テクスチャは、ここに Windows Imaging Component (WIC) を使用する単純なヘルパーを使用して読み込まれ、学習を簡単にするように設計されています。 運用環境で使用する場合は、DirectX ツール キットの
[DDSTextureLoader、](https://github.com/Microsoft/DirectXTK12/wiki/DDSTextureLoader)
および [WICTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/WICTextureLoader) を参照してください。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


