# 단순 플레이 사운드 스트림 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).*

# 설명

이 샘플에서는 Xbox One에서 XAudio2를 사용하여 wav 파일을 스트리밍하는 방법을 보여 줍니다.

![](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

샘플에는 보기 버튼을 통한 종료 외에는 컨트롤이 없습니다.

# 구현 참고 사항

이 샘플에서는 자체 WAV 파일 파서로 wav 파일을 스트리밍하는 방법을 보여 줍니다.

XAudio2를 사용하여 스트리밍하는 다른 예제는 [GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)를 참조하세요.

- **XAudio2AsyncStream** - Win32의 버퍼링되지 않은 중첩된 I/O를 지원하기 위한 디스크의 .WAV 데이터 준비

- **XAudio2MFStream** - 미디어 파운데이션 원본 판독기를 사용하여 WMA 파일에서 데이터의 압축 해제

- 모든 XAaudio2 형식에 대해 버퍼링되지 않은 겹치는 I/O를 구현하는 *DirectX 도구 키트의* **SoundStreamInstance**입니다.

# 알려진 문제

이 샘플은 xWMA .wav 파일 스트리밍을 지원하지 않습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


