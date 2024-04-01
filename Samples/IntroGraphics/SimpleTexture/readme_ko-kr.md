# 간단한 텍스처 샘플

*이 샘플은 Microsoft 게임 개발 키트(2020년 6월)와 호환 가능합니다.*

# 설명

이 샘플은 Direct3D 12를 사용하여 단순한 텍스처 처리된 쿼드를 렌더링하는 방법을 보여줍니다.

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S 개발 도구를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

샘플에는 종료 외에는 컨트롤이 없습니다.

# 구현 참고 사항

텍스처는 WIC(Windows 이미징 구성 요소)를 사용하는 간단한 도우미를 사용하여 여기에 로드되며 학습의 단순성을 위해 설계되었습니다. 프로덕션용으로 DirectX 도구 키트를 살펴보아야 합니다.
[DDSTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/DDSTextureLoader)
및 [WICTextureLoader](https://github.com/Microsoft/DirectXTK12/wiki/WICTextureLoader).

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


