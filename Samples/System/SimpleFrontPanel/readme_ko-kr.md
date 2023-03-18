  ![](./media/image1.png)

#   SimpleFrontPanel 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 

# 설명

SimpleFrontPanel 샘플은 Xbox One X Devkit 및 Project Scarlett Devkit
전면 패널 디스플레이에 대한 프로그래밍을 시작하는 데 필요한 기본 기능을
포함하는 XFrontPanelDisplay API를 보여 줍니다. 이 샘플에서는 전면 패널을
작동하고 전면 패널이 없는 경우(예: Xbox One 또는 Xbox One S devkit)를
처리하는 방법을 보여 줍니다. 또한 이 샘플에서는 전면 패널 단추 상태에
대한 폴링, 전면 패널 라이트 상태 가져오기/설정, 전면 패널 LCD
디스플레이에 간단한 비트 패턴 그리기 등의 기본 기능도 다룹니다. 또한 이
샘플에서는 전면 패널 디스플레이 버퍼를 .dds 질감 파일에 저장하는 방법도
보여 줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

이 샘플은 전면 패널이 통합된 Xbox One X Devkit 및 Project Scarlett
Devkit에 사용됩니다. 이 샘플을 시작하면 바둑판 패턴이 전면 패널
디스플레이로 렌더링됩니다. 전면 패널 D-패드(왼쪽, 오른쪽)를 사용하여
디스플레이 비트 패턴을 변경하고 픽셀의 밝기(위쪽, 아래쪽)를 변경합니다.
D-패드 단추를 누르거나 선택하여 전면 패널 디스플레이에 대한 버퍼를
캡처할 수도 있습니다. 5개의 전면 패널 단추 각각에는 통합 LED가 연결되어
있습니다. 단추를 누르면 라이트 켜짐 또는 꺼짐이 토글됩니다.

![](./media/image3.png)

이 샘플은 통합 전면 패널을 사용하여 모든 입력과 출력을 수행하고, 게임
패드 또는 연결된 디스플레이와 상호 작용하지 않습니다. 이 샘플은 Xbox One
또는 Xbox One S에서 실행되지만, 전면 패널 디스플레이가 없는 devkit에서는
흥미로운 기능을 제공하지 않습니다.

## 바둑판 무늬 화면

| 작업                                |  전면 패널                      |
|-------------------------------------|--------------------------------|
| 이전 화면                           |  D-패드 왼쪽                    |
| 다음 화면                           |  D-패드 오른쪽                  |
| 밝기 증가                           |  D-패드 위로                    |
| 밝기 감소                           |  D-패드 아래로                  |
| 전면 패널 캡처                      |  D-패드 선택                    |
| 단추 라이트 토글                    |  전면 패널 단추                 |

![](./media/image5.gif)

## 

## 

## 

## 

## 

## 그라데이션 화면

![](./media/image6.gif)

구현 참고 사항

-   Xbox 1 X Devkit 또는 Project Scarlett Devkit에서
    ::XFrontPanelIsAvailable()은 true를 반환하며 전체 API를 사용할 수
    있게 됩니다. 그렇지 않으면 ::XFrontPanelIsAvailable()은 false를
    반환하고 다른 ::XFrontPanel\*() 함수는 실패한 HRESULT 코드를
    반환합니다(Xbox One, Xbox One S 또는 실제 전면 패널이 없는 정품
    콘솔).

-   모든 프레임(::XFrontPanelPresentBuffer())의 전면 패널을 제공할
    필요는 없습니다. 대신, 하나 이상의 픽셀이 변경되었을 때만 제공하면
    됩니다. 따라서 이 샘플에는 디스플레이 버퍼가 변경될 때마다 설정되는
    m_dirty 멤버가 있습니다.

-   또한 변경이 있는 경우에만 라이트 상태를 설정하는 것이 좋습니다.

-   ::XFrontPanelGetScreenPixelFormat()은 DXGI_FORMAT_R8_UNORM을
    반환하지만 화면 자체는 16개의 회색 음영만 지원합니다. 규칙에 따라 각
    8비트 픽셀에 4개의 상위 비트만 사용하여 회색조 값을 인코딩해야
    합니다. 하위 비트는 무시됩니다. 예를 들어
    Sample::CheckerboardFillPanelBuffer() 및
    Sample::GradientFillPanelBuffer()를 확인합니다.

-   이 API는 디스플레이의 밝기 변경을 지원하지 않습니다. 이 예제에서는
    각 픽셀을 0x10 크기만큼 증가/감소하여 이러한 작업을 지원합니다. 예를
    들어 Sample::BrightenPanelBuffer() 및 Sample::DimPanelBuffer()를
    확인합니다.

-   전면 패널 버퍼에 직접 액세스할 수 없습니다. 대신 자체 버퍼를
    관리하고 버퍼 주소를 ::XFrontPanelPresentBuffer()에 전달해야 합니다.
    Sample::CaptureFrontPanelScreen()은 단순히 m_panelBuffer의 내용을
    DDS 표면의 픽셀 페이로드로 사용합니다.

# 업데이트 기록

2019년 4월, 샘플의 첫 번째 릴리스

2019년 11월, Project Scarlett Devkit 지원

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플의 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 일반 개인정보취급방침에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
