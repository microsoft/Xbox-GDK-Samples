# 단순 MSAA

*이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).*

![](./media/image1.png)

# 설명

이 샘플은 MSAA 렌더링 대상 및 DirectX 12를 사용한 3D 씬의 농도/스텐실 버퍼를 구현합니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을 참조하세요.__

# 샘플 사용

| 동작 | Gamepad |
|---|---|
| MSAA 및 단일 샘플 토글 | A button |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

UI는 MSAA 없이 그려지며 MSAA 스왑 체인의 암시적 해결에 의존하지 않고 명시적 해결을 사용합니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


