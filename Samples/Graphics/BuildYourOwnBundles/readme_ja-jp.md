![](./media/image1.png)

# 各自のバンドル サンプルを構築する

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

Xbox One では、バッファーにデータを直接書き込むことができます。これは ExecuteIndirectBundleX API を使用して、GPU が直接読み取り、実行できます。 このサンプルでは、このデータを書き出し、通常の描画と描画バンドルを比較するさまざまな手法について説明します。 バッファー データは、CPU または GPU によって書き込まれ、後続のパスで GPU に送ることができます。 この機能は独自バンドル構築 (BYOB) と呼ばれます。 "高周波" API 呼び出しはすべて、バッファーに直接書き込むことができます。 コードは d3d12_x.h から使用できるものと同じですが、コマンド バッファーに直接書き込むのではなく、指定されたバッファーに書き込まれます。

GPU 上で実行されるバッファーにデータを書き込むときに留意すべき点は、次のとおりです。

- バッファーは、PAGE_GPU_EXECUTE フラグを使用して作成する必要があります

- 実行するバッファー サイズは 4 MB を超えてはなりません

- 間違ったパケット情報を渡すと、GPU がハングします

- GPU によって無視されるカスタム データを提供するために使用できる NO_OP パケットがあります

- 書き込み結合バッファーを使用して GPU から書き込む場合は、すべての書き込み結合規則を確認してください。

このサンプルでは、メッシュのインスタンスを描画する複数の方法を示します。 複数の PSO が使用され、インスタンスにランダムに割り当てられます。 GPU の時間は各メソッドで同じです。 16384 メッシュの場合、
カリングなしで 80.3 ミリ秒、カリング付きで 39.2 ミリ秒 (7439 メッシュを描画)。 CPU 時間はさまざまで、メソッドの説明と共に一覧表示されます。 描画メソッド:

- **BYOB - 各インスタンスを更新して描画する**

この手法では、各インスタンスの BYOB バンドル データを更新し、そのインスタンスの ExecuteIndirectBundleX を呼び出します。 この手法を使用するバッファーは、1 つのインスタンスのデータしかないため、サイズが小さくなります。 各インスタンスの描画データが GPU に 1 つずつ送信されます。 CPU: カリングなし = 7.01 ミリ秒、カリング = 6.65 ミリ秒

- **BYOB - すべてのインスタンスを更新してから描画する**

すべてのインスタンスの BYOB バンドル データを更新して 1 つのバッファーに書き込み、最後にバッファーを GPU に渡します。 この手法に使用されるバッファーのサイズは以前の手法よりも大きくなりますが、すべてのデータは 1 つの ExecuteIndirectBundleX で GPU に送信されます。 CPU: カリングなし = 33.5 ミリ秒、カリング = 19 ミリ秒

- **BYOB - 実行時にバンドルを構築する**

実行時にバッファー全体をビルドし、1 つの ExecuteIndirectBundleX コマンドでバッファーを送信します。 CPU: カリングなし = 7.4 ミリ秒、カリング =
6.8 ミリ秒

- **BYOB - GPU を使用した描画**

GPU パケット データを書き込み、コンピュート シェーダーを使用してすべてのメッシュ インスタンスをバッファーに描画します。ExecuteIndirectBundleX を使用してそのバッファーを GPU に渡します。 カリングは GPU で実行されます。 CPU: カリングなし = 6.0 ミリ秒、カリング = 6.0 ミリ秒

- **バンドルを使用して描画する**

この手法では、通常のバンドルを使い ExecuteBundle API を使用してデータを描画します。 CPU: カリングなし = 6.9 ミリ秒、カリング = 6.5 ミリ秒

- **直接描画**

DrawIndexedInstanced を使用してメッシュを直接描画します。 CPU: カリングなし =
7.7 ミリ秒、カリング = 6.9 ミリ秒

CPU に対するすべてのバッファー書き込みは、1 つのスレッドを使用して実行されます。 複数のスレッドを使用して実行可能バッファーに書き込む場合、パフォーマンス上の利点があります。 各状態設定および描画のサイズが一定に保たれている場合、複数のスレッドが CPU 上のバッファーに簡単に書き込むことができます。 NO_OP パケットを使用して、バッファー内の未使用の領域を埋めることができます。 GPU を介してパケット データを書き込むと CPU 時間が短縮され、パケット データの書き出しはコストがかかりません。 このメソッドは、テストされたサンプルの中で最も速い方法です。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

## Screenshot

![](./media/image3.png)

| 操作 | ゲームパッド |
|---|---|
| 描画手法を変更する | A または B ボタン |
| カリング メッシュ | X button |
| 選択した手法に関する詳細な説明を表示する/非表示にする | Y button |
| Exit | ビュー ボタン |

# 実装メモ

バンドルを書き出すサンプルで使用されるすべてのコードは、WriteOwnBundlesHelper.h で入手できます。 バッファーへのパケット データの書き込みは、ヘッダー ファイル d3d12_x.h からのコードに基づいています。

たとえば、DrawIndexedInstanced は次のように定義されます。

```cpp
D3DINLINE void D3DAPI DrawIndexedInstanced(
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    D3D12XBOX_PPUT pPut = m_Putter.m_pCurrent;
    if (pPut < m_Putter.m_pLimit_Draw)
    {
        m_Putter.PutD(pPut, D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED);
        m_Putter.PutD(pPut, InstanceCount);
        m_Putter.PutD(pPut, StartIndexLocation);
        m_Putter.PutD(pPut, IndexCountPerInstance);
        m_Putter.PutD(pPut, BaseVertexLocation);
        m_Putter.PutD(pPut, StartInstanceLocation);
        m_Putter.m_pCurrent = pPut;
    }
    else
    {
        CommandListFunction()-\>DrawIndexedInstanced(this,
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation);
    }
}
```


バンドルを書き出す際は、次のコードに変更してください。

```cpp
void DrawIndexedInstancedBYOB(
    _Inout_ UINT32** writeAddress,
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    **writeAddress = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
    *(*writeAddress + 1) = InstanceCount;
    *(*writeAddress + 2) = StartIndexLocation;
    *(*writeAddress + 3) = IndexCountPerInstance;
    *(*writeAddress + 4) = BaseVertexLocation;
    *(*writeAddress + 5) = StartInstanceLocation;
    *writeAddress += 6;
}
```


# 既知の問題

ExecuteIndirectBundleX を使用している場合、PredicationBuffer は現在機能しません。 PredicationBufferOffset に 0 を渡します。

# 更新履歴

- 2019 年 4 月: 初期リリース

- 2019 年 11 月: プロジェクト Xbox Series X|S をサポートするため更新


