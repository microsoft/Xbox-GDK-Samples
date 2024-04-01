![](./media/image1.png)

# Meshlet 转换工具

*\* 此工具与电脑兼容。*

# 说明

Meshlet 转换器是一种在电脑上使用的命令行工具。 Visual Studio 解决方案包含三个项目：

- ConverterApp -- 一种可执行命令行工具，使用 DirectXMesh 生成 meshlet 数据

- 运行时 -- 包含 meshlet 数据结构的运行时版本的静态库项目

ConverterApp 项目是一个命令行工具，可用于从 FBX 文件、OBJ 文件或 SDKMesh 文件生成 meshlet 数据。 该工具利用 meshlet 生成的 DirectXMesh 集成，从顶点和从输入 FBX 文件读取的基元数据生成 meshlet。

运行时项目提供自包含的运行时 meshlet 代码，该代码演示如何在运行时反序列化和上传 meshlet。 它不是独立的演示，但可以轻松集成到现有基本代码中。

# 设置

若要编译该工具，需要安装 FBX SDK 2019.2。 安装后，将名为&ldquo;FBX_SDK&rdquo;的环境变量配置为指向安装目录（通常为 *C:\\Program Files\\Autodesk\\FBX\\FBX SDK\\2019.2*）。

# 使用示例

命令行工具只有几个选项：

- -h -- 显示帮助消息

- -v \<int\> - 指定 meshlet 的最大顶点计数。 必须介于 32 和 256（含） 之间。 默认值为 128

- -p \<int\> - 指定 meshlet 的最大基元计数。 必须介于 32 和 256（含） 之间。 默认值为 128

- -s \<float\> - 指定场景几何图形的全局缩放因子。 默认值为 1.0

- -fz -- 翻转场景几何图形的 Z 轴。 默认值为 false

- -ft -- 翻转场景几何图形的三角形绕行顺序。 默认值为 false

- -i -- 强制顶点索引为 32 位，即使 16 位足够。 默认值为 false

- -t - 使用 FbxGeometryConverter 功能对场景 Mesh 文件进行三角化。 默认值为 false

- \<文件列表\> - 要处理的相对文件路径的列表。 必须至少提供一个。

示例用法可以是：

ConverterApp.exe -v 256 -p 256 -f Path/To/MyFile1.fbx Path/To/MyFile2.fbx

# 实现说明

命令行工具不会修改或导出 Mesh 顶点数据。 可以在命令行上指定自动 FBX SDK 三角化。

由于 FBX 文件可能包含多个 Mesh ，导出的文件可能会打包多个 meshlet 集。 当前没有按 Mesh 名称为不同 meshlet 编制索引的方案，但可能会在以后的迭代中添加。 Mesh 根据 FBX 节点树的顺序、广度优先遍历进行处理和导出。

# 用法说明

在将 Mesh 转换为引擎运行时格式的过程中，必须小心确保没有索引或顶点数据的重新排序。 由于顶点数据不是使用命令行工具导出的，任何重新排序都会使 meshlet 数据失效。

# 更新历史记录

2019/12/2 &ndash; 创建示例。

2019/2/20 -- 重新路由 meshlet 生成器以支持不同的顶点/基元计数、更一致的空间和方向属性。

2020/4/11 -- 使用更精简的 DirectXMesh 接口替换了 meshlet 生成接口。

2022/10/17 -- 添加了对从 SDKMesh 文件读取的支持。


