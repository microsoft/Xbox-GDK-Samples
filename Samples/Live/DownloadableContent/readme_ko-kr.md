  ![](./media/image1.png)

#   DLC(다운로드 가능한 콘텐츠) 샘플

*Windows PC: 이 샘플은 Microsoft GDK와 호환됩니다(2020년 6월).*

*Xbox One/Xbox Series X|S: 이 샘플은 Microsoft GDKX와 호환됩니다(2020년
6월).*

# 

# 설명

이 샘플에서는 XPackage 및 XStore API를 통해 다운로드 가능한 콘텐츠의
구매, 다운로드, 열거 및 로드를 구현하는 방법을 보여줍니다.

![ビデオゲームの画面のスクリーンショット 低い精度で自動的に生成された説明](./media/image3.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정합니다.

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정합니다.

Windows PC를 사용하는 경우 활성 솔루션 플랫폼을 Gaming.Desktop.x64로
설정합니다.

*자세한 내용은 GDK 설명서에서* 샘플 실행을 *참조하세요.*

# 샘플 실행

이 샘플은 XDKS.1 샌드박스에서 작동하도록 구성되었지만 라이선스
모드에서는 반드시 이 샌드박스에서 실행해야 하는 것은 아닙니다.

화면 왼쪽에는 설치된 패키지가 표시됩니다. 라이선스 확인을 포함하는
패키지를 탑재/탑재 해제할 수 있습니다. 패키지를 올바르게 탑재할 수 있는
경우 샘플은 패키지의 이미지를 표시합니다. 이 기능은 라이선스 없이
작동하지만 아래에 설명된 DLC 패키지를 로컬로 설치해야 합니다.

샘플이 Microsoft Store 제품을 사용할 자격이 있는 테스트 계정을 사용하여
XDKS.1에서 실행되는 경우(아래 참조) 오른쪽에 사용 가능한 지속성 추가
기능 목록이 표시됩니다. 항목을 선택하면 계정이 소유하지 않은 경우 구매
UI가 표시됩니다. 이 경우 항목을 선택하면 패키지가 다운로드됩니다.
완료되면 패키지가 왼쪽 목록에 표시됩니다. 이는 Microsoft Store에서 DLC를
구매하고 패키지가 CDN에서 설치되는 실제 소매 흐름을 가장 가깝게
나타냅니다.

| 작업                       |  키보드             |  게임 패드          |
|----------------------------|--------------------|--------------------|
| 패키지 선택  |  위쪽 및 아래쪽 화살표 키 |  위쪽 및 아래쪽 방향 패드          |
| 로컬 패키지 또는 저장소 패키지 간 전환 |  왼쪽 및 오른쪽 화살표 키 |  왼쪽 및 오른쪽 방향 패드          |
| 패키지 탑재 또는 탑재 해제(왼쪽 열) 패키지 구매 또는 다운로드(오른쪽 열) |  Enter 키  |  A 버튼 |
| XPackageEnumeratePackages 종류 및 범위 설정/해제 |  Page Up/Page Down 키 |  LB/RB |
| 열거된 패키지 새로 고침    |  Y                  |  Y 버튼             |
| 끝내기                     |  Esc                |  보기 버튼          |

XStore API가 작동하려면 유효한 라이선스가 필요하며 특정 구성 작업을
적용해야 합니다. 자세한 내용은 "**XStore 개발 및 테스트 사용**"이라는
GDK 설명서 섹션을 참조하세요.

이를 올바르게 수행하지 않는 경우 XStore API는 일반적으로 유효한
라이선스를 찾지 못했음을 나타내는 0x803f6107 (IAP_E\_UNEXPECTED)을
반환합니다.

# 제품 설정 방법

이 제품의 Microsoft Store ID는 9NQWJKKNHF1L입니다.

Microsoft Store 페이지에 도달하려면 게임 명령 프롬프트에서 다음을
사용합니다.

`xbapp launch ms-windows-store://pdp/?productid=9NQWJKKNHF1L`

또는 Windows에서 `ms-windows-store://pdp/?productid=9NQWJKKNHF1L`을
사용합니다.

![ロゴ が含まれている画像 自動的に生成された説明](./media/image4.jpeg)

이 문서 작성 시점을 기준으로, 9NQWJKKNHF1L에는 사용 가능한 플랫폼에 대한
패키지의 일반적인 조합을 나타내는 세 개의 추가 기능이 포함되어 있습니다.

-   9P96RFVJQ562에는 Xbox Series, Xbox One GDK 및 PC용 패키지가 포함되어
    있습니다.

-   9PPJJCWPCWW4에는 Xbox One ERA 패키지가 포함되어 있습니다.

-   9PGJRLSPSN3V에는 Xbox One GDK 패키지 및 PC가 포함되어 있습니다.

Scarlett 개발 키트에서 실행되는 샘플은 Scarlett DLC(9P96RFVJQ562)
패키지를 사용할 수 있어야 하며 Microsoft Store에서 설치된 패키지에는
\_xs 접미사가 있어야 합니다. Xbox One 개발 키트에서 실행되는 샘플은 세
가지 패키지 모두에 액세스할 수 있어야 하며 9P96RFVJQ562의 경우 패키지에
대신 \_x 접미사가 있습니다. PC에서 실행되는 샘플은 9P96RFVJQ562 및
9PGJRLSPSN3V 패키지에만 액세스할 수 있어야 합니다.

스토어에서 설치된 샘플은 올바른 라이선스가 제공되고 제대로 작동하지만
이전 버전의 샘플을 나타낼 수 있습니다.

# 로컬 패키지로 실행

이 샘플은 Microsoft Store에서 다운로드하여 설치한 DLC 패키지를 사용하여
실행할 수 있지만, 일반적인 개발에는 DLC 콘텐츠를 로컬로 반복하는 작업이
포함됩니다. 이를 수행하는 방법에는 몇 가지가 있습니다. 자세한 내용은
"**다운로드 가능한 콘텐츠 관리 및 라이선스 부여**"라는 GDK 설명서에서
확인할 수 있습니다.

샘플 및 DLC의 패키지된 버전을 생성하는 데 사용하는 여러 스크립트 파일도
동일하게 포함됩니다. 샘플(즉, 기본 게임)의 경우 makepcpkg,
makexboxonepkg 및 makecarlettpkg가 각각 해당 패키지를 만듭니다.
스크립트는 파트너 센터에서 9NQWJKKNHF1L에 대해 제출된 패키지와 연결된
올바른 contentID를 사용하여 패키지를 빌드합니다.

테스트용 로드 게임 패키지는 EKBID를 재정의해야 합니다.

`xbapp setekbid 41336MicrosoftATG.ATGDownloadableContent_2022.3.8.0_neutral\_\_dspnxghe87tn0 {00000000-0000-0000-0000-000000000001}`

Xbox One 및 Scarlett의 경우 올바른 **TargetDeviceFamily** 노드를
Gaming.Xbox.\*.x64\\Layout\\Image\\Loose\\MicrosoftGame.config에
삽입해야 하며, 그렇지 않으면 makepkg에서 오류가 발생합니다.

```xml
<ExecutableList>
    <Executable Name="DownloadableContent.exe"
        Id="Game"
        TargetDeviceFamily="Scarlett"/\>
</ExecutableList>
```


DLC의 경우 DLCPackage 디렉터리에 다음에 필요한 모든 파일이 포함됩니다.

1.  Scarlett GDK DLC(\_xs.xvc)

2.  Xbox One GDK DLC(\_x.xvc)

3.  Xbox One ERA DLC(확장명 없음)

Xbox에 적합합니다. DLCPackagePC에는 PC .msixvc에 필요한 파일이
포함됩니다.

각 플랫폼 내에는 각 플랫폼의 DLC 패키지를 생성하는 makedlcpkg 명령이
있습니다.

이를 통해 게임 및 DLC용 패키지 빌드를 만들 수 있습니다. 설치하려면
**xbapp install**(Xbox) 또는 **wdapp install**(PC)을 사용하거나 사용
가능한 동등한 도구를 사용합니다. 이 구성에서는 샘플 자체가 라이선스
모드에서 실행되지 않더라도 설치된 모든 DLC가 왼쪽에 표시되고 탑재
가능해야 합니다.

느슨한 파일을 사용하여 완전히 실행할 수도 있습니다. 이렇게 하려면
**xbapp deploy**(Xbox) 또는 **wdapp register**(PC)를 사용하고
MicrosoftGame.config가 있는 다음과 같은 디렉터리를 전달합니다.

`xbapp deploy .\\DLCPackage\\Package_Scarlett`

`wdapp register .\\DLCPackagePC\\Package`

패키지된 기본 게임 + 느슨한 DLC, 느슨한 기본 게임 + 패키지된 DLC, 느슨한
기본 게임 + Microsoft Store DLC 등과 같이 혼합해서 사용할 수 있어야
합니다. 단, 특정 조합과 관련된 문제는 알려진 문제 섹션을 참조하세요.

# 알려진 문제

# 업데이트 기록

**초기 릴리스:** 2019년 4월

**업데이트:** 2022년 3월

PC에서 DLC를 만드는 방법을 보여 주는 DLCPackagePC 폴더가 추가되었습니다.

라이선스 손실 시 발생하는 충돌이 해결되었습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
