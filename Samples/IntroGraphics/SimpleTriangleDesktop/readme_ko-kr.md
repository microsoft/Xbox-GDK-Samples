# 간단한 삼각형 샘플(PC)

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2019년 11월).*

# 설명

이 샘플에서는 화면에 삼각형을 렌더링하는 정적 Direct3D 12 버텍스 버퍼를
만드는 방법을 보여 줍니다.

![](./media/image1.png)

# 샘플 사용

이 샘플에는 종료 이외의 다른 컨트롤이 없습니다.

이 샘플은 DirectX 12 지원 비디오 카드가 장착된 모든 Windows 10
시스템에서 실행됩니다. 디버그 구성에서는 DirectX 12 지원 비디오 카드가
없다면, 사용 가능한 경우 WARP12를 사용합니다(*그래픽 도구* 선택적
Windows 구성 요소 필요).

# 구현 참고 사항

이 샘플의 주요 목적은 독자에게 ATG 샘플 템플릿 구조를 소개하고 Direct3D
12 API를 사용하는 간단한 데모를 제공하는 것입니다.

> **CreateDeviceDependentResources**: 컴파일된 꼭짓점과 픽셀 셰이더
> BLOB을 로드하고 다양한 Direct3D 렌더링 리소스를 만듭니다. *셰이더는
> Visual Studio에서 컴파일됩니다.*
>
> **Render:** 삼각형을 렌더링하고 화면에 표시합니다.

디바이스 만들기 및 프레젠테이션 처리에 대한 자세한 내용은
[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)를
참조하세요.

루프 타이머를 사용하는 방법에 대한 자세한 내용은
[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)를
참조하세요.
