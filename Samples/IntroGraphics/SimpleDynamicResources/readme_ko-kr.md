# 단순 동적 리소스 샘플

*이 샘플은 Microsoft 게임 개발 키트(2021년 4월)와 호환됩니다.*

# 설명

이 샘플은 HLSL 셰이더 모델 6.6에서 HLSL 동적 리소스를 사용하는 방법을
보여줍니다. 이 샘플은 HLSL에서 ResourceDescriptorHeap\[\] 및
SamplerDescriptorHeap\[\]을 사용하여 힙을 통해 리소스에 직접
액세스한다는 점을 제외하면 SimpleTexture와 기능적으로 동일합니다.

![C:\\temp\\xbox_screenshot.png](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정합니다.

PC용 빌드(Gaming.Desktop.x64)에는 HLSL SM 6.6 기능을 사용하기 때문에
[DirectX Agility
SDK](https://devblogs.microsoft.com/directx/gettingstarted-dx12agility/)가
필요합니다. Agility SDK는 샘플에 NuGet 패키지로 포함되어 있습니다. 또한
Microsoft.Windows.SDK.CPP NuGet 패키지를 사용하여 DXC.exe 컴파일러의
최신 Windows SDK(22000) 버전을 가져옵니다. 개발자는
[Github](https://github.com/microsoft/DirectXShaderCompiler/releases)에서
직접 최신 DXC를 사용할 수도 있습니다.

*자세한 내용은 GDK 설명서에서* 샘플 실행을 *참조하세요.*

# 샘플 사용

샘플에는 종료 외에는 컨트롤이 없습니다.

# 구현 참고 사항

이 샘플은 SimpleTexture의 거의 모든 코드를 대여합니다. 유일한 차이점은
리소스 액세스에 있습니다.

이 샘플은 루트 서명에서 바인딩된 리소스를 제거하여 HLSL 셰이더 코드에서
ResourceDescriptorHeap\[\] 및 SamplerDescriptorHeap\[\] 액세스로
대체합니다. 이렇게 하려면 SetGraphicsRootSignature() 전에
SetDescriptorHeaps()가 호출되도록 하고 루트 서명에 플래그
CBV_SRV_UAV_HEAP_DIRECTLY_INDEXED 및 SAMPLER_HEAP_DIRECTLY_INDEXED를
추가해야 합니다.

HLSL 6.6 동적 리소스에 대한 자세한 내용은 [HLSL SM 6.6 동적
리소스](https://microsoft.github.io/DirectX-Specs/d3d/HLSL_SM_6_6_DynamicResources.html)를
참조하세요.

동적 리소스의 고급 사용 방법은 Graphics\\VisibilityBuffer 샘플을
참조하세요.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
