![](./media/image1.png)

# SystemInfo 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2020년 6월)와 호환 가능합니다.*

# 설명

이 샘플은 시스템 정보 및 하드웨어 기능 쿼리를 위한 몇 가지 API를 보여줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

Windows 10 2019년 5월 업데이트(버전 1903; 빌드 18362) 릴리스 이상의 PC를 사용하는 경우 활성 솔루션 플랫폼을 Gaming.Deskop.x64로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행__을 참조하세요.&nbsp;

# 샘플 사용

이 샘플에서는 기술 정보가 포함된 일련의 텍스트 페이지를 표시합니다.

![C:\\temp\\xbox_screenshot.png](./media/image3.png)

게임 패드 컨트롤러를 사용하여 페이지 사이를 전환하려면 A 또는 방향패드 오른쪽/B 또는 방향패드 왼쪽을 사용합니다.

키보드의 경우 왼쪽 또는 Enter/오른쪽 또는 백스페이스 키를 사용합니다.

# 구현 참고 사항

중요한 코드는 **Render** 함수 내의 switch 케이스에 있습니다.

# 업데이트 기록

2018년 10월: 초기 GDK 릴리스

2020년 4월 &ndash; Gaming.Desktop.x64를 지원하도록 업데이트됨

2020년 6월 &ndash; GetLogicalProcessorInformation /Get LogicalProcessorInformationEx 사용이 추가됨

2021년 10월 -- Windows 11 DirectX 12 업데이트용 업데이트(22000).

2022년 9월 -- Windows 11 버전 22H2용 업데이트(22621)

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


