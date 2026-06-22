![](./media/image1.png)

# 데스크톱용 Unity 소셜 관리자

*이 샘플은 다음과 호환됩니다.*

- *Xbox 확장이 포함된 Microsoft 게임 개발 키트(2022년 3월) 이상*

- *Unity 편집기 2021.3.4f1 이상*

- *[GDK Unity 패키지](https://github.com/microsoft/gdk-unity-package)*

# 설명

데스크톱용 Unity 소셜 관리자 샘플은 Unity 게임 엔진을 사용하여 Xbox Live 소셜 관리자를 사용하는 방법을 보여줍니다. 다양한 수준의 현재 상태 및 관계를 기반으로 친구 그룹을 변경하여 Xbox 서비스 API에서 특정 사용자 그룹을 찾을 수 있습니다.

![그래픽 사용자 인터페이스, 자동으로 생성된 애플리케이션 설명](./media/image3.png)

# 주목할 만한 코드 파일

**XboxManager.cs**: 사용자 로그인 및 샌드박스 및 타이틀 ID와 같은 다양한 정보 쿼리와 함께 Xbox GDK 및 Xbox Live 서비스 API의 초기화를 포함합니다.

**XboxSocialManager.cs**: Xbox Live 소셜 관리자의 사용을 보여주는 API가 포함되어 있습니다.

# 샘플 빌드

**중요:** 샘플에는 [*GDK Unity 패키지*](https://github.com/microsoft/gdk-unity-package)가 **필요합니다**. 샘플을 성공적으로 빌드하려면 패키지에 제공된 '*GDK-API'* 및 '*GDK-Tools'*를 모두 포함해야 합니다. 포함된 '*GDK-Tools*'를 미리 구성된 '*MicrosoftGame.config*' 파일의 가져온 '*GDK-Tools'*에 결합합니다. '*Project'* 폴더는 아래 이미지를 미러링합니다.

![그래픽 사용자 인터페이스, 텍스트, 애플리케이션 설명이 자동으로 생성됨](./media/image4.png)

*Unity GDK*를 빌드하려면 Unity 빌드 메뉴 대신 *GDK Builder*를 사용해야 합니다. 작성기는 Unity 메뉴 모음의 *GDK -\> PC -\> 빌드 및 실행* 옵션을 통해 사용할 수 있습니다.

![그래픽 사용자 인터페이스, 텍스트, 애플리케이션 설명이 자동으로 생성됨](./media/image5.png)

*자세한 내용은* *GDK 설명서의*[ 샘플 실행을 참조하세요.](https://docs.microsoft.com/en-us/gaming/gdk/_content/gc/get-started-with-pc-dev/get-started-with-unity-pc/gdk-unity-end-to-end-guide)

# 샘플 실행

소셜 그룹 변경을 실행하고 사용자 상태를 검색하려면 Xbox Live 테스트 계정이 로그인되어 있어야 합니다.

데스크톱의 샌드박스는 XDKS.1로 **설정해야 합니다**.

**중요:** '검색한 Xbox 사용자' 섹션에서 결과를 보려면 친구 또는 즐겨찾기를 통해 로그인한 Xbox 사용자에 연결된 친구가 한 명 이상 있어야 합니다. 현재 상태 변경 또는 소셜 그룹 업데이트와 같은 일부 Xbox 서비스 이벤트에서 새로 고침이 자동으로 트리거됩니다.

*소셜 그룹 명령:*

- '*모든 친구'*

   - XblPresenceFilter = XblPresenceFilter.All

   - XblRelationshipFilter = XblRelationshipFilter.Friends

- '모든 즐겨찾기'

   - XblPresenceFilter = XblPresenceFilter.All

   - XblRelationshipFilter = XblRelationshipFilter.Favorite

- *'모든 온라인 친구'*

   - XblPresenceFilter = XblPresenceFilter.AllOnline

   - XblRelationshipFilter = XblRelationshipFilter.Friends

- *'타이틀 온라인 친구'*

   - XblPresenceFilter = XblPresenceFilter.TitleOnline

   - XblRelationshipFilter = XblRelationshipFilter.Friends

**중요:** 이러한 항목은 Xbox 서비스 현재 상태와 관계 필터의 미리 정의된 조합을 사용하여 호출하는 Xbox 사용자와 관련된 검색된 Xbox 사용자를 확인합니다.

*검색한 Xbox 사용자 명령:*

- '*게이머 태그 ... 상태'* -- 현재 소셜 그룹 설정의 소셜 기준을 충족하는 것으로 확인된 첫 사용자 5명의 게이머 태그와 연결 상태를 표시합니다. 선택하면 Xbox UI를 통해 사용자의 Xbox 프로필도 표시됩니다.

**중요:** 미리 정의된 필터에서 지정된 조건을 충족하는 사용자가 있는 경우에만 표시됩니다. 그렇지 않으면 공백으로 유지됩니다.

*추가 명령:*

- '*로그인'* -- 새 사용자가 Xbox 사용자 선택 UI를 사용하여 로그인할 수 있습니다.

- '*새로 고침' * -- Xbox 사용자의 사용자 목록을 수동으로 새로 고칩니다.

- '*로그 지우기'* -- 모든 기존 로그의 콘솔을 지웁니다.

- '*닫기' --* 샘플을 닫습니다.

# 알려진 문제

샘플은 이 문서의 패키지와 버전에 대해 개발 및 테스트되었습니다. 최신 버전의 GameCore 또는 Unity 편집기를 사용하면 빌드 오류와 비호환성이 발생할 수 있습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 취급 방침에 대한 자세한 내용을 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.

# 업데이트 내역
2023년 5월 18일 - 주목할 만한 코드 파일 추가


