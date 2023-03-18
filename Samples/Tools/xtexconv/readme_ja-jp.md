# xtexconv サンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 説明

このサンプルは PC 側のコマンドライン
ツールです。このツールは、テクスチャ変換および準備用の標準的な TexConv
コマンドライン ツールを拡張し、Xbox 1 オフライン テクスチャ
タイルをサポートしており、Createside Edresourcex API で使用できます。

このツールは、.jpg,、.png、.tiff、.bmp、HD Photo/JPEG XR および Targa
Truevision .tga ファイル、RGBE .hdr、OpenEXR .exr ファイル、.dds
など、入力テクスチャ形式として、さまざまな種類の画像形式をサポートしています。また、ユーザーが指定したフィルターを使用する完全な
mip チェーンのほか、テクスチャ配列、キューブ
マップ、キューブマップ配列、ボリューム マップもサポートしています。

パラメーターを指定しないでツールを実行すると、次のようなヘルプ画面が表示されます。

![](./media/image1.png)

# 使用法

XTexConv ツールは、標準の TexConv ツールと同じコマンドライン
パラメーターおよび構文をサポートしています。詳細なドキュメントは
[GitHub](https://github.com/Microsoft/DirectXTex/wiki/Texconv)
で入手できます。

このツールには追加スイッチ \`**-xbox**\`
が含まれています。これにより、出力 DDS ファイルに Xbox One タイル
テクスチャ データと \`XBOX\` DDS
ファイルのバリアントが含まれるようになります。使用されるタイルモードは
XGComputeOptimalTileMode
によって決まります。これらのオフラインの準備済み Xbox One
テクスチャは、XG_BIND_SHADER_RESOURCE で使用される予定です。

\`XBOX\` DDS
ファイルのバリアントが入力ファイルとして使用されている場合、その入力ファイルは自動的にタイル表示解除されるため、このツールを使用すると、\`XBOX\`
DDS ファイルを標準の DDS ファイルに変換できます。

また、このツールでは、\`**-xgmode**\`スイッチを使用して、タイルのハードウェア
バージョンを選択することができます。

Xbox One X の優先タイルを設定するには、\`-xgmode:xboxonex\`
を使用します。それ以外の場合は、既定で Xbox One / Xbox One S
に設定されます。

## Project Scarlett 

Xbox One と比較して、Project Scarlett には 2 つの異なるバージョンの XG
があるように、xtexconv にも 2 つのバージョンがあります。xteconv_xs は
Project Scarlett バージョンで **-xgmode**
スイッチをサポートしていません。

# Xbox One の DDS ファイル

標準的な DDS ファイル形式については、[Microsoft
Docs](https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide)
を参照してください。\' XBOX \' DDS ファイル バリアントは、\' DX10 \'
ヘッダー拡張機能と似ています。\' XBOX \' DDS
ファイルの構造は次のとおりです。

> DWORD dwMagic
>
> DDS_HEADER header
>
> DDS_HEADER_XBOX
>
> {
>
> DXGI_FORMAT dxgiFormat;
>
> uint32_t resourceDimension;
>
> uint32_t miscFlag; // see DDS_RESOURCE_MISC_FLAG
>
> uint32_t arraySize;
>
> uint32_t miscFlags2; // see DDS_MISC_FLAGS2
>
> uint32_t tileMode; // see XG_TILE_MODE / XG_SWIZZLE_MODE
>
> uint32_t baseAlignment;
>
> uint32_t dataSize;
>
> uint32_t xdkVer; // matching \_XDK_VER
>
> } headerXbox
>
> \<Remainder of file is a tiled texture binary layout suitable for use
> with CreatePlacement APIs\>

\' XBOX \' バリアント DDS
ファイルからテクスチャを読み込んで作成するコードの例は、XboxDDSTextureLoader
([DX11](https://github.com/Microsoft/DirectXTK/wiki/XboxDDSTextureLoader)
/ [DX
12](https://github.com/Microsoft/DirectXTK12/wiki/XboxDDSTextureLoader))
モジュールの *DirectX Tool Kit*
([DX11](https://github.com/Microsoft/DirectXTK) / [DX
12](https://github.com/Microsoft/DirectXTK12)) にあります。

# Xbox One の DirectXTex

# XTexConv は TexConv を少し変更したバージョンであり、[DirectXTex](https://github.com/Microsoft/DirectXTex/) ライブラリに機能が追加されています。[TexConv](https://github.com/Microsoft/DirectXTex/wiki/Texconv) と DirectXTex の標準バージョンは、GitHub にあります。

# DirectXTex 向けの Xbox One 補助関数 ( Xbox C++ 名前空間のDirectXTexXbox.h) には、次のものが含まれます。

-   # XboxImage: タイル テクスチャ データのコンテナー

-   DDS ファイルの XBOX バリアントを保存およびロードする関数

    -   GetMetadataFromDDSMemory

    -   GetMetadataFromDDSFile

    -   LoadFromDDSMemory

    -   LoadFromDDSFile

    -   SaveToDDSMemory

    -   SaveToDDSFile

-   標準の線形データを Xbox One のタイル
    テクスチャにタイリングする機能およびその逆の操作を実行する関数:

    -   Tile

    -   Detile

-   タイリングした Xbox One のタイル画像から Direct3D 12
    拡張機能を使ってテクスチャ リソースを作成する関数

    -   CreateTexture

    -   FreeTextureMemory

# 依存関係

このツールと Xbox One の Tile/Detile 補助関数の DirectXTex には、DLL
(bin\\XboxOne フォルダー内の Microsoft GDK) または XG_XS.DLL
(bin\\Scarlett フォルダー内の Microsoft GDK) が必要です。

# OpenEXR のサポート

xtexconv
ツールは、[openexr](https://www.nuget.org/packages/openexr-msvc14-x64/)
および [zlib](https://www.nuget.org/packages/zlib/)
(それぞれのライセンス契約の対象となります) の NuGet
パッケージを利用して[OpenEXR](http://www.openexr.com/)
ライブラリを使用します。このサポートは、USE_OPENEXR
の定義の無効化、DirectXTexEXR.\* の削除、および NuGet
マネージャーによるパッケージの削除によって無効にできます。

OpenEXR は [zlib](http://zlib.net/zlib_license.html)
と同じく、それ自体の[ライセンス契約](https://github.com/openexr/openexr/blob/develop/OpenEXR/LICENSE)の対象となっています。

詳細については、「[Adding
OpenEXR](https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR)」を参照してください。

# 更新履歴

