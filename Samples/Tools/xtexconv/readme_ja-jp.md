# xtexconv サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

このサンプルは、標準の TexConv コマンドライン ツールを拡張する PC 側のコマンドライン ツールであり、Xbox One のオフライン テクスチャ タイリングをサポートして **CreatePlacedResourceX API** で使用するためのテクスチャの変換および準備を行います。

このツールは、サポートされているコーデック .jpg、.png、.tiff、.bmp、HD Photo/JPEG XR、Targa Truevision .tga ファイル、RGBE .hdr、OpenEXR .exr ファイルなど、幅広い画像 Windows Imaging Component形式を受け入れます。また、入力テクスチャ形式として .dds\-- も受け入れます。 ユーザー指定のフィルターを使用した完全なミップ チェーンの生成に加えて、テクスチャ配列、キューブ マップ、キューブ マップ配列、ボリューム マップをサポートします。

パラメーターを指定せずにツールを実行すると、次のようにヘルプ画面が表示されます:

![](./media/image1.png)

# 使用方法

XTexConv ツールは、標準の TexConv ツールと同じコマンド ライン パラメーターと構文のセットをサポートします。 詳細なドキュメントは [GitHub](https://github.com/Microsoft/DirectXTex/wiki/Texconv) で入手できます。

これには、出力 DDS ファイルにタイル 化されたテクスチャ データと 'XBOX' DDS ファイル バリアントXbox One含める追加のスイッチ `-xbox` が含まれています。 使用されるタイル モードは、XGComputeOptimalTileMode によって決まります。 これらのオフラインで準備された Xbox Oneテクスチャは、XG_BIND_SHADER_RESOURCE で使用されるものと見なされます。

'XBOX' DDS ファイルバリアントを入力ファイルとして使用する場合、後続の処理の前に自動的にデタイル化され、ツールを使用して 'XBOX' DDS ファイルを標準 DDS ファイルに変換できます。

このツールでは、タイリングのハードウェア バージョンを選択するためのスイッチ `-xgmode` もサポートされています。

`-xgmode:xboxonex` を使用して、Xbox One X の優先タイリングを設定します。 それ以外の場合は、既定で Xbox One/Xbox One S になります。

## Xbox Series X|S

1 つは、xtexconv の 2 つのバージョンがあります。xteconv_xs は Xbox Series です
| | |
|---|---|
|Xbox Series X|S と Xbox の XG には 2 つの異なるバージョンがあるため|
|X|S バージョンは **-xgmode** スイッチをサポートしていません。|

# Xbox One の DDS ファイル

標準の DDS ファイル形式は、[Microsoft Docs](https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide) で説明されています。 'XBOX' DDS ファイルバリアントは、'DX10' ヘッダー拡張子に似ています。 'XBOX' DDS ファイルは、次のようにレイアウトされます。

```
DWORD dwMagic
DDS_HEADER header
DDS_HEADER_XBOX
{
   DXGI_FORMAT dxgiFormat;
   uint32_t resourceDimension;
   uint32_t miscFlag; // see DDS_RESOURCE_MISC_FLAG
   uint32_t arraySize;
   uint32_t miscFlags2; // see DDS_MISC_FLAGS2
   uint32_t tileMode; // see XG_TILE_MODE / XG_SWIZZLE_MODE
   uint32_t baseAlignment;
   uint32_t dataSize;
   uint32_t xdkVer; // matching \_XDK_VER
} headerXbox
<Remainder of file is a tiled texture binary layout suitable for use with CreatePlacement APIs\>
```


'XBOX' バリアント DDS ファイルからテクスチャを読み込んで作成するためのコード例は、XboxDDSTextureLoader ([DX11](https://github.com/Microsoft/DirectXTK) / [DX 12](https://github.com/Microsoft/DirectXTK12)) モジュールの *DirectX ツール キット* ([DX11](https://github.com/Microsoft/DirectXTK/wiki/XboxDDSTextureLoader) / [DX 12](https://github.com/Microsoft/DirectXTK12/wiki/XboxDDSTextureLoader)) で入手できます。

# Xbox One 用 DirectXTex

> XTexConv は少し変更されたバージョンの TexConv で、[DirectXTex](https://github.com/Microsoft/DirectXTex/) ライブラリに追加機能が追加されています。 [TexConv](https://github.com/Microsoft/DirectXTex/wiki/Texconv) と DirectXTex の標準バージョンは、GitHub で入手できます。

DirectXTex のXbox One補助機能 (Xbox C++ 名前空間の DirectXTexXbox.h) には、次のものが含まれます。

- XboxImage: タイル テクスチャ データ用のコンテナー

- DDS ファイルの XBOX バリアントを格納および読み込む関数

   - GetMetadataFromDDSMemory

   - GetMetadataFromDDSFile

   - LoadFromDDSMemory

   - LoadFromDDSFile

   - SaveToDDSMemory

   - SaveToDDSFile

- 標準の線形データを Xbox One のタイル化されたテクスチャにタイリングする機能と、その逆の操作を行う機能。

   - タイル

   - Detile

- Direct3D 12 拡張機能を使用してタイル化された Xbox One タイル画像からテクスチャ リソースを作成するための関数

   - CreateTexture

   - FreeTextureMemory

# 依存関係

このツールおよび補助タイル/デタイル関数 Xbox One DirectXTex を使用するには、XG.DLL (bin\\XboxOne フォルダーの Microsoft GDK にあります) または XG_XS.DLL (bin\\Scarlett フォルダーの下にある Microsoft GDK にあります) を標準 DLL 検索パスに配置する必要があります。

# OpenEXR のサポート

xtexconv ツールは [OpenEXR](http://www.openexr.com/) ライブラリを 使用し、独自のライセンス条項の対象となる [openexr](https://www.nuget.org/packages/openexr-msvc14-x64/) および [zlib](https://www.nuget.org/packages/zlib/) 用の NuGet パッケージを使用します。 このサポートは、USE_OPENEXR の定義解除、DirectXTexEXR.\* の削除、NuGet マネージャーによるパッケージの削除によって無効にできます。

OpenEXR は独自の
[ライセンス](https://github.com/openexr/openexr/blob/develop/OpenEXR/LICENSE)
[zlib](http://zlib.net/zlib_license.html) の場合と同様です。

詳細については、「 [OpenEXR](https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR) の追加」を参照してください。

# 更新履歴

| 日付 | 注意 |
|---|---|
| 2019 年 2 月 | 初回リリース |
| 2019 年 11 月 | Xbox Series X | S サポート。 |
| 2020 年 2 月 | XG ライブラリの変更の更新。 |


