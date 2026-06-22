![](./media/image1.png)

# DevkitTooling 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

Xbox 개발 키트 머신은 단순히 게임 타이틀을 실행하고 테스트하는 것 이상의 용도로 활용할 수 있는 강력한 하드웨어입니다. Xbox Game Core 응용 프로그램은 개발 키트의 전체 처리 리소스를 활용하며 API 사용을 WINAPI_FAMILY_GAMES 하위 집합으로 제한하는 Win32 응용 프로그램을 실행할 수 있습니다.

DevkitTooling 샘플은 Xbox 개발 키트에서 도구로 실행할 수 있도록 CPU 및 GPU 하위 프로세스를 시작하는 방법을 보여 줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

DevkitTooling 샘플은 하위 프로세스로 실행되는 도구의 로깅 출력이 있는 단일 화면을 보여 줍니다.

![자동으로 생성된 텍스트 설명](./media/image3.png)

예제 CPU 도구 하위 프로세서를 시작하려면 게임 패드에서 \[X\]를 누릅니다. 예제 GPU 도구 하위 프로세서를 시작하려면 게임 패드에서 \[Y\]를 누릅니다. 예제 하위 프로세스는 총 5초 동안 실행된 후 자동으로 종료됩니다. 로그 출력이 화면에 표시됩니다.

GPU 도구 하위 프로세스를 실행할 때 샘플 대신 도구의 렌더링을 표시하도록 화면이 변경됩니다(간단한 삼각형). GPU 도구가 완료되면 샘플이 다시 렌더링을 시작합니다. 이를 지원하기 위해 *SuspendX*로 렌더링이 일시 중단되고 *ResumeX*로 다시 시작됩니다. 자세한 내용은 구현 참고 사항을 참조하세요.

![도형 설명이 자동으로 생성됨](./media/image4.png)

# 구현 참고 사항

이 샘플에서는 Xbox 타이틀에서 일반적이지 않은 일부 기능을 사용합니다. 또한 개발 키트에서 예기치 않은 오류가 발생하지 않도록 하기 위해 반드시 준수해야 하는 특수 요구 사항이 있습니다. 아래의 각 섹션에서는 개발 키트에서 도구를 실행하기 위한 다양한 기능, 요구 사항 및 기타 고려 사항을 설명합니다.

## Xbox 타이틀 파티션

Xbox 개발 키트에서 실행되는 응용 프로그램이 하드웨어 리소스에 대한 모든 권한을 얻으려면 타이틀 파티션 내에서 실행해야 합니다. 타이틀 파티션은 특히 이러한 용도를 위한 Xbox 콘솔의 가상 머신입니다.

일반적으로 Xbox용 타이틀은 개발 키트에 배포되거나 시스템 등록으로 이어지는 패키지로 설치됩니다. 그러면 Xbox 시스템은 타이틀 파티션에서 이러한 타이틀을 제대로 시작할 수 있습니다.

시스템에 등록하지 않은 도구 또는 다른 독립 실행형 프로세스가 있더라도 타이틀 파티션에서 계속 실행할 수 있습니다. 그러나 타이틀 파티션이 먼저 활성 상태여야 합니다. 시스템에서 샘플을 시작할 때 타이틀 파티션이 활성화되도록 하기 때문에 DevkitTooling 응용 프로그램에서 이를 처리합니다. 그런 다음 DevkitTooling에서 다음을 사용하여 도구 또는 프로세스를 시작합니다.
[Createprocess](https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessa)
타이틀 파티션 내에 있고 전체 하드웨어 리소스 액세스 권한이 있습니다.

참고: *xbrun.exe* 도구는 타이틀 파티션에서 응용 프로그램을 시작할 수도 있습니다. 그러나 *xbrun.exe*는 타이틀 파티션이 우선 활성 상태가 되도록 보장하지 않습니다. 타이틀이 현재 Xbox 개발 키트에서 실행 중인 경우 *xbrun.exe*에 대한 "*/x/title*" 매개 변수는 타이틀 파티션에서 실행 파일을 실행하도록 지정했습니다. *xbrun.exe*에 대한 자세한 내용은 GDK 설명서를 참조하세요.

## 포트 및 Xbox 방화벽

Xbox 콘솔 및 Xbox 개발 키트에는 TCP 및 UDP 둘 다에 대해 기본적으로 가장 원치 않는 인바운드 연결 요청을 방지하는 방화벽이 있습니다. 소매 타이틀의 경우 원치 않는 인바운드 TCP는 항상 차단되며 이를 피할 방법은 없습니다. 그러나 UDP는 *XNetworkingQueryPreferredLocalUDPMultiplayerPort\[Async\]*를 사용하여 쿼리할 수 있는 기본 로컬 UDP 멀티플레이터 포트를 사용할 수 있습니다.

MicrosoftGame.config 파일에 입력하여 원치 않는 인바운드 TCP 및 UDP 패킷을 허용하도록 Xbox 개발 키트를 구성할 수 있습니다.

```
<?xml version="1.0" encoding="utf-8"?>
<Game configVersion="0">
...
  <DevelopmentOnly>
    <DebugNetworkPortList>
      <DebugNetworkPort>4600</DebugNetworkPort>
      <DebugNetworkPort>4601<DebugNetworkPort>
    </DebugNetworkPortList>
  </DevelopmentOnly>
</Game>
```


요청되지 않은 인바운드 연결을 허용하는 각 포트에 대해 서로 다른t *DebugNetworkPort* 항목을 *DebugNetworkPortList*에 추가해야 합니다. 이는 TCP를 통해 직접 서로 통신하는 P2P 도구와 같은 경우에 필요합니다.

MicrosoftGame.config 파일이 업데이트되면 업데이트 시 픽업할 수 있도록 타이틀을 다시 배포해야 합니다.

## Xbox 개발 키트의 Win32 응용 프로그램

타이틀 파티션은 특정 Xbox 통합을 만들지 않은 Win32 콘솔 응용 프로그램을 직접 실행할 수 있습니다. 그러나 Win32 API 사용 가능성은 Windows PC에서 일반적으로 사용할 수 있는 기능이 줄어든 하위 집합입니다.

Win32 응용 프로그램을 사용 가능한 API로만 제한하려면 Windows 헤더를 포함하기 전에 *WINAPI_FAMILY*를 *WINAPI_FAMILY_GAMES*로 정의해야 합니다.

```
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#include <Windows.h>
```


CPUTool 프로젝트는 이에 대한 매우 간단한 예제입니다. *WINAPI_FAMILY_GAMES*로 제한된 콘솔 Win32 응용 프로그램을 빌드합니다. 그 결과로 CPUTool.exe는 Windows PC와 Xbox 콘솔 모두에서 실행될 수 있습니다.

## Xbox 개발 키트의 GPU 프로세스

타이틀 파티션에서 실행되는 프로세스에는 D3D GPU 사용에 대한 제한 사항이 더 많이 있습니다.

* 타이틀 파티션의 프로세스는 한 번에 1개만 Direct3D를 활용할 수 있습니다.

* Xbox 관련 D3D12 디바이스 메서드 *SuspendX/ResumeX*를 사용하여 D3D를 활용하는 여러 프로세스를 관리할 수 있습니다. 두 프로세스는 D3D를 동시에 적극적으로 활용해서는 안 됩니다.

* Xbox 전용 D3D 헤더 및 라이브러리를 사용해야 합니다.

이러한 고려 사항을 부적절하게 처리하면 응용 프로그램이 충돌하거나 렌더링이 손상되거나 콘솔 작동이 중단될 수 있습니다. DevkitTooling 샘플은 GPU를 활용하는 하위 프로세스를 만들기 전에 *SuspendX*를 사용하여 이러한 사례를 처리합니다. 해당 하위 프로세서가 완전히 종료되면 *ResumeX*를 사용하여 샘플의 렌더링을 다시 사용하도록 설정합니다.

GPUTool 프로젝트는 화면에 삼각형을 렌더링하는 단순한 프로세스입니다. GPU 고려 사항이 존중될 때 타이틀 파티션에서 GPU 하위 프로세서를 사용하는 방법을 보여 주는 데 사용됩니다.

## 다수의 프로세스가 있는 GameInput

GameInput 라이브러리는 현재(이 샘플을 만든 시점) 이 샘플을 만들 때 한 번에 1개 이상의 프로세스에 사용되는 것을 지원하지 않습니다. 또한 프로세스 중에나 프로세스를 종료하고 시작하여 정리하고 다시 초기화할 수 없습니다. 따라서 GameInput을 초기화하는 첫 번째 프로세스가 이를 사용할 수 있는 유일한 프로세스입니다. 초기화를 시도하는 이후의 모든 프로세스는 작동이 중단됩니다.

# 업데이트 기록

2021년 6월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


