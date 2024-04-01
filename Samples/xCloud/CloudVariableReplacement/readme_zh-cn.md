# 云变量替换示例

*该示例与 Microsoft 游戏开发工具包（2022 年 10 月）兼容*

# 说明

该示例演示如何从游戏中更改触摸适配工具包的状态。

![文本 描述自动生成](./media/image1.jpeg)

# 使用示例

在启动示例之前，请确保已启用游戏流式传输。 此外，请确保在流式传输客户端 (（例如 Xbox 游戏流式传输测试应用）) 中启用&ldquo;设置 > 开发人员 > 触摸适配 > 启用旁加载&rdquo;。 使用流式传输客户端应用连接到运行示例的控制台。 连接后，该示例应更改以反映流式传输客户端的存在。 在游戏命令提示符中运行以下命令，确保加载&ldquo;sample-layouts&rdquo;捆绑包：

```
tak serve --takxconfig sample-layouts\takxconfig.json
```


左右按压 dpad 键，可更改 TAK 上 B 按钮的不透明度。 向上按 dpad 键可切换 Y 按钮的可见性，向下按 dpad 键可切换 A 按钮的启用状态。

# 实现说明

此示例演示如何使用适用于 xCloud 的云感知 API。

如需了解更多信息，请参阅以下文档：https://learn.microsoft.com/gaming/gdk/_content/gc/system/overviews/game-streaming/building-touch-layouts/game-streaming-touch-changing-layouts-game-state

# 版本历史记录

2024 年 4 月：
- 更新了示例以使用`OnClient` API，从而实现按客户端控制屏幕上的触摸布局。
- 向 `sample-layouts` 添加了 `takxconfig.json`。

2021 年 7 月：初始示例

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


