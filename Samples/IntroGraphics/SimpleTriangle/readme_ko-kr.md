# 단순 삼각형 샘플

*이 샘플은 Microsoft 게임 개발 키트(2020년 6월)와 호환 가능합니다.*

# 설명

이 샘플은 화면에 삼각형을 렌더링하기 위해 정적 Direct3D 12 버텍스 버퍼를 만드는 방법을 보여줍니다.

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S 개발 도구를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

샘플에는 종료 외에는 컨트롤이 없습니다.

# 구현 참고 사항

이 샘플의 주요 목적은 판독기가 ATG 샘플 템플릿 구조에 익숙해지고 Direct3D 12 API를 사용하는 간단한 데모를 제공하는 것입니다.

> **CreateDeviceDependentResources**: 여기에서 컴파일된 정점
> 및 픽셀 셰이더 Blob이 로드되고 다양한 Direct3D 렌더링
> 리소스가 생성됩니다. *Visual Studio에서 셰이더를 컴파일합니다.*
>
> **렌더링:** 여기에서 삼각형이 렌더링되어
> 화면이 표시됩니다.

[DeviceResources](https://github.com/Microsoft/DirectXTK12/wiki/DeviceResources)에서 장치 만들기 및 프레젠테이션 처리에 대해 자세히 알아보세요.

[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)에서 루프 타이머 사용에 대한 세부 정보를 알아보세요.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


