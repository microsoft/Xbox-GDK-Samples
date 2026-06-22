  ![](./media/image1.png)

#   MP4Encoder 샘플

*이 샘플은 Microsoft 게임 개발 키트(2020년 8월)와 호환됨*

# 

# 설명

이 샘플은 H264 코덱을 사용하여 게임의 백 버퍼를 MP4 비디오 파일로
인코딩하는 데 미디어 파운데이션 API를 사용하는 방법을 보여 줍니다.

![A picture containing chart Description automatically generated](./media/image3.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정합니다.

Project Scarlett를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정합니다.

자세한 내용은 GDK 설명서에서 샘플 실행을 참조하세요.

# 샘플 사용

이 샘플에서는 다음 컨트롤을 사용합니다.

| 작업                                         |  게임 패드             |
|----------------------------------------------|-----------------------|
| 인코딩 중지                                  |  A                     |
| 종료                                         |  보기 단추             |

# 구현 참고 사항

이 샘플에서는 컴퓨팅 셰이더를 사용하여 백 버퍼를 비디오 인코딩에 사용할
수 있는 NV12 텍스처로 변환합니다. NV12 텍스처에는 광도 데이터(Y)가 있는
전체 해상도 평면 0과 채도 데이터(UV)가 있는 절반 해상도 평면 1의 두
부분이 포함됩니다. 컴퓨팅 셰이더는 광도를 계산하기 위해 간단한 이중 선형
다운샘플을 수행합니다.

미디어 파운데이션 API는 Xbox 하드웨어 인코더를 사용하여 NV12 텍스처를
H264 비디오 스트림이 있는 MP4 비디오 파일로 인코딩하는 데 사용됩니다. 두
개의 파일이 devkit의 시스템 스크래치 드라이브에 기록되는데, 하나는 원시
H264 비디오 스트림과 H264 비디오 스트림이 포함된 MP4 파일입니다. 이러한
파일은 "xbdir xd:\\\\MP4EncoderSampleOutput.mp4"를 사용하여 찾을 수
있습니다.

타이틀은 Xbox 하드웨어 인코더를 사용하여 최대 1080p @ 30Hz로 인코딩할 수
있습니다. 인코더는 게임 DVR의 시스템 수준에서도 사용되기 때문입니다.
30Hz에서만 인코딩할 수 있지만 샘플은 60Hz로 렌더링됩니다. 따라서 샘플은
렌더링이 중단되지 않도록 인코딩을 위해 별도의 스레드를 만듭니다.

MP4 파일을 쉽게 보려면 xbcp를 사용하여 파일을 복사하거나 Xbox 관리자
도구에서 파일 탐색기 기능을 사용할 수 있습니다.

![Graphical user interface, application Description automatically generated](./media/image4.png)

![Graphical user interface Description automatically generated](./media/image5.png)

![Graphical user interface, text, application Description automatically generated](./media/image6.png)

# 알려진 문제

없음

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
