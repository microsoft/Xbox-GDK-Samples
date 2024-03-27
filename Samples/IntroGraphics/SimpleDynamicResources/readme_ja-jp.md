# 単純な動的リソースのサンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2021 年 4 月)*

# 説明

このサンプルでは、HLSL シェーダー モデル 6.6 の HLSL 動的リソースの使用方法を示します。 これは SimpleTexture と機能的には同じですが、リソースが HLSL の ResourceDescriptorHeap\[\] と SamplerDescriptorHeap\[\] を使用するヒープを介して直接アクセスするリソースが異なります。

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

PC 用のビルド (Gaming.Desktop.x64) には、HLSL SM 6.6 機能が使用されているため、[DirectX Agility SDK](https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/) が必要となります。 Agility SDK は、サンプル内には NuGet パッケージとして含まれています。 また、Microsoft.Windows.SDK.CPP NuGet パッケージを使用して、DXC.exe コンパイラの最新バージョンの Windows SDK (22000) を取得します。 開発者は、[Github](https://github.com/microsoft/DirectXShaderCompiler/releases) から最新の DXC を直接使用することもできます。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

このサンプルには、終了以外のコントロールはありません。

# 実装メモ

このサンプルでは、SimpleTexture からほぼすべてのコードを借用します。 唯一の違いは、リソースへのアクセスです。

このサンプルでは、バインドされたリソースをルート署名から削除し、HLSL シェーダー コードで ResourceDescriptorHeap\[\] と SamplerDescriptorHeap\[\] アクセスに置き換えます。 これには、SetGraphicsRootSignature() の前に SetDescriptorHeaps() が呼び出されていることを確認し、フラグCBV_SRV_UAV_HEAP_DIRECTLY_INDEXED と SAMPLER_HEAP_DIRECTLY_INDEXED をルート署名に追加する必要があります。

HLSL 6.6 動的リソースの詳細については、「 [HLSL SM 6.6 動的リソース](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html)」を参照してください。

動的リソースの詳細な使用方法については、Graphics\\VisibilityBuffer サンプルを参照してください。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


