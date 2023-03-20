# 단순한 질감 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 설명

이 샘플에서는 Direct3D 12를 사용하여 간단한 질감이 있는 4분면을
렌더링하는 방법을 보여 줍니다.

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

이 샘플에는 종료 이외의 다른 컨트롤이 없습니다.

# 구현 참고 사항

이 질감은 WIC(Windows 이미징 구성 요소)를 사용하는 간단한 도우미를 통해
여기에 로드되며 간편하게 배울 수 있게 디자인되었습니다. 프로덕션에서
사용하려는 경우에는 DirectX 도구 키트의
[DDSTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/DDSTextureLoader)
및
[WICTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/WICTextureLoader)를
확인해야 합니다.

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플의 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
