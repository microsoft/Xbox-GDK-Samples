# 간단한 장치 및 스왑 체인 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

이 샘플은 Xbox 타이틀용 Direct3D 12장치 및 PresentX 스왑 체인을 생성하는 방법을 보여줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox One X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 사용

샘플에는 종료 외에는 컨트롤이 없습니다.

# 구현 참고 사항

Xbox 타이틀 Direct3D 설정은 다른 Microsoft 플랫폼과 매우 유사하지만, 이 샘플에서는 몇 가지 주요 차이점을 보여 줍니다.

- 표준 D3D12CreateDevice 대신** D3D12XboxCreateDevice** 사용

- Xbox Series X / Xbox One X의 경우 4k 사용, Xbox Series S의 경우 1440p 사용, Xbox One의 경우 1080p 사용

- 프레젠테이션에 DXGI를 사용하는 대신 새 **PresentX** API 사용

Direct3D 12 장치 만들기 및 스왑 체인의 모범 사례에 대한 자세한 내용은 [Direct3D 12 장치 만들기 분석](https://walbourn.github.io/anatomy-of-direct3d-12-create-device/)과 [최신 스왑 체인의 관리 및 공급](https://walbourn.github.io/care-and-feeding-of-modern-swapchains/)을 참조하세요.

루프 타이머 사용에 대한 자세한 내용은 [StepTimer](https://github.com/Microsoft/DirectXTK/wiki/StepTimer)를 참조하세요.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 취급 방침에 대한 자세한 내용을 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.

# 업데이트 내역

2018년 10월 -- Microsoft GDK 초기 버전

2019년 10월 -- 콘솔 검색을 위해 XSystemGetDeviceType으로 전환

2021년 10월 -- Xbox Series S에서 1440p를 사용하도록 업데이트

2022년 8월 -- 원본 이벤트를 대기할 위치에 대한 PresentX 모범 사례 개선.


