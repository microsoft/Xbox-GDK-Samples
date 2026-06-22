# Auto HDR 示例

*此示例可用于 Microsoft 游戏开发工具包 (2022 年 3 月)*

# 说明

游戏自动添加 HDR 来在视觉上增强游戏。 该功能
产生性能影响。 即未添加额外的 CPU 或 GPU、额外的内存或带宽，以及额外的延迟。 该功能适用于大多数向后兼容的 ERA 游戏，但也可用于原生的 Xbox Series
通过使用 D3D 设备创建标志 `D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR` 执行 HDR。 ![一个视频游戏的屏幕截图。自动生成的描述](./media/image1.png)
| | |
|---|---|
|Auto HDR 是一项 Xbox Series X|S 功能，可通过在系统级别向 SDR|
|使用 Xbox Series X|S 硬件，因此不会对游戏或系统|
|X|S GDK 游戏。 此示例展示了 GDK 游戏如何通过使用 D3D


此示例中使用的图像是从 <https://www.halowaypoint.com/en-us> 本示例演示中获取并 <https://gearsofwar.com/en-US/> 用于的

# 生成示例

仅使用 `Gaming.Xbox.Scarlett.x64` 的 Xbox Series X|S 支持此示例。

*有关详细信息，请参阅* *GDK 文档*中的__运行示例__。&nbsp;

# 使用示例

该示例使用以下控制。

| 操作 | 游戏板 |
|---|---|
| 切换调整 UI 亮度 | A |
| 调整重建的颜色饱和度 | 方向键向左键/向右键 |
| 下一张图像 | 右肩键 |
| 上一张图像 | 左肩键 |

# 实现说明

**呈现为 SDR**

由于自动 HDR 在系统级别应用，因此游戏应仅呈现为 SDR。 该示例呈现并显示为 SDR。

**交换缓冲区格式**

我们强烈建议使用高精准率交换缓冲区格式以避免如条带等精准率非自然信号。

```cpp
m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
                                                          DXGI_FORMAT_UNKNOWN,
                                                          2,
                                                          DX::DeviceResources::c_Enable4K_UHD);
```


**将电视切换到 HDR 模式，并使用自动 HDR 标志创建 D3D 设备**

即使游戏以 SDR 呈现所有内容，游戏仍需要将电视切换到 HDR 模式。

```cpp
if (SwitchDisplayToHDR())
{
    params.CreateDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR;
}
```

**在恢复/不受限后将电视切换到 HDR 模式**

当游戏暂停/受约束时，主机的显示设置可能已更改，因此游戏在恢复时必须再次将电视切换到 HDR 模式。

```cpp
void Sample::OnResuming()
{
    // Display modes could have changed while title was suspended, so we need to make
    // sure that the TV is still in HDR mode. This is required for native HDR and
    // Auto HDR.

    m_deviceResources->SwitchDisplayToHDR();
```


**调整 UI 亮度**

```cpp
    if (isDisplayInHDRMode)
    {
        // Auto HDR will show pure white pixels as 1000 nits, so text/UI/HUD will become
        // much too bright. We linearly scale down the brightness of the UI
        m_UIBrightnessScale = m_bAdjustUIBrighness ? 0.8f : 1.0f;
    }
    else
    {
        // If the TV is in SDR mode, we don\'t do any brightness scaling
        m_UIBrightnessScale = 1.0f;
    }
}
```

# 更新历史记录

2021 年 6 月初始版本

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


