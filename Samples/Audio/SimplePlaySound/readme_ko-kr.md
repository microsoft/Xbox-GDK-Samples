# 간단한 사운드 재생 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).*

# 설명

이 샘플에서는 Xbox One에서 XAudio2를 사용하여 wav 파일을 재생하는 방법을 보여 줍니다.

![](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

샘플에는 보기 버튼을 통한 종료 외에는 컨트롤이 없습니다. 각 파일이 완료되면 샘플 wav 파일을 통해 자동으로 진행됩니다.

# 구현 참고 사항

이 샘플에서는 PCM, ADPCM, xWMA 및 XMA2 형식 wav 파일을 재생하는 방법을 보여 줍니다. *ATG 도구 키트* 파일 **WAVFileReader.h/.cpp**에서 도우미 코드를 사용합니다. 지원되는 사운드 형식의 재생 시간을 계산하기 위한 코드와 함께 단순한 wav 파일 파서가 구현됩니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


