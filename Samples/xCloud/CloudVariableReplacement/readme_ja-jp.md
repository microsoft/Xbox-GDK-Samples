# クラウド変数の置換のサンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 10 月)*

# 説明

このサンプルでは、タッチ アダプテーション キットの状態をゲーム内から変更する方法を示します。

![テキストの説明が自動的に生成されました](./media/image1.jpeg)

# サンプルの使用

サンプルを起動する前に、ゲーム ストリーミングが有効になっていることを確認します。 また、ストリーミング クライアント (Xbox ゲーム ストリーミング テスト アプリなど) で、[設定] > [開発者] > [タッチ適応] > [サイドロードを有効にする] が有効になっていることを確認します。 ストリーミング クライアント アプリを使用して、サンプルを実行しているコンソールに接続します。 接続したら、ストリーミング クライアントの存在を反映するようにサンプルを変更する必要があります。 ゲーム コマンド プロンプトで次のコマンドを実行して、"sample-layouts" バンドルが読み込まれることを確認します。

```
tak serve --takxconfig sample-layouts\takxconfig.json
```


TAK の B ボタンの不透明度を変更するには dpad を左右に押します。 Y ボタンの表示を切り替えるには dpad を上に押し、A ボタンの有効な状態を切り替えるには dapd を下に押します。

# 実装メモ

このサンプルでは、クラウド対応 API for xCloud を使用する方法を示します。

詳細については、次のドキュメントを参照してください: https://learn.microsoft.com/gaming/gdk/_content/gc/system/overviews/game-streaming/building-touch-layouts/game-streaming-touch-changing-layouts-game-state

# バージョン履歴

2024 年 4 月:
- スクリーン上のタッチ レイアウトのクライアントごとの制御を可能にするために `OnClient` API を使用するようにサンプルを更新しました。
- `takxconfig.json` に `sample-layouts` を追加しました。

2021 年 7 月: 初期サンプル

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の「サンプル使用状況テレメトリ」というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


