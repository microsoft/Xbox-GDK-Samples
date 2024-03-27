# 간단한 삼각형 샘플(PC)

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

이 샘플은 화면에 삼각형을 렌더링하기 위해 정적 Direct3D 12 버텍스 버퍼를 만드는 방법을 보여줍니다.

![](./media/image1.png)

# 샘플 사용

샘플에는 종료 외에는 컨트롤이 없습니다.

이 샘플은 DirectX 12 지원 비디오 카드가 장착된 모든 Windows 10 시스템에서 실행됩니다. 디버그 구성에서 DirectX 12 지원 비디오 카드를 찾을 수 없으면, 가능한 경우 WARP12를 사용합니다(*그래픽 도구* 선택적 Windows 구성 요소 필요).

# 구현 참고 사항

이 샘플의 주요 목적은 판독기가 ATG 샘플 템플릿 구조에 익숙해지고 Direct3D 12 API를 사용하는 간단한 데모를 제공하는 것입니다.

> **CreateDeviceDependentResources**: 여기에서 컴파일된 정점
> 및 픽셀 셰이더 Blob이 로드되고 다양한 Direct3D 렌더링
> 리소스가 생성됩니다. *셰이더는 Visual Studio에 의해 컴파일됩니다.*
>
> **렌더링:** 여기에서 삼각형이 렌더링되어
> 화면이 표시됩니다.

[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)에서 장치 만들기 및 프레젠테이션 처리에 대해 자세히 알아보세요.

루프 타이머 사용에 대한 자세한 내용은 [StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)를 참조하세요.


