  ![](./media/image1.png)

#   Desktop Unity GDK 럼블 샘플

*\* 이 샘플은 Unity 2020.3.12f1 및 Win 10의 Microsoft GDK Feb QFE2
2021을 사용하여 개발되었습니다.*

# 

# 설명

이 샘플은 다음과 같은 Microsoft GDK 다운로드 포털 페이지에 일반적으로
포함되는 NetRumble 샘플의 포트입니다.

![A picture containing text Description automatically generated](./media/image3.png)

개발자가 다음 기능을 수행하는 데 사용할 Xbox Live, PlayFab 및 PlayFab
파티 API를 보여주는 간단한 멀티 플레이어 게임입니다.

-   Xbox Live 서비스에 로그인 및 PlayFab 서비스

-   Xbox Live 친구 프로필 정보 검색

-   MPM을 사용하여 Xbox Live 멀티 플레이어 세션 시작

-   Xbox Live 친구를 멀티 플레이어 세션에 초대

-   친구의 Xbox Live 멀티 플레이어 세션에 참여

-   Xbox Live 사용자를 SmartMatch 멀티 플레이어 세션으로 연결

-   음성 채팅을 지원하는 파티 네트워크에 입력

-   안정적이고 신뢰할 수 없는 메시징을 사용하여 게임 플레이에 참여

# 

# 샘플 빌드

이 샘플은 2021년 2월 Microsoft GDK 설치 및 Unity 버전 2020.3.12f1에 대해
개발 및 테스트되었습니다. 샘플은 샘플의 필수 SDK 종속성을 충족하기 위해
샘플의 Assets/ 폴더에 압축을 풀 수 있는 ZIP 파일(SDKs/ 폴더에 있음)에
제공된 스냅샷이 있는 여러 SDK에 따라 다릅니다. 샘플은
Assets/Sample/Scenes 폴더에 있는 \"SampleScene.unity\"라는 하나의
장면으로만 구성됩니다.

샘플을 빌드하는 데 필요한 모든 Unity, Xbox Live 및 PlayFab 구성은 모든
SDK의 압축을 풀고 샘플을 Unity에 로드하면 이미 설정되어 있어야 합니다.
샘플은 \"빌드\" 버튼을 사용하여 Unity IDE 내에서 빌드해야 합니다.

![Graphical user interface Description automatically generated](./media/image4.png)

\... 샘플은 현재 \"Builds\" 폴더를 제공하며 빌드 출력이 다음과 같이
\"Builds/PC/Loose\"라는 폴더에 들어갈 것으로 예상합니다.

![Graphical user interface Description automatically generated with medium confidence](./media/image5.png)

\"MicrosoftGame.config\" 파일과 5개의 PNG 파일은 GDK 응용 프로그램에
필요한 지원 파일을 제공하는 \"GdkMetadata\" 폴더에서 찾을 수 있습니다.

Unity가 빌드 프로세스를 완료하고 메타데이터 파일이 빌드 출력 폴더에
복사되면 두 가지 방법으로 \"사이드 로드\"할 수 있습니다. 애플리케이션을
Windows로 전환합니다.

1.  \"Gaming VS 2019 명령 프롬프트\"를 사용하면 샘플 루트 폴더에서
    \"wdapp register Builds\\PC\\Loose\" 명령을 실행할 수 있습니다.

2.  \"Gaming VS 2019 명령 프롬프트\"를 사용하여 \"Builds\\package.bat\"
    스크립트를 실행하여 \"wdapp install
    Builds\\PC\\Package\\\<name_of_app_identity\>.MSIXVC 후속 명령을
    사용하여 설치된 패키지 빌드를 만들 수 있습니다.

# 샘플 실행

개발 Windows 10 PC에 앱을 성공적으로 빌드, 패키징 및 사이드 로드하는 데
성공했다면 Windows 10 시작 메뉴에 \"UnityRumbleGDK\"라는 앱 아이콘이
표시되어야 합니다. 앱 아이콘을 클릭하면 샘플이 크기를 조정할 수 없는
1920x1080 HD 창으로 실행됩니다.

성공적으로 실행하려면 타이틀이 구성 및 배포된 Xbox Live 샌드박스
\"XDKS.1\"(또는 자체 샌드박스)에서 PC가 실행되고 있어야 합니다. Gaming
VS 2019 명령 프롬프트에서 실행되는 \"XBLPCSandbox\"라는 GDK 도구는 활성
샌드박스 전환을 지원할 수 있습니다. 샌드박스에 로그인할 수 있는 테스트
사용자 배치도 필요합니다.

**파트너 센터의 UnityRumbleGDK 게임 개요**

![A screenshot of a computer Description automatically generated](./media/image6.png)

위의 스크린샷은 파트너 센터 UnityRumbleGDK 게임 타이틀의 현재 상태를
보여줍니다. 타이틀이 완전히 구성, 패키징, 배포 및 XDKS.1 샌드박스에
게시되었습니다. 타이틀에 대해 구성된 총 3개의 멀티 플레이어 세션
템플릿과 하나의 SmartMatch 호퍼가 있음을 알 수 있습니다.

**멀티 플레이어 세션 템플릿**

![Text Description automatically generated](./media/image7.png)

**LobbySessionTemplate**

{

\"constants\": {

\"system\": {

\"version\": 1,

\"maxMembersCount\": 5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": true

},

\"memberInitialization\": {

\"membersNeededToStart\": 1

}

},

\"custom\": {}

}

}

일반적인 멀티 플레이어 관리자 시나리오에서 자세한 내용은 GDK 설명서를
참조하세요. 대기실용, 호스트된 게임 플레이용, 매치 메이드 게임 플레이용
세션 템플릿 등 3개 이상의 세션 템플릿을 유지 관리합니다.

대기실 템플릿을 사용하여 대기실에 있을 수 있는 참가자 수를 5명으로
제한합니다. 초대 프로토콜은 초대가 게임 애플리케이션에 제공되어야 함을
지정합니다. 또한 구성원의 연결이 필요하며, 이는 RTA(실시간 활동)
서비스를 통해 관리됩니다.

또한 샘플이 검색 기능을 보여주지 않더라도 세션을 검색 가능하거나
공개하도록 선택했습니다. 마지막으로 대기실 세션을 시작하는 데 필요한
멤버 수는 정확히 1개이며, 이는 세션의 시작 호스트가 됩니다.

**GameSessionTemplate**

{

\"constants\": {

\"system\": {

\"version\": 1,

\"maxMembersCount\": 5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

}

},

\"custom\": {}

}

}

\<blah\>.

**MatchSessionTemplate 및 SmartMatch 호퍼**

{

\"constants\": {

\"system\": {

\"version\": 1,

\"maxMembersCount\": 5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

},

\"memberInitialization\": {

\"membersNeededToStart\": 2

}

},

\"custom\": {}

}

}

![A screenshot of a computer Description automatically generated with medium confidence](./media/image8.png)

\<blah\>.

다음 섹션에서는 샘플의 UI 화면을 화면별로 다루고 해당 화면이 올바르게
작동하는 GDK 개발 환경에서 어떻게 작동하는지 설명합니다.

## 샘플 시작 화면

![Graphical user interface, website Description automatically generated](./media/image9.png)

위의 스크린샷은 샘플이 시작될 때 예상되는 첫 번째 화면을 보여줍니다.
\"시작\" 버튼은 Xbox Live 및 PlayFab에 대한 사용자 로그인 흐름을
시작합니다. 로그인 단계 중 하나라도 실패하면 화면 하단에 실패 메시지가
표시되고 사용자는 시작 화면에 남아 있습니다.

일반적인 실패 상황은 사용자가 현재 샌드박스에 액세스할 수 없거나, 현재
인터넷 연결이 끊어졌거나, Xbox Live 서비스 또는 PlayFab 서비스에서
일종의 서비스 중단이 발생하는 것일 수 있습니다.

## 

## 샘플 주 메뉴 화면

![A screenshot of a computer Description automatically generated with low confidence](./media/image10.png)

테스트 사용자가 Xbox Live PlayFab 둘 다에 성공적으로 로그인되면 샘플의
주 메뉴 화면이 표시됩니다. 제시된 세 가지 주요 기능은 다음과 같습니다.

1.  SmartMatch를 사용하여 타이틀을 실행하는 다른 사용자와 내 사용자를
    일치시킵니다.

2.  사용자가 게임 대기실 세션을 호스트하도록 합니다.

3.  친구의 현재 활성 게임 대기실에 참여하거나 테스트 사용자가 이미 수신
    및 수락한 초대와 관련된 세션에 직접 참여를 시도합니다.

위의 세 가지 기능 중 하나를 수행하지 못하면 화면 하단에 표시되고
사용자는 주 메뉴 화면에 남게 됩니다.

## 

## 

## 친구 대기실 참여 화면

![A screenshot of a computer Description automatically generated with low confidence](./media/image11.png)

메인 메뉴에서 \"친구 참가\" 버튼을 선택하면 최대 3개의 친구 로비 버튼과
메인 메뉴로 돌아갈 수 있는 옵션이 표시됩니다. 현재 Unity Rumble 게임
대기실을 실행 중인 친구가 없으면 친구 대기실을 찾을 수 없다는 메시지와
함께 목록이 비어 있습니다.

## 

## 

## SmartMatch 화면 찾기

![A screenshot of a computer Description automatically generated with low confidence](./media/image12.png)

주 메뉴에서 \"매치 찾기\" 버튼 옵션을 선택하면 샘플이 Multiplayer
Manager API에서 관리하는 매치 메이킹 티켓을 통해 즉시 매치 메이킹을
시도합니다. 매치 메이킹 티켓은 일반적으로 두 가지 일반적인 결과 중
하나로 이어집니다.

1.  매치 메이킹 티켓이 충족되고 매치 메이킹 세션(앞서 제시한 매치 세션
    템플릿 사용)이 생성되고 매치 메이킹된 모든 구성원이 새 세션에
    배치됩니다.

2.  매치 메이킹 티켓이 처리되지 않아 시간이 초과되었습니다. 그 결과 화면
    하단에 오류가 표시되고 사용 가능한 주 메뉴 옵션이 다시 활성화됩니다.

중매 티켓은 활성 상태인 한 취소할 수도 있습니다. 매치 메이킹 티켓이
취소되면 매치 메이킹 요청이 실패한 것처럼 메인 메뉴 옵션이 다시
활성화됩니다.

## 게임 대기실 화면

![Graphical user interface, website Description automatically generated](./media/image13.png)

테스트 사용자가 다음 중 하나를 수행하면 성공적으로 매치 메이킹되거나
\"친구 참가\" 또는 \"초대 참가\" 기본 메뉴 옵션을 통해 친구의 게임
대기실에 성공적으로 참가하거나 자신의 세션 호스트를 성공적으로 시작하면
사용자에게 게임 대기실 화면이 표시됩니다.

UI 화면에는 대기실 세션에 있는 구성원이 표시됩니다. \"나가기\" 옵션을
사용하면 사용자가 대기실에 있는 것을 중단할 수 있으며 사용자가 선택하면
화면 왼쪽에 표시되는 대기실 구성원 목록에서 해당 구성원이 제거됩니다.

\"준비\" 버튼 위의 함선 및 색상 아이콘 버튼을 사용하면 게임을 플레이할
함선 스타일과 색상을 선택할 수 있으며 해당 선택 사항은 게이머태그 옆의
대기실 구성원 목록에 표시되는 것처럼 다른 모든 대기실 구성원과
동기화됩니다. 사용자 게이머태그 왼쪽에 있는 아이콘은 세션의 호스트와
\"준비\" 상태도 보여줍니다. 모든 구성원이 \"준비\" 버튼으로 준비 상태를
\"켜기\"로 전환하면 대기실이 화면 하단에서 시작 카운트다운을 시작합니다.

## 호스트 대기실에 친구 초대

![Graphical user interface, website Description automatically generated](./media/image14.png)

테스트 사용자가 \"게임 호스트\" 메인 메뉴 옵션을 통해 게임 대기실을
호스트하는 경우 게임 대기실 화면에 \"초대\" 버튼이 표시됩니다. \"초대\"
버튼을 선택하면 사용자에게 대기실 세션에 초대할 수 있는 친구 목록을
표시하는 셸 UI 화면이 표시됩니다.

배후에서 친구 선택 및 서버 측 초대 프로토콜 메커니즘을 처리하기 위해
초대 API가 호출됩니다. 초대가 취소되지 않으면 앱에 있든 없든 수신자
사용자는 초대를 받았으며 초대를 수락하거나 무시할 수 있다는 Shell UI
알림을 받게 됩니다. 수락되면 앱이 실행되고(아직 실행되지 않은 경우) 기본
메뉴에서 \"초대 참여\" 버튼이 활성화됩니다.

## 친구 수신 화면에서 초대

![A screenshot of a computer Description automatically generated with low confidence](./media/image15.png)

![A screenshot of a computer Description automatically generated with medium confidence](./media/image16.png)

앞서 언급했듯이 호스트 사용자의 초대를 수락하면 초대가 수락되면 기본
메뉴 화면에 \"초대 참가\"가 표시됩니다.

**대기실 전체 준비 화면**

![Graphical user interface, website Description automatically generated](./media/image17.png)

모든 대기실 구성원이 \"준비\" 토글을 통해 사용자를 \"준비\"하면 호스트가
모든 사람에게 게임을 시작할 준비가 되었다는 신호를 보내라는 메시지가
표시됩니다. 이때 세션 네트워크는 동기화에 사용되며 대기실 화면 하단에
카운트다운 타이머가 표시됩니다.

사용자가 앱을 종료하면 해당 구성원은 게임에서 제거되고 세션 네트워크에서
사라집니다. 카운트다운이 완료되면 사용자는 게임 플레이 화면으로
들어갑니다.

****

**게임 플레이 화면**

![Graphical user interface, application Description automatically generated](./media/image18.png)

게임 플레이 화면의 왼쪽에는 활성 참가자 멤버가 표시되며 여기에는 멤버
게이머태그 옆에 \"킬\" 및 \"데스\" 카운트가 포함됩니다. 호스트는 호스트
이름 왼쪽에 있는 Xbox 아이콘으로 표시됩니다. 사용자가 \"게임 끝내기\"
버튼을 통해 게임을 조기에 종료하면 다른 모든 회원 목록에서 사라집니다.

플레이어가 총 5명의 킬에 도달하면 게임이 자연스럽게 종료됩니다. 그
시점에서 플레이어가 게임을 종료하기로 선택한 것처럼 사용자는 주 메뉴로
돌아갑니다.

# 구현 참고 사항

\"Assets/Sample/Scripts\" 폴더 아래에 있는 샘플 스크립트 코드는 주로
UI/View 관련 코드와 핵심 논리로 나뉩니다. 핵심 논리 내에서 코드는 Xbox
Live/PlayFab/Networking 특정 논리를 게임 플레이 및 기타 일반 논리 비트와
분리하기 위해 더 세분화됩니다.

-   로그인, 친구, 멀티플레이어 등을 위한 Xbox Live 기능은
    Assets\\Sample\\Script\\Logic\\XboxLive에 있습니다.

-   로그인을 위한 PlayFab 기능은 Assets\\Sample\\Logic\\PlayFab에
    있습니다.

-   네트워킹 및 세션 문서에 대한 기능은
    Assets\\Sample\\Script\\Logic\\Session에 있습니다.

-   Unity의 GameCore 패키지를 사용하려면 거기에 있는 정의 대신
    \"USE_UNITY_GAMECORE\" 및 \"UNITY_GAMECORE\"를 정의해야 합니다.

# 중요!

SDKS\\ 폴더에 있는 샘플과 함께 제공되는 SDK 스냅샷은 순전히 샘플 실행의
편의를 위한 것이며 게시된 타이틀을 배송하는 데 적합한 \"공식\" SDK
버전으로 취급되어서는 **안 됩니다.** 항상 타이틀의 특정 SDK 버전에 대해
개발에 익숙한 최신 QFE를 사용하세요.

# 알려진 문제

정확히 \"알려진\" 문제는 아니지만 샘플의 \"SDKs\" 폴더 내에 제공된 SDK
스냅샷에 대해 샘플을 개발하고 테스트했습니다. Unity의 GameCore 패키지를
사용하여 최신 버전의 Microsoft Unity GDK 플러그인을 통합하거나 다른
버전의 PlayFab 또는 PlayFab 파티를 통합하는 경우 Unity 플러그인은 더
이상 사용되지 않거나 동작 특성이 변경된 API와의 비호환성을 노출할 수
있습니다.

# 

# 업데이트 기록

| 설명                        |  출시일             |  버전             |
|-----------------------------|--------------------|------------------|
| 샘플에 대한 README의 초기 초안. 빌드 요구 사항, 사용 세부 정보, 참고 사항 및 문제가 포함됩니다. |  2021년 3월 22일  |  1.0 |
| Unity 2020.3 LTS 및 2021년 2월 QFE2 GDK에서 실행되도록 샘플을 업데이트했습니다. |  2021년 6월 18일  |  1.1 |

# 

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
