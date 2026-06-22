  ![](./media/image1.png)

#   Scarlett/XboxOne Unity GDK Rumble 示例

*\* 此示例是使用 Unity 2020.3.12f1 和 Win 10 的 Microsoft GDK Feb QFE2
2021 开发的。*

# 

# 说明

此示例是 NetRumble 示例的端口，通常包含在 Microsoft GDK
下载门户页上，如下所示：

![A picture containing text Description automatically generated](./media/image3.png)

这是一个简单的多人游戏，演示开发人员用于执行以下功能的 Xbox
Live、PlayFab 和 PlayFab Party API：

-   登录到 Xbox Live 服务和 PlayFab 服务

-   检索 Xbox Live 好友配置文件信息

-   使用 MPM 启动 Xbox Live 多人游戏会话

-   邀请 Xbox Live 好友加入多人游戏会话

-   加入好友的 Xbox Live 多人游戏会话

-   将 Xbox Live 用户匹配到 SmartMatch 多人游戏会话中

-   进入支持语音聊天的群网络

-   使用可靠和不可靠的消息参与游戏

# 

# 生成示例

此示例是针对 2021 年 2 月 Microsoft GDK 安装和适用于 GameCore 的 Unity
版本 2020.3.12f1 开发和测试的。 该示例依赖于多个 SDK，所有这些 SDK 都在
ZIP 文件（位于 SDKs/ 文件夹中）中提供了快照，可以将其解压缩到示例的
Assets/ 文件夹中以实现示例所需的 SDK 依赖项。
该示例仅包含一个名为"SampleScene.unity"的场景，位于 Assets/Sample/Scenes
文件夹中。

解压缩所有 SDK 并将示例加载到 Unity 中后，应已设置生成示例所需的所有
Unity、Xbox Live 和 PlayFab 配置。 应使用"生成"按钮在 Unity IDE
中生成示例：

![Graphical user interface Description automatically generated](./media/image4.png)

\...
示例当前在所选生成文件夹下放置松散部署文件夹，例如，如果为"Builds/Scarlett"，则部署文件夹变为"Builds/Scarlett/Loose"（或"Builds/XboxOne/Loose"），如下所示：

![A picture containing graphical user interface Description automatically generated](./media/image5.png)

在 Scarlett 和 XboxOne 配置设置中指定"MicrosoftGame.config"文件和五个
PNG 文件，并提供 GDK 应用程序所需的支持文件。

Unity
完成生成过程，并且元数据文件位于生成输出文件夹中后，可通过两种方法将应用程序"旁加载"到
Scarlett/XboxOne 工具包中：

1.  使用"Gaming VS 2019 命令提示符"，可以从示例根文件夹执行"xbapp deploy
    Builds\\Scarlett\\Loose"命令。

2.  使用"Gaming VS 2019
    命令提示符"，可以运行"makepkg"命令来制作使用后续命令"xbapp install
    Builds\\\<path_to_package\>\\\<name_of_app_identity\>.XVC"安装的打包生成（请参阅"makepkg"的
    GDK 文档）

# 运行示例

如果已设法成功生成（可能为包），并将应用旁加载到开发 Scarlett 或 XboxOne
工具包中，则应该会在"开发者主页"菜单中看到名为"UnityRumbleGDK.exe"的应用图标。
使用 Xbox 管理器或工具包本身启动应用应显示初始屏幕，后跟示例全屏。

如果要成功运行，必须在已配置和部署游戏的 Xbox Live
沙盒"XDKS.1"（或你自己的沙盒）中运行开发工具包。 Xbox
管理器工具可帮助切换此工具包上的活动沙盒。
你还需要一批可登录到沙盒的测试用户。

**合作伙伴中心上的 UnityRumbleGDK 游戏概述**

![A screenshot of a computer Description automatically generated](./media/image6.png)

上面的屏幕截图显示了合作伙伴中心上 UnityRumbleGDK 游戏的当前状态。
游戏已完全配置、打包、部署并发布到 XDKS.1 沙盒。
现在总共为游戏配置了三个多人游戏会话模板和一个 SmartMatch 漏斗。

**多人游戏会话模板**

![Text Description automatically generated](./media/image7.png)

**LobbySessionTemplate**

{

\"constants\": {

\"system\": {

\"version\":1,

\"maxMembersCount\":5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": true

},

\"memberInitialization\": {

\"membersNeededToStart\":1

}

},

\"custom\": {}

}

}

在常见的多人游戏管理器方案中，请查阅 GDK
文档了解更多详细信息，我们至少有 3
个会话模板：一个用于大厅，一个用于托管游戏，另一个用于匹配的游戏。

通过大厅模板，我们将大厅的参与者人数限制为 5 人。
邀请协议指定应向游戏应用程序发出邀请。 此外，成员还需要连接，这通过
RTA（实时活动）服务进行管理。

我们还会将会话设置为可搜索或公开，即使示例没有显示任何搜索功能。
最后，启动大厅会话所需的成员数正好为 1 人，其将是会话的开启人。

**GameSessionTemplate**

{

\"constants\": {

\"system\": {

\"version\":1,

\"maxMembersCount\":5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

}

},

\"custom\": {}

}

}

\<blah\>.

**MatchSessionTemplate 和 SmartMatch Hopper**

{

\"constants\": {

\"system\": {

\"version\":1,

\"maxMembersCount\":5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

},

\"memberInitialization\": {

\"membersNeededToStart\":2

}

},

\"custom\": {}

}

}

![A screenshot of a computer Description automatically generated with medium confidence](./media/image8.png)

\<blah\>.

以下部分将按屏幕逐个介绍示例的 UI，并说明如何在正常运行的 GDK
开发环境中运行这些屏幕。

## 示例开始屏幕

![A screenshot of a computer Description automatically generated with medium confidence](./media/image9.png)

上面的屏幕截图显示了启动示例时出现的第一个屏幕。 "开始"按钮将启动 Xbox
Live 和 PlayFab 用户登录流。
如果任何一个登录步骤失败，屏幕底部会显示一条失败消息，并且用户将仍停留在开始屏幕。

常见的失败情况可能是用户无法访问当前沙盒、互联网连接当前已关闭，或者
Xbox Live 服务或 PlayFab 服务遇到某种类型的服务中断。

## 

## 示例主菜单屏幕

![A screen shot of a computer Description automatically generated with low confidence](./media/image10.png)

测试用户成功登录到 Xbox Live 和 PlayFab 后，将显示示例的主菜单屏幕。
显示的三个主要功能是：

1.  使用 SmartMatch 将我的用户与其他运行游戏的用户匹配。

2.  让我的用户主持游戏大厅会话。

3.  尝试加入好友当前活跃的游戏大厅，或直接加入测试用户已接受的邀请相关联的会话。

如果不能执行上述任何一个功能，那么失败消息将显示在屏幕底部，并且用户将留在主菜单屏幕。

## 

## 

## 加入好友大厅屏幕

![A screenshot of a computer Description automatically generated with medium confidence](./media/image11.png)

从主菜单中选择"加入好友"按钮，最多会显示三个好友大厅按钮以及返回到主菜单的选项。
如果当前没有好友在运行 Unity Rumble
游戏大厅，则列表将为空，同时显示一条消息，指出找不到好友大厅。

## 

## 

## 查找 SmartMatch 屏幕

![A screenshot of a computer Description automatically generated with low confidence](./media/image12.png)

从主菜单中选择"查找匹配"按钮选项时，该示例将立即尝试通过多人游戏管理器
API 管理的匹配票证进行匹配。 匹配票证通常会导致以下两种常见结果：

1.  可以完成匹配票证并创建匹配会话（使用前面提供的匹配会话模板），并且将一起匹配的所有成员放入新会话中。

2.  匹配票证无法完成并超时。
    如果是这样的结果，则在屏幕底部会显示错误，并且会重新启用可用的主菜单选项。

只要匹配票证仍处于活动状态，就可以取消该票证。
如果匹配票证被取消，则会重新启用主菜单选项，就像匹配请求失败一样。

## 游戏大厅屏幕

![A screenshot of a computer Description automatically generated with medium confidence](./media/image13.png)

一旦测试用户：成功匹配，通过"加入好友"或"加入邀请"主菜单选项成功加入好友的游戏大厅，或成功开始主持自己的会话，那么用户就会看到游戏大厅屏幕。

UI 屏幕将显示大厅会话中出现的成员。
"离开"选项允许用户中止在大厅中，如果用户选择该选项，则将从屏幕左侧的大厅成员列表中删除该成员。

"就绪"按钮上方的"轮船和颜色"图标按钮允许用户选择想要在游戏中使用的轮船风格和颜色，并且选择内容与所有其他大厅成员同步，如玩家代号旁的大厅成员列表中所示。
用户玩家代号左侧的图标还显示会话的主持人及其"就绪"状态。
当所有成员都已通过"就绪"按钮将就绪状态切换为"开启"时，大厅将在屏幕底部启动倒计时。

## 主持人邀请好友进入大厅

![A screenshot of a computer Description automatically generated with low confidence](./media/image14.png)

如果测试用户通过"主持游戏"主菜单选项主持游戏大厅，则"邀请"按钮将在游戏大厅屏幕上显示。
选择"邀请"按钮将向用户显示 Shell UI
屏幕，其中显示了可以受邀加入大厅会话的好友列表。

在后台，将调用邀请 API 来处理好友选择和服务器端邀请协议机制。
如果邀请未取消，那么接收用户（无论是否在应用中）将收到 Shell UI
通知，获悉已向他们发送邀请，他们可以接受该邀请，也可以将其关闭。
如果接受邀请，则将启动应用（如果尚未运行），并在主菜单上启用新按钮"加入邀请"。

## 从好友收到的屏幕邀请

![A screenshot of a computer Description automatically generated with low confidence](./media/image15.png)

![A screen shot of a computer Description automatically generated with low confidence](./media/image16.png)

如前所述，当接受主持用户发送的邀请时，主菜单屏幕将在接受邀请时显示"加入邀请"。

**大厅全部就绪屏幕**

![A screenshot of a computer Description automatically generated with low confidence](./media/image17.png)

所有大厅成员通过"就绪"切换按钮准备就绪后，将提示主机向所有人发出游戏准备启动的消息。
此时，会话网络将用于同步，倒计时计时器将在大厅屏幕底部显示。

如果用户终止应用，则该成员将从游戏中移除并从会话网络中消失。
倒计时完成后，用户将进入游戏屏幕。

****

**游戏播放屏幕**

![A screenshot of a computer Description automatically generated with medium confidence](./media/image18.png)

游戏屏幕的左侧显示了活跃的参与成员，其中还包含其成员玩家代号旁显示的"终止"和"死亡"计数。
主机名称左侧的 Xbox 图标表示主机。
如果用户通过"退出游戏"按钮提前退出游戏，他们将从所有其他成员列表中消失。

玩家总计达到五次终止时，游戏自然会结束。
此时，就像玩家选择退出游戏一样，用户将返回到主菜单。

# 实施说明

示例脚本代码位于"Assets/Sample/Scripts"文件夹下，主要细分为
UI/视图相关代码和核心逻辑。 在核心逻辑中，代码将进一步细分，以将 Xbox
Live/PlayFab/Networking 多人游戏特定逻辑与游戏和其他通用逻辑位分开。

-   用于登录、好友、多人游戏等的 Xbox Live 函数位于
    Assets\\Sample\\Script\\Logic\\XboxLive 中

-   用于登录的 PlayFab 函数位于 Assets\\Sample\\Logic\\PlayFab 中

-   用于网络和会话文档的函数位于 Assets\\Sample\\Script\\Logic\\Session
    中

-   如果要使用 Unitys 的 GameCore 包，请确保定义 "USE_UNITY_GAMECORE" 和
    "UNITY_GAMECORE" 来代替其中的定义。

# 重要事项！

随示例提供的 SDK 快照位于 SDKS\\
文件夹中，纯粹是为了方便运行示例，**不应**将其视为适合随已发布游戏交付的"官方"SDK
版本。 请始终为游戏的特定 SDK 版本使用开发熟悉的最新 QFE。

# 

# 已知问题

该示例不完全针对"已知"问题，而是针对示例 "SDKs" 文件夹中提供的 SDK
快照开发和测试的。 集成较新版本的 Microsoft Unity GDK 插件、使用 Unity
的 GameCore 包，或集成不同版本的 PlayFab 或 PlayFab Party 时，Unity
插件可能会暴露与可能已弃用或行为特征发生改变的 API 的不兼容性。

# 

# 更新历史记录

| 说明                        |  发布日期           |  版本             |
|-----------------------------|--------------------|------------------|
| 示例自述文件初稿。 包括生成要求、用法 详细信息，以及说明和问题。 |  2021 年 3 月 22 日  |  1.0 |
| 示例已更新，可在 Unity 2020.3 LTS 和 Feb 2021 QFE2 GDK 上运行。 |  2021 年 6 月 18 日  |  1.1 |

# 

# 隐私声明

在编译和运行示例时，将向 Microsoft
发送示例可执行文件的文件名以帮助跟踪示例使用情况。若要选择退出此数据收集，你可以删除
Main.cpp 中标记为"示例使用遥测"的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft
隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。
