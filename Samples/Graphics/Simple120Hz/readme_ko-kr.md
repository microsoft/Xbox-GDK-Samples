# 단순 120Hz 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

이 샘플은 GXDK의 여러 새로 고침 빈도(30Hz, 40Hz, 60Hz, 120Hz)에서 렌더링의 기본 사항을 보여줍니다. 디스플레이가 HDR 및 120Hz 출력을 동시에 지원하지 않을 때 HDR 또는 새로 고침 빈도의 선호 설정을 지정하는 새로운 기능을 보여 줍니다. 대상
GameDVR용 SDR 이미지를 생성하는 하드웨어 3D LUT. ![](./media/image1.png)
| | |
|---|---|
|기본 제공된 Xbox Series X|S에서 앱이 활용하는 HDR 렌더링 촉진|


# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 컨트롤

| 동작 | Gamepad |
|---|---|
| 새로 고침 빈도 선택 | 방향 패드 위/아래 |
| HDR 모드 기본 설정 토글 | A 버튼 |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

이 샘플은 HDR 또는 120Hz에 대한 프레임 속도 및 받아쓰기 기본 설정을 선택하기 위한 두 가지 중요한 GXDK API를 보여줍니다.

첫 번째는 `ID3D12Device::SetFrameIntervalX`(으)로, 프레임 시작부터 프레임 표시까지의 기간을 함께 표시하여 프레임 간격(마이크로초)과 프레임 기간을 간격(간격 수)으로 설정합니다. 선택된 프레임 간격은 디스플레이에 의해 지원되어야 하며, 이는
Xbox One의 `IDXGIOutput::GetDisplayModeList`입니다. 두 번째는 `XDisplayTryEnableHdrMode`(으)로, 디스플레이의 HDR 모드를 사용하려고 시도합니다. 디스플레이가 이들을 동시에 지원하지 않는 경우 사용자가 HDR 또는 120Hz 새로 고침 빈도에 대한 기본 설정을 선택할 수 있습니다. 타이틀은 해당 기능을 다시 호출하여 어느 시점에서나 기본 설정을 변경할 수 있습니다. 이것은 지원되는 프레임 간격의 새로 고침을 필요로 하는 `IDXGIOutput::GetDisplayModeList(X)`의 결과를 변경할 수 있습니다.
| | |
|---|---|
|Xbox Series X|S의 `IDXGIOutput::GetDisplayModeListX`을(를) 사용하여 쿼리됨 또는|


HDR 디스플레이에 대한 렌더링은 GameDVR, 스크린샷, 브로드캐스팅에 적합한 SDR 이미지를 생성하여 그 자체로 타이틀에 대한 우려를 수반합니다.
HDR에서 SDR로의 색 변환을 자동으로 수행하는 하드웨어 3D LUT의 기능. 이렇게 하면 타이틀에서 Xbox One에서 일반적으로 사용되는 드라이버의 소프트웨어 3D LUT를 수동으로 변환하거나 사용하는 모든 CPU, GPU, 메모리 및/또는 대역폭 비용이 절약됩니다. 이는 `DXGI_COLOR_SPACE_RGB_FULL_G2084_NONE_P2020` 색 공간이 있는 단일 디스플레이 평면을 표시하고 백버퍼 텍스처에서 `D3D12XBOX_RESOURCE_FLAG_ALLOW_AUTOMATIC_GAMEDVR_TONE_MAP` 플래그를 생략할 때의 기본 설정입니다.
| | |
|---|---|
|및 스트리밍. 이 샘플은 새 Xbox Series X|S를 보여줍니다.

# 업데이트 기록

2020년 6월 4일 -- 샘플 만들기.

2022년 3월 -- 24Hz 지원이 추가됨.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


