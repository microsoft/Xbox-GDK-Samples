  ![](./media/image1.png)

#   OfflineRT 샘플

*이 샘플은 Microsoft 게임 개발 키트(2021년 6월)와 호환됩니다.*

# 

# 설명

이 샘플은 Scarlett 플랫폼용 Microsoft 게임 개발 키트에서 사용할 수 있는
오프라인 가속 구조(BVH) 빌더 및 광선 추적 파이프라인 상태 개체(RTPSO)
직렬화 기능을 사용하는 방법을 보여줍니다.

타이틀은 이러한 기능을 기존 콘텐츠 및 셰이더 빌드 파이프라인에 통합하여
Xbox 네이티브 형식으로 자산 데이터를 생성하여 런타임 시 값비싼 셰이더
컴파일러 호출을 방지하고 BVH 품질을 높이고 스크래치 메모리 요구 사항을
줄일 수 있습니다.

# 샘플 빌드

샘플은 두 개의 개별 솔루션으로 구성됩니다. RTBuilder 및 OfflineRT.
RTBuilder는 오프라인 BVH 및 RTPSO를 구축하는 데 필요한 코드가 포함된 PC
애플리케이션이고 OfflineRT는 오프라인에서 생성된 데이터를 소비하고
시각화할 수 있는 Scarlett 애플리케이션입니다.

**이 생산자-소비자 관계 때문에 Scarlett에서 OfflineRT 샘플을 컴파일하고
실행하기 전에 RTBuilder 프로젝트를 컴파일하고 실행해야 합니다.** 그렇게
하지 않으면 컴파일 및 배포 오류가 발생합니다.

RTBuilder를 실행하면 다음과 같은 콘솔 출력이 표시됩니다.

![Text Description automatically generated](./media/image3.png)

도구가 완료되면 샘플의 Build 디렉터리에 다음과 같이 생성된 파일이
표시되어야 합니다.

![Graphical user interface, text, application Description automatically generated](./media/image4.png)

이제 Scarlett 개발 키트에서 OfflineRT 샘플을 컴파일하고 실행할 수
있습니다.

# 샘플 사용

![A picture containing text Description automatically generated](./media/image5.png)

샘플은 간단한 RTPSO를 사용하여 단일 모델을 레이트레이싱합니다.
게임패드를 사용하여 빨간색 런타임(.sdkmesh 파일)과 녹색 오프라인(.mdat
파일) 생성 모델 BVH 간에 전환하고 런타임, 오프라인 컬렉션 기반 및 완전
오프라인 RTPSO를 순환할 수 있습니다.

| 행동                                         |  게임패드              |
|----------------------------------------------|-----------------------|
| 스위치 모델(BVH)                             |  방향 패드 왼쪽/오른쪽 |
| 스위치 RTPSO                                 |  방향 패드 위/아래     |
| 궤도 카메라                                  |  오른쪽 엄지스틱       |
| 카메라 다시 설정                             |  오른쪽 엄지스틱 버튼  |
| 확대/축소/롤 카메라                          |  왼쪽 엄지스틱         |
| 종료                                         |  보기 버튼             |

샘플은 선택한 BVH 및 RTPSO에 대한 몇 가지 기본 통계를 보여줍니다.
오프라인에서 생성된 BVH는 메모리가 훨씬 작으며 런타임에 스크래치 공간도
필요하지 않습니다. 중요한 것은 또한 더 빠르게 추적할 수 있다는 것입니다.
오프라인 RTPSO의 경우 사실상 전체 생성 시간 비용을 빌더에게 전가할 수
있음을 알 수 있습니다.

# 구현 참고 사항

RTBuilder 애플리케이션은 PC 버전의 Scarlett 그래픽 드라이버(UMD)에 있는
기능을 활용하여 OBJ 모델 및 HLSL 셰이더 라이브러리를 Xbox 기본 BVH 및
RTPSO로 바꿉니다. 드라이버의 PC API는
%GXDKLatest%\\toolKit\\include\\Scarlett\\d3d12_xs.h에서 사용할 수
있으며 런타임에 필요한 바이너리는 %GXDKLatest%\\bin\\Scarlett에서 찾을
수 있습니다. 샘플은 사용자 지정 빌드 단계를 사용하여 출력 디렉터리에
이진 파일을 배포합니다.

오프라인 BVH는 런타임 BVH를 생성하는 데 사용하는 것과 동일한
ID3D12GraphicsCommandList6::BuildRaytracingAccelerationStructure API를
사용하여 생성되지만 이러한 작업은 CPU에서(호출하는 즉시) CPU에서
발생하는 것이 아니라 CPU에서 발생한다는 점에 유의해야 합니다.
ExecuteCommandLists 시간에 GPU. 동일한 라인을 따라 입력 및 출력으로
전달된 모든 D3D12_GPU_VIRTUAL_ADDRESS 매개 변수는 일반 CPU 가상 메모리
주소로 해석됩니다. 내부적으로 PC UMD는 BVH를 생성하기 위해 런타임에
드라이버에서 사용되는 GPU 컴퓨팅 기반 솔루션 대신 [Intel
Embree](https://www.embree.org/) 빌더를 활용합니다. Embree 빌더는
일반적으로 순회 시간에서 5% - 10%(모델 데이터에 따라 더 높을 수 있음)
속도를 높이는 고품질 BVH를 생성합니다.

런타임과 오프라인 빌드 BVH 간의 성능 차이는 이 샘플의 콘텐츠에서 작지만
일반적인 게임 콘텐츠는 더 큰 성능 승리를 보여줍니다. Intel Embree
오프라인 빌더는 \'삼각형 분할\' 기술을 사용하여 길고 가는 삼각형의 유효
표면적을 줄이는 반면 런타임 빌더는 그렇지 않습니다. 오프라인 빌더는 또한
런타임 빌더보다 더 높은 수준으로 쿼드 리프를 생성할 수 있기 때문에 더
작은(메모리별) BVH 구조를 생성합니다. 마지막으로, Xbox Series S 콘솔에
특히 유용할 수 있는 런타임 빌드에 스크래치 메모리가 필요하지 않습니다.

BVH 생성 후 구조는 복사 모드가
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_SERIALIZE인
CopyRaytracingAccelerationStructure API를 사용하여 직렬화되고 디스크에
기록됩니다.

OfflineRT 샘플(AddModelFromMDat 함수 참조)에서 가속 구조는 복사 모드가
D3D12_RAYTRACING_ACCELERATION_STRUCTURE_COPY_MODE_DESERIALIZE인
CopyRaytracingAccelerationStructure API를 사용하여 GPU에서 다시
역직렬화됩니다. 비동기 컴퓨팅 파이프에서 다른 그래픽 작업과 병렬로
역직렬화 작업을 수행하는 것이 좋습니다.

오프라인 RTPSO는 RTBuilder에서 세 단계로 생성됩니다.

-   Scarlett 셰이더 컴파일러(DXC)를 사용하여 DXIL 라이브러리(lib_6\_6
    대상)로 HLSL 컴파일

-   Scarlett PC UMD를 사용한 RTPSO 생성

-   Scarlett PC UMD를 사용한 RTPSO 직렬화

초기 컴파일은 Visual Studio의 *HLSL 컴파일러* 사용자 지정 도구에서
처리합니다. 프로젝트의 *Assets* 폴더에 있는 .hlsl 파일을 마우스 오른쪽
버튼으로 클릭하면 이 프로세스에 사용된 매개 변수를 볼 수 있습니다. 중간
DXIL 라이브러리는 샘플의 Build\\Int 디렉터리에 저장됩니다. 두 번째 및 세
번째 단계는 RTBuilder를 실행할 때 수행됩니다. 이 샘플은 컬렉션과 완전히
연결된 RTPSO 생성 및 직렬화를 모두 지원합니다. 두 경우 모두 PC UMD는
ShaderConfig, PipelineConfig 및 루트 서명이 알려진 경우
CreateStateObject가 호출될 때 Xbox 네이티브 셰이더를 완전히
컴파일합니다(DXR 사양의 [컬렉션 상태
개체](https://microsoft.github.io/DirectX-Specs/d3d/Raytracing.html#collection-state-object)
섹션 참조). 또한 주의해야 할 중요한 점은 직렬화
프로세스(SerializeStateObjectX 호출로 시작됨)가 상태 개체에서 모든 DXIL
및 기타 메타데이터를 제거하므로 런타임 하위 개체 연결이 불가능하다는
것입니다. 그러나 셰이더는 하위 개체가 아니기 때문에 셰이더 바인딩
테이블에 대한 셰이더 식별자를 찾는 것은 여전히 ​​가능합니다.

OfflineRT 샘플은 RTPSO를 구성하는 3가지 다른 방법을 보여줍니다.

-   DXIL 라이브러리에서 런타임 생성(AddPipelineFromEmbeddedDXILLib 함수
    참조): 이 방법은 전체 하위 개체 연결 동작을 허용하지만 런타임 시
    전체 컴파일 및 연결 비용이 발생하므로 유연성이 극대화됩니다.

-   오프라인 컬렉션을 기반으로 하는 런타임
    생성(AddPipelineFromSerializedCollections 참조): 모든 셰이더가
    완전히 사전 컴파일되고 내부적으로 연결되어 있으므로 상태 개체 생성이
    빠릅니다. 이 방법은 RTPSO가 여러(잠재적으로 공유되는) 컬렉션에서
    \"구성\"되도록 하여 유연성을 유지합니다.

-   런타임 역직렬화(AddPipelineFromSerializedRTPSO 함수 참조): 모든 것이
    미리 컴파일되고 연결되어 있으므로 상태 개체 생성이 빠릅니다. 진정한
    유연성이 없습니다.

# 알려진 문제

관련 Scarlett 그래픽 드라이버 버그:

-   버그 33668549: 제도법: XDXR은
    D3D12_STATE_SUBOBJECT_TYPE_EXISTING_COLLECTION을 통해 RTPSO에서
    참조되는 AddRef 컬렉션을 올바르게 추가하지 않습니다.

# 업데이트 기록

-   2021년 6월: 최초 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
