# xtexconv 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기와 호환됩니다(2019년
11월).*

# 설명

이 샘플은 Xbox One 오프라인 타일링을 CreatePlacedResourceX API와 함께
사용하도록 지원하기 위한 텍스처 변환 및 준비용 표준 TexConv 명령줄
도구를 확장하는 PC 쪽 명령줄 도구입니다.

이 도구에서는 다양한 이미지 파일 형식(예: Windows 이미징 구성 요소 지원
코덱 .jpg, .png, .tiff, .bmp, HD Photo/JPEG XR, Targa Truevision .tga
파일, RGBE .hdr, OpenEXR .exr 파일 및 .dds)을 입력 텍스처 형식으로
사용할 수 있습니다. 사용자 지정 필터를 사용한 전체 밉 체인 생성 외에
텍스처 배열, 큐브 맵, 큐브 맵 배열, 볼륨 맵을 지원합니다.

아무 매개 변수 없이 도구를 실행하면 다음과 같은 도움말 화면이
표시됩니다.

![](./media/image1.png)

# 사용

XTexConv 도구는 표준 TexConv 도구와 동일한 명령줄 매개 변수 및 구문
집합을 지원합니다. 자세한 설명서는
[GitHub](https://github.com/Microsoft/DirectXTex/wiki/Texconv)에서 볼 수
있습니다.

이 도구에는 출력 DDS 파일에 Xbox One 타일 텍스처 데이터와 \'XBOX\' DDS
파일 변형이 포함되도록 하는 추가 스위치 \'**-xbox**\'가 포함되어
있습니다. 사용되는 타일 모드는 XGComputeOptimalTileMode에 의해
결정됩니다. 오프라인에서 작성된 이 Xbox One 텍스처는
XG_BIND_SHADER_RESOURCE와 함께 사용하는 것으로 간주됩니다.

\'XBOX\' DDS 파일 변형을 입력 파일로 사용하는 경우에는 추가 처리 이전에
자동으로 디타일링되므로 이 도구를 사용하여 \'XBOX\' DDS 파일을 표준 DDS
파일로 변환할 수 있습니다.

이 도구는 사용자가 타일링에 사용할 하드웨어 버전을 선택하는 스위치
\'**-xgmode**\'도 지원합니다.

Xbox One X용 기본 설정 타일링을 지정하려면 \`-xgmode:xboxonex\`를
사용합니다. 그렇지 않은 경우 기본적으로 Xbox One/Xbox One S로
설정됩니다.

## Project Scarlett

XG는 Project Scarlett용 버전과 Xbox One용 버전 두 가지가 있으므로
xtexconv도 두 가지 버전이 있습니다. xteconv_xs는 **-xgmode** 스위치를
지원하지 않는 Project Scarlett 버전입니다.

# Xbox One용 DDS 파일

표준 DDS 파일 형식은 [Microsoft
Docs](https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide)에
설명되어 있습니다. \'XBOX\' DDS 파일 변형은 \'DX10\' 헤더 확장과
비슷합니다. \'XBOX\' DDS 파일의 레이아웃은 다음과 같습니다.

> DWORD dwMagic
>
> DDS_HEADER header
>
> DDS_HEADER_XBOX
>
> {
>
> DXGI_FORMAT dxgiFormat;
>
> uint32_t resourceDimension;
>
> uint32_t miscFlag; // see DDS_RESOURCE_MISC_FLAG
>
> uint32_t arraySize;
>
> uint32_t miscFlags2; // see DDS_MISC_FLAGS2
>
> uint32_t tileMode; // see XG_TILE_MODE / XG_SWIZZLE_MODE
>
> uint32_t baseAlignment;
>
> uint32_t dataSize;
>
> uint32_t xdkVer; // matching \_XDK_VER
>
> } headerXbox
>
> \<파일의 나머지 부분은 CreatePlacement API와 함께 사용할 수 있는 타일
> 텍스처 이진 레이아웃입니다.\>

\'XBOX\' 변형 DDS 파일에서 텍스처를 로드하고 작성하는 예제 코드는
XboxDDSTextureLoader([DX11](https://github.com/Microsoft/DirectXTK/wiki/XboxDDSTextureLoader)/[DX
12](https://github.com/Microsoft/DirectXTK12/wiki/XboxDDSTextureLoader))
모듈의 *DirectX 도구
키트*([DX11](https://github.com/Microsoft/DirectXTK)/[DX
12](https://github.com/Microsoft/DirectXTK12))에서 사용할 수 있습니다.

# Xbox One용 DirectXTex

# XTexConv는 TexConv가 약간 수정된 버전으로, [DirectXTex](https://github.com/Microsoft/DirectXTex/) 라이브러리에 일부 기능이 추가되어 있습니다. [TexConv](https://github.com/Microsoft/DirectXTex/wiki/Texconv) 및 DirectXTex의 표준 버전은 GitHub에서 사용할 수 있습니다.

# DirectXTex용 Xbox One 보조 함수(Xbox C++ 네임스페이스의 DirectXTexXbox.h)는 다음과 같습니다.

-   # XboxImage: 타일 텍스처 데이터의 컨테이너

-   DDS 파일의 XBOX 변형을 저장하고 로드하기 위한 함수

    -   GetMetadataFromDDSMemory

    -   GetMetadataFromDDSFile

    -   LoadFromDDSMemory

    -   LoadFromDDSFile

    -   SaveToDDSMemory

    -   SaveToDDSFile

-   표준 선형 데이터를 Xbox One 타일 텍스처로 타일링 및 디타일링을
    수행하기 위한 함수

    -   Tile

    -   Detile

-   Direct3D 12 확장을 사용하여 Xbox One 타일 이미지에서 텍스처 리소스를
    만들기 위한 함수

    -   CreateTexture

    -   FreeTextureMemory

# 종속성

이 도구와 DirectXTex용 Xbox One 보조 Tile/Detile 함수를 사용하려면
XG.DLL(bin\\XboxOne 폴더의 Microsoft GDK에 위치) 또는
XG_XS.DLL(bin\\Scarlett 폴더의 Microsoft GDK에 위치)이 표준 DLL 검색
경로에 위치해야 합니다.

# OpenEXR 지원

xtexconv 도구는 자체 라이선스 조건이 적용되는
[openexr](https://www.nuget.org/packages/openexr-msvc14-x64/) 및
[zlib](https://www.nuget.org/packages/zlib/)용 NuGet 패키지를 통해
[OpenEXR](http://www.openexr.com/) 라이브러리를 사용합니다. 이 지원은
USE_OPENEXR의 정의를 해제하고, DirectXTexEXR.\*을 삭제하고, NuGet
관리자를 통해 패키지를 제거하여 사용하지 않도록 설정할 수 있습니다.

OpenEXR은 [zlib](http://zlib.net/zlib_license.html)과 마찬가지로 자체
[라이선스](https://github.com/openexr/openexr/blob/develop/OpenEXR/LICENSE)가
적용됩니다.

자세한 내용은 [OpenEXR
추가](https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR)를
참조하세요.

# 업데이트 기록

