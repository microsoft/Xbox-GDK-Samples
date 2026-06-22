![](./media/image1.png)

# 뱅크 회전 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

*뱅크 회전*은 동일한 그리기 호출에서 여러 렌더링 대상에 대한 쓰기 속도를 높이는 데 도움이 되는 기술입니다. 이는 여러 개의 GBuffer를 쓸 때 지연된 패스 동안 유용합니다. 뱅크 회전이 ESRAM의 대상에 영향을 주지 않으므로 대상이 DRAM에 있는 경우에만 도움이 됩니다.

Xbox One과 Xbox One S에는 8개의 뱅크가 있고, Xbox One X에는 16개의 뱅크가 있으며, Xbox
X에는 모든 GBuffer가 DRAM에 위치합니다. **관찰된 결과:**
| | |
|---|---|
|Series X|S에는 2개의 뱅크가 있습니다. 이 기능은 Xbox One|에서 특히 유용합니다.


4개의 GBuffer를 사용하면 샘플은 각 GBuffer가 다른 뱅크에 할당된 Xbox One에서 약 **10%-14%**의 이득을 보여주고 Xbox One X에서는 약 **20%**의 이득을 보여줍니다. 패스에 GBuffer가 더 많으면 이득이 증가합니다.

# ![](./media/image3.png)샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 사용

| 동작 | Gamepad |
|---|---|
| 리소스 할당 기술 변경 | A button |
| 끝내기 | 보기 버튼 |

# 구현 참고 사항

뱅크 회전 지원은 DirectX 12 API를 사용하는 경우에만 사용할 수 있습니다.
매우 유사합니다. 샘플 소스 코드를 참조하세요. *커밋된 리소스:*
| | |
|---|---|
|Xbox One의 코드 예제는 다음과 같습니다. Xbox Series X|S의 코드는 다음과 같습니다.|


```cpp
ResDesc.Layout = D3D12XBOX_BANK_ROTATED_TILE_MODE(D3D12_TEXTURE_LAYOUT, bankRotationIndex);
```

그런 다음 리소스를 생성하는 동안 이 새 레이아웃을 CreateCommittedResource와 함께 사용할 수 있습니다.

*배치된 리소스 또는 구성 요소 배치 리소스:*

```cpp
D3D12_GPU_VIRTUAL_ADDRESS rotatedAddress;

assert(XGComputeBankRotationAddress((XG_GPU_VIRTUAL_ADDRESS)gpuAddress, 
                                    &xgResLayout[gbufferIndex], 
                                    0, 
                                    0, 
                                    bankRotationIndex, 
                                    &rotatedAddress));
```


이 새 rotatedAddress를 사용하여 리소스를 배치할 수 있습니다.

# 알려진 문제

없음

# 업데이트 기록

2017년 5월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


