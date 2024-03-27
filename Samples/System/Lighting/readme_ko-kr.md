![](./media/image1.png)

# LampArray 샘플

*이 샘플은 Microsoft 게임 개발 키트(2023년 3월 QFE1)와 호환됩니다.*

# 설명

이 샘플에서는 LampArray API를 사용하여 키보드 및 마우스와 같은 RGB 디바이스에서 조명을 작동하는 방법을 보여 줍니다.

> **참고:** 2023년 3월 QFE1 릴리스를 기준으로 GDK LampArray API는 콘솔에서 다음 디바이스만 지원합니다. 추가 디바이스에 대한 지원은 향후 복구 릴리스에서 추가될 예정입니다.
> - Xbox One용 Razer Turret(키보드 및 마우스)
> - Razer BlackWidow 토너먼트 에디션 Chroma V2

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우에는 활성 솔루션 플랫폼을 Gaming.Xbox.XboxOne.x64로 설정합니다.

| | |
|---|---|
| Xbox Series X를 사용하는 경우 | S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 Gaming.Xbox.Scarlett.x64로 설정합니다. |

*자세한 내용은* *GDK 설명서의*__ 샘플 실행을 참조하세요.__

# 샘플 사용

호환되는 디바이스가 연결되어 있는지 확인합니다.  키보드 화살표 키 또는 게임 패드의 DPad를 사용하여 샘플 효과 사이를 이동합니다.

<Esc> 키 또는 보기 버튼을 눌러 종료합니다.

# 구현 참고 사항

효과 구현은 `LightingEffects.cpp` 파일에서 확인할 수 있습니다.  콜백 및 기타 기능은 `Lighting.cpp` 파일에 있습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


