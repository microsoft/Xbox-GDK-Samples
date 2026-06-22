![](./media/image1.png)

# FutexCombo 示例

*此示例可用于 Microsoft 游戏开发工具包（2020 年 6 月）*

# 说明

旋转锁的传统用途是提供线程同步（当锁通常保留一小段时间时）。 使用正确的旋转计数，等待锁的线程可以保持用户模式，并在最短时间内获取锁。 但是，有时无法在旋转时间内获取锁，因此等待线程应允许其他线程利用 CPU，以便整体工作可以继续。

旋转锁的许多实现使用 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 为核心上的其他就绪线程留出时间。 但是，这些函数的含义和用例与旋转锁的含义和用例截然相反。 当线程有重要工作需要执行并且需要快速获取锁以便它可以继续运行时，将使用旋转锁。 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 函数表示线程没有其他操作，因此请将其余的量子提供给包括较低优先级线程在内的任何其他线程。 根据线程的当前状态，游戏可在优先级较高的作业线程上轻松看到最多 30 秒的停止，从而导致帧停止。

此示例提供使用 [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle) 的旋转锁的实现。 这允许旋转线程仍为其他就绪线程留出时间，但也允许它在获取锁后立即继续执行。 这会导致线程之间的执行时间更顺畅，并删除使用 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 导致的峰值。 总体效果是完成的工作更一致，与线程之间的计划优先级匹配。

# 使用示例

该示例将使用不同的旋转锁实现持续运行各种线程设置。 在每个配置中，它将计算对一组前台线程以及一组后台线程执行的工作量。 在控制台上，可以使用控制器上的&ldquo;A&rdquo;按钮在屏幕之间循环。 在桌面上，屏幕将每 5 秒自动循环一次。

# 实现说明

正在测量三个不同的旋转锁实现。

- Slowtex -- 使用 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread 的实现](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)

- Futex -- 使用 [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle 的实现](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle)

- Nulltex -- 立即调用的实现
   [睡眠状态](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)
   超时为零

   - 这是为了显示最坏的情况，其中旋转始终失败

总共创建了 8 个线程，其中包含 4 个前台线程和 4 个后台线程。 有两种相关性配置。 第一种是允许线程在内核之间自由浮动。 第二个原因是线程锁定到单个核心。 单个前台线程和单个后台线程将被锁定到同一核心。

高优先级线程位于一个循环中，计算它们可以执行的操作数。 他们每隔一次尝试获取旋转锁，按住它一段时间，然后释放它。 后台线程位于计算操作数的同一循环中;但是，对于默认变量，它们不会尝试获取旋转锁。

测量三种不同的争用级别：高、中和低。 这些控制获取旋转锁的频率以及持有锁的时长。

可以通过调整 FutexTest.cpp 顶部定义的控件变量来控制锁的旋转时间以及每个线程持有锁的时间。

# 结果

这是从 Xbox Series X 中运行的 Xbox Series X 收集的数据示例
禁用 SMT 的 3.8GHz。

Futex 旋转锁

| 争用 | 优先级 | 关联已锁定 | 关联浮动 |
|---|---|---|---|
| 高 | 前景 | 1,695,453 | 1,705,016 |
|  | 背景 | 15,646 | 10,662 |
| 中等 | 前景 | 1,831,193 | 1,767,261 |
|  | 背景 | 2,352 | 3,566 |
| 低 | 前景 | 2,106,397 | 1,900,651 |
|  | 背景 | 538 | 2,086 |

Slowtex 旋转锁

| 争用 | 优先级 | 关联已锁定 | 关联浮动 |
|---|---|---|---|
| 高 | 前景 | 671,615 | 743,589 |
|  | 背景 | 1,191,006 | 494,248 |
| 中等 | 前景 | 929,646 | 992,124 |
|  | 背景 | 972,009 | 399,833 |
| 低 | 前景 | 1,248,495 | 1,173,585 |
|  | 背景 | 718,960 | 369,296 |

Nulltex 旋转锁

| 争用 | 优先级 | 关联已锁定 | 关联浮动 |
|---|---|---|---|
| 高 | 前景 | 18,782 | 428,358 |
|  | 背景 | 1,919,796 | 419,653 |
| 中等 | 前景 | 32,644 | 160,260 |
|  | 背景 | 1,880,733 | 988,944 |
| 低 | 前景 | 50,038 | 333,363 |
|  | 背景 | 1,887,807 | 1,132,100 |

此处要查看的关键数字是在前台线程上完成的工作量，这表示需要为框架继续处理而完成的工作量。 使用通过 [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle) 实现的旋转锁对象时，前台线程始终在所有争用级别执行更多工作。 这会导致帧速率更一致，因为关键工作完成速度更快。 原因是允许线程根据其优先级完全抢占。

仅当允许线程在内核之间浮动时，使用 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 实现的旋转锁对象才能合理执行。 但是，即使在这种情况下， [由于 Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 行为将剩余量子提供给另一个线程，并且不允许旋转线程切换核心，仍会浪费时间。 此浪费时间给后台线程，导致前台线程上的工作需要更长时间才能完成，从而导致等待关键工作完成的更多停止。

# 更新历史记录

初始版本 2022 年 8 月

# 隐私声明

在编译和运行示例时，将向 Microsoft 发送示例可执行文件的文件名以帮助跟踪示例使用情况。 若要选择退出此数据收集，你可以删除 Main.cpp 中标记为&ldquo;示例使用遥测&rdquo;的代码块。

有关 Microsoft 的一般隐私策略的详细信息，请参阅 [Microsoft 隐私声明](https://privacy.microsoft.com/en-us/privacystatement/)。


