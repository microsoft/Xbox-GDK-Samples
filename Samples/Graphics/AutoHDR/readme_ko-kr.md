# 자동 HDR 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

시스템 수준에서 HDR을 자동으로 추가하여 타이틀 이 기능
타이틀 또는 시스템. 즉. 추가 CPU 또는 GPU, 추가 메모리 또는 대역폭, 추가 대기 시간이 없습니다. 이 기능은 대부분의 이전 버전과 호환되는 ERA 타이틀에 적용되지만 기본 Xbox Series에서도 사용할 수 있습니다.
D3D 장치 생성 플래그 `D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR`을(를) 사용하여 HDR 구현. ![자동으로 생성된 비디오 게임 설명 스크린샷](./media/image1.png)
| | |
|---|---|
|자동 HDR은 SDR을 시각적으로 향상할 수 있는 Xbox Series X|S 기능입니다.|
|Xbox Series X|S 하드웨어를 사용하므로 다음에 대한 성능 비용이 없습니다.|
|X|S GDK 타이틀. 이 샘플은 GDK 타이틀이 자동 HDR을 어떻게 사용하는지 보여 줍니다.|


이 샘플에 사용된 이미지는 이 샘플 데모에 사용하기 위해 <https://www.halowaypoint.com/en-us> 및 <https://gearsofwar.com/en-US/>에서 가져왔습니다.

# 샘플 빌드

이 샘플은 `Gaming.Xbox.Scarlett.x64`을(를) 사용하는 Xbox Series X|S만 지원합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

이 샘플에서는 다음 컨트롤을 사용합니다.

| 동작 | Gamepad |
|---|---|
| UI 밝기 조정 전환 | A |
| 재구성된 색상 채도 조정 | 방향 패드 왼쪽/오른쪽 |
| 다음 이미지 | 오른쪽 어깨 |
| 이전 이미지 | 왼쪽 어깨 |

# 구현 참고 사항

**SDR로 렌더링**

Auto HDR은 시스템 수준에서 적용되기 때문에 타이틀은 SDR로 렌더링되어야 합니다. 샘플은 SDR로 렌더링 및 표시됩니다.

**스왑 버퍼 형식**

밴딩과 같은 정밀 아티팩트를 방지하려면 고정밀 스왑 버퍼 형식을 사용하는 것이 좋습니다.

```cpp
m_deviceResources = std::make_unique<DX::DeviceResources>(DXGI_FORMAT_R9G9B9E5_SHAREDEXP,
                                                          DXGI_FORMAT_UNKNOWN,
                                                          2,
                                                          DX::DeviceResources::c_Enable4K_UHD);
```


**TV를 HDR 모드로 전환하고 자동 HDR 플래그가 있는 D3D 장치 만들기**

타이틀이 모든 것을 SDR로 렌더링하더라도 타이틀은 여전히 ​​TV를 HDR 모드로 전환해야 합니다.

```cpp
if (SwitchDisplayToHDR())
{
    params.CreateDeviceFlags = D3D12XBOX_CREATE_DEVICE_FLAG_ENABLE_AUTO_HDR;
}
```

**다시 시작/제한 해제 후 TV를 HDR 모드로 전환**

타이틀이 일시 중단/제한되는 동안 콘솔의 디스플레이 설정이 변경되었을 수 있으므로 다시 시작할 때 타이틀은 TV를 HDR 모드로 다시 전환해야 합니다.

```cpp
void Sample::OnResuming()
{
    // Display modes could have changed while title was suspended, so we need to make
    // sure that the TV is still in HDR mode. This is required for native HDR and
    // Auto HDR.

    m_deviceResources->SwitchDisplayToHDR();
```


**UI 밝기 조정**

```cpp
    if (isDisplayInHDRMode)
    {
        // Auto HDR will show pure white pixels as 1000 nits, so text/UI/HUD will become
        // much too bright. We linearly scale down the brightness of the UI
        m_UIBrightnessScale = m_bAdjustUIBrighness ? 0.8f : 1.0f;
    }
    else
    {
        // If the TV is in SDR mode, we don\'t do any brightness scaling
        m_UIBrightnessScale = 1.0f;
    }
}
```

# 업데이트 기록

2021년 6월 최초 출시

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


