![](./media/image1.png)

# メッシュレット コンバーター アプリ

*\* このツールは PC と互換性があります。*

# 説明

メッシュレット コンバーターは、PC で使用するためのコマンド ラインツールです。 この Visual Studio ソリューションには、3 つのプロジェクトが含まれています。

- ConverterApp -- DirectXMesh を使用してメッシュレット データを生成する実行可能なコマンド ライン ツール

- ランタイム -- メッシュレット データ構造のランタイム バージョンを含むスタティック ライブラリ プロジェクト

ConverterApp プロジェクトは、FBX ファイル、OBJ ファイル、または SDKMesh ファイルからメッシュレット データを生成するために使用できるコマンド ライン ツールです。 このツールでは、メッシュレット生成の DirectXMesh 統合を利用して、入力 FBX ファイルから読み取られた頂点データとプリミティブ データからメッシュレットを生成します。

ランタイム プロジェクトには、実行時にメッシュレットを逆シリアル化してアップロードする方法を示す自己完結型ランタイム メッシュレット コードが用意されています。 スタンドアロンのデモではありませんが、既存のコードベースに簡単に統合できます。

# セットアップ

ツールをコンパイルするには、FBX SDK 2019.2 がインストールされている必要があります。 インストールが完了したら、インストール ディレクトリを指すように 'FBX_SDK' という名前の環境変数を構成します (通常 *C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2019.2*)。

# サンプルの使用方法

コマンド ライン ツールには、いくつかのオプションしかありません。

- -h -- ヘルプ メッセージを表示する

- -v \<int\> - メッシュレットの最大頂点数を指定します。 これは 32 から 256 の間でなければなりません。 既定値は 128 です。

- -p \<int\> - メッシュレットの最大プリミティブ数を指定します。 これは 32 から 256 の間でなければなりません。 既定値は 128 です。

- -s \<float\> - シーン ジオメトリのグローバル スケーリング 係数を指定します。 既定値は 1.0 です。

- -uf -- シーン ジオメトリの Z 軸を反転します。 既定値は falseです。

- -ft -- シーン ジオメトリの三角形の曲がり順を反転します。 既定値は falseです。

- -i -- 16 ビットで十分な場合でも、頂点インデックスを強制的に 32 ビットに設定します。 既定値は falseです。

- -t - FbxGeometryConverter 機能を使用してシーン メッシュ ファイルを三角形化します。 既定値は falseです。

- \<file list\> - 処理する相対ファイル パスの一覧。 少なくとも 1 つを指定する必要があります。

使用例を次に示します。

ConverterApp.exe -v 256 -p 256 -f Path/To/MyFile1.fbx Path/To/MyFile2.fbx

# 実装メモ

コマンド ライン ツールは、メッシュ頂点データを変更またはエクスポートしません。 FBX SDK の自動三角形分割は、コマンド ラインで指定できます。

FBX ファイルには複数のメッシュが含まれる可能性があるため、エクスポートされたファイルは複数のメッシュレットのセットをパックする可能性があります。 現在、メッシュ名で異なるメッシュレットにインデックスを付けるスキームはありませんが、後のイテレーションで追加できます。 メッシュは、FBX ノード ツリーの順序どおりに幅優先のトラバーサルに従って処理およびエクスポートされます。

# 使用上の注意

メッシュをエンジンランタイム形式に変換する際に、インデックスまたは頂点データの並べ替えが行われないように注意する必要があります。 頂点データはコマンド ライン ツールでエクスポートされないため、並べ替えによってメッシュレット データが無効になります。

# 更新履歴

2019 年 12 月 2 日 -- サンプル作成。

2019 年 2 月 20 日 -- メッシュレット ジェネレーターを書き直して、さまざまな頂点/プリミティブ数、よりコヒーレントな空間プロパティと方向プロパティをサポートしました。

2020 年 4 月 11 日 -- メッシュレット生成インターフェイスを、DirectXMesh のような細いインターフェイスに置き換えました。

2022 年 10 月 17 日 -- SDKMesh ファイルからの読み取りのサポートが追加されました。

