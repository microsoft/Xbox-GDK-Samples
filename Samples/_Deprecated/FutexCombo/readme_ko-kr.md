![](./media/image1.png)

# FutexCombo 샘플

*이 샘플은 Microsoft 게임 개발 키트(2020년 6월)와 호환 가능합니다.*

# 설명

스핀 잠금의 일반적인 용도는 일반적으로 잠금이 소량 동안 유지되는 경우 스레드 동기화를 제공하는 것입니다. 적절한 스핀 수를 사용하면 잠금을 기다리는 스레드가 사용자 모드로 유지되고 최소 시간 내에 잠금을 획득할 수 있습니다. 그러나 때로는 스핀 시간 내에 잠금을 획득할 수 없으므로 대기 스레드에서 다른 스레드가 CPU를 활용할 수 있도록 허용하여 전체 작업을 계속할 수 있습니다.

스핀 잠금의 많은 구현에서는 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)를 사용하여 코어의 다른 준비 스레드에 시간을 제공합니다. 그러나 이러한 함수의 의미와 사용 사례는 스핀 잠금의 의미 및 사용 사례와는 대칭적으로 반대됩니다. 스핀 잠금은 스레드가 수행해야 할 중요한 작업이 있고 계속 실행되도록 잠금을 신속하게 획득해야 하는 경우에 사용됩니다. [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 함수는 스레드에 수행할 다른 작업이 없음을 의미하므로 나머지 양자는 우선 순위가 낮은 스레드를 포함한 다른 스레드에 퀀텀을 제공합니다. 스레드의 현재 상태에 따라 타이틀은 프레임 정지로 이어지는 우선 순위가 높은 작업 스레드에서 최대 30ms 정지를 쉽게 볼 수 있습니다.

이 샘플에서는 [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle)을 사용하는 스핀 잠금의 구현을 제공합니다. 이렇게 하면 회전 스레드가 다른 준비 스레드에 계속 시간을 제공할 수 있지만 잠금을 획득하는 즉시 실행을 계속할 수도 있습니다. 이로 인해 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 사용으로 인한 급증이 제거되어 스레드 간에 훨씬 더 원활한 실행 시간이 발생합니다. 전반적인 효과는 스레드 간의 계획된 우선 순위와 일치하는 더 일관된 작업 수행입니다.

# 샘플 사용

샘플은 다양한 스핀 잠금 구현을 사용하여 다양한 스레드 설정을 지속적으로 실행합니다. 각 구성에서 포그라운드 스레드 집합과 백그라운드 스레드 집합에서 수행되는 작업의 양을 계산합니다. 콘솔에서 컨트롤러의 A 단추를 사용하여 화면 사이를 순환할 수 있습니다. 데스크톱에서 화면은 5초마다 자동 순환됩니다.

# 구현 참고 사항

세 가지 스핀 잠금 구현을 측정하고 있습니다.

- Slowtex -- [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)를 사용하는 구현

- Futex -- [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle)을 사용하는구현

- Nulltex -- 즉시 호출하는 구현
   [절전](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)
   시간 제한이 0인 경우

   - 이는 스핀이 항상 실패하는 최악의 시나리오를 표시하기 위한 것입니다.

포그라운드 스레드 4개와 백그라운드 스레드 4개로 총 8개의 스레드가 만들어집니다. 두 가지 선호도 구성이 있습니다. 첫 번째는 스레드가 코어 간에 자유롭게 부동할 수 있다는 것입니다. 두 번째는 스레드가 단일 코어에 잠겨 있다는 것입니다. 단일 포그라운드 및 단일 백그라운드 스레드가 동일한 코어로 잠깁니다.

우선 순위가 높은 스레드는 수행할 수 있는 작업 수를 계산하는 루프에 있습니다. 자주 스핀 잠금을 획득하고 일정 시간 동안 보관한 다음 해제하려고 시도합니다. 백그라운드 스레드는 작업 수를 계산하는 동일한 루프에 있습니다. 그러나 기본 변수를 사용하면 스핀 잠금을 획득하려고 시도하지 않습니다.

세 가지 경합 수준은 측정, 높음, 중간 및 낮음입니다. 스핀 잠금을 획득하는 빈도와 잠금을 보유하는 기간을 제어합니다.

잠금에서 사용하는 스핀 시간과 각 스레드가 잠금을 보유하는 시간은 FutexTest.cpp의 맨 위에 정의된 컨트롤 변수를 조정하여 제어할 수 있습니다.

# 결과

이는 다음에서 실행되는 Xbox Series X에서 수집된 데이터의 예입니다.
SMT를 사용하지 않도록 설정된 3.8GHz입니다.

Futex 스핀 잠금

| 경합 | 우선 순위 | 선호도 잠김 | 선호도 부동 |
|---|---|---|---|
| 높음 | 전경 | 1,695,453 | 1,705,016 |
|  | Background | 15,646 | 10,662 |
| 보통 | 전경 | 1,831,193 | 1,767,261 |
|  | Background | 2,352 | 3,566 |
| 낮음 | 전경 | 2,106,397 | 1,900,651 |
|  | Background | 538 | 2,086 |

Slowtex 스핀 잠금

| 경합 | 우선 순위 | 선호도 잠김 | 선호도 부동 |
|---|---|---|---|
| 높음 | 전경 | 671,615 | 743,589 |
|  | Background | 1,191,006 | 494,248 |
| 보통 | 전경 | 929,646 | 992,124 |
|  | Background | 972,009 | 399,833 |
| 낮음 | 전경 | 1,248,495 | 1,173,585 |
|  | Background | 718,960 | 369,296 |

Nulltex 스핀 잠금

| 경합 | 우선 순위 | 선호도 잠김 | 선호도 부동 |
|---|---|---|---|
| 높음 | 전경 | 18,782 | 428,358 |
|  | Background | 1,919,796 | 419,653 |
| 보통 | 전경 | 32,644 | 160,260 |
|  | Background | 1,880,733 | 988,944 |
| 낮음 | 전경 | 50,038 | 333,363 |
|  | Background | 1,887,807 | 1,132,100 |

여기서 살펴볼 주요 숫자는 포그라운드 스레드에서 수행되는 작업의 양입니다. 이는 프레임이 처리를 계속하기 위해 수행해야 하는 작업을 나타냅니다. [WaitOnAddress](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-waitonaddress)/[WakeByAddressSingle](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-wakebyaddresssingle)로 구현된 스핀 잠금 개체를 사용하는 경우 포그라운드 스레드는 모든 경합 수준에서 지속적으로 더 많은 작업을 수행합니다. 이로 인해 중요한 작업이 더 빠르게 완료됨에 따라 프레임 속도가 더 일관됩니다. 그 이유는 스레드가 우선 순위에 따라 완전히 선점할 수 있기 때문입니다.

[Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread)를 사용하여 구현된 스핀 잠금 개체는 스레드가 코어 간에 부동될 수 있는 경우에만 합리적으로 수행됩니다. 그러나 이 경우 나머지 양자를 다른 스레드에 제공하고 회전 스레드가 코어를 전환할 수 없도록 하는 [Sleep(0)](https://docs.microsoft.com/windows/win32/api/synchapi/nf-synchapi-sleep)/[SwitchToThread](https://docs.microsoft.com/windows/win32/api/processthreadsapi/nf-processthreadsapi-switchtothread) 동작으로 인해 시간이 낭비됩니다. 이 낭비되는 시간은 백그라운드 스레드에 제공되므로 포그라운드 스레드의 작업이 완료되는 데 시간이 더 오래 걸리기 때문에 중요한 작업이 완료되기를 기다리는 더 많은 중단이 발생합니다.

# 업데이트 기록

2022년 8월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


