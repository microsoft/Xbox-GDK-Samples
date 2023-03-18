# 간단한 WASAPI 캡처 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 설명

이 샘플에서는 Xbox One에서 WASAPI를 사용하여 오디오를 캡처하는 방법을
보여 줍니다.

![Sample Screenshot](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

게임 패드를 사용하여 캡처 장치를 선택합니다. 이 샘플은 자동으로 기본
오디오 렌더러를 사용합니다. **캡처와 렌더 간에는 샘플링 속도 변환이
없으므로 속도가 일치하지 않으면 재생 소리가 올바르지 않습니다.**

# 구현 참고 사항

이 샘플에서는 WASAPI를 사용하여 오디오를 캡처하는 방법을 보여 줍니다.
캡처된 샘플은 순환 버퍼에 배치된 후 샘플 렌더링에 사용됩니다. 이
예제에서는 렌더러와 캡처 사이에 공유 WASAPI 인스턴스를 사용합니다.
WASAPI를 사용하는 고급 방법에 대한 자세한 내용은 [Windows WASAPI
샘플](https://code.msdn.microsoft.com/windowsapps/Windows-Audio-Session-22dcab6b)을
참조하세요.

# 업데이트 기록

초기 릴리스 2019년 5월

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
