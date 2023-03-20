  ![](./media/image1.png)

#   SimpleCrossGenMPSD Sample

*이 샘플은 Microsoft GDK(데스크톱) 및 GDKX(Xbox)(2020년 11월)와
호환됩니다.*

# 설명

이 샘플은 MPSD를 사용하여 교차 생성 게임과 단일 세대 게임 모두에 세션 및
매치메이킹을 구현하는 방법을 보여줍니다. 이 샘플에서는 MPSD 기능의 전체
범위를 보여 주지 않습니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정합니다.

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정합니다.

PC를 사용하는 경우 활성 솔루션 플랫폼을 Gaming.Desktop.x64로 설정합니다.

Xbox Series X|S 개발 키트에서 샘플의 Xbox One 버전을 실행하려면 활성
솔루션 플랫폼을 Gaming.Xbox.XboxOne.x64로 설정하세요. 또한
MicrosoftGame.config 파일에서 TargetDeviceFamily를 XboxOne으로 설정해야
합니다.

![](./media/image3.png)

*자세한 내용은 GDK 설명서에서* 샘플 실행을 *참조하세요.*

# 샘플 사용

이 샘플은 단일 세대 및 교차 세대 시나리오 모두에서 MPSD를 사용하여
세션을 관리하는 방법에 대한 단순화된 데모를 보여줍니다.

**게임 세션 호스트** - 진행 중인 초대 및 참여를 지원하는 간단한 게임
세션을 호스트합니다.

**매치 메이킹 시작** -- 로비 세션을 생성하고 매치 메이킹 프로세스를
시작합니다.

**게임 세션 호스트(CrossGen)** - 세대 간 초대를 지원하고 진행 중인
참여를 지원하는 간단한 게임 세션을 호스트합니다.

## 

**매치 메이킹 시작(CrossGen)** - 로비 세션을 생성하고 세대 간 매치
메이킹 프로세스를 시작합니다.

**매치 메이킹 취소** -- 매치 메이킹을 취소하고 활성 로비 세션을
종료합니다.

## 

**세션 나가기** -- 활성 게임 세션을 나갑니다.

**친구 초대** -- 사용자의 친구 목록에 있는 플레이어에게 초대를 보내기
위한 셸 UI를 엽니다.

## 

## 메인 화면

![Graphical user interface, text, website Description automatically generated](./media/image4.png)

# 구현 참고 사항

MPSD 사용량은 모두 SessionManager.h/.cpp에서 찾을 수 있습니다. 여기에서
다음과 같은 데모를 찾을 수 있습니다.

-   세션 생성, 참여 및 나가기

-   매치 메이킹 시작 및 취소

-   MPSD 구독 및 이벤트 관리

-   초대 보내기

-   활동 관리

자세한 API 참고 사항 및 사용법은 MPSD 문서를 참조하세요.

# 세션 템플릿

이것은 4개의 세션 템플릿에도 동일하게 사용됩니다. GameSession,
GameSessionCrossGen, LobbySession 및 LobbySessionCrossGen. 이러한 세션
간의 핵심 차이점은 crossPlay 기능에 대해 설정한 값입니다. 교차 생성을
지원하는 세션의 경우 crossPlay 기능은 true로 설정되고 단일 생성과 함께
사용하기 위한 세션에 대해서는 false로 설정됩니다.

![Text Description automatically generated](./media/image5.png)

# 업데이트 기록

2021년 2월 - 2021년 2월 초기 릴리스

# 개인정보 처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
