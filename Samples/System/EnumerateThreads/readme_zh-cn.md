![](./media/image1.png)

# EnumerateThread 示例

*此示例与 Microsoft 游戏开发工具包(2022 年 3 月)兼容*

# 说明

[ToolHelp](https://docs.microsoft.com/windows/win32/api/tlhelp32/) API 提供用于枚举游戏进程内运行的所有线程。 虽然这些 API 在 OneCore 库中可用，但它们当前在 WINAPI_PARTITION_GAMES 中不可用，需要一些额外代码才能使它们对游戏代码可用。 当前的解决方案是在使用以下代码包含 TlHelp32.h 之前重新定义 WINAPI_FAMILY_PARTITION。

//注意: 游戏分区中当前尚未定义工具帮助 API。 这是强制 ToolHelp API 可供游戏调用的解决方法。 在 Xbox 主机上，游戏还需要连接到 onecore_apiset.lib 库。 此时，可以毫无问题地使用 ToolHelp API。

> # undef WINAPI_FAMILY_PARTITION
>
> # 定义 WINAPI_FAMILY_PARTITION (分区) 1
>
> # 包含 \<TlHelp32.h\>
>
> # undef WINAPI_FAMILY_PARTITION
>
> # 定义 WINAPI_FAMILY_PARTITION (分区) (分区)

# 使用示例

该示例将创建五个后台线程，然后枚举并列出进程中运行的所有线程及其名称和优先级。

# 更新历史记录

初始版本 2022 年 8 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


