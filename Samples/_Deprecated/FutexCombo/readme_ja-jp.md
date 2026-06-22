![](./media/image1.png)

# FutexCombo サンプル

*このサンプルは Microsoft Game Development Kit (2020 年 6 月) と互換性があります*

# 説明

スピンロックの従来の用途は、通常、ロックが少量の時間保持されている場合にスレッド同期を提供することです。 適切なスピン カウントを使用すると、ロックを待機しているスレッドはユーザー モードのままになり、最小限の時間でロックを取得できます。 ただし、スピン時間内にロックを取得できない場合があるため、待機中のスレッドは、全体的な作業を続行できるように、他のスレッドが CPU を利用できるようにする必要があります。

スピンロックの多くの実装では、[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) を使用して、コア上の他の準備完了スレッドに時間を与えます。 ただし、これらの関数の意味とユース ケースは、スピンロックの意味とユース ケースとは対照的です。 スピンロックは、スレッドに重要な作業があり、実行を継続できるようにロックをすばやく取得する必要がある場合に使用されます。 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 関数は、スレッドに他に何もする必要はないことを意味します。そのため、残りの部分は、優先順位の低いスレッドを含む他のスレッドに量子を与えます。 スレッドの現在の状態によっては、タイトルは優先度の高いジョブ スレッドで最大 30 ミリ秒のストールを簡単に確認し、フレームのストールを引き起こす可能性があります。

このサンプルでは、[WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle) を使用するスピンロックの実装を提供します。 これにより、スピンスレッドは他の準備完了スレッドに時間を与えることができますが、ロックが取得されるとすぐに実行を続行することもできます。 これにより、スレッド間の実行時間が大幅に短縮され、[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) の使用によるスパイクが削除されます。 全体的な効果は、スレッド間の計画された優先順位に一致する、より一貫性のある作業が行われることです。

# サンプルの使用方法

このサンプルでは、さまざまなスピンロック実装を使用して、さまざまなスレッドセットアップを継続的に実行します。 各構成では、一連のフォアグラウンド スレッドと一連のバックグラウンド スレッドに対して実行される作業量が計算されます。 本体では、コントローラーの A ボタンを使用して画面間を切り替えることができます。 デスクトップでは、画面は 5 秒ごとに自動サイクルされます。

# 実装メモ

3 つの異なるスピンロック実装が測定されています。

- Slowtex -- [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) を使用する実装

- Futex -- [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle) を使用する実装

- Nulltex -- すぐに呼び出す実装
   [スリープ](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)
   タイムアウトが 0 の場合

   - これは、スピンが常に失敗する最悪のシナリオを示すために

合計 8 つのスレッドが、4 つのフォアグラウンド スレッドと 4 つのバックグラウンド スレッドで作成されます。 アフィニティ構成は 2 つあります。 1 つ目は、スレッドがコア間で自由に浮動できるということです。 2 つ目は、スレッドが 1 つのコアにロックされていることです。 1 つのフォアグラウンド スレッドと 1 つのバックグラウンド スレッドが同じコアにロックされます。

優先度の高いスレッドは、実行できる操作の数をカウントするループ内に配置されます。 多くの場合、スピンロックを取得し、ある程度の時間保持してから解放しようとします。 バックグラウンド スレッドは、操作の数をカウントする同じループに配置されます。ただし、既定の変数では、スピンロックの取得は試行されません。

3 つの異なる競合レベルが測定され、高、中、低です。 これらのコントロールは、スピン ロックを取得する頻度とロックを保持する期間を制御します。

ロックによって使用されるスピン時間と各スレッドがロックを保持する時間は、FutexTest.cpp の上部で定義されているコントロール変数を調整することによって制御できます。

# 結果

これは、 で実行されている Xbox Series X から収集されたデータの例です。
SMT が無効になっている 3.8 GHz。

Futex スピンロック

| 競合 | 優先順位 | アフィニティロック済み | アフィニティ フローティング |
|---|---|---|---|
| 高 | 前景 | 1,695,453 | 1,705,016 |
|  | 背景 | 15,646 | 10,662 |
| Medium | 前景 | 1,831,193 | 1,767,261 |
|  | 背景 | 2,352 | 3,566 |
| Low | 前景 | 2,106,397 | 1,900,651 |
|  | 背景 | 538 | 2,086 |

Slowtex ピンロック

| 競合 | 優先順位 | アフィニティロック済み | アフィニティ フローティング |
|---|---|---|---|
| 高 | 前景 | 671,615 | 743,589 |
|  | 背景 | 1,191,006 | 494,248 |
| Medium | 前景 | 929,646 | 992,124 |
|  | 背景 | 972,009 | 399,833 |
| Low | 前景 | 1,248,495 | 1,173,585 |
|  | 背景 | 718,960 | 369,296 |

Nulltex スピンロック

| 競合 | 優先順位 | アフィニティロック済み | アフィニティ フローティング |
|---|---|---|---|
| 高 | 前景 | 18,782 | 428,358 |
|  | 背景 | 1,919,796 | 419,653 |
| Medium | 前景 | 32,644 | 160,260 |
|  | 背景 | 1,880,733 | 988,944 |
| Low | 前景 | 50,038 | 333,363 |
|  | 背景 | 1,887,807 | 1,132,100 |

ここで確認する重要な数値は、フォアグラウンド スレッドで行われている作業の量です。これは、フレームが処理を続行するために実行する必要がある作業を表します。 [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress) /[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle) で実装されたスピンロック オブジェクトを使用する場合のフォアグラウンド スレッドは、すべての競合レベルでより多くの処理を一貫して実行します。 これにより、重要な作業が迅速に完了するため、フレーム レートの一貫性が向上します。 理由は、スレッドが優先順位に基づいて完全に割り込むのが許可されているためです。

[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) を使用して実装されたスピンロック オブジェクトは、スレッドがコア間で浮動できる場合にのみ合理的に実行されます。 ただし、その場合でも、[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) の動作により、残りの量子を別のスレッドに提供し、スピン スレッドでコアを切り替えることが許可されないため、時間が無駄になります。 この無駄な時間はバックグラウンド スレッドに与えられ、フォアグラウンド スレッドでの作業が完了するまでに時間がかかり、重大な作業が完了するのを待つストールが増えます。

# 更新履歴

初回のリリース: 2022 年 8 月

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


