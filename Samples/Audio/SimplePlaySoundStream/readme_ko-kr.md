# 간단한 재생 소리 스트림 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기와 호환됩니다(2019년
11월).*

# 설명

이 샘플에서는 Xbox One에서 XAudio2를 사용하여 wav 파일을 스트리밍하는
방법을 보여 줍니다.

![](./media/image1.png)

# 샘플 빌드하기

Xbox One devkit을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용하기

이 샘플에는 보기 버튼으로 종료하는 것 외에 다른 컨트롤은 없습니다.

# 구현 참고 사항

이 샘플에서는 자체 WAV 파일 파서를 사용하여 wav 파일을 스트리밍하는
방법을 보여 줍니다.

XAudio2를 사용하여 스트리밍하는 다른 예제는
[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)를
참조하세요.

-   Win32 버퍼링되지 않은 중복 I/O를 지원하기 위해 디스크의 .WAV
    데이터를 준비하는 **XAudio2AsyncStream**

-   미디어 파운데이션 원본 읽기 프로그램을 사용하여 WMA 파일에서 데이터
    압축을 푸는 **XAudio2MFStream**

# 알려진 문제

이 샘플은 xWMA .wav 파일 스트리밍을 지원하지 않습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용 원격 분석\"이라고 표시된 Main.cpp에서 코드
블록을 제거할 수 있습니다.

Microsoft의 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
