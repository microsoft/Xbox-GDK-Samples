![](./media/image1.png)

# デスクトップ用 Unity Social Manager

*このサンプルは、次と互換性があります:*

- *Xbox 拡張機能を備えた Microsoft Game Development Kit (2022 年 3 月) 以降*

- *Unity エディター 2021.3.4f1 以降*

- *[GDK Unity パッケージ](https://github.com/microsoft/gdk-unity-package)*

# 説明

デスクトップ用 Unity Social Manager のサンプルは、Unity ゲーム エンジンを使用した Xbox Live Social Manager の使用方法を示すものです。 さまざまなレベルのプレゼンスと関係に基づいてフレンド グループを変更し、Xbox Services API から特定のユーザー グループを見つけることができます。

![グラフィカル ユーザー インターフェイス、アプリケーション、自動生成された説明](./media/image3.png)

# メモ可能なコード ファイル

**XboxManager.cs**: Xbox GDK および Xbox Live Services API の初期化と、ユーザーへのサインインと、サンドボックスやタイトル ID などのさまざまな情報の照会が含まれます。

**XboxSocialManager.cs**: Xbox Live Social Manager の使用方法を示す API が含まれています。

# サンプルの構築

**重要:** サンプルには [*GDK Unity パッケージ*](https://github.com/microsoft/gdk-unity-package)が**必要です**。 サンプルを正常にビルドするには、パッケージに用意されている '*GDK-API*' と '*GDK-Tools*' の両方を含める必要があります。 事前構成済みの '*MicrosoftGame.config*' ファイル用に、含まれている '*GDK-Tools*' をインポートされた '*GDK-Tools*' に結合します。 '*Project*' フォルダーは、以下の画像をミラー化する必要があります。

![グラフィカル ユーザー インターフェイス、テキスト、アプリケーション、自動生成された説明](./media/image4.png)

*Unity GDK* 用のビルドでは、Unity ビルド メニューの代わりに *GDK Builder* を使用する必要があります。 ビルダーは、Unity メニュー バーの *[GDK] -\> [PC] -\> [ビルドと実行]* オプションで使用できます。

![グラフィカル ユーザー インターフェイス、テキスト、アプリケーション、自動生成された説明](./media/image5.png)

*詳細については、**GDK ドキュメント*の「[サンプルの実行](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/get-started-with-pc-dev/get-started-with-unity-pc/gdk-unity-end-to-end-guide)」を参照してください。

# サンプルの実行

ソーシャル グループの変更を実行してユーザーの状態を取得するには、サインインした Xbox Live テスト アカウントが必要です。

デスクトップのサンドボックスは XDKS.1 に設定する**必要があります**。

**重要:** [見つかった Xbox ユーザー] セクションに結果を表示するには、少なくとも 1 人のフレンドが [フレンド] または [お気に入り] を使用してログインしている Xbox ユーザーにリンクされている必要があります。 プレゼンスの変更やソーシャル グループの更新など、一部の Xbox サービス イベントで更新が自動的にトリガーされます。

*ソーシャル グループ コマンド:*

- '*すべてのフレンド'*

   - XblPresenceFilter = XblPresenceFilter.All

   - XblRelationshipFilter = XblRelationshipFilter.Friends

- すべてのお気に入り'

   - XblPresenceFilter = XblPresenceFilter.All

   - XblRelationshipFilter = XblRelationshipFilter.Favorite

- *'すべてのオンライン フレンド'*

   - XblPresenceFilter = XblPresenceFilter.AllOnline

   - XblRelationshipFilter = XblRelationshipFilter.Friends

- *'タイトル オンライン フレンド'*

   - XblPresenceFilter = XblPresenceFilter.TitleOnline

   - XblRelationshipFilter = XblRelationshipFilter.Friends

**重要:** これらは、Xbox サービス プレゼンスとリレーションシップ フィルターの定義済みの組み合わせを使用して、呼び出し元の Xbox ユーザー関連の取得された Xbox ユーザーを決定します。

*Xbox ユーザー コマンドが見つかりました:*

- '*ゲーマー タグ...状態'* -- 現在のソーシャル グループ設定のソーシャル条件を満たす最初の 5 人のユーザーのゲーマー タグと接続状態を表示します。 選択すると、Xbox UI を介してユーザーの Xbox プロフィールも表示されます。

**重要:** これらは、定義済みのフィルターから指定された条件を満たすユーザーがいる場合にのみ表示されます。 それ以外の場合は、空白のままになります。

*その他のコマンド:*

- '*Sign In'* -- 新しいユーザーが Xbox ユーザー選択 UI を使用してサインインできるようにします。

- '*Refresh' ­*-- Xbox ユーザーのユーザー一覧を手動で更新します。

- '*Clear Logs'* -- 既存のすべてのログのコンソールをクリアします。

- '*Close' --* サンプルを閉じます。

# 既知の問題

このサンプルは、このドキュメントのパッケージとバージョンに対して開発およびテストされました。 新しいバージョンの GameCore または Unity エディターを使用すると、ビルド エラーや非互換性が発生する可能性があります。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の「サンプル使用状況テレメトリ」というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。

# 更新履歴
2023 年 5 月 18 日 - 注目すべきコード ファイルを追加しました


