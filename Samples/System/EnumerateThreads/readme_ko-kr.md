![](./media/image1.png)

# EnumerateThread 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

[ToolHelp API](https://docs.microsoft.com/windows/win32/api/tlhelp32/)는 타이틀 프로세스 내에서 실행되는 모든 스레드를 열거하기 위해 제공합니다. 이러한 API는 현재 WINAPI_PARTITION_GAMES에서 사용할 수 없고 OneCore 라이브러리에서 사용할 수 있지만 타이틀 코드에서 사용할 수 있도록 몇 가지 추가 코드가 필요합니다. 현재 해결 방법은 다음 코드와 함께 TlHelp32.h를 포함하기 전에 WINAPI_FAMILY_PARTITION을 재정의하는 것입니다.

// 참고: 도구 도움말 API는 현재 게임 파티션에 정의되어 있지 않습니다. 이는 타이틀이 호출할 수 있도록 ToolHelp API를 강제로 사용할 수 있도록 하는 해결 방법입니다. Xbox 콘솔에서 타이틀은 또한 onecore_apiset.lib 라이브러리에 연결해야 합니다. 이 시점에서 ToolHelp API를 문제 없이 사용할 수 있습니다.

> # undef WINAPI_FAMILY_PARTITION
>
> # define WINAPI_FAMILY_PARTITION(Partitions) 1
>
> # include \<TlHelp32.h\>
>
> # undef WINAPI_FAMILY_PARTITION
>
> # define WINAPI_FAMILY_PARTITION(Partitions)(Partitions)

# 샘플 사용

샘플에서는 다섯 개의 백그라운드 스레드를 만든 다음, 해당 이름 및 우선 순위와 함께 프로세스에서 실행되는 모든 스레드를 열거하고 나열합니다.

# 업데이트 기록

2022년 8월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


