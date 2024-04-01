# 간단한 WASAPI 캡처 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).*

# 설명

이 샘플에서는 Xbox One에서 WASAPI를 사용하여 오디오를 캡처하는 방법을 보여 줍니다.

![샘플 스크린샷](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

게임 패드를 사용하여 캡처 디바이스를 선택합니다. 샘플에서는 기본 오디오 렌더링을 자동으로 사용합니다. **캡처와 렌더링 간에는 샘플 속도 변환이 없으므로 속도가 일치하지 않는 한 소리가 제대로 들리지 않습니다.**

# 구현 참고 사항

이 샘플에서는 WASAPI를 사용하여 오디오를 캡처하는 방법을 보여 줍니다. 캡처된 샘플은 샘플 렌더링에 사용되는 원형 버퍼에 배치됩니다. 이 예제에서는 렌더러와 캡처 간에 공유 WASAPI 인스턴스도 사용합니다. WASAPI의 고급 사용은 [Windows WASAPI 샘플](https://code.msdn.microsoft.com/windowsapps/Windows-Audio-Session-22dcab6b)을 참조하세요.

# 업데이트 기록

2019년 5월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


