# 简单自定义 APO 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

此示例演示如何使用自定义 APO 效果在 Xbox 上使用 XAudio2 播放 wav 文件。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

如果使用 Xbox Series X|S，请将活动解决方案平台设置为 `Gaming.Xbox.Scarlett.x64`。

有关电脑上的等效示例，请参阅 [GitHub](https://aka.ms/xaudio2samples) 上的 **XAudio2CustomAPO**。

*有关更多信息，请参阅*&nbsp;__运行示例__（位于 *GDK&nbsp;文档）中。*

# 使用示例

该示例允许使用 Dpad 操作介于 0 和 1 之间的浮点参数。 &ldquo;视图&rdquo;按钮将退出示例。

# 实现说明

此示例使用与 **SimplePlaySound** 示例类似的设置，并将 xAPO 效果添加到源语音。

**SimpleAPO** 通过将处理的示例值相乘来应用简单的增益因子。

APO 是通过使用帮助程序模板类 SampleAPOBase 实现的，该类处理共享注册、类工厂和参数处理操作。 不需要使用此模板类，但它用于简化示例。

# 已知问题

如果使用 Windows SDK (22000)，则在使用该 xapobase.lib 时，链接时将收到以下错误：

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"long \_\_cdecl StringCchPrintfW(unsigned short \*,unsigned \_\_int64,unsigned short const \*,\...)\" (?StringCchPrintfW@@YAJPEAG_KPEBGZZ) 中引用了未解析的外部符号 \_vsnwprintf

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"enum FEATURE_ENABLED_STATE \_\_cdecl wil::details::GetFeatureEnabledStateHelper(unsigned int,enum FEATURE_CHANGE_TIME,int \*)\" (?GetFeatureEnabledStateHelper@details@wil@@YA?AW4FEATURE_ENABLED_STATE@@IW4FEATURE_CHANGE_TIME@@PEAH@Z) 中引用了未解析的外部符号 GetFeatureEnabledState

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"unsigned int \_\_cdecl wil::details::GetFeatureVariantHelper(unsigned int,enum FEATURE_CHANGE_TIME,unsigned int \*,int \*,int \*)\" (?GetFeatureVariantHelper@details@wil@@YAIIW4FEATURE_CHANGE_TIME@@PEAIPEAH2@Z) 中引用了未解析的外部符号 GetFeatureVariant

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) 中引用了未解析的外部符号 RecordFeatureUsage

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) 中引用了未解析的外部符号 RecordFeatureError

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) 中引用了未解析的外部符号 SubscribeFeatureStateChangeNotification

xapobase.lib(oapmatrixmix.obj) : 错误 LNK2019: 函数 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) 中引用了未解析的外部符号 UnsubscribeFeatureStateChangeNotification

最佳解决方案是使用没有此问题的较新版 Windows SDK。 较旧的 SDK 也没有此 bug。

或者，可以与 SHCORE.LIB 链接，但请注意，这可能会允许游戏操作系统不支持的 API 进行链接。

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


