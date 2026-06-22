![](./media/image1.png)

# 나만의 번들 빌드 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

Xbox One에서 데이터를 버퍼에 직접 쓸 수 있습니다. 이는 ExecuteIndirectBundleX API를 사용하여 GPU에서 직접 읽고 실행할 수 있습니다. 이 샘플에서는 이 데이터를 작성하는 다양한 방법을 알아보고 일반 그리기 및 그리기 번들과 비교합니다. 버퍼 데이터는 CPU 또는 GPU를 통해 쓸 수 있으며 후속 패스에서 GPU에 다시 입력할 수 있습니다. 이 기능을 BYOB(사용자 번들 만들기)라고 합니다. 모든 &ldquo;고빈도&rdquo; API 호출은 버퍼에 직접 쓸 수 있습니다. 코드는 d3d12_x.h에서 사용할 수 있는 것과 동일하지만, 명령 버퍼에 직접 쓰는 대신 제공된 버퍼에 기록합니다.

GPU에서 실행할 버퍼에 데이터를 쓰는 동안 유의해야 할 몇 가지 사항은 다음과 같습니다.

- PAGE_GPU_EXECUTE 플래그를 사용하여 버퍼를 만들어야 합니다.

- 실행할 버퍼 크기는 4MB를 초과할 수 없습니다.

- 잘못된 패킷 정보를 전달하면 GPU를 쉽게 중지할 수 있습니다.

- GPU에서 무시되는 일부 사용자 지정 데이터를 제공하는 데 사용할 수 있는 NO_OP 패킷이 있습니다.

- 쓰기 결합 버퍼를 사용하여 GPU에서 쓰는 경우 모든 쓰기 결합 규칙을 주의해야 합니다.

이 샘플에서는 메시 인스턴스를 그리는 여러 메서드를 보여 줍니다. 인스턴스에 임의로 할당되는 여러 PSO가 사용됩니다. GPU 시간은 각 메서드에 대해 동일합니다. 16384 메시의 경우
컬링이 없는 80.3ms 및 컬링이 있는 39.2ms(7439개의 메시 그리기). CPU 시간은 다양하며 메서드 설명과 함께 나열됩니다. 그리기 메서드:

- **BYOB - 각 인스턴스 업데이트 및 그리기**

이 기술은 각 인스턴스에 대한 BYOB 번들 데이터를 업데이트한 다음 해당 인스턴스에 대해 ExecuteIndirectBundleX를 호출합니다. 이 기술을 사용하는 버퍼는 단일 인스턴스에 대한 데이터만 포함하므로 크기가 작습니다. 각 인스턴스에 대한 그리기 데이터를 GPU에 하나씩 개별적으로 보냅니다. CPU: 컬링 없음 = 7.01ms, 컬링 = 6.65ms

- **BYOB - 모든 인스턴스를 업데이트한 다음 그리기**

모든 인스턴스에 대한 BYOB 번들 데이터를 업데이트하고 단일 버퍼에 쓰고 마지막으로 버퍼를 GPU에 전달합니다. 이 기술에 사용되는 버퍼의 크기는 이전 기술보다 크지만 모든 데이터는 단일 ExecuteIndirectBundleX에서 GPU로 전송됩니다. CPU: 컬링 없음 = 33.5ms, 컬링 = 19ms

- **BYOB - 런타임에 번들 빌드**

런타임에 전체 버퍼를 빌드한 다음 단일 ExecuteIndirectBundleX 명령으로 버퍼를 보냅니다. CPU: 컬링 없음 = 7.4ms, 컬링 =
6.8ms

- **BYOB - GPU를 사용하여 그리기**

컴퓨팅 셰이더를 사용하여 모든 메시 인스턴스를 버퍼에 그리는 GPU 패킷 데이터를 작성한 다음 ExecuteIndirectBundleX를 사용하여 해당 버퍼를 GPU에 전달합니다. 컬링은 GPU에서 수행됩니다. CPU: 컬링 없음 = 6.0ms, 컬링 = 6.0ms

- **번들을 사용하여 그리기**

이 기술은 일반 번들을 사용하여 ExecuteBundle API를 사용하여 데이터를 그립니다. CPU: 컬링 없음 = 6.9ms, 컬링 = 6.5ms

- **직접 그리기**

DrawIndexedInstanced를 사용하여 메시를 직접 그립니다. CPU: 컬링 없음 =
7.7ms, 컬링 = 6.9ms

CPU에 대한 모든 버퍼 쓰기는 단일 스레드를 사용하여 수행됩니다. 여러 스레드를 사용하여 실행 버퍼에 쓸 때 성능 이점이 있습니다. 각 상태 설정 및 그리기의 크기가 일정하게 유지되는 경우 여러 스레드가 CPU의 버퍼에 쉽게 쓸 수 있습니다. NO_OP 패킷을 사용하여 버퍼에서 사용되지 않는 공간을 채울 수 있습니다. GPU를 통해 패킷 데이터를 쓰면 CPU 시간이 줄어들고 패킷 데이터를 쓰는 데 비용이 많이 들지 않습니다. 이 메서드는 샘플에서 테스트된 메서드 중에서 가장 빠릅니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 사용

## Screenshot

![](./media/image3.png)

| 동작 | Gamepad |
|---|---|
| 그리기 기술 변경 | A 또는 B 단추 |
| 컬링 메시 | X 버튼 |
| 선택한 기술에 대한 자세한 설명 표시/숨기기 | Y 버튼 |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

번들을 작성하기 위해 샘플에서 사용되는 모든 코드는 WriteOwnBundlesHelper.h에서 사용할 수 있습니다. 버퍼에 패킷 데이터를 쓰는 것은 헤더 파일 d3d12_x.h의 코드를 기반으로 합니다.

예를 들어 DrawIndexedInstanced는 다음과 같이 정의됩니다.

```cpp
D3DINLINE void D3DAPI DrawIndexedInstanced(
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    D3D12XBOX_PPUT pPut = m_Putter.m_pCurrent;
    if (pPut < m_Putter.m_pLimit_Draw)
    {
        m_Putter.PutD(pPut, D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED);
        m_Putter.PutD(pPut, InstanceCount);
        m_Putter.PutD(pPut, StartIndexLocation);
        m_Putter.PutD(pPut, IndexCountPerInstance);
        m_Putter.PutD(pPut, BaseVertexLocation);
        m_Putter.PutD(pPut, StartInstanceLocation);
        m_Putter.m_pCurrent = pPut;
    }
    else
    {
        CommandListFunction()-\>DrawIndexedInstanced(this,
        IndexCountPerInstance,
        InstanceCount,
        StartIndexLocation,
        BaseVertexLocation,
        StartInstanceLocation);
    }
}
```


번들을 작성하는 동안 다음으로 변경하면 됩니다.

```cpp
void DrawIndexedInstancedBYOB(
    _Inout_ UINT32** writeAddress,
    _In_ UINT IndexCountPerInstance,
    _In_ UINT InstanceCount,
    _In_ UINT StartIndexLocation,
    _In_ INT BaseVertexLocation,
    _In_ UINT StartInstanceLocation)
{
    **writeAddress = D3D12XBOX_PACKET_DRAW_INDEXED_INSTANCED;
    *(*writeAddress + 1) = InstanceCount;
    *(*writeAddress + 2) = StartIndexLocation;
    *(*writeAddress + 3) = IndexCountPerInstance;
    *(*writeAddress + 4) = BaseVertexLocation;
    *(*writeAddress + 5) = StartInstanceLocation;
    *writeAddress += 6;
}
```


# 알려진 문제

ExecuteIndirectBundleX를 사용하는 경우 PredicationBuffer가 현재 작동하지 않습니다. PredicationBufferOffset에 대해 0을 전달합니다.

# 업데이트 기록

- 2019년 4월: 초기 릴리스

- 2019년 11월: 프로젝트 Xbox Series X|S를 지원하도록 업데이트됨


