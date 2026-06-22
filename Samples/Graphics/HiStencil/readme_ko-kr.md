![](./media/image1.png)

# Hi-Stencil 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

Hi-stencil은 기본적으로 D3D12.X에서 꺼져 있습니다. 하지만 좋은 소식은 타이틀이 Hi-Stencil을 구성할 수 있다는 것입니다. 타이틀에서 설정할 수 있는 2가지 hi-stencil 비교 상태가 있습니다. 스텐실 작업은 샘플 속도로 실행하는 대신 타일 속도로 실행하여 성능 게인을 생성할 수 있습니다. 스텐실 테스트를 사용하는 모든 타이틀은 수동으로 hi-stencil을 설정하여 성능 향상을 볼 수 있습니다. 이 샘플에서는 hi-stencil API 사용량을 보여 줍니다.

또한 이 샘플에서는 컴퓨팅 셰이더를 사용하여 HTile 버퍼에서 hi-stencil 결과를 읽는 방법을 보여줍니다. 그런 다음 비동기 컴퓨팅을 사용하여 스텐실 패스를 실행할 수 있으며 전반적인 성능이 향상될 수 있습니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

에뮬레이션을 통해 devkit에서 실행됩니다. *GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*
| | |
|---|---|
|이 샘플은 htile의 변경으로 인해 Xbox Series X|S를 지원하지 않습니다. It|


# 샘플 사용

픽셀 셰이더를 실행하는 샘플:

![](./media/image3.png)

컴퓨팅 셰이더를 실행하는 샘플:

![](./media/image4.png)

| 동작 | Gamepad |
|---|---|
| Hi-stencil 켜기/끄기 토글 | A button |
| 컴퓨팅 셰이더 및 픽셀 셰이더 구현 토글 | X 버튼 |
| 다시 요약 설정/해제 토글 | Y 버튼 |
| HiStencilControlX API 토글 | B 버튼 |
| HiStencilControlX 토글을 재정의하도록 리소스 설정 | 왼쪽 트리거 버튼 |
| 외부 메시를 그리기 전에 HTile 값 수집 | 오른쪽 트리거 버튼 |
| ESRAM에서 리소스 토글 | 왼쪽 범퍼 |
| 디스패치 호출의 개수 표시 | 오른쪽 범퍼 |
| 도움말 | 메뉴 단추 |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

Hi-stencil은 기본적으로 D3D12.X에서 꺼져 있습니다. 타이틀(State0 및 State1)에서 설정할 수 있는 2가지 hi-stencil 비교 상태가 있습니다.

```cpp
typedef struct D3D12XBOX_HISTENCIL_COMPARE_STATE
{
    D3D12XBOX_HISTENCIL_COMPARE_FUNCTION CompareFunction : 4;
    UINT CompareValue : 8;
    UINT CompareMask : 12;
    BOOL Enabled : 8;
} D3D12XBOX_HISTENCIL_COMPARE_STATE;

typedef struct D3D12XBOX_HISTENCIL_CONTROL
{
    D3D12XBOX_HISTENCIL_COMPARE_STATE State0;
    D3D12XBOX_HISTENCIL_COMPARE_STATE State1;
} D3D12XBOX_HISTENCIL_CONTROL;
```


SetHiStencilStateX 및 SetHiStencilControlX API가 D3D12에 추가되었습니다. 이러한 스텐실 비교 상태를 설정하는 데 도움이 되는 X입니다.

- **SetHiStencilStateX**는 깊이 스텐실 리소스에 대해 hi-stencil compare 테스트를 설정하는 데 사용되며 바인딩될 때 지정된 리소스에서 유지됩니다.

- **SetHiStencilControlX**는 일시적으로 상태를 전역으로 설정하는 데 사용되며 연결된 hi-stencil 상태의 깊이 스텐실이 파이프라인에 바인딩될 때까지 모든 스텐실 리소스에 사용됩니다.

```cpp
void D3DAPI SetHiStencilStateX(
    _In_ ID3D12Resource* pResource,
    _In_opt_ const D3D12XBOX_HISTENCIL_CONTROL* pControl);

void D3DAPI SetHiStencilControlX(
    _In_opt_ const D3D12XBOX_HISTENCIL_CONTROL* pControl);
```


비교 결과는 HTile 버퍼에 저장됩니다. 프레임의 뒷부분에서 스텐실 테스트와 상관 관계가 있는 이러한 테스트는 개별 샘플 스텐실 값을 읽는 대신 타일을 거부/수락하는 효과적인 방법이 될 수 있습니다.

## HTile 버퍼:

HTile 버퍼는 깊이 스텐실 버퍼의 픽셀 8x8 블록마다 32비트 메타데이터를 저장합니다.

다음 상황에서 HTile 비트 해석

- 스텐실이 없습니다(이 샘플에서는 다루지 않음).

![](./media/image5.png)

- 스텐실이 있음

![](./media/image6.png)

- **SMem**: 스텐실 메모리 형식은 타일의 스텐실 값이 저장되는 방식을 나타내는 2비트 값입니다.

| SMem | 설명 |
|---|---|
| 0 | **Clear** -- 전체 타일에 Clear 값이 있습니다. |
| 1 | **Single Value** **--** 모든 타일이 하나의 스텐실 값을 갖습니다. 스텐실 버퍼 타일의 첫 8비트 데이터는 전체 타일의 값입니다. |
| 2 | **Expanded and Clear** -- 현재 XBox에서 사용되지 않습니다. 스텐실 버퍼의 이 타일에 대한 모든 샘플에는 명확한 값이 있습니다. |
| 3 | **Expanded** -- 타일이 확장되었으므로 스텐실 버퍼의 개별 샘플에 올바른 스텐실 값이 있습니다. |

- **SR\***: 계층 구조 스텐실 사전 테스트 결과입니다. HTile에 저장할 수 있는 2가지 독립적인 비교 결과(SR0 및 SR1)가 있습니다. API SetHiStencilStateX 및 SetHiStencilControlX를 사용하여 비교 함수, 마스크 및 비교 값을 설정할 수 있습니다. SR0 및 SR1에 각각 2비트가 있습니다.

   - **Bit 0**: 실패할 수 있음

   - **Bit 1**: 통과할 수 있음

> 다음 2비트에서 설정한 값의 해석:

| SR\* | 설명 |
|---|---|
| 0 | **지워지거나 비교되지 않음** |
| 1 | **May Fail** -- 하나 이상의 샘플이 hi-stencil 테스트에 실패했습니다. |
| 2 | **May Pass** -- 하나 이상의 샘플이 hi-stencil 테스트를 통과했습니다. |
| 3 | **May Pass 또는 May Fail** - 하나 이상의 샘플이 hi-stencil 테스트를 통과했으며 하나 이상의 샘플은 hi-stencil에 실패했습니다. 또한 하드웨어는 hi-stencil 상태 테스트가 꺼져 있거나 스텐실 패스가 ALWAYS 또는 NEVER이거나 비교 마스크가 0이거나 스텐실 쓰기가 있는 경우에도 이러한 비트를 설정할 수 있습니다. 따라서 이러한 경우 타일에 SR\*의 값이 3인 경우에도 모든 샘플이 실패할 수 있습니다. |

- **샘플의 Hi-Stencil 상태**

샘플에서 SetHiStencilStateX는 스텐실 값 COMPARE_VALUE 및 (COMPARE_VALUE+1)로 그려지는 디스크 2개에 대한 테스트를 통과하는 데 사용됩니다.

```cpp
hiStencilControl.State0.Enabled = TRUE;
hiStencilControl.State0.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
hiStencilControl.State0.CompareValue = COMPARE_VALUE;
hiStencilControl.State0.CompareMask = 0xFF;
```


SetHiStencilControlX를 사용하는 경우 두 번째 비교 상태를 설정하여 스텐실 값(COMPARE_VALUE_2 + 1)으로 그린 다른 디스크가 HiStencil 테스트를 통과하도록 합니다.

```cpp
hiStencilControl.State1.Enabled = TRUE;
hiStencilControl.State1.CompareFunction = D3D12XBOX_HISTENCIL_COMPARE_FUNCTION_LEQUAL;
hiStencilControl.State1.CompareValue = COMPARE_VALUE_2 + 1;
hiStencilControl.State1.CompareMask = 0xFF;
```

## 컴퓨팅 셰이더:

HTile 버퍼를 사용하여 최대 스텐실 사전 테스트 결과를 읽을 수 있으며, 이를 사용하여 컴퓨팅 셰이더에서 스텐실 테스트를 구현할 수 있습니다. 이는 일반적인 픽셀 셰이더 구현보다 느린 경향이 있지만 비동기 컴퓨팅에서 실행할 수 있으므로 그래픽 파이프에서 약간의 시간을 단축할 수 있다는 이점이 있습니다. 샘플에는 두 개의 컴퓨팅 셰이더가 사용됩니다.

- **HTile 값 해석:** 이 셰이더는 추가 버퍼를 사용하여 SR\*의 "MayPass" 비트가 설정된 타일 좌표를 저장합니다. 디버깅을 위해 셰이더는 "MayFail" 비트가 설정된 타일 수와 단일 값이 있는 타일을 전달할 수 있는 수(SMem =1)를 계산할 수도 있습니다.

- **확장된 스텐실 결과 결정:** 두 번째 컴퓨팅 셰이더는 위의 추가 버퍼의 값을 사용하여 스텐실을 전달하고 녹색 또는 파란색을 대상과 혼합한 샘플을 확인합니다. 단일 스텐실 값(SMem = 1)(혼합 출력과 녹색 색 혼합)이 있는 타일에 SGPR을 사용하고, 각 스레드가 스텐실 버퍼에서 별도의 값을 읽어야 하므로 타일이 확장되는 경우 VGPR을 사용합니다(혼합 출력은 파란색으로 혼합).

## 유용한 PIX 카운터:

| 카운터 | 설명 |
|---|---|
| DB_P ERF_SEL_DB_SC_S\_TILE_RATE | 타일 속도의 스텐실 작업입니다. 카운터는 Scorpio에서만 사용할 수 있습니다. |
| DB_PER F_SEL_DB_SC_TILE_TILE_RATE | 타일 속도로 깊이 및 스텐실 작업 위의 카운터(DB_PERF_SEL_DB_SC_S\_TILE_RATE)를 사용할 수 없으므로 Durango에서 사용할 수 있습니다. 깊이 비교가 꺼져 있으면 이 카운터는 스텐실 작업만 표시합니다. |
| DB _PERF_SEL_DB_SC_TILE_TILES | 총 타일 수(4K 대상의 경우 129,600개, 1080p의 경우 32,400개) |
| PreZSamplesFailingS | 샘플 속도에서 실패한 스텐실 작업 수 |
| PreZSamplesPassing | 깊이 작업이 꺼져 있는 경우 샘플 속도로 전달된 스텐실 작업 수입니다. 깊이 비교가 켜져 있는 경우 depth+스텐실이 포함됩니다. |

# 결과:

**픽셀 셰이더를 사용하여 스텐실을 통과할 때 픽셀을 쓰는 결과:**

| 카운터 | Scorpio -- 4K | Durango -- 1080p |
|---|---|---|---|---|
|  | Hi-S tencil 끄기 | Hi-S tencil 켜기 | Hi-S tencil 끄기 | Hi-St encil 켜기 |
| EOP에서 EOP까지의 기간 | 0.089ms | 0.062ms | 0.072ms | 0.055ms |
| \% Hi-Z 타일 거부됨 | 0 | 40% | 0 | 39.8% |
| PreZSamplesPassing(샘플 속도로 실행되는 스텐실 테스트) | 1,2 35,187 | 23,187 | 3 08,857 | 1 2,025 |
| DB_PERF_SEL_DB_SC_S\_TILE_RATE (Scorpio) 또는 DB_PERF_SEL_DB_SC_TILE_TILE_RATE (Durango)(타일 속도로 실행되는 스텐실 테스트) | 0 | 18,939 | 0 | 4,638 |
| 타일 속도로 실행되는 샘플 전달의 \% | 0 | 98.12% | 0 | 9 6.16% |

Hi-Stencil을 사용하는 경우 Scorpio는 **30%**의 게인을 표시하고 Durango는 **23%**의 게인을 표시합니다.

**스텐실이 통과하면 픽셀에 쓰기:**

| 카운터 | Scorpio -- 4K | Durango -- 1080p |
|---|---|---|---|---|
|  | PS | CS | PS | CS |
| 스텐실을 전달하는 샘플 쓰기 | 1,2 35,187 | 3 08,857 |
| 소요 시간(DRAM의 DSV+RTV) | 0.062ms | 0.088ms | 0.055ms | 0.103ms |
| 소요 시간(ESRAM의 DSV+RTV) | \- | \- | 0.027ms | 0.044ms |

대상이 ESRAM에 있는 경우 그리기 호출이 픽셀 속도로 바인딩될 수 있으므로 컴퓨팅 셰이더에서 소요된 시간이 픽셀 셰이더 타이밍에 더 가까울 수 있습니다. DRAM에서 호출은 DRAM 대역폭에 의해 바인딩됩니다. 위에서 설명한 것처럼 컴퓨팅 셰이더를 실행하는 데 더 많은 시간이 걸리지만 비동기에서 실행하면 전체 프레임 시간이 단축될 수 있습니다.

# 업데이트 기록

2019년 2월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


