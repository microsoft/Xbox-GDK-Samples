![](./media/image1.png)

# Hi 模具 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

默认情况下，Hi 模具在 D3D12.X 上处于关闭状态。 但好消息是，游戏可以配置 Hi 模具。 有两个可由游戏设置的 Hi 模具比较状态。 模具操作可在平铺速率下运行，而不是以采样率运行，从而导致性能获得提高。 使用模具测试的任何游戏都有可能通过手动设置 Hi 模具来看到性能改进。 此示例显示高模具 API 使用情况。

该示例还演示如何使用计算着色器从 HTile 缓冲区读取高模具结果。 然后，可以使用异步计算运行模具传递，并可提高总体性能。

# 生成示例

如果使用 Xbox One 开发工具包，请将活动解决方案平台设置为 `Gaming.Xbox.XboxOne.x64`。

将通过仿真在 devkit 上运行。 *有关详细信息，请参阅* *GDK 文档中的*&ldquo;__运行示例__&rdquo;。
| | |
|---|---|
|此示例不支持 Xbox Series X|S。因为 htile 发生更改。 It|


# 使用示例

运行像素着色器的示例：

![](./media/image3.png)

运行计算着色器的示例：

![](./media/image4.png)

| 操作 | 游戏板 |
|---|---|
| 打开/关闭 Hi 模具 | A button |
| 切换计算着色器和像素着色器实现 | X button |
| 切换&ldquo;重新序列化&rdquo;打开/关闭 | Y button |
| 切换 HiStencilControlX API | B button |
| 设置资源以替代 HiStencilControlX 切换 | 左扳机键 |
| 在绘制外部网格之前收集 HTile 值 | 右扳机键 |
| 在 ESRAM 中切换资源 | 左侧肩按钮 |
| 显示调度调用中的计数 | 右侧肩按钮 |
| 帮助 | Menu button |
| 退出 | &ldquo;视图&rdquo;按钮 |

# 实现说明

默认情况下，Hi 模具在 D3D12.X 上处于关闭状态。 游戏（State0 和 State1）可以设置 2 种高模具比较状态。

```cpp
typedef struct D3D12XBOX_HISTENCIL_COMPARE_STATE
{
    D3D12XBOX_HISTENCIL_COMPARE_FUNCTION CompareFunction : 4;
    UINT CompareValue : 8;
    UINT CompareMask : 12;
    BOOL Enabled : 8;
} D3D12XBOX_HISTENCIL_COMPARE_STATE;

typedef struct D3D12XBOX_HISTENCIL_CONTROL
{
    D3D12XBOX_HISTENCIL_COMPARE_STATE State0;
    D3D12XBOX_HISTENCIL_COMPARE_STATE State1;
} D3D12XBOX_HISTENCIL_CONTROL;
```


SetHiStencilStateX 和 SetHiStencilControlX API 已添加到 D3D12。X 可帮助设置这些高模具比较状态。

- **SetHiStencilStateX** 用于在深度模具资源上设置高模具比较测试，并在绑定给定资源时保留在给定资源上。

- **SetHiStencilControlX** 用于全局临时设置状态，并将用于任何模具资源，直到具有关联 Hi 模具状态的深度模具绑定到管道。

```cpp
void D3DAPI SetHiStencilStateX(
    _In_ ID3D12Resource* pResource,
    _In_opt_ const D3D12XBOX_HISTENCIL_CONTROL* pControl);

void D3DAPI SetHiStencilControlX(
    _In_opt_ const D3D12XBOX_HISTENCIL_CONTROL* pControl);
```


比较结果存储在 HTile 缓冲区中。 稍后在帧中与模具测试相关联时，这些测试可以是拒绝/接受磁贴而不是读取单个示例模具值的有效方法。

## HTile 缓冲区：

HTile 缓冲区存储深度模具缓冲区每 8x8 块像素的 32 位元数据。

HTile 位的解释，时间

- 模具不存在（此示例中未介绍）

![](./media/image5.png)

- 模具存在

![](./media/image6.png)

- **SMem**：模具内存格式是一个 2 位值，指示如何存储磁贴中的模具值

| SMem | 说明 |
|---|---|
| 0 | **Clear** -- 整个磁贴具有 Clear 值。 |
| 1 | **单个值** **--** 整个磁贴具有单个模具值。 1^st^ 模具缓冲区磁贴中的 8 位数据是整个磁贴的值 |
| 2 | **扩展和清除** -- 当前未在 XBox 上使用。 模具缓冲区中此磁贴的所有示例都具有清晰的值。 |
| 3 | **展开** -- 磁贴已展开，因此模具缓冲区中的各个示例具有正确的模具值 |

- **SR\***：分层模具预测试结果。 可在 HTile 中存储 2 个独立的比较结果（SR0 和 SR1）。 API SetHiStencilStateX 和 SetHiStencilControlX 可用于设置比较函数、遮罩和比较值。 SR0 和 SR1 各有 2 位：

   - **位 0**：可能失败

   - **位 1**：可能通过

> 按以下 2 位设置的值的解释：

| SR\* | 说明 |
|---|---|
| 0 | **已清除或未比较** |
| 1 | **可能失败** -- 至少有一个示例未通过 Hi 模具测试 |
| 2 | **可能通过** -- 至少一个示例已通过 Hi 模具 测试 |
| 3 | **5 月通过或 5 月失败** - 至少一个示例通过了 Hi 模具测试，并且至少有一个示例未能通过 Hi 模具。 如果高模具状态测试已关闭，或者模具通过为 ALWAYS 或 NEVER，或者比较遮罩为 0 或模具写入正在运行，硬件还可以设置这两个位。 因此，在这些情况下，即使磁贴显示 SR\* 的值为 3，所有示例都可能会失败。 |

- **示例中的 Hi 模具 状态**

在示例中，SetHiStencilStateX 用于通过对使用模具值COMPARE_VALUE和 （COMPARE_VALUE+1）绘制的 2 张光盘的测试。

```cpp
hiStencilControl.State0.Enabled = TRUE;
hiStencilControl.State0.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
hiStencilControl.State0.CompareValue = COMPARE_VALUE;
hiStencilControl.State0.CompareMask = 0xFF;
```


使用 SetHiStencilControlX 时，它会设置第二个比较状态，以确保使用模具值（COMPARE_VALUE_2 + 1）绘制的另一张光盘通过 HiStencil 测试：

```cpp
hiStencilControl.State1.Enabled = TRUE;
hiStencilControl.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
hiStencilControl.State1.CompareValue = COMPARE_VALUE_2 + 1;
hiStencilControl.State1.CompareMask = 0xFF;
```

## 计算着色器:

HTile 缓冲区可用于读取高模具预测试结果，这些结果可用于在计算着色器中实现模具测试。 这往往比平常的像素着色器实现慢，但其优点是可以在异步计算中运行，从而从图形管道中缩短一些时间。 示例中使用了 2 个计算着色器：

- **解释 HTile 值：** 此着色器使用追加缓冲区来存储设置了&ldquo;MayPass&rdquo;位 SR\* 的磁贴坐标。 出于调试目的，着色器还可以计算&ldquo;MayFail&rdquo;位集的磁贴数，以及可能传递具有单个值的磁贴数 （SMem =1）。

- **确定展开的模具结果：** 第二个计算着色器使用上述追加缓冲区的值来确定已传递模具的样本，并将绿色或蓝色与目标混合。 它对具有单个模具值（SMem = 1）（混合输出与绿色）的磁贴使用 SGPR，如果磁贴已展开，因为每个线程需要从模具缓冲区读取单独的值（使用蓝色混合输出），则使用 VGPR。

## 有用的 PIX 计数器：

| 计数器 | 描述 |
|---|---|
| DB_P ERF_SEL_DB_SC_S\_TILE_RATE | 平铺速率下的模具操作。 计数器仅在 Scorpio 上可用。 |
| DB_PER F_SEL_DB_SC_TILE_TILE_RATE | 以平铺速率执行深度和模具操作。 可在 Durango 上使用，因为上述计数器（DB_PERF_SEL_DB_SC_S\_TILE_RATE）不可用。 如果深度比较已关闭，则此计数器仅显示模具操作。 |
| DB _PERF_SEL_DB_SC_TILE_TILES | 磁贴总数（129,600 表示 4K 目标，32,400 表示 1080p） |
| PreZSamplesFailingS | 按采样率失败的模具操作数 |
| PreZSamplesPassing | 如果关闭深度操作，则以采样速率传递的模具操作数。 如果深度比较处于打开状态，则它包括 depth+模具 |

# 结果：

**使用像素着色器传递模具时写入像素的结果：**

| 计数器 | Scorpio -- 4K | Durango -- 1080p |
|---|---|---|---|---|
|  | Hi-S 租户关闭 | Hi-S tencil 开启 | Hi-S 租户关闭 | Hi-St encil 开启 |
| EOP 到 EOP 持续时间 | 0.089 ms | 0.062 ms | 0.072 ms | 0.055 ms |
| \% Hi-Z 磁贴被拒绝 | 0 | 40% | 0 | 39.8% |
| PreZSamplesPassing（模具测试以采样速率运行） | 1,2 35,187 | 23,187 | 3 08,857 | 1 2,025 |
| DB_PERF_SEL_DB_SC_S\_TILE_RATE （Scorpio） 或 DB_PERF_SEL_DB_SC_TILE_TILE_RATE（Durango）（平铺速率运行的模具测试） | 0 | 18,939 | 0 | 4,638 |
| \% 的传递样本以平铺速率运行 | 0 | 98.12% | 0 | 9 6.16% |

使用 Hi 模具 时，Scorpio 显示 **30%** 的增益，Durango 显示 **23%** 的增益。

**模具通过时写入像素：**

| 计数器 | Scorpio -- 4K | Durango -- 1080p |
|---|---|---|---|---|
|  | PS | CS | PS | CS |
| 示例写入传递模具 | 1,2 35,187 | 3 08,857 |
| 所用时间（DRAM 中的 DSV+RTV） | 0.062 ms | 0.088 ms | 0.055 ms | 0.103 ms |
| 所用时间（ESRAM 中的 DSV+RTV） | \- | \- | 0.027 ms | 0.044 ms |

当目标位于 ESRAM 中时，计算着色器花费的时间可能更接近像素着色器计时，因为绘图调用可能受像素速率的约束。 在 DRAM 中时，调用由 DRAM 带宽绑定。 如上所述，尽管计算着色器需要更多时间来执行，但是在 Async 上运行它可以减少总帧时间。

# 更新历史记录

初始版本 2019 年 2 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


