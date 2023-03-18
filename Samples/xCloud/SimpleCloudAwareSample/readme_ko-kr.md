# 단순 클라우드 인식 샘플

*이 샘플은 Microsoft 게임 개발 키트(2021년 4월)와 호환 가능합니다.*

# 설명

이 샘플에서는 게임 스트리밍 클라이언트를 검색하고, 화면의 컨트롤
레이아웃을 변경하고, 터치 포인트를 읽는 방법을 보여 줍니다.

![A screenshot of a computer Description automatically generated with medium confidence](./media/image1.png)

# 샘플 사용

샘플을 시작하기 전에 게임 스트리밍이 사용하도록 설정되어 있는지
확인합니다. 스트리밍 구역의 개발자 홈에서 이 작업을 수행하거나 게임 명령
프롬프트에서 xbgamestream startlistening을 실행할 수 있습니다.

호환 가능한 클라이언트 앱(예: Xbox 게임 스트리밍 테스트 앱)을 사용하여
샘플을 실행하는 콘솔에 연결합니다. 연결되면 스트리밍 클라이언트의 현재
상태를 반영하도록 샘플을 변경해야 합니다.

샘플에는 다음과 같은 게임 명령 프롬프트에서 실행하여 로드할 수 있는 샘플
레이아웃이 포함되어 있습니다.

tak serve \--takx-file sample-layouts.takx

버튼을 눌러 표시되는 것을 확인하고 엄지스틱과 트리거를 이동하여 판독값을
확인합니다. A(또는 오버레이의 A에 해당하는 버튼)를 눌러 새 오버레이로
전환합니다. 클라이언트에서 터치를 사용하도록 설정한 경우 화면을 터치하여
터치포인트가 읽히는지 확인합니다.

# 구현 참고 사항

이 샘플에서는 xCloud에 클라우드 인식 API를 사용하는 방법을 보여 줍니다.

레이아웃 출처는 샘플 레이아웃 GitHub
<https://github.com/microsoft/xbox-game-streaming-tools/tree/master/touch-adaptation-kit/touch-adaptation-bundles>입니다.

# 버전 기록

2021년 5월: 시작 샘플

2022년 3월: 초기화 코드를 수정하도록 업데이트

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
