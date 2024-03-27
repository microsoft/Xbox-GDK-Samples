![](./media/image1.png)

# SimpleExceptionHandling サンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、タイトルで発生する可能性のある例外を処理するいくつかの方法を示します。

- ハンドルされない例外フィルター -- [ハンドルされない例外フィルター](https://docs.microsoft.com/windows/win32/api/errhandlingapi/nf-errhandlingapi-setunhandledexceptionfilter) を使用して、タイトルの一般的な例外をキャッチして処理する方法を示します。

- 構造化例外 -- [構造化例外処理](https://docs.microsoft.com/cpp/cpp/structured-exception-handling-c-cpp) システムの使用方法を示します。

- ベクター例外ハンドラー -- [ベクター例外処理](https://docs.microsoft.com/windows/win32/debug/vectored-exception-handling)システムを使用する方法を示します。

- C++ 言語の例外 -- [C++ 言語](https://docs.microsoft.com/cpp/cpp/try-throw-and-catch-statements-cpp) に組み込まれている例外システムの使用方法を 示します。

- 推奨パターン -- 他のシステムの組み合わせを使用する推奨パターンを示します。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|Sを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

デスクトップを使用している場合は、アクティブなソリューション プラットフォームを に設定します `Gaming.Desktop.x64`。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」 を参照してください。

# サンプルの使用方法

デモごとにコントローラーの対応するボタンを押します。 表示には、例外が発生したときにコードで発生する操作の順序が表示されます。

注: ハンドルされない例外フィルターの例は、デバッガーがアタッチされている場合は動作が異なります。コメントには追加の詳細があります。

# 実装メモ

すべての例は、Examples フォルダーに含まれています。 各システムの詳細と動作方法について詳しく説明されています。

# 更新履歴

初期リリース: 2021 年 4 月

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


