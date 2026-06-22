![](./media/image1.png)

# AmbientOcclusion 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

이 샘플에서는 MiniEngine의 SSAO(Screen Space Ambient Occlusion)와 Intel의 GTAO(Ground Truth Ambient Occlusion)의 두 가지 주변 폐색 기술을 보여 줍니다. 샘플은 또한 품질을 약간 낮춰 성능을 향상시킬 수 있는 일부 셰이더의 FP16 버전을 보여 줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 사용

왼쪽 및 오른쪽 엄지스틱을 사용하여 카메라를 장면 주위로 이동할 수 있습니다. 위쪽 및 아래쪽 방향 패드 버튼은 현재 옵션을 선택합니다. 왼쪽 및 오른쪽 방향 패드 버튼은 옵션 값을 수정합니다. A 버튼은 AO 텍스처와 장면 사이의 디스플레이를 변경합니다. B 버튼은 SSAO와 GTAO 사이를 전환합니다. Y 버튼은 장면 렌더링에서 AO를 켜거나 끕니다. X 버튼은 FP16과 FP32 셰이더 사이를 전환합니다(Scarlett만 해당).

![](./media/image2.bmp)

# 컨트롤

| 동작 | Gamepad |
|---|---|
| AO만 렌더링 설정/해제 | A button |
| SSAO/GTAO 설정/해제 | B 버튼 |
| AO 켜기/끄기 | Y 버튼 |
| FP16/FP32 셰이더 설정/해제 | X 버튼 |
| 옵션 선택 | 방향 패드 위/아래 |
| 선택한 옵션 수정 | 방향 패드 오른쪽/왼쪽 |
| 카메라 이동 | 왼쪽 엄지스틱 |
| 카메라 회전 | 오른쪽 엄지스틱 |

# 구현 참고 사항

이 샘플에서는 앰비언트 폐색을 렌더링하는 두 가지 방법을 보여 줍니다.

1. 화면 공간 앰비언트 폐색(SSAO): SSAO 구현은 [MiniEngine](https://github.com/microsoft/DirectX-Graphics-Samples/tree/master/MiniEngine)에서 수행됩니다. HTile을 사용하여 압축된 깊이 버퍼에서 직접 읽고 다운샘플링된 버전을 만듭니다. 그런 다음 깊이 텍스처를 샘플링하고 AO 텍스처의 여러 해상도를 만듭니다. 마지막 패스는 AO 텍스처에서 읽고 업샘플링하고 흐리게 처리합니다.

2. GTAO(Ground Truth Ambient Occlusion): GTAO 구현은 [Intel](https://github.com/GameTechDev/XeGTAO/tree/master)에서 수행됩니다. 먼저 깊이 버퍼를 다운샘플링하여 밉 체인을 만듭니다. 엔진이 아직 법선을 내보내지 않은 경우 깊이 버퍼에서 법선을 생성하는 패스를 실행합니다. 그런 다음 기본 패스를 실행하여 깊이 버퍼에서 샘플링하여 초기 AO 텍스처와 가장자리 텍스처를 생성합니다. 마지막 패스는 AO 텍스처의 소음을 제거합니다.

두 방법 모두 FP32 버전에 비해 향상된 성능을 제공하는 패스에 대해 Scarlett의 FP16 셰이더를 지원합니다. FP16를 통한 성능의 향상은 두 값을 하나의 레지스터로 패킹함으로 인한 점유 증가 혹은 패킹된 수학 명령어 사용으로 인한 VALU 연산 감소의 조합에서 비롯됩니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


