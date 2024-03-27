![](./media/image1.png)

# NLS 및 지역화 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).*

# 설명

이 샘플에서는 MicrosoftGame.Config에서 참조되는 문자열 및 자산을 지역화하는 방법과 지역화된 게임 내 리소스에 제대로 액세스하는 방법을 보여 줍니다. 몇 가지 일반적인 NLS(국가별 언어 지원) API도 보여 줍니다.

## 메인 화면

![](./media/image3.png)

*이 이미지에서 콘솔 사용자의 언어 설정은 es-AR입니다.*

| 동작 | Gamepad | 키보드 |
|---|---|---|
| 실행할 단추 선택 | 방향 패드 위/아래 | 화살표 키/마우스 |
| 버튼 누르기 | A 버튼 | Enter/왼쪽 클릭 |
| 끝내기 | 보기 버튼 | Esc |

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

Windows 10을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Desktop.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 구현 참고 사항

# 이 샘플에서는 타이틀 지역화의 기본 사항을 보여 줍니다. 샘플에서 사용되는 리소스 파서는 매우 기본적이며 오류 검사를 제공하지 않습니다. 게임에서 이 설정을 따르려는 경우 이 리소스 파서를 직접 복사해서는 안 되지만, 대신 이 컨텍스트에서 더 완전한 기능을 갖춘 리소스 파서가 어떻게 작동할지의 예제로 보아야 합니다. 샘플의 목표는 개발자가 리소스 지역화 절차를 숙지하도록 돕는 것입니다.

**현재 지역화 설정 열거**

# 응용 프로그램이 시작되면 몇 가지 일반적인 NLS API의 출력이 표시됩니다. "현재 지역화 설정 열거" 단추를 눌러 수동으로 다시 표시할 수 있습니다.

GetUserDefaultLocaleName API는 콘솔이 설정된 로캘을 검색하고 GetUserDefaultGeoName API는 콘솔이 설정된 위치를 검색합니다. XPackageGetUserLocale API는 패키지 로캘과 가장 일치하는 사용자 로캘을 검색합니다. 결과는 LCID로 변환할 수 있습니다.

**시스템 설정 기반 언어 선택**

샘플에서는 해당 응용 프로그램 언어 설정에 따라 게임 내 이미지 및 텍스트를 변경합니다. 샘플의 기본 언어는 XPackageGetUserLocale API에 의해 결정됩니다. XPackageGetUserLocale API는 사용자 시스템에서 타이틀에 가장 적합한 로캘을 결정하기 때문에 로캘 선택에 대한 게임의 진실 공급원이어야 합니다. 로캘은 콘솔 및 사용자 설정을 포함하여 사용 가능한 모든 데이터와 타이틀에서 지원하는 내용을 기반으로 합니다. 결과는 언제나 게임 구성에서 선언된 언어거나, 선언된 언어가 없다면 사용자 언어가 됩니다. 이 로캘은 런타임 시 샘플에서 표시할 적절한 지역화된 이미지 및 텍스트를 선택하는 데 사용됩니다.

샘플에서 7개의 로캘이 MicrosoftGame.config에 정의됩니다.

- 

- en-US

- en-GB

- zh-Hans-CN

- zh-Hant

- ja-JP

- es

- fr

이러한 로캘은 일반적인 대체 시나리오를 보여 주도록 선택됩니다. 대체는 콘솔 언어 및 위치 설정이 게임에서 지원하는 언어와 정확히 일치하지 않고 사용 가능한 옵션 중에서 선택해야 하는 경우에 발생합니다.

예를 들어 콘솔 언어가 "en-CA"인 경우 "en-GB"로 대체됩니다. 또 다른 시나리오는 "fr"입니다. 여기 이 샘플에서는 지역 없이 "fr"만 정의됩니다. 이 경우 콘솔 언어가 프랑스어인 경우 콘솔 언어 지역이 어디든 상관없이 "fr"로 대체됩니다.

콘솔 설정은 언어, 언어 지역 및 위치 간에 지역화 설정을 나눕니다. Xbox One 관리자에서 기본 설정 언어는 처음 두 가지를 병합하고 지리적 지역은 위치에 해당합니다. 이 샘플의 경우 언어 설정만 언어 선택에 영향을 미칩니다. 위치/지리적 지역은 GetUserDefaultGeoName에만 영향을 줍니다.

**MicrosoftGame.Config 문자열 지역화**

샘플의 표시 이름 및 설명은 콘솔 언어를 기준으로 지역화되기도 합니다. MicrosoftGame.Config에서 이러한 필드의 지역화를 지원하려면 다음과 같이 ms-resource 참조로 설정합니다.

OverrideDisplayName=\"ms-resource:ApplicationDisplayName\"

그런 다음 이러한 값은 해당 로캘 아래의 Resources.resw 파일에 있는 값에 따라 채워집니다. 예를 들어 콘솔 언어가 일본어(ja), 지역은 일본(JP)으로 설정된 경우 프로젝트의 ja-JP 폴더에 있는 Resources.resw 파일에서 표시 이름을 가져옵니다. 이 샘플의 경우 문자열 "NLS 및 지역화(ja-JP)"가 됩니다. 기본적으로 Resources.resw 파일 및 각 언어 폴더는 루트 프로젝트 폴더에 있어야 합니다. Resources.resw 파일이 이 샘플에서 사용하는 "문자열" 디렉터리와 같은 다른 위치에 있는 경우(예: ProjectFolder\\Strings\\Resources.resw, ProjectFolder\\Strings\\ja-JP\\Resources.resw 등 ) 프로젝트 속성 페이지의 "패키지 지역화 디렉터리" 속성에 폴더를 지정해야 합니다.

**MicrosoftGame.Config 패키지 이미지 지역화**

샘플의 패키지 이미지는 콘솔 언어에 따라 지역화할 수도 있습니다. 먼저 기본 이미지의 경로는 MicrosoftGame.config에서 평소처럼 지정됩니다. 아래 코드 조각은 "Images" 디렉터리 아래의 샘플에서 기본 이미지가 있는 위치를 나타냅니다.

StoreLogo=\"Images\\StoreLogo.png\"

Square480x480Logo=\"Images\\Square480x480Logo.png\"

Square150x150Logo=\"Images\\Square150x150Logo.png\"

Square44x44Logo=\"Images\\Square44x44Logo.png\"

SplashScreenImage=\"Images\\SplashScreen.png\"

그런 다음 기본 이미지와 동일한 디렉터리에서 MicrosoftGame.config에 정의된 각 언어의 하위 디렉터리에 지역화된 변형이 포함됩니다.(예: ProjectFolder\\Images\\ja-JP\\StoreLogo.png, ProjectFolder\\Images\\ja-JP\\Square480x480Logo.png 등). 모든 언어에 대해 먼저 해당 언어 폴더에서 이미지 파일이 있는지 확인되며, 파일이 있으면 이 파일이 사용됩니다. 언어 폴더에 이미지가 없거나 언어가 지원되지 않으면(즉, 언어 폴더가 없으면) 기본 로고로 대체됩니다. 샘플이 빌드될 때 각 로고가 Loose 폴더에 올바르게 복사되었는지 확인하세요.

MicrosoftGame.config 설정에 해당하는 제품이 실제로 게시되고 해당 샌드박스(또는 일반 정품)의 계정이 로그인된 경우, 문자열 및 이미지는 파트너 센터 제품 페이지에 구성된 동등한 필드에 의해 재정의될 수 있습니다. 또한 파트너 센터의 게임 및 스토어 목록에서 지원하는 언어의 일대일 대응은 꼭 필요하지는 않지만 매우 유용합니다.

# 업데이트 기록

2020년 4월 초기 릴리스

2021년 6월 샘플은 UX 새로 고침을 비롯한 추가 NLS 기능을 지원하도록 업데이트되었습니다.

2021년 7월 MicrosoftGame.config가 업데이트되었습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


