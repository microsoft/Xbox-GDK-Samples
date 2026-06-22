  ![](./media/image1.png)

#   가변 비율 음영(VRS) 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기와 호환됩니다(2019년
11월).*

# 

# 설명

Anaconda(Xbox Series X)와 Lockhart는 모두 tier2.x 가변 비율 음영을
지원합니다. 이는
<https://microsoft.github.io/DirectX-Specs/d3d/VariableRateShading.html>에서
설명하는 tier2 VRS의 상위 집합입니다.

이 기법은 픽셀당 한 번 이하로 픽셀 셰이더를 호출하고 결과를 여러 픽셀에
브로드캐스팅하여 픽셀 셰이더 부하를 줄입니다. 이 기능은 투명도를
비롯하여 포워드 렌더링에 특히 유용하지만 지연 렌더러에도 유용합니다.

이 샘플은 정확하게 동일한 장면을 프레임당 두 번 렌더링하고 둘 모두의
시간을 측정하여 VRS에 따른 절약을 보고합니다. VRS를 사용하여 렌더링되는
장면은 음영 비율을 실제 측정이 아니라 VRS로 렌더링된 이전 프레임에서
도출합니다. 실제 측정, VRS로 렌더링되는 장면, 절대 차이, 사용된 음영
비율을 볼 수 있도록 디버그 모드가 제공됩니다.

광선 행진 음영 및 AO는 픽셀 셰이더에게 몇 가지 유용한 작업을 제공하는 데
사용됩니다.

# ![](./media/image3.png)

#  샘플 빌드

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

이 샘플은 Xbox One을 지원하지 않습니다.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

| 작업                             |  게임패드                          |
|----------------------------------|-----------------------------------|
| 디버그 모드 변경                 |  D-패드 왼쪽/오른쪽                |
| 태양 제어                        |  Y 버튼 및 숄더 버튼               |
| 음영 비율 허용 오차 수정         |  트리거 버튼                       |
| 카메라                           |  스틱 및 D-패드 위로/아래로        |
| 시각화 변경                      |  X 버튼                            |
| 음영 비율 생성 셰이더를 순환     |  B 버튼                            |
| 종료                             |  보기 단추                         |

# 

# 구현 참고 사항

VRS용 API 노출 영역은 아주 작습니다. 주요 항목은 다음과 같습니다.

Terrain::RenderVRS -- RSSetShadingRate 및 RSSetShadingRateImage를
호출합니다.

Terrain::CalculateShadingRat -- 이전 프레임으로부터 음영 비율을 생성하는
계산 셰이더를 실행합니다.

또한 HLSL에서 GenerateShadingRate.hlsl의 BuildShadingRate 함수

전체 해상도 데이터에서 절반 해상도 데이터까지 작동하는 wave32 및
wave64용 네 가지 변형 BuildShadingRate가 있습니다. 실험에서 확인한
바로는 wave64가 더 빠르며 절반 해상도가 권장될 수 있습니다.

# 알려진 문제

이 샘플은 아직 Lockhart 하드웨어에서 테스트되지 않았습니다.

# 업데이트 기록

초기 릴리스 2020년 1월

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 일반 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
