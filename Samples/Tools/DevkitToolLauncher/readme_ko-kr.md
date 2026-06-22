![](./media/image1.png)

# DevkitToolLauncher 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

Xbox 개발 키트 머신은 단순히 게임 타이틀을 실행하고 테스트하는 것 이상의 용도로 활용할 수 있는 강력한 하드웨어입니다. Xbox Game Core 응용 프로그램은 개발 키트의 전체 처리 리소스를 활용하며 API 사용을 WINAPI_FAMILY_GAMES 하위 집합으로 제한하는 Win32 응용 프로그램을 실행할 수 있습니다.

DevkitToolLauncher 샘플은 도구 목적으로 개발 키트의 타이틀 파티션에서 프로세스를 쉽게 실행할 수 있도록 하는 Xbox 응용 프로그램을 제공합니다. 또한 Win32 콘솔 CPU 전용 프로세스와 Xbox D3D12 GPU 프로세스를 모두 사용할 수 있는 방법을 보여주는 두 가지 예제 프로세스가 제공됩니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

빌드, 배포 및 실행과 같은 샘플의 명령줄 사용을 용이하게 하기 위해 여러 스크립트도 제공되었습니다. 스크립트를 사용하려면 "*Xbox \[One/Scarlett\] VS \[2017/2019\] 게임 명령 프롬프트*"를 연 다음 샘플 디렉터리로 이동합니다. 스크립트는 아래 표에 설명되어 있습니다.

| 스크립트 | 사용법 |
|---|---|
| Build.bat | Build.bat \[Configuration\] \[Platform\]<br /><br />지정된 구성(디버그, 릴리스) 및 플랫폼(XboxOne, Scarlett)에 대한 DevkitToolLauncher.exe, CPUTool.exe, GPUTool.exe를 빌드합니다. CPUTool.exe는 항상 x64 구성을 사용하여 간단한 콘솔 Win32 응용 프로그램으로 빌드됩니다. |
| Deploy.bat | Deploy.bat \[Platform\]<br /><br />이전에 기본 개발 키트에 빌드된 DevkitToolLauncher.exe의 느슨한 빌드를 배포합니다. |
| DeployExampleTools.bat | DeployExampleTools.bat \[Configuration\]\[Platform\]<br /><br />CPUTool.exe 및 GPUTool.exe를 기본 개발 키트의 SystemScratch 드라이브에 복사합니다. 이러한 응용 프로그램은 "d:\\DevkitToolLauncherExampleTools\\\[CPU/GPU\]Tool" 폴더에 배포됩니다. |
| BuildAndDeploy.bat | BuildAndDeploy.bat \[Configuration\] \[Platform\]<br /><br />Build.bat, Deploy.bat 및 DeployExampleTools.bat를 하나의 스크립트로 결합합니다. |
| Run.bat | Run.bat \[CommandLine\]<br /><br />이전에 배포된 DevkitToolLauncher.exe를 시작하고 스크립트의 모든 매개 변수를 DevkitToolLauncher.exe의 명령줄에 전달합니다. 명령줄 매개 변수 사용에 대한 자세한 내용은 아래 [명령줄 사용](#Command_Line_Usage) 섹션을 참조하세요. |

# 샘플 사용

이 샘플에서는 DevkitToolLauncher.exe, CPUTool.exe 및 GPUTool.exe의 3가지 프로세스를 제공합니다. DevkitToolLauncher.exe는 기본 샘플 프로세스이며 다른 두 프로세스는 DevkitToolLauncher.exe에서 실행할 수 있는 CPU 전용 및 GPU 사용 프로세스의 예로 제공됩니다.

## 샘플 기능

DevkitToolLauncher 프로세스는 도구 브라우저, 시작 설정 및 런타임 로그의 3개 화면으로 분할됩니다. 여러 화면에서 실행할 프로세스를 찾고, 해당 프로세스의 시작 정보를 설정하며, 실행 중인 프로세스의 콘솔 출력을 볼 수 있습니다.

### 도구 브라우저

![자동으로 생성된 텍스트 설명](./media/image3.png)

도구 브라우저는 개발 키트의 시스템 스크래치 드라이브(d:\\)를 통해 파일 브라우저 인터페이스를 제공합니다. 이 브라우저를 사용하여 실행 파일로 이동하여 실행하고 선택할 수 있습니다. 실행 파일을 선택한 후 샘플은 "*시작 설정"* 화면으로 진행됩니다.

### 설정 시작

![그래픽 사용자 인터페이스, 자동으로 생성된 텍스트 설명](./media/image4.png)

"*시작 설정*" 화면에서 선택한 도구 실행 파일의 시작 동작을 설정할 수 있습니다. 이 화면을 사용하여 명령줄 매개 변수, 작업 디렉터리 및 GPU 지원을 포함하여 샘플을 시작할지 여부를 설정할 수 있습니다.

텍스트 입력을 선택하면 화상 키보드가 표시되어 매개 변수를 입력할 수 있습니다. 명령줄 매개 변수 및 작업 디렉터리가 "DevkitoolLauncherParameterCache.json"이라는 시스템 스크래치 드라이브의 루트에 있는 캐시 파일에 저장됩니다. 이 캐시를 삭제하여 저장된 매개 변수를 다시 설정할 수 있습니다.

GPU 기능이 있는 도구를 지원하려면 "*시작(CPU 및 GPU)"* 버튼은 도구를 시작하기 전에 샘플에서 렌더링을 일시 중단합니다. 도구 실행 파일이 닫히거나 종료되면 렌더링이 자동으로 다시 시작됩니다. 이 샘플에서 GPU 프로세스가 지원되는 방법에 대한 자세한 내용은 [Xbox 개발 키트의 GPU 프로세스](#_Hlk70930525) 섹션을 참조하세요.

*"시작(CPU만 해당)"*을 선택하면 렌더링을 일시 중단하지 않고 "*런타임 로그*" 화면으로 진행됩니다.

선택한 도구 실행 파일에 GPU 기능이 있고 "*시작(CPU만 해당)*"이 선택된 경우 콘솔에서 두 프로세스가 GPU를 사용하려고 시도한 결과로 정의되지 않은 동작 문제가 발생합니다. 예를 들어 응용 프로그램이 충돌하거나, 화면에 아티팩트가 표시되거나, 콘솔이 종료될 수 있습니다. 자세한 내용은 [Xbox 개발 키트의 GPU 프로세스](#_Hlk70930525)를 참조하세요.

실행 중인 도구 실행 파일이 원치 않는 인바운드 연결을 허용해야 하는 경우 해당 포트는 MicrosoftGame.config 파일을 통해 열어야 합니다. 자세한 내용은 [포트 및 Xbox 방화벽](#Ports_Firewall)을 참조하세요.

### 런타임 로그

![자동으로 생성된 텍스트 설명](./media/image5.png)

런타임 로그 화면은 "*시작 설정*" 화면에서 시작 옵션 중 하나를 선택할 때 나타나는 최종 화면입니다.

이 화면에서는 선택한 실행 파일을 하위 프로세스로 시작하고 stdout 및 stderr 쓰기를 DevkitToolLauncher 프로세스에 파이프합니다. 이렇게 파이프된 출력은 화면에 표시됩니다. 게임 패드를 사용하여 화상 로그를 탐색할 수 있습니다.

도구 하위 프로세스에서 GPU를 사용하고 렌더링이 "*시작 설정*" 화면의 "*시작(CPU 및 GPU)*" 버튼을 통해 일시 중단된 경우 이 화면이 표시되지 않습니다. 그러나 샘플은 여전히 출력을 파이프하고 실행 중인 프로세스를 관리합니다. 도구 하위 프로세스가 종료되면 이 화면은 발생한 모든 로깅과 함께 자동으로 표시됩니다.

렌더링이 일시 중단되었는지 여부에 관계없이 게임 패드에서 \[B\]를 길게 눌러 도구 하위 프로세스를 언제든지 종료할 수 있습니다. 도구 하위 프로세서가 더 이상 실행되지 않으면 \[B\]를 눌러 "*도구 브라우저*" 화면으로 돌아가고 다시 시작할 수 있습니다.

## 명령줄 사용

제공된 스크립트 또는 명령줄을 사용하여 샘플을 사용할 필요는 없습니다. 그러나 DevkitToolLauncher 샘플을 사용하여 명령줄을 통해 도구를 시작할 수 있는 방법을 보여 주도록 제공되었습니다.

시작하려면 Xbox 게임 명령 프롬프트 중 하나를 엽니다. 그런 다음 " *cd /D \[SamplePath\]*" 명령을 사용하여 샘플 디렉터리로 이동합니다.

### 빌딩

빌드는 샘플의 디렉터리에 제공된 Build.bat 스크립트를 사용하여 수행할 수 있습니다. 명령줄 매개 변수는 다음과 같습니다.

```
Build.bat [Configuration] [Platform]
```


예를 들어 Scarlett용으로 빌드하려면 "*build.bat release scarlett*" 명령을 사용합니다.

### 배포

배포하기 위해 Deploy.bat라는 다른 스크립트가 제공되었습니다. DevkitToolLauncher 샘플을 배포하면 내부적으로 "*xbapp deploy*" 명령을 사용하여 느슨한 빌드를 개발 키트에 복사합니다.

```
Deploy.bat [Platform]
```


GPUTool 및 CPUTool을 배포하려면 DeployExampleTools.bat 스크립트를 사용합니다. 이 스크립트는 이러한 예제 도구를 "DevkitToolLauncherExampleTools"라는 폴더의 SystemScratch 드라이브에 복사합니다. 이러한 도구는 샘플의 기본 설치 폴더에 있는 대신 샘플의 *도구 브라우저* 화면에서 검색할 수 있도록 SystemScratch 드라이브에 배포됩니다.

```
DeployExampleTools.bat [Configuration] [Platform]
```


### 한 단계로 빌드 및 배포

BuildAndDeploy.bat 스크립트는 단순성을 위해 위의 세 스크립트를 하나의 스크립트로 결합합니다.

BuildAndDeploy.bat \[Configuration\] \[Platform\]

### 실행 중

DevkitToolLauncher.exe에는 "*도구 브라우저*" 및 "*시작 설정*" 화면을 건너뛰고, 도구 프로세스를 시작하고, "*런타임 로그*" 화면으로 바로 이동할 수 있도록 하는 명령줄 설정이 포함되어 있습니다. 명령줄 설정은 다음과 같습니다.

DevkitToolLauncher.exe \[-gpu/processUsesGpu\] \[-workingDir \"path\"\] \[\-- \"process\" \[commandline\]\]

이 명령줄을 사용하면 "*시작 설정"* 화면에서 설정할 수 있는 것과 동일한 정보를 설정할 수 있습니다.

| 매개 변수 | 설명 |
|---|---|
| -gpu 또는 - processUsesGpu | 선택 사항. 지정된 경우 DevkitToolLauncher.exe 샘플은 하위 프로세스 기간 동안 렌더링을 일시 중단합니다. |
| -workingDir | 실행할 프로세스를 지정하는 경우 필요합니다. 하위 프로세스에 대해 설정할 작업 디렉터리를 지정합니다. |
| \-- | 선택 사항. 명령줄에 "\--\" 토큰이 표시되면 샘플은 다음 매개 변수를 실행할 하위 프로세스로 해석합니다. 하위 프로세스는 프로세스의 전체 경로여야 합니다. 프로세스 후의 모든 매개 변수는 해당 하위 프로세스에 대한 명령줄로 전달됩니다. |

예를 들어 DeployExampleToole.bat를 사용하여 시스템 스크래치 드라이브에 복사된 예제 CPUTool을 5초 동안 실행하려면 다음 명령을 사용합니다.

```
Run.bat -workingDir "d:\DevkitToolLauncherExampleTools\CPUTool" --"d:\DevkitToolLauncherExampleTools\CPUTool\CPUTool.exe" 5
```


렌더링이 있는 GPUTool 예제를 실행하려면 다음 명령을 사용합니다.

```
Run.bat -gpu -workingDir "d:\DevkitToolLauncherExampleTools\GPUTool"
-- "d:\DevkitToolLauncherExampleTools\GPUTool\GPUTool.exe"
```


참고: GPUTool.exe 프로세스는 화면에 삼각형을 렌더링하고 자동으로 종료되지 않습니다. 게임 패드에서 \[B\]를 길게 눌러 GPUTool.exe(DevkitToolLauncher.exe에서 시작할 때)를 종료할 수 있습니다.

### 종료 중

Run.bat 명령줄은 *xbapp*을 사용하여 DevkitToolLauncher 샘플을 시작하고 명령줄을 전달합니다. 명령줄에서 샘플을 종료하려면 "*xbapp terminate*&rdquo;을 실행할 수 있습니다.

## 자체 도구

DevkitToolLauncher 샘플은 자체 도구를 실행하기 위한 완전한 호스팅뿐만 아니라 이러한 경우에 개발 키트를 사용하는 방법에 대한 교육 리소스를 제공하기 위해 만들어졌습니다. 자체 도구가 있거나 제작을 계획하는 경우 위의 Run.bat 단계를 사용하여 호스트할 수 있습니다.

도구로 실행할 수 있는 콘솔 응용 프로그램에 대한 자세한 내용은 [Xbox Devkit의 Win32 응용 프로그램](#Win32_Info)을 참조하세요. 개발 키트의 GPU를 활용할 수 있는 도구에 대한 자세한 내용은 [Xbox 개발 키트의 GPU 프로세스](#_Hlk70930525)를 참조하세요.

DevkitToolLauncher에서 호스팅할 수 있으려면 개발 키트의 시스템 스크래치 드라이브에 도구를 배치해야 합니다. 이렇게 하려면 "*xbcp.exe*" 도구를 사용할 수 있습니다.

```
xbcp [ToolLocationPathOnLocalPC] xd:\[Folder]
```


# 구현 참고 사항

이 샘플에서는 Xbox 타이틀에서 일반적이지 않은 일부 기능을 사용합니다. 또한 개발 키트에서 예기치 않은 오류가 발생하지 않도록 하기 위해 반드시 준수해야 하는 특수 요구 사항이 있습니다. 아래의 각 섹션에서는 개발 키트에서 도구를 실행하기 위한 다양한 기능, 요구 사항 및 기타 고려 사항을 설명합니다.

## Xbox 타이틀 파티션

Xbox 개발 키트에서 실행되는 응용 프로그램이 하드웨어 리소스에 대한 모든 권한을 얻으려면 타이틀 파티션 내에서 실행해야 합니다. 타이틀 파티션은 특히 이러한 용도를 위한 Xbox 콘솔의 가상 머신입니다.

일반적으로 Xbox용 타이틀은 개발 키트에 배포되거나 시스템 등록으로 이어지는 패키지로 설치됩니다. 그러면 Xbox 시스템은 타이틀 파티션에서 이러한 타이틀을 제대로 시작할 수 있습니다.

시스템에 등록하지 않은 도구 또는 다른 독립 실행형 프로세스가 있더라도 타이틀 파티션에서 계속 실행할 수 있습니다. 그러나 타이틀 파티션이 먼저 활성 상태여야 합니다. 시스템에서 DevkitToolLauncher.exe를 시작할 때 타이틀 파티션이 활성화되도록 하기 때문에 DevkitToolLauncher 응용 프로그램에서 이를 처리합니다. 그런 다음 DevkitToolLauncher가 다음을 사용하여 도구 또는 프로세스를 시작합니다.
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
      <DebugNetworkPort>4601</DebugNetworkPort>
    </DebugNetworkPortList>
  </DevelopmentOnly>
</Game>
```


요청되지 않은 인바운드 연결을 허용하는 각 포트에 대해 서로 다른t *DebugNetworkPort* 항목을 *DebugNetworkPortList*에 추가해야 합니다. 이는 TCP를 통해 직접 서로 통신하는 P2P 도구와 같은 경우에 필요합니다.

MicrosoftGame.config 파일이 업데이트되면 업데이트 시 픽업할 수 있도록 타이틀을 다시 배포해야 합니다. DevkitToolLauncher 샘플에 대해 이 작업을 수행하는 간단한 방법은 *BuildAndDeploy.bat* 스크립트를 다시 실행하는 것입니다.

## Xbox 개발 키트의 Win32 응용 프로그램

타이틀 파티션은 특정 Xbox 통합을 만들지 않은 Win32 콘솔 응용 프로그램을 직접 실행할 수 있습니다. 그러나 Win32 API 사용 가능성은 Windows PC에서 일반적으로 사용할 수 있는 기능이 줄어든 하위 집합입니다.

Win32 응용 프로그램을 사용 가능한 API로만 제한하려면 Windows 헤더를 포함하기 전에 *WINAPI_FAMILY*를 *WINAPI_FAMILY_GAMES*로 정의해야 합니다.

```
#define WINAPI_FAMILY WINAPI_FAMILY_GAMES
#include <Wwindows.h>
```


CPUTool 프로젝트는 이에 대한 매우 간단한 예제입니다. *WINAPI_FAMILY_GAMES*로 제한된 콘솔 Win32 응용 프로그램을 빌드합니다. 그 결과로 CPUTool.exe는 Windows PC와 Xbox 콘솔 모두에서 실행될 수 있습니다.

참고: 이러한 응용 프로그램은 콘솔 응용 프로그램만 될 수 있으며 렌더링 라이브러리를 사용하지 않아야 합니다. 렌더링의 경우 다음 섹션에서 설명하는 Xbox D3D12를 사용할 수 있습니다.

## Xbox 개발 키트의 GPU 프로세스

타이틀 파티션에서 실행되는 프로세스에는 D3D GPU 사용에 대한 제한 사항이 더 많이 있습니다.

* 타이틀 파티션의 프로세스는 한 번에 1개만 Direct3D를 활용할 수 있습니다.
* Xbox 관련 D3D12 디바이스 메서드 *SuspendX/ResumeX*를 사용하여 D3D를 활용하는 여러 프로세스를 관리할 수 있습니다. 두 프로세스는 D3D를 동시에 적극적으로 활용해서는 안 됩니다.
* Xbox 전용 D3D 헤더 및 라이브러리를 사용해야 합니다.

이러한 고려 사항을 부적절하게 처리하면 응용 프로그램이 충돌하거나 렌더링이 손상되거나 콘솔 작동이 중단될 수 있습니다. DevkitToolLauncher 샘플은 GPU를 활용하는 하위 프로세서를 만들기 전에 *SuspendX*를 사용하여 이러한 사례를 처리합니다. 해당 하위 프로세서가 완전히 종료되면 *ResumeX*를 사용하여 샘플의 렌더링을 다시 사용하도록 설정합니다.

GPUTool 프로젝트는 화면에 삼각형을 렌더링하는 단순한 프로세스입니다. GPU 고려 사항이 존중될 때 타이틀 파티션에서 GPU 하위 프로세서를 사용하는 방법을 보여 주는 데 사용됩니다.

## 다수의 프로세스가 있는 GameInput

GameInput 라이브러리는 현재(이 샘플을 만든 시점) 이 샘플을 만들 때 한 번에 1개 이상의 프로세스에 사용되는 것을 지원하지 않습니다. 또한 프로세스 중에나 프로세스를 종료하고 시작하여 정리하고 다시 초기화할 수 없습니다. 따라서 GameInput을 초기화하는 첫 번째 프로세스가 이를 사용할 수 있는 유일한 프로세스입니다. 초기화를 시도하는 이후의 모든 프로세스는 작동이 중단됩니다.

DevkitToolLauncher 샘플은 GameInput을 사용하여 입력 처리를 제공하므로 도구 하위 프로세스는 현재 GameInput을 활용할 수 없습니다.

## CPU 사용량 오버헤드 샘플링

일반적으로 DevkitToolLauncher 샘플은 CPU 오버헤드가 매우 낮습니다. [절전](https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep)을 사용하여 프레임 속도를 30FPS로 제한하여 사용량을 낮게 유지합니다. 절전 모드가 아닌 경우 대부분의 샘플 기능은 매우 빠릅니다. 그러나 도구 하위 프로세스가 모든 CPU 코어의 처리 시간을 최대한 많이 확보해야 하는 경우 디버그 구성에서 UITK를 렌더링하면 문제가 될 수 있습니다.

디버그 구성에서 최상의 도구 하위 처리 성능을 얻으려면 gpu가 아닌 프로세스를 비롯한 모든 프로세스에서 "*시작(CPU 및 GPU)*"(또는 명령줄을 사용하는 경우 -gpu)을 사용할 수 있습니다. 이렇게 하면 샘플 렌더링이 일시 중단되므로 DevkitToolLauncher 샘플의 오버헤드가 대부분 제거됩니다.

릴리스 구성을 사용하여 빌드할 때 UITK의 성능 영향도 효과적으로 제거됩니다.

# 업데이트 기록

2021년 6월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


