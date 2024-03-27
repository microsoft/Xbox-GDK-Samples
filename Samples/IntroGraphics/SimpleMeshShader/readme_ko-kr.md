![](./media/image1.png)

# 단순 Mesh 셰이더 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월) 및 Windows 10(버전 2004) 2020년 5월 업데이트와 호환됩니다.*

# 설명

이 샘플은 Mesh 셰이더를 사용한다는 점을 제외하고 적절히 오래된 샘플 SimpleTriangle의 컴패니언입니다. 그 목적은 Mesh 셰이더 파이프 라인을 얻는 데 필요한 모든 조각을 간단히 보여주는 것입니다.
| | |
|---|---|
|PC와 Xbox Series X|S 모두에서 초기화되고 실행됩니다.|

참고: Xbox One 본체 제품군에는 Mesh 셰이더 지원이 없으므로 해당 플랫폼에 사용할 수 있는 빌드 구성이 없습니다.

![](./media/image3.png)

# 샘플 빌드

Xbox Series X|S 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

적절한 하드웨어 및 Windows 10 릴리스가 설치된 PC를 사용하는 경우 활성 솔루션 플랫폼을 Gaming.Deskop.x64로 설정합니다.

이 샘플은 Xbox One을 지원하지 않습니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 참조하세요.*

# 샘플 사용

| 동작 | Gamepad |
|---|---|
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

이 샘플에서 보여 준 단계는 다음과 같습니다.

1. DirectX12로 렌더링할 ID3D12Device를 초기화하고 API 개체를 요청하세요.

2. ID3D12Device::CheckFeatureSupport() 함수를 사용하여 Mesh 셰이더 기능 지원을 확인합니다.

3. ID3D12Device2::CreatePipelineState() 함수를 사용하여 Mesh 셰이더 파이프라인을 만듭니다.

4. 루트 서명, 파이프라인 상태 및 리소스를 명령 목록에 바인딩합니다. 필수 매개 변수와 ID3D12GraphicsCommandList6::D ispatchMesh() 함수를 사용하여 Mesh 바인딩 파이프라인을 디스패치합니다.

# 업데이트 기록

2019년 10월 31일 -- 샘플 만들기.

2020년 4월 28일 - 메시 셰이더 파이프라인 생성에 D3DX12 도우미를 사용하도록 업데이트됨

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


