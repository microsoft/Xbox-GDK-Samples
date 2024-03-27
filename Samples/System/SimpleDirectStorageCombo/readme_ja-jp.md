![](./media/image1.png)

# SimpleDirectStorageCombo サンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

このサンプルでは、コンソールとデスクトップの両方で DirectStorage を使用するいくつかの方法を示します。

- SimpleLoad -- DirectStorage の初期化、ファイルの開封、要求のエンキュー、完了の待機を行う最小インターフェイス。

- StatusBatch -- 通知の状態配列を使用して要求のバッチを作成する方法を示します。

- StatusFence -- 通知に ID3DFence を使用して要求のバッチを作成する方法を示します。

- MultipleQueues -- 異なる優先度レベルを使用して複数のキューを作成する方法を示します。

- キャンセル -- 保留中の要求を取り消す方法を示します。

- RecommendedPattern -- DirectStorage を使用して最大のパフォーマンスを実現するのにおすすめのパターンを示します。

- Xbox ハードウェアの圧縮解除 -- ハードウェアの使用方法を示します
   | | |
   |---|---|
   |Xbox Series X|S 本体で実行している際の zlib 展開。|

- Xbox イン メモリ ハードウェアの圧縮解除 -- 以下を使用する方法を示します
   既にメモリ内にあるデータの圧縮を解除する場合。
   | | |
   |---|---|
   |Xbox Series X|S 本体で使用可能なハードウェア zlib 展開|

- Xbox ソフトウェアの圧縮解除 -- Xbox One ファミリ 本体で実行する場合にソフトウェア zlib 展開を使用する方法を示します。

- デスクトップ CPU の圧縮解除 -- デスクトップ上の DirectStorage でタイトルが提供する CPU の圧縮解除コーデックのサポートを使用する方法を示します。

# サンプルのビルド

このサンプルでは、次のプラットフォームがサポートされています

- Gaming.Desktop.x64

   - PC API セットでの DirectStorage の使用。

- Gaming.Scarlett.xbox.x64

   - Xbox で使用可能な Xbox DirectStorage 実装の利用
      | | |
      |---|---|
      |Xbox Series X|S 本体|

- Gaming.XboxOne.xbox.x64

   - Xbox DirectStorage 実装の機能を備えたソフトウェア エミュレーション レイヤーを利用しますが、内部的に設定された Win32 API を使用する。

*詳細については*、__GDK ドキュメントの「サンプルの実行__」 *を参照してください。*

# サンプルの使用

このサンプルは、データ ファイルを自動的に作成し、前述の各サブ要素を実行します。

# 実装メモ

すべての実装は SampleImplementations フォルダーに含まれています。 実行された各手順の詳細と共に大量に文書化されています。

BCPack 圧縮の使用方法の例については、「TextureCompression サンプル」を参照してください。

zlib ライブラリ (バージョン 1.2.11) には、次のライセンスが適用されます。 <http://zlib.net/zlib_license.html>

# 更新履歴

初期リリース 2022 年 2 月

デスクトップ CPU の圧縮解除を追加するために、2022 年 10 月に更新しました

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


