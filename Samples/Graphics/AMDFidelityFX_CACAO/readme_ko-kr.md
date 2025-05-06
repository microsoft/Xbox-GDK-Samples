![](./media/image1.png)![](./media/image2.png)

# FidelityFX 앰비언트 오클루전(CACAO)

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

결합된 적응형 컴퓨팅 앰비언트 오클루전(CACAO)은 고도로 최적화된 적응형 샘플링 앰비언트 오클루전 구현입니다.&nbsp;이 샘플에는 환경 요구에 맞게 조정할 수 있는 다양한 품질 사전 설정이 포함되어 있습니다.

자세한 내용은 다음의 GPUOpen에서 확인할 수 있습니다. <https://gpuopen.com/fidelityfx-cacao/>

![](./media/image3.jpeg)

# 샘플 빌드

Windows 데스크톱을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Desktop.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

Xbox One을 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Windows 데스크톱을 실행할 때 최신 그래픽 드라이버가 설치되어 있는지 확인합니다.

*자세한 내용은* *GDK 설명서의* __샘플 실행을__ 참조하세요.&nbsp;

# 샘플 사용

| 동작 | Gamepad |
|---|---|
| 보기 회전 | 왼쪽 엄지스틱 |
| 끝내기 | 보기 버튼 |
| CACAO 켜기/끄기 | X 버튼 |
| CACAO 품질 미리 설정 | A/B 단추 |
| CACAO 버퍼 보기 | Y 버튼 |
| AO 버퍼 복합 혼합 요소 | 오른쪽/왼쪽 트리거 |

# 구현 참고 사항

CACAO는 Intel ASSAO(Adaptive Screen Space 앰비언트 폐색) 구현의 고도로 최적화된 적응입니다. 5개 품질 수준(가장 낮음, 낮음, 중간, 높음, 적응형)을 제공합니다.

![](./media/image4.png) ![](./media/image5.png)

## 통합

이 통합 샘플은 GDKX 및 Xbox 특정 DX12 헤더와 함께 사용하도록 수정된 GPUOpen의 D3D12 PC 구현 버전을 제공합니다. 이 샘플에서는 모든 셰이더 순열이 생성되고 사용자 지정 빌드 단계에서 미리 빌드됩니다. 미리 빌드된 셰이더는 C++ 호환 헤더 파일에 해당 DXIL 바이트 코드를 포함합니다.

이 사용자 지정 빌드 단계에는 Xbox 대상에 맞게 최적화된 옵션이 있습니다.

이 샘플에서는 매우 간단한 복합 연산을 가변적인 양의 혼합 요소와 함께 사용하여 장면에서 어떻게 보이는지 보여 줍니다. 엔진 통합의 경우 앰비언트 조명 계산에서 CACAO의 출력 버퍼를 사용하는 것이 좋습니다.

샘플 프레임워크는 ffx_cacao_impl_gdkx.cpp에 정의된 다음 함수를 사용합니다.

- FFX_CACAO_D3D12GetContextSize

- FFX_CACAO_D3D12InitContext

- FFX_CACAO_D3D12DestroyContext

- FFX_CACAO_D3D12InitScreenSizeDependentResources

- FFX_CACAO_D3D12DestroyScreenSizeDependentResources

- FFX_CACAO_D3D12UpdateSettings

- FFX_CACAO_D3D12Draw

이러한 작업을 직접 호출하거나, 함수를 예제로 사용하여 엔진 리소스 처리와 통합하기 위한 추가 작업을 수행할 수 있습니다.

CACAO가 작동하려면 두 개의 매트릭스(프로젝션, normalsToView)가 표준 및 깊이 리소스와 결합되어 있어야 합니다. 출력은 DXGI_FORMAT_R8_UNORM 형식의 UAV입니다.

## 품질 모드

이 샘플은 품질 미리 설정을 노출하고 다운샘플링된 해상도에서 작동합니다. 이러한 형식은 CACAO.h 내에 정의된 FFX_CACAO_PRESETS 구조로 제공됩니다.

| 기본값 | radius = 1.2f<br/>shadowMultiplier = 1.0f<br/>shadowPower = 1.50f<br/>shadowClamp = 0.98f<br/>horizonAngleThreshold = 0.06f<br/>fadeOutFrom = 20.0f<br/>fadeOutTo = 40.0f<br/>adaptiveQualityLimit = 0.75f<br/>sharpness = 0.98f<br/>detailShadowStrength = 0.5f<br/>generateNormals = FFX_CACAO_FALSE<br/>bilateralSigmaSquared = 5.0f<br/>bilateralSimilarityDistanceSigma = 0.1f |
|---|---|
| 적응 품질 네이티브 해상도 | qualityLevel = FFX_CACAO_QUALITY_HIGHEST blurPassCount = 2 |
| 고품질 네이티브 해상도 | qualityLevel = FFX_CACAO_QUALITY_HIGH blurPassCount = 2 |
| 중간 품질 네이티브 해상도 | qualityLevel = FFX_CACAO_QUALITY_MEDIUM blurPassCount = 2 |
| 저품질 네이티브 해상도 | qualityLevel = FFX_CACAO_QUALITY_LOW blurPassCount = 6 |
| 최저 품질 네이티브 해상도 | qualityLevel = FFX_CACAO_QUALITY_LOWEST blurPassCount = 6 |
| 적응 품질 다운샘플링된 해상도 | qualityLevel = FFX_CACAO_QUALITY_HIGHEST blurPassCount = 2 |
| 고품질 다운샘플링된 해상도 | qualityLevel = FFX_CACAO_QUALITY_HIGH blurPassCount = 2 |
| 중간 품질 다운샘플링된 해상도 | qualityLevel = FFX_CACAO_QUALITY_MEDIUM blurPassCount = 3 bilateralSigmaSquared = 5.0f bilateralSimilarityDistanceSigma = 0.2f |
| 저품질 다운샘플링된 해상도 | qualityLevel = FFX_CACAO_QUALITY_LOW blurPassCount = 6 bilateralSigmaSquared = 8.0f bilateralSimilarityDistanceSigma = 0.8f |
| 최저 품질 다운샘플링된 해상도 | qualityLevel = FFX_CACAO_QUALITY_LOWEST blurPassCount = 6 bilateralSigmaSquared = 8.0f bilateralSimilarityDistanceSigma = 0.8f |

# 업데이트 기록

이 샘플은 2021년 9월에 작성되었습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.

# 고지 사항

여기에 포함된 정보는 정보 제공의 목적으로만 제공되며 예고 없이 변경될 수 있습니다. 이 문서를 준비하는 동안 모든 예방 조치를 취했지만 기술적인 부정확성, 누락 및 인쇄상의 오류가 포함될 수 있으며 AMD는 이 정보를 업데이트하거나 수정할 의무가 없습니다. Advanced Micro Devices, Inc.는 여기에 설명된 AMD 하드웨어, 소프트웨어 또는 기타 제품의 작동 또는 사용과 관련하여 비침해, 상품성 또는 특정 목적에의 적합성에 대한 묵시적 보증을 포함하여 이 문서 내용의 정확성 또는 완전성과 관련하여 어떠한 진술이나 보증도 하지 않으며 어떤 종류의 책임도 지지 않습니다. 묵시적이거나 금반언에 의해 발생하는 것을 포함하여 이 문서에서는 지적 재산권에 대한 라이선스를 부여하지 않습니다. AMD 제품의 구매 또는 사용에 적용되는 약관 및 제한 사항은 당사자 간의 서명된 계약 또는 AMD의 표준 판매 약관에 명시되어 있습니다.

AMD, AMD Arrow 로고, Radeon, RDNA, Ryzen 및 이들의 조합은 Advanced Micro Devices, Inc.의 상표입니다. 이 간행물에 사용된 기타 제품 이름은 식별 목적으로만 사용되었으며 해당 회사의 상표일 수 있습니다.

Windows는 미국 및/또는 기타 국가에서 Microsoft Corporation의 등록 상표입니다.

Xbox는 미국 및/또는 기타 국가에서 Microsoft Corporation의 등록 상표입니다.

© 2021 Advanced Micro Devices, Inc. All rights reserved.


