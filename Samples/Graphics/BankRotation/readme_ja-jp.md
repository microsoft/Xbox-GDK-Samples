![](./media/image1.png)

# バンク回転のサンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

*バンク 回転*は、同じ描画呼び出しで複数のレンダー ターゲットへの書き込みを高速化するのに役立つ手法です。 これが便利なのは、複数の GBuffer に書き込むときに遅延パスにある間です。 ただし、役に立つのは、バンク回転としてターゲットが DRAM にあり、ESRAM 内のターゲットに影響しない場合のみです。

Xbox One と Xbox One S には 8 つのバンクがあり、Xbox One X には 16 のバンクと Xbox があります
すべての GBuffer が DRAM に配置されるため、X です。 **観察された結果:**
| | |
|---|---|
|シリーズ X|S には 2 つのバンクがあります。 この機能は、Xbox One で特に便利です|


4 GBuffers を使用すると、サンプルでは、各 GBuffer が別のバンクに割り当てられている Xbox Oneで約 **10% から 14%** のゲインが見られ、Xbox One X では約 **20%** が見られます。 パスに GBuffer が多い場合、ゲインは増加します。

# ![](./media/image3.png) サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

| 操作 | ゲームパッド |
|---|---|
| リソースの割り当ての手法を変更する | A button |
| Exit | ビュー ボタン |

# 実装メモ

バンク回転は、DirectX 12 API を使用する場合にのみのサポートされます。
よく似ています。サンプルのソース コードを参照してください。 *コミットされたリソース:*
| | |
|---|---|
|Xbox One のコード例を次に示します。 Xbox Series X|S のコードは|


```cpp
ResDesc.Layout = D3D12XBOX_BANK_ROTATED_TILE_MODE(D3D12_TEXTURE_LAYOUT, bankRotationIndex);
```

この新しいレイアウトは、リソースの作成時に CreateCommittedResource と共に使用できます。

*配置されたリソースまたはコンポーネント配置されたリソース:*

```cpp
D3D12_GPU_VIRTUAL_ADDRESS rotatedAddress;

assert(XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)gpuAddress, 
                                    &xgResLayout[gbufferIndex], 
                                    0, 
                                    0, 
                                    bankRotationIndex, 
                                    &rotatedAddress));
```


この新しい rotatedAddress を使用して、リソースを配置できます。

# 既知の問題

なし

# 更新履歴

初期リリース 2017 年 5 月

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


