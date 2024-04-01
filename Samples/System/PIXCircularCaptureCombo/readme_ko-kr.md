![](./media/image1.png)

# SimpleDirectStorageCombo 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2023년 10월).*

# 설명

이 샘플에서는 PIXBeginCapture API의 순환 모드를 사용하여 항상 성능 데이터를 순환 버퍼에 기록한 다음, 요청 시 캡처 파일에 버퍼를 저장하는 방법을 보여줍니다.

# 샘플 빌드

이 샘플은 다음 플랫폼을 지원합니다.

- Gaming.Scarlett.xbox.x64
- Gaming.XboxOne.xbox.x64

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

샘플은 순환 버퍼에서 자동으로 캡처를 시작합니다. 1000 프레임마다 또는 'a' 버튼을 누르면 샘플에서 캡처를 파일에 저장한 다음 캡처를 다시 시작합니다. 'b' 버튼을 누르면 버퍼가 플러시되지만 캡처는 삭제됩니다.

# 구현 참고 사항

Xbox에서 API는 PIX UI를 통해 타이밍 캡처로 변환해야 하는 'pevt' 파일을 생성합니다.

PIXEndCapture의 Xbox 버전은 비동기이며 캡처가 완전히 중지될 때까지 E_PENDING을 반환합니다.

# 업데이트 기록

2023년 10월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


