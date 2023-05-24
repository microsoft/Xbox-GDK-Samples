  ![](./media/image1.png)

#   AdvancedExceptionHandling Sample

*このサンプルは Microsoft Game Development Kit (2020 年 6 月)
と互換性があります。*

# 説明

このサンプルでは、タイトルで発生する可能性のある例外を処理するいくつかの高度な方法を示します。

-   別プロセスを使用してクラッシュ ダンプを保存する --
    別プロセスを使用してクラッシュ
    ダンプを作成する方法を示します。これは、クラッシュ
    ダンプ作成の推奨パターンです。

-   Windows エラー報告へのカスタム データの追加 --
    後の分析のためにクラッシュ ダンプと共に Microsoft
    サーバーにアップロードされる Windows
    エラー報告システムにデータを追加する方法を示します。

-   クラッシュ ダンプのアップロード -- クラッシュ
    ダンプを自らのサーバーにアップロードして、タイトルの実行を妨げず、場合によってはさらに多くの例外を発生させないようにする方法を示します。

-   中断 / 再開中の例外の処理 (PLM) -- PLM の中断 /
    再開パスの間に発生する例外を処理する方法を示します。

-   完全な例外システム --
    すべてのピースを完全な例外システムにまとめます。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブ ソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Xbox Series X|S を使用している場合、アクティブ ソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

デスクトップを使用している場合には、アクティブ ソリューション
プラットフォームを Gaming.Desktop.x64 に設定します。

*詳細については、GDK
のドキュメントの*「サンプルの実行」*を参照してください。*

# サンプルの使用方法

各デモについては、コントローラーの対応するボタンを押します。ディスプレイには、例外が発生したときにコード内で発生する操作の順序が表示されます。

# 実装上の注意

すべての例は Examples
フォルダーに含まれています。各システムの詳細とその機能について詳しく説明されています。

# 更新履歴

初回リリース 2021 年 6 月

# プライバシー ステートメント

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。