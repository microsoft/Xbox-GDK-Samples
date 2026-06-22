![](./media/image1.png)

# AdvancedPSO 샘플

*이 샘플은 2022년 3월 GDK와 호환됩니다.*

# 설명

이 샘플에서는 주식 DirectX 12 메서드(CreateGraphicsPipelineState)를 대상으로 메모리 사용량 및 런타임 성능을 개선하는 Xbox 특정 PSO(파이프라인 상태 개체) 생성 방법을 보여 줍니다. 이 샘플은 PC에서 실행되는 PSOGen과 Xbox에서 실행되는 AdvancedPSO라고 하는 두 가지 Visual Studio 솔루션으로 구성됩니다.

PSOGen 솔루션은 오프라인 PSO 생성을 보여줍니다. 이것은 PC에서 실행되고 주어진 셰이더 조합에 대해 직렬화된 PSO 구성 요소 세트를 생성합니다. 소스 코드는 PSO 직렬화 API를 사용하는 방법을 보여줍니다. (SerializeGraphicsPipelineStateX)

AdvancedPSO 솔루션은 콘솔에서 실행됩니다. PSO 세트는 런타임 시 요청 시 로드되고 역직렬화됩니다(DeserializeGraphicsPipelineStateX). 집합은 중복 제거라는 프로세스를 사용하여 모든 조합에 필요한 최소 데이터 양입니다.

AdvancedPSO 샘플은 PSO *파생*도 보여줍니다. 이는 API(CreateDerivedGraphicsPipelineState)에서 정의한 특정 속성에 따라 다른 새 PSO를 만드는 매우 성능이 좋고 메모리 효율적인 방법입니다.

![샘플 스크린샷](./media/image3.png)

# 샘플 빌드

AdvancedPSO 프로젝트는 PSOGen에서 생성한 미리 컴파일된 PSO 마이크로코드를 사용합니다. 셰이더 컴파일러는 GDK 버전 간에 변경될 수 있으므로 출력도 변경됩니다. 이러한 이유로 AdvancedPSO 프로젝트에서는 사전 컴파일된 PSO와 빌드 환경 간의 GDK 버전이 일치해야 합니다.

샘플에서 버전 관리 실패를 감지하면 런타임 오류가 발생합니다. 다른 GDK에 대해 컴파일하는 경우 PSOGen 도구를 빌드하고 실행하여 역직렬화된 PSO를 업데이트한 다음 AdvancedPSO 샘플을 다시 실행하세요.

솔루션을 빌드하고 실행하면 Visual Studio에서는

- PSOGen 프로젝트를 빌드합니다.
- PC에서 PSOGen 프로젝트를 실행하여 오프라인 PSO를 생성합니다.
- AdvancedPSO 프로젝트를 빌드합니다.
- 콘솔에서 AdvancedPSO 실행 파일을 실행하여 오프라인 PSO를 사용합니다.

# 샘플 사용

샘플을 사용하려면 AdvancedPSO 샘플을 실행하기만 하면 됩니다. 화면에 각 PSO 생성 방법에 대한 메트릭이 표시됩니다. 다른 셰이더 조합을 생성하거나 셰이더 소스를 편집하려면 PSOGen 실행 파일을 다시 실행한 다음 AdvancedPSO 샘플을 실행합니다.

PC에서 PSOGen 프로젝트를 디버그하려면 아래와 같이 패스 환경 변수로 GDK 바이너리를 가리켜야 합니다.

![PSOGen 설정](./media/PSOGen-settings.png)

# 구현 참고 사항

PSOGen 프로젝트는 PC용으로 제작된 D3D12.x\[s\] UMD 드라이버, d3d12_x\[s\].h, .lib 및 .dll의 맞춤 버전과 연결되어야 합니다. 이는 각각 `<GDK root\>\bin\XboxOne` 또는 `<GDK root\>\bin\Scarlett`의 GDK 설치 디렉터리 내에서 찾을 수 있습니다. 이 라이브러리는 해당 디렉토리 내의 다른 여러 사용자 지정 라이브러리와 연결됩니다. **D3D12CreateDevice**가 성공적으로 반환하려면 경로에 있어야 합니다. PSOGen 프로젝트는 이러한 라이브러리를 프로젝트 디렉터리에 복제하는 사용자 지정 빌드 단계로 구성됩니다. Xbox One과 Xbox Series X\|S의
| | |
|---|---|
라이브러리 집합은 다릅니다.

| Xbox One | Scarlett(Xbox Series X\|S) |
|---|---|---|
| d3d12_x.dll | d3d12_xs.dll |
| xgs12_pc_x.dll | xgs12_pc_xs.dll |
| xg.dll | xg_xs.dll |
| dxcompiler_x.dll | dxcompiler_xs.dll |
| sc_dll.dll | xbsc_xs.dll |
| scdxil.dll | newbe_xs.dll |

미리 컴파일된 셰이더 Blob 및 직렬화된 파이프라인 상태 패킷은
컴파일된 PSO를 프로젝트 구성에 의해 설정된 플랫폼별 디렉터리로 출력한 다음 AdvancedPSO 프로젝트에 의해 해당 대상 플랫폼에 배포됩니다. PSOSet 클래스는 PSO 구성 요소 집합을 생성하고 로드하는 데 사용됩니다. 이 구현은 필요한 단계를 가장 먼저 보여주기 때문에 특히 최적이 아닙니다. 구성 요소를 개별 파일로 저장하며 실제로 일반적으로 Blob Storage에 보관됩니다. 또한 셰이더 조합을 미리 로드하거나 직렬화 또는 역직렬화에 다중 스레딩을 사용하려는 시도가 없습니다.
| | |
|---|---|
Xbox One과 Xbox Series X|S 간에 교환할 수 없습니다. PSOGen은|


샘플은 XMemAlloc 후크(Minitracker 클래스)를 사용하여 다양한 생성 방법의 드라이버 메모리 사용량을 측정합니다. 드라이버는 첫 번째 PSO가 생성될 때 추가 내부 저장소를 미리 할당합니다. 샘플에서는 ObjectId 특성을 사용하여 통계에서 이러한 할당을 제외합니다. ObjectId 속성의 의미는 공개 헤더에 노출되지 않으며 변경될 수 있습니다. 향후 GDK 릴리스에서 내보낼 수 있습니다.

# 업데이트 기록

2020년 12월 14일 &ndash; Xbox One XDK GDK로 이식했습니다. 2023/11/28 -- PSO를 자동으로 생성하도록 솔루션이 다시 구성됨


