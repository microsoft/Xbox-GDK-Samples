  ![](./media/image1.png)

#   OfflineRT サンプル

*このサンプルは Microsoft Game Development Kit (2021 年 6 月)
と互換性があります。*

# 

# 説明

このサンプルでは、Scarlett プラットフォーム用の Microsoft Game
Development Kit で使用できるオフライン アクセラレーション構造 (BVH)
ビルダーとレイトレーシング パイプライン状態オブジェクト (RTPSO)
シリアル化機能を使用する方法を説明します。

タイトルでは、これらの機能を既存のコンテンツおよびシェーダー ビルド
パイプラインに統合して、Xbox ネイティブ形式でアセット
データを生成し、実行時にコストのかかるシェーダー
コンパイラの呼び出しを防ぎ、BVH の品質を高め、スクラッチ
メモリの要件を減らすことができます。

# サンプルのビルド

このサンプルは、次の 2
つの別々のソリューションで構成されています。RTBuilder と
OfflineRT。RTBuilder は、オフライン
BVHとRTPSOを構築するために必要なコードを含む PC
アプリケーションであり、OfflineRT
は、オフラインで生成されたデータを使用して視覚化できる Scarlett
アプリケーションです。

**このプロデューサーとコンシューマーの関係により、Scarlett で OfflineRT
サンプルをコンパイルして実行する前に、RTBuilder
プロジェクトをコンパイルして実行する必要があります。**
これを行わないと、コンパイルとデプロイのエラーが発生します。

RTBuilder を実行すると、次のようなコンソール出力が表示されます。

![Text Description automatically generated](./media/image3.png)

ツールが完了すると、サンプルのBuildディレクトリに次の生成されたファイルが表示されます。

![Graphical user interface, text, application Description automatically generated](./media/image4.png)

これで、Scarlett 開発キットで OfflineRT
サンプルのコンパイルと実行に進むことができます。

# サンプルの使用

![A picture containing text Description automatically generated](./media/image5.png)

サンプルでは、単純な RTPSO を使用して 1
つのモデルをレイトレースします。赤いランタイム (.sdkmesh ファイル)
と緑のオフライン (.mdat ファイル) で生成されたモデル BVH
を切り替え、ゲームパッドを使用してランタイム、オフライン
コレクションベース、完全オフラインの RTPSO を循環させることができます。

| 操作                                         |  ゲームパッド          |
|----------------------------------------------|-----------------------|
| モデルの切り替え (BVH)                       |  Dpad 左/右            |
| RTPSO の切り替え                             |  Dpadの上/下           |
| カメラの旋回                                 |  右サムスティック      |
| カメラのリセット  |  右サムスティック ボタン                |
| ズーム/ロール カメラ                         |  左サムスティック      |
| 終了                                         |  ビュー ボタン         |

このサンプルでは、選択した BVH と RTPSO
の基本的な統計をいくつか示します。オフラインで生成される BVH
はメモリ内で大幅に小さくなり、実行時にスクラッチ領域も必要ありません。重要なのは、トレース速度も速い点です。オフラインの
RTPSO
では、実質的に作成時間コストをすべてビルダーに移行できることがわかります。

# 実装上の注意

RTBuilder アプリケーションは、PC バージョンの Scarlett グラフィックス
ドライバー (UMD) に存在する機能を利用して OBJ モデルと HLSL シェーダー
ライブラリを Xbox ネイティブ BVH および RTPSO に変換します。ドライバーの
PC API は %GXDKLatest%\\toolKit\\include\\Scarlett\\d3d12_xs.h
で入手でき、ランタイム時に必要なバイナリは %GXDKLatest%\\bin\\Scarlett
にあります。このサンプルでは、カスタム ビルド
ステップを使用してバイナリを出力ディレクトリにデプロイします。

オフライン BVH はランタイム BVH の生成に使用するのと同じ
ID3D12GraphicsCommandList6::BuildRaytracingAccelerationStructure API
を使用して生成されますが、これらの操作は CPU 上で (呼び出すとすぐに)
発生し、EXECUTECommandLists 時の GPU
上ではないことに注意することが重要です。同じように、入力と出力として渡されるすべてのD3D12_GPU_VIRTUAL_ADDRESS
パラメーターは、通常の CPU 仮想メモリ
アドレスとして解釈されます。内部的には、PC UMD
はランタイム時にドライバーで使用される GPU コンピューティング
ベースのソリューションではなく [Intel Embree](https://www.embree.org/)
ビルダーを利用して BVH を生成します。Embree ビルダーは高品質の BVH
を生成し、通常はトラバーサル時間で 5% から 10% (モデル
データによってはさらに高くなる可能性があります) の高速化につながります。

このサンプルのコンテンツでは、ランタイムとオフラインビルド BVH
の間のパフォーマンスの差は小さくなりますが、一般的なゲーム
コンテンツはパフォーマンスの向上を示します。Intel Embree オフライン
ビルダーでは、「三角形分割」
技術を使用して、長くて薄い三角形の有効な表面領域を減らしますが、ランタイム
ビルダーではその技術は使っていません。オフライン
ビルダーでは、ランタイム ビルダーよりも高いレベルのクワッド
リーフを生成できるため、より小さな（メモリ単位の）BVH構造も生成します。最後に、ランタイム時のビルドにスクラッチ
メモリは必要ありません。これは、Xbox Series S
コンソールに特に役立ちます。

BVH の構築後、構造は CopyRaytracingAccelerationStructureAPI
を使用してシリアル化され、コピー モード
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZEで、ディスクに書き込まれます。

OfflineRT サンプル ( AddModelFromMDat 関数を参照) では、コピー モード
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZEで、CopyRaytracingAccelerationStructure
API 使って、GPU
上でアクセラレーション構造が再び逆シリアル化されます。非同期計算パイプで、他のグラフィックス処理と並行して逆シリアル化操作を実行することをお勧めします。

オフライン RTPSO は、RTBuilder で次の 3 つのステップで生成されます。

-   Scarlett シェーダー コンパイラ (DXC) を使用した DXIL ライブラリ
    (ターゲットlib_6\_6) への HLSL コンパイル

-   Scarlett PC UMD を使用した RTPSO の作成

-   Scarlett PC UMD を使用した RTPSO のシリアル化

初期コンパイルは、Visual Studio の カスタム ツール、*HLSL
Compiler*によって処理されます。プロジェクトの *Assets*
フォルダー内のいずれかの .hlsl
ファイルを右クリックすると、このプロセスに使用されるパラメーターを確認できます。中間
DXIL ライブラリは、サンプルの Build \\Int ディレクトリに保存されます。2
番目と 3 番目のステップは、RTBuilder
の実行時に行われます。このサンプルでは、コレクションと完全にリンクされた
RTPSO
の作成とシリアル化の両方がサポートされています。どちらの場合も、ShaderConfig、PipelineConfig、およびルート署名（s）がわかっている場合、PC
UMD は CreateStateObject が呼ばれると Xbox ネイティブ
シェーダーを完全にコンパイルします（DXR 仕様書の [Collection state
object](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#collection-state-object)
の項を参照してください）。また、シリアル化プロセス（SerializeStateObjectXコールで開始）では、状態オブジェクトからすべてのDXILと他のメタデータが削除されるため、ランタイムのサブオブジェクトの関連付けが不可能になることも重要な点です。ただし、シェーダーはサブオブジェクトではないため、シェーダー
バインド テーブルのシェーダー識別子を検索することはできます。

OfflineRT サンプルは、RTPSO を構築する 3 つの異なる方法を示しています。

-   DXIL ライブラリからのランタイムの作成 (
    AddPipelineFromEmbeddedDXILLib
    関数を参照)。このメソッドは、サブオブジェクトの完全な関連付けの動作を許可しますが、ランタイム時に完全なコンパイルとリンクのコストが発生するため、最大限の柔軟性を備えています。

-   オフライン コレクションに基づくランタイムの作成 (
    AddPipelineFromSerializedCollections
    を参照)。すべてのシェーダーが完全にプリコンパイルされ、内部的にリンクされているため、状態オブジェクトの作成は高速です。このメソッドは、RTPSO
    を複数の (共有される可能性がある) コレクションから「構成」
    できるようにすることで、ある程度の柔軟性を維持します。

-   ランタイム逆シリアル化 ( AddPipelineFromSerializedRTPSO
    関数を参照)。状態オブジェクトの作成は、すべてがプリコンパイルされリンクされているため高速です。実際の柔軟性はありません。

# 既知の問題

関連する Scarlett グラフィックス ドライバーのバグ。

-   バグ33668549:グラフィックス:XDXR
    は、D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTIONを介して RTPSO
    から参照される AddRef コレクションを正しく行いません

# 更新履歴

-   2021 年 6 月初回リリース。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
