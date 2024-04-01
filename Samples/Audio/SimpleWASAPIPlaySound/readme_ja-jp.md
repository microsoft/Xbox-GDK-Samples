# シンプルな WASAPI 再生サウンドのサンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、セットアップを再生し、Xbox One の WASAPI レンダリング エンドポイントに対して単純なサウンド (サイン トーン) を再生する方法を示します。

![](./media/image1.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Project Scarlett を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

キーボードの Space キーを使用するか、ゲームパッドのボタン A を使用して再生を開始および停止します。 キーボードの Esc キーまたは [表示] ボタンを使用して、アプリを終了します。

# 実装メモ

WASAPI の詳細については、「[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/dd371455.aspx)」を参照してください。

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


