# 간단한 장치 및 SwapChain 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 설명

이 샘플에서는 Xbox One 앱용 Direct3D 12 장치 및 PresentX 스왑 체인을
만드는 방법을 보여 줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

이 샘플에는 종료 이외의 다른 컨트롤이 없습니다.

# 구현 참고 사항

Xbox One 앱 Direct3D 설정은 다른 Microsoft 플랫폼과 매우 유사하지만 이
샘플은 몇 가지 주요 차이점을 보여 줍니다.

-   표준 D3D12CreateDevice 대신 **D3D12XboxCreateDevice** 사용

-   4K 네이티브 스왑 체인 및 1080p 사용

-   이 경우에는 프레젠테이션에 DXGI를 사용하는 대신 새 **PresentX**
    API를 사용합니다.

Direct3D 12 장치 만들기의 모범 사례에 대한 자세한 내용은 [Direct3D 12
장치 만들기
분석](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)을
참조하세요.

루프 타이머를 사용하는 방법에 대한 자세한 내용은
[StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)를
참조하세요.

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플의 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
