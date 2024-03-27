![](./media/image1.png)

# 프랜차이즈 게임 허브 샘플

이 샘플은 Microsoft GDK(2023년 10월)와 호환됩니다

# 설명

프랜차이즈 허브는 사용자가 게임 간에 공유된 환경을 유지할 수 있도록 퍼블리셔가 관련 타이틀에 대해 큐레이팅한 구성을 제공하는 방법입니다. 프랜차이즈 허브는 관련 타이틀을 획득, 설치 및 시작할 수 있는 중앙 지점의 역할을 할 뿐만 아니라, 관련 타이틀에서 사용할 수 있는 영구 로컬 저장소 공간을 호스트합니다.

이 샘플에서는 사용자 환경에 필요한 다양한 작업을 구현하는 방법을 보여줍니다. 다음 세 가지 프로젝트로 구성됩니다.
- 관련 타이틀과 각 타이틀에서 할 수 있는 작업을 보여주는 게임 허브 제품(GameHub)
- 게임 허브를 시작해야 하고 허브에서 작성한 공유 데이터를 읽을 수 있는 허브 인식 제품(RequiredGame)
- 게임 허브를 시작할 필요가 없는 허브 비인식 제품(RelatedGame). 이는 여전히 게임 허브와 연결되기를 원하지만, 이전 GDK에서 제공되었기 때문에 게임 허브 특정 필드와 API를 활용할 수 없는 이전 타이틀을 나타냅니다.

![](./media/image3.png)

# 샘플 빌드

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Windows PC를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Desktop.x64`(으)로 설정합니다. 코드가 컴파일하지만, 프랜차이즈 게임 허브는 주로 콘솔 기능이며 일부 기능이 작동하지 않을 수 있습니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 실행

이 샘플은 XDKS.1 샌드박스에서 작동하도록 구성됩니다. 구매 및 설치 작업에 테스트 계정이 필요합니다. 모든 @xboxtest.com 테스트 계정이 XDKS.1 샌드박스에서 작동합니다.

![](./media/screenshot1.jpg)

맨 위 행은 게임 허브와 관련된 게임을 보여줍니다. 이 경우에는 두 가지가 있습니다. 각 타이틀을 선택하면 연결된 각 추가 기능이 아래에 나열됩니다. 각각을 구매할 수 있으며, 패키지가 있는 게임 또는 DLC의 경우 설치할 수 있습니다.

일단 설치되면 각 게임을 시작할 수 있습니다. 허브 인식 게임은 공유 PLS에 기록된 파일의 내용을 표시할 뿐만 아니라 허브로 다시 전환할 수 있습니다.

![](./media/screenshot2.jpg)


허브 비인식 게임은 시작만 할 수 있습니다. 그렇지 않으면 허브에서 온 것을 인식하지 못합니다.

이 모든 작업은 Visual Studio 디버깅을 통해 실행되는 게임 허브에서 수행할 수 있지만, 스크립트를 사용하여 각 프로젝트를 패키지로 만들 수 있습니다.
- `makepcpkg.cmd`
- `makescarlett.cmd`
- `makexboxone.cmd`

패키지를 만든 후 `xbapp install`을(를) 사용하여 각각 설치합니다. 이렇게 하면 Microsoft Store에서 가져와 설치할 필요 없이 로컬로 빌드한 패키지와 상호 작용할 수 있습니다.

이는 업데이트를 테스트하려고 할 때 특히 중요한데, 로컬 빌드(배포 또는 패키지)는 스토어 빌드로 업데이트할 수 없기 때문입니다. 이를 위해 UpdateTesting 디렉터리에서 별도의 스크립트 집합을 사용할 수 있습니다.
- `buildpackages.cmd`: 연결된 게임과 각각에 대한 DLC 모두에 v100 및 v200 버전의 패키지를 빌드합니다.
- `installandstageupdates.cmd`: 패키지의 `xbapp install` v100 및 각각의 `xbapp update /a` v200으로 업데이트의 가용성을 시뮬레이션합니다.

그 결과 게임에서 각각에 대해 업데이트를 사용할 수 있음을 반영하고 업데이트 흐름이 가능해야 합니다.

![](./media/screenshot3.jpg)


# 구현 참고 사항

`XPackageEnumeratePackages`은(는) `ThisAndRelated` 및 `ThisPublisher` 범위 모두에 동일한 결과를 반환하는 것처럼 보일 수 있습니다. 차이점을 확인하려면 XDKS.1 샌드박스에서 제공되는 다른 샘플 하나를 설치하세요. 예: InGameStore, DownloadableContent.

GameHub의 microsoftgame.config에 있는 `RelatedProduct` 노드를 사용하여 RelatedGame(허브 비인식)을 GameHub에 연결하는 방법.

해당 microsoftgame.config에 있는 GameHub의 `FranchiseGameHubId`과(와) RequiredGame의 `AssociatedFranchiseGameHubId`을(를) 일치시켜 RequiredGame (허브 인식)을 GameHub에 연결하는 방법.

주요 차이점은 허브 비인식 게임은 GameHub에서 참조되는 타이틀로 다시 게시할 필요가 없지만 허브 인식 게임은 처음부터 프랜차이즈 게임 허브 시나리오로 만들어진다는 점입니다. 그래서 허브 인식 게임은 titleId가 다시 `XLaunchUri`이(가) 가능하다는 것을 알기 때문에 GameHub로 돌아갈 수 있습니다.

2023년 10월 복구에서 제공되지 않는 UI 변경이 있을 예정이며, 이는 프랜차이즈 게임 허브의 작동 방식, 즉 허브 인식 게임이 내 게임에 표시되지 않고 게임 허브와 같이 사용할 때만 설치 및 실행할 수 있음을 보여줄 예정입니다.

게임 허브 및 관련 게임이 동일한 GDK로 빌드되어야 한다는 요구 사항은 없을 예정입니다. RelatedGame은 2023년 10월 이전 GDK를 사용하여 빌드하도록 설정할 수 있습니다. RequiredGame 및 GameHub는 2023년 10월에 새로 추가되는 API 및 microsoftgame.config 필드를 사용하므로 안 됩니다.

`XStoreEnumerateProductsQuery`이(가) 발생하는 경우 `XStoreProductsQueryHasMorePages`이(가) 호출되지 않는 것은 의도적인 것입니다. 첫째, 이 샘플에는 매우 적은 수의 제품이 포함됩니다. 둘째, 2023년 10월에는 `XStoreQueryAssociatedProducts` 또는 `XStoreQueryAssociatedProductsForStoreId`에 전달된 maxItems를 타이틀의 예상 제품 수를 포함하도록 확장하기만 하면 모든 제품이 Result 함수의 쿼리 핸들에 반환됩니다. 콜백은 모든 제품이 열거된 경우에만 적중되며, 많은 제품이 있는 타이틀에는 다소 시간이 걸릴 수 있습니다.

# 알려진 문제

모든 PC 기능을 프로토타입으로 간주합니다. 이 샘플은 콘솔용입니다.

허브가 오프라인일 때는 DLC를 구분할 수 없습니다(`XStoreQueryAssociatedProductsForStoreId`은(는) 오프라인에서 작동하지 않음).

`XStoreQueryGameAndDlcPackageUpdates` 또는 여러 ID로 전달된 `XStoreQueryPackageUpdates`은(는) `xbapp update` 스테이징을 사용할 때 사용 가능한 업데이트를 일관되게 반환하지 않습니다.

`XStoreDownloadAndInstallPackageUpdates`을(를) 통해 업데이트를 설치하고 `XPackageCreateInstallationMonitor`(으)로 모니터링하는 경우 `XPackageGetInstallationProgress`에서 `completed` = true를 조기에 반환합니다.

# 업데이트 기록

**초기 릴리스:** 2023년 10월

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


