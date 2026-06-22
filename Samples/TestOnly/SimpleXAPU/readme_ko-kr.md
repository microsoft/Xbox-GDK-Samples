# 간단한 XAPU 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와 호환됩니다.*

# 설명

이 샘플에서는 Scarlett에서 XAudio2를 사용하여 Opus OGG 파일을 스트리밍하는 방법을 보여줍니다.

![](./media/image1.png)

# 샘플 빌드

이 샘플은 Project Scarlett에서만 작동합니다.

*자세한 내용은* *GDK 설명서*의 __샘플 실행__을 참조하세요.

# 샘플 사용

샘플에는 보기 버튼을 통한 종료 외에는 컨트롤이 없습니다.

# 구현 참고 사항

이 샘플에서는 기본 OGG 구문 분석을 사용하여 Opus 파일을 스트리밍하는 방법을 보여줍니다.

XAudio2를 사용하여 스트리밍하는 다른 예제는 [GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/XAudio2)를 참조하세요.

- **XAudio2AsyncStream** - Win32의 버퍼링되지 않은 중첩된 I/O를 지원하기 위한 디스크의 .WAV 데이터 준비

- **XAudio2MFStream** - 미디어 파운데이션 원본 판독기를 사용하여 WMA 파일에서 데이터의 압축 해제

# 알려진 문제

이 샘플은 OGG 메타데이터 프레임을 지원하지 않습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


