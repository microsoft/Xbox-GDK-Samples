  ![](./media/image1.png)

#   SimpleDirectStorageCombo Sample

*このサンプルは Microsoft Game Development Kit (2020 年 6 月)
と互換性があります。*

# 説明

このサンプルでは、コンソールとデスクトップの両方で DirectStorage
を使用するためのいくつかの方法を示します。

-   SimpleLoad -- DirectStorage
    の初期化、ファイルの表示、要求のエンキュー、完了の待機を行うための最小限のインターフェイス。

-   StatusBatch --
    通知のための状態配列を使用してリクエストのバッチを作成する方法を示します。

-   StatusFence -- 通知のための ID3DFence
    を使用してリクエストのバッチを作成する方法を示します。

-   MultipleQueues --
    異なる優先順位レベルを使用して複数のキューを作成する方法を示します。

-   キャンセル -- 保留中の要求を取り消す方法を示します。

-   RecommendedPattern -- DirectStorage
    を使用して最大のパフォーマンスを実現するための推奨されるパターンを示します。

-   Xbox ハードウェア展開 -- Xbox Series X|S
    コンソールで実行する場合に、ハードウェア zlib
    展開を使用する方法を示します。

-   Xbox イン メモリ ハードウェア展開 -- Xbox Series X|S
    コンソールで使用可能なハードウェア zlib
    展開を使用して、既にメモリ内にあるデータを展開する方法を示します。

-   Xbox ソフトウェア展開 -- Xbox One ファミリ
    コンソールで実行する場合に、ソフトウェア zlib
    展開を使用する方法を示します。

# サンプルのビルド

このサンプルは、次のプラットフォームをサポートしています。

-   Gaming.Desktop.x64

    -   PC API セットでの DirectStorage の使用。

-   Gaming.Scarlett.xbox.x64

    -   Xbox Series X|S コンソールで使用可能な Xbox DirectStorage
        実装の使用

-   Gaming.XboxOne.xbox.x64

    -   Xbox DirectStorage 実装の機能を提供する提供されたソフトウェア
        エミュレーション レイヤーの使用 (ただし Win32 API
        セットを内部的に使用)

*詳細については、GDK
のドキュメントの*「サンプルの実行」*を参照してください。*

# サンプルの使用方法

このサンプルでは、データ
ファイルが自動的に作成され、前述の各サブ要素が実行されます。

# 実装上の注意

すべての実装は SampleImplementations
フォルダーに含まれています。実行される各ステップの詳細について詳しく説明されています。

BCPack
圧縮の使用方法の例については、テクスチャ圧縮のサンプルを参照してください。

zlib ライブラリ (バージョン 1.2.11) は、次のライセンスの対象となります:
<http://zlib.net/zlib_license.html>

# 更新履歴

2022 年 2 月の初期リリース

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
