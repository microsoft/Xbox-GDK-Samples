![](./media/image1.png)

# CMaskDecode 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

# 이 샘플은 cmask 서페이스에서 Fast Clear 값을 추출하는 방법을 보여줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 사용

![](./media/image2.jpeg)

| 동작 | Gamepad |
|---|---|
| MSAA 설정/해제 | A button |
| 하위 타일 표시 설정/해제 | B 버튼 |
| 뷰 회전 | 왼쪽 엄지스틱 |
| 보기 다시 설정 | 왼쪽 엄지스틱(클릭) |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

cmask 표면은 렌더링 대상과 연결된 메타데이터 표면입니다. 렌더링 대상의 각 8x8 타일에는 연결된 4비트 cmask 항목이 있습니다. cmask 항목에는 다음과 같은 정보가 포함됩니다.

• MSAA를 사용하지 않는 경우 cmask의 각 비트는 하나의 16 픽셀 하위 타일의 "지우기" 상태를 나타냅니다(하위 타일의 모양은 데이터 형식,
[CMask 디코딩](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/cmask-decoding)의 마이크로 타일링 모드에 따라 다름) • MSAA를 사용하도록 설정하면 cmask의 두 개의 상위 비트가 각각 1개의 32픽셀 하위 타일의 "지우기" 상태를 나타냅니다(정확한 하위 타일 셰이프의 경우 [CMask 디코딩](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/cmask-decoding) 설명서를 참조하세요). 두 개의 낮은 순서 비트에는 fmask 메타데이터가 포함됩니다.
| | |
|---|---|
|Xbox One 및 Xbox Series X|S의 스위즐링 모드에 대한 자세한 내용을 설명서를 참조하세요.


이 샘플에서는 cmask를 텍스처로 디코딩하는 방법과 빠른 지우기 비트를 해석하는 방법을 보여 줍니다. 이 정보는 기본 표면의 지우기 영역 검색을 가속화하는 데 사용할 수 있습니다. 한 응용 프로그램(여기에 설명되지 않음)은 대부분 비어 있는 텍스처의 빠른 지우기 제거를 건너뛰는 것일 수도 있습니다.

샘플은 디코딩된 빠른 지우기 정보를 기본 장면 위에 빨간색/녹색 오버레이로 표시합니다. 녹색 픽셀은 완전히 투명한 타일(또는 하위 타일)에 속합니다. 빨간색 픽셀은 부분적으로 기록된 타일(또는 하위 타일)에 속합니다.

이 샘플은 DirectX 12.X를 사용하여 구현됩니다. 루트 서명을 제거하는 경우 DirectX 11.X 구현에 동일한 셰이더(ColorDecompressUtility.hlsli / CMaskDecodeCS.hlsl)를 사용할 수 있습니다.

# 알려진 문제

이 샘플은 향후 변경될 수 있는 특정 드라이버 선택에 따라 달라집니다.

• cmask 표면이 타일 또는 선형으로 만들어지는지 여부

| | |
|---|---|
| • 어떠한 타일링 모드(Xbox One) 또는 스위즐링 모드(Xbox Series X | S)가 cmask 표면에 대해 선택되었는지 |

# 업데이트 기록

샘플의 원래 버전은 XSF 기반 프레임워크를 사용하여 작성되었습니다. 2월에 ATG 샘플 템플릿을 사용하도록 다시 작성됨
2020년 5월.
| | |
|---|---|
|2018년 Xbox One X에 대한 지원. Xbox Series X|S에 대한 지원이 추가됨|

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


