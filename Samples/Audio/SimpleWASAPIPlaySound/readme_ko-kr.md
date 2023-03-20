# 간단한 WASAPI 소리 재생 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 설명

이 샘플에서는 Xbox One의 WASAPI 렌더 끝점에 대해 설정을 재생하고 간단한
소리(사인 톤)를 재생하는 방법을 보여 줍니다.

![](./media/image1.png)

# 샘플 빌드하기

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

키보드의 스페이스바나 게임 패드의 단추 A를 사용하여 재생을 시작 및
중지합니다. 키보드의 Esc 키나 보기 단추를 사용하여 앱을 종료합니다.

# 구현 참고 사항

WASAPI에 대한 자세한 내용은
[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/dd371455.aspx)을
참조하세요.

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플의 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
