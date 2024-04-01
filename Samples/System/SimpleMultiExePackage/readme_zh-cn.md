# 简单 MultiExecutable 包示例
_此示例与 Microsoft 游戏开发工具包（2022 年 3 月）映像兼容 _ ![](SampleImage.png)


### 说明
演示如何设置具有多个可执行文件的解决方案的示例。 此实现是通过创建多个项目，并按允许将它们打包一起运行的方式进行设置来完成的。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

如果在个人电脑上运行，请将活动解决方案平台设置为 `Gaming.Xbox.Desktop.x64`。

### 运行示例。

#### 方法 1，从 Visual Studio 运行
- 只需单击 F5 即可运行示例。 默认体验将首先加载。

#### 方法 2，从包创建运行
- 还可以为此示例生成包，并从中运行它。
- 生成包。
   1. 生成项目。
   2. 在支持 Makepkg 的终端中运行 GenConsoleXVCPackage.bat 或 GenDesktopMSIXVCPackage.bat。
      1. 控制台包文件可以在 .\\DefaultExperience\\$Target\\Layout\\Image\\ 中找到。
      2. 桌面包文件可以在 .\\Gaming.Desktop.x64\\Layout\\Image 中找到
- 安装和运行包。
   1. 如果在 Xbox 上运行，请通过 Xbox 管理器将 .xvc 文件复制到开发工具包中。 还可以使用 xbapp install 安装包。
   2. 如果在桌面上运行，请通过 WDAPP install 安装 .MSIXVC 文件。


## 更新历史记录

**初始版本：**Microsoft 游戏开发工具包（2023 年 6 月） 

2023 年 6 月：初始版本

## 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


