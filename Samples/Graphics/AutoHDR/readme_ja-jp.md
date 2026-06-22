# Auto HDR サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

システム レベルで HDR を自動的に追加します。 機能
タイトルまたはシステム。 つまり、CPU や GPU、追加のメモリや帯域幅を使用せず、遅延も発生しません。 この機能は、下位互換性のある ERA タイトルの大部分に適用されますが、ネイティブの Xbox Series でも使用できます。
D3D デバイス作成フラグ `D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR` を使用した HDR 実装。 ![ビデオ ゲームのスクリーンショット 自動生成された説明](./media/image1.png)
| | |
|---|---|
|自動 HDR は、SDR を視覚的に強化できる Xbox Series X|S 機能です|
|Xbox Series X|S ハードウェアを使用するため、パフォーマンス コストは発生しません|
|X|S GDK タイトル。 このサンプルでは、GDK タイトルで自動 HDR を使用する方法を示します|


このサンプルで使用されている画像は、このサンプル デモで使用するために <https://www.halowaypoint.com/en-us> と <https://gearsofwar.com/en-US/> から取得されたものです

# サンプルのビルド

このサンプルは、`Gaming.Xbox.Scarlett.x64` を使用する Xbox Series X|S でのみサポートされています。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」 を参照してください。

# サンプルの使用方法

このサンプルでは、次のコントロールを使用します。

| 操作 | ゲームパッド |
|---|---|
| UI の明るさの調整を切り替える | A |
| 再構築された色の彩度を調整する | 方向パッドの左右 |
| 次の画像 | 右肩 |
| 前の画像 | 左肩 |

# 実装メモ

**SDR としてレンダリングする**

自動 HDR はシステム レベルで適用されるため、タイトルは単に SDR としてレンダリングする必要があります。 このサンプルは、SDR としてレンダリングおよび表示されます。

**スワップ バッファー形式**

バンディングなどの精度アーティファクトを避けるために、高精度スワップ バッファー形式を使用することを強くお勧めします。

```cpp
m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
                                                          DXGI_FORMAT_UNKNOWN,
                                                          2,
                                                          DX::DeviceResources::c_Enable4K_UHD);
```


**テレビを HDR モードに切り替え、自動 HDR フラグを使用して D3D デバイスを作成する**

タイトルはすべて SDR でレンダリングされますが、タイトルは引き続きテレビを HDR モードに切り替える必要があります。

```cpp
if (SwitchDisplayToHDR())
{
    params.CreateDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR;
}
```

**再開または制約解除後にテレビを HDR モードに切り替える**

タイトルが一時停止または制限されている間は、本体のディスプレイ設定が変更された可能性があるため、再開時にテレビを HDR モードに切り替える必要があります。

```cpp
void Sample::OnResuming()
{
    // Display modes could have changed while title was suspended, so we need to make
    // sure that the TV is still in HDR mode. This is required for native HDR and
    // Auto HDR.

    m_deviceResources->SwitchDisplayToHDR();
```


**UI の明るさを調整する**

```cpp
    if (isDisplayInHDRMode)
    {
        // Auto HDR will show pure white pixels as 1000 nits, so text/UI/HUD will become
        // much too bright. We linearly scale down the brightness of the UI
        m_UIBrightnessScale = m_bAdjustUIBrighness ? 0.8f : 1.0f;
    }
    else
    {
        // If the TV is in SDR mode, we don\'t do any brightness scaling
        m_UIBrightnessScale = 1.0f;
    }
}
```

# 更新履歴

初期リリース 2021 年 6月

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


