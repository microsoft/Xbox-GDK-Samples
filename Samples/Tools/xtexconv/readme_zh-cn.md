# xtexconv 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例是一个电脑端命令行工具，它扩展了用于纹理转换和准备的标准 TexConv 命令行工具，以支持 Xbox One 脱机纹理平铺，以便与 **CreatePlacedResourceX** API 配合使用。

此工具接受各种图像格式，例如 Windows 图像处理组件支持的编解码器 .jpg、.png、.tiff、.bmp 和 HD Photo/JPEG XR 以及 Targa Truevision .tga 文件、RGBE .hdr 和 OpenEXR .exr 文件，以及 .dds 文件作为输入纹理格式。 它支持使用用户指定的筛选器生成完整的 mip 链，以及支持纹理阵列、多维数据集映射、多维数据集映射数组和卷映射。

运行不带任何参数的工具将显示帮助屏幕，如下所示：

![](./media/image1.png)

# 用法

XTexConv 工具支持与标准 TexConv 工具相同的命令行参数和语法集。 [GitHub](https://github.com/Microsoft/DirectXTex/wiki/Texconv) 上提供了详细的文档。

它包括一个附加开关 `-xbox`，可以令输出 DDS 文件包含 Xbox One 平铺纹理数据和&ldquo;XBOX&rdquo;DDS 文件变体。 使用的磁贴模式由 XGComputeOptimalTileMode 确定。 假定这些脱机准备 Xbox One 纹理与 XG_BIND_SHADER_RESOURCE 一起使用。

如果将&ldquo;XBOX&rdquo;DDS 文件变体用作输入文件，则在进一步处理之前会自动将其删除，从而允许使用该工具将&ldquo;XBOX&rdquo;DDS 文件转换为标准 DDS 文件。

该工具还支持用于选择用于平铺的硬件版本的开关 `-xgmode`。

使用 `-xgmode:xboxonex` 设置 Xbox One X 的首选平铺。 否则，它默认为 Xbox One/Xbox One S。

## Xbox 系列 X\|S

第一，有两个版本的 xtexconv。xteconv_xs 属于 Xbox Series
| | |
|---|---|
|由于 Xbox Series X|S 与 Xbox 有两个不同版本的 XG|
|X|S 版本，不支持 **-xgmode** 开关。|

# Xbox One 的 DDS 文件

[Microsoft Docs](https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide) 中对标准 DDS 文件格式进行了介绍。 &ldquo;XBOX&rdquo;DDS 文件变体类似于&ldquo;DX10&rdquo;标头扩展名。 &ldquo;XBOX&rdquo;DDS 文件的布局如下：

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


用于从&ldquo;XBOX&rdquo;变体 DDS 文件加载和创建纹理的示例代码可在 XboxDDSTextureLoader ([DX11](https://github.com/Microsoft/DirectXTK/wiki/XboxDDSTextureLoader) / [DX 12](https://github.com/Microsoft/DirectXTK12/wiki/XboxDDSTextureLoader)) 模块中的 *DirectX 工具套件* ([DX11](https://github.com/Microsoft/DirectXTK) /[ DX 12](https://github.com/Microsoft/DirectXTK12)) 中找到。

# Xbox One 的 DirectXTex

> XTexConv 是略微修改的 TexConv 版本，其附加功能已添加到 [DirectXTex](https://github.com/Microsoft/DirectXTex/) 库中。 GitHub 上提供了 [TexConv](https://github.com/Microsoft/DirectXTex/wiki/Texconv) 和 DirectXTex 的标准版本。

DirectXTex 的 Xbox One 辅助功能（在 Xbox C++ 命名空间的 DirectXTexXbox.h 中）包括：

- XboxImage：平铺纹理数据的容器

- 用于存储和加载 DDS 文件的 XBOX 变体的函数

   - GetMetadataFromDDSMemory

   - GetMetadataFromDDSFile

   - LoadFromDDSMemory

   - LoadFromDDSFile

   - SaveToDDSMemory

   - SaveToDDSFile

- 用于将标准线性数据平铺到 Xbox One 平铺纹理以及反向操作的函数：

   - 磁贴

   - Detile

- 使用 Direct3D 12 扩展从平铺 Xbox One 平铺图像创建纹理资源的函数

   - CreateTexture

   - FreeTextureMemory

# 依赖关系

此工具和 Xbox One 的 directXTex 辅助磁贴/Detile 函数要求 XG.DLL（位于 bin\\XboxOne 文件夹下的 Microsoft GDK 中）或 XG_XS.DLL（位于 bin\\Scarlett 文件夹下的 Microsoft GDK 中）位于标准 DLL 搜索路径中。

# OpenEXR 支持

xtexconv 工具使用 [OpenEXR](http://www.openexr.com/) 库，将 NuGet 包用于 [openexr](https://www.nuget.org/packages/openexr-msvc14-x64/) 和 [zlib](https://www.nuget.org/packages/zlib/) ，这些包受其自己的许可条款约束。 可以通过取消定义 USE_OPENEXR、删除 DirectXTexEXR.\* 以及通过 NuGet 管理器删除包来禁用此支持。

请注意，OpenEXR 受其自己的约束
[许可证](https://github.com/openexr/openexr/blob/develop/OpenEXR/LICENSE)
和 [zlib](http://zlib.net/zlib_license.html) 一样。

有关其他详细信息，请参阅 [添加 OpenEXR](https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR)。

# 更新历史记录

| 日期 | 说明 |
|---|---|
| 2019 年 2 月 | 初始版本。 |
| 2019 年 11 月 | Xbox Series X | S 支持。 |
| 2020 年 2 月 | XG 库更改的更新。 |


