# 간단한 공간 음향 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).*

# 설명

이 샘플은 ISpatialAudioClient를 사용하여 Windows Sonic 기술을 사용한 높이 채널 포함 정적 오디오를 재생하는 방법을 보여줍니다. 키보드 컨트롤을 사용하여 재생할 2개 파일을 선택하고 시작/정지, 일시 정지/재생 및 파일을 선택할 수 있습니다.

![](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

| 동작 | Controller |
|---|---|
| 시작/중지 | A button |
| 다음 파일 주기 | B 버튼 |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

이 샘플에서는 ISpatialAudioClient로 공간 기술을 사용하여 정적 침대 사운드를 재생하는 방법을 보여줍니다. ISpatialAudioClient가 초기화 및 시작되면 콜백을 사용하여 버퍼 프레임을 요청합니다.

# 업데이트 기록

2019년 3월 초기 릴리스


