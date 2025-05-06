<div style="float: center"><img style="float: left" src="./media/image1.png" /><img style="float: right" src="./media/image2.png" /> <br/><br/><br/><br/><br/></div>

# FidelityFX 変数のシェーディング サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

このサンプルでは、シーンに FidelityFX 変数シェーディング アルゴリズムを適用する方法を示します。

![](./media/image3.jpg)

![](./media/image4.png)

# サンプルのビルド

Windows デスクトップを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Desktop.x64` に設定します。

Xbox Series X|S を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

このサンプルでは、Xbox One はサポートされていません。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

| 操作 | ゲームパッド |
|---|---|
| サイクル シェーディング レート イメージ生成モード | A button |
| シェーディング レート イメージ オーバーレイの切り替え | X button |
| 回転ビュー | 左サムスティック |
| ビューのリセット | 左サムスティック (クリック) |
| 分散カットオフの増加/減少 | 方向パッド (上/下) |
| Exit | ビュー ボタン |

# 実装メモ

FidelityFX Variable Shading 手法は、後続のフレームで使用するためにシーンの輝度に基づいてシェーディング レート イメージを生成します。 この手法は、同様の色出力を含むプリミティブのサーフェイス全体で、高コストのピクセル シェーダー呼び出しの頻度を減らすことを目的としており、高い解像度で帯域幅と計算要件を削減します。

可変レート シェーディングが有効になっている場合、この手法によって生成されるシェーディング レート イメージが使用中で、シェーディング レート コンバイナーが適切に設定されている間、レンダリングされたプリミティブは、プリミティブがカバーするタイルのシェーディング レート イメージによって定義されたシェーディング レートを利用します。

このアルゴリズムの実装の詳細については、<https://gpuopen.com/fidelityfx-variable-shading/> にある <https://github.com/GPUOpen-Effects/FidelityFX-VariableShading/blob/master/docs/FFX-VariableShading-Introduction.pdf> の詳細なプレゼンテーションを参照してください。

## 統合

`variance cutoff` を大きくすると、アルゴリズムが低いシェーディング レートから高いシェーディング レートに切り替わる前に、ピクセル間の輝度分散が増加します。 `variance cutoff` を下げるとシェーディング レートが増加しますが、`variance cutoff` を大きくするとシェーディング レートが低下します。

## CPU 側のセットアップ

ファイル ffx_variable_shading.h を CPU コードに含めます。

次の構造体は、CPU によって入力する必要があります。

```cpp
struct FFX_VariableShading_CB
{
    uint32_t width, height; // Width and height of scene color source specified by
                            // Texture2D texColor within the shader.
    uint32_t tileSize;      // Tile size returned within
                            // D3D12_FEATURE_DATA_D3D12_OPTIONS6::ShadingRateImageTileSize.

    float varianceCutoff;   // A value between 0.0 and 1.0.
    float motionFactor;     // Currently unused.
};
```


リソース記述子は、次のルート署名に適切に設定する必要があります。

```cpp
#define ComputeVariableShadingRS \
    "CBV(b0, visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(SRV(t0 , numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)," \
    "DescriptorTable(UAV(u0, numDescriptors=1), visibility=SHADER_VISIBILITY_ALL)"
```


`b0` は、FFX_VariableShading_CB0 の CBV 記述子のベース シェーダー レジスタに対応します。

`t0` は、texColor という名前のシーン入力色を含む記述子のベース シェーダー レジスタに対応します。

`u0` は、imgDestination というアルゴリズムによって出力されるシェーディング レート イメージの UAV 記述子のベース シェーダー レジスタに対応します。

imgDestination の予想サイズは、ヘルパー関数 FFX_VariableShading_GetVrsImageResourceDesc によって決定できます。

予期されるディスパッチ サイズは、ヘルパー関数 FFX_VariableShading_GetDispatchInfo によって決定できます。

`D3D12_FEATURE_DATA_D3D12_OPTIONS6::ShadingRateImageTileSize` (Scarlett では常に 8) と `D3D12_FEATURE_DATA_D3D12_OPTIONS6::AdditionalShadingRatesSupported` に基づいた、実行するシェーダーの正しいバリアントの選択
シェーダー バリアントは VRS_ImageGen_Shader_TileSize8.hlsl です。 注: UI に表示されるワークロードのタイミングは、電源プロファイルによってデスクトップ構成によって異なる場合があります。最も正確なタイミングでは、PIX のプロファイルが最も正確です。
| | |
|---|---|
|(Scarlett では常に FALSE)。 Xbox Series X|S では、これは正しいことを意味します|


# 更新履歴

このサンプルは 2021 年 1 月に作成されました。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。

# 免責事項

ここに記載されている情報は情報提供のみを目的としており、予告なしに変更される場合があります。 このドキュメントの準備段階ではすべての予防措置が講じられていますが、技術的な不正確さ、省略、文字体裁上の誤りが含まれている可能性があり、AMD は、この情報を更新または修正する義務を負いません。 Advanced Micro Devices, Inc. は、本ドキュメントの内容の正確性または完全性に関して一切の表明または保証を行いません。また、AMD ハードウェア、ソフトウェア、またはここに記載されているその他の製品の運用または使用に関して、特定の目的に対する非侵害、商品性、適合性に関する暗黙の保証を含め、いかなる種類の責任も負いません。 このドキュメントでは、いかなる知的財産権に対しても、黙示的または禁反言によるライセンスは付与されません。 AMD 製品の購入または使用に適用される使用条件と制限は、両当事者間の署名済み契約または AMD の標準販売条件に記載されています。

AMD、AMD Arrow ロゴ、Radeon、RDNA、Ryzen、およびその組み合わせは Advanced Micro Devices, Inc. の商標です。 この文書で使用されるその他の製品名は、識別のみを目的としており、それぞれの会社の商標である可能性があります。

Windows は、米国およびその他の国における Microsoft Corporation の商標です。

Xbox は、米国およびその他の国における Microsoft Corporation の商標です。

© 2021 Advanced Micro Devices, Inc. All rights reserved.


