# xtexconv 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

이 샘플은 **CreatePlacedResourceX** API와 함께 사용할 Xbox One 오프라인 텍스처 타일링을 지원하기 위한 텍스처 변환 및 준비를 위해 표준 TexConv 명령줄 도구를 확장하는 PC 측 명령줄 도구입니다.

이 도구는 Windows 이미징 구성 요소 지원 코덱 .jpg, .png, .tiff, .bmp 및 HD Photo/JPEG XR과 Targa Truevision .tga 파일, RGBE .hdr 및 OpenEXR .exr 파일\--및 .dds\과 같은 광범위한 이미지 형식을 입력 텍스처 형식으로 허용합니다. 사용자 지정 필터를 사용하여 전체 밉 체인 생성을 지원하고 텍스처 배열, 큐브 맵, 큐브 맵 배열 및 볼륨 맵을 지원합니다.

매개 변수 없이 도구를 실행하면 다음과 같이 도움말 화면이 표시됩니다.

![](./media/image1.png)

# 사용법

XTexConv 도구는 표준 TexConv 도구와 동일한 명령줄 매개 변수 및 구문 세트를 지원합니다. 자세한 설명서는 [GitHub](https://github.com/Microsoft/DirectXTex/wiki/Texconv)에서 확인할 수 있습니다.

여기에는 출력 DDS 파일에 Xbox One 타일식 텍스처 데이터와 'XBOX' DDS 파일 변형이 포함되도록 하는 추가 스위치 `-xbox`이(가) 포함되어 있습니다. 사용되는 타일 모드는 XGComputeOptimalTileMode에 의해 결정됩니다. 이러한 오프라인으로 준비된 Xbox One 텍스처는 XG_BIND_SHADER_RESOURCE와 함께 사용되는 것으로 간주됩니다.

'XBOX' DDS 파일 변형이 입력 파일로 사용하는 경우 추가 처리 전에 자동으로 분리되므로 도구를 사용하여 'XBOX' DDS 파일을 표준 DDS 파일로 변환할 수 있습니다.

도구는 타일링에 대한 하드웨어 버전을 선택하는 스위치 `-xgmode` 또한 지원합니다.

`-xgmode:xboxonex`을(를) 사용하여 Xbox One X 선호하는 타일링을 설정합니다. 그렇지 않으면 기본적으로 Xbox One/Xbox One S로 설정됩니다.

## Xbox Series X|S

첫째, xtexconv에는 두 가지 버전이 있습니다. xteconv_xs는 Xbox Series입니다.
| | |
|---|---|
|Xbox Series X|S용 XG ​​버전과|
|**-xgmode** 스위치를 지원하지 않는 Xbox X|S 버전용 XG 버전, 두 가지가 있기 때문입니다.|

# Xbox One용 DDS 파일

표준 DDS 파일 형식은 [Microsoft Docs](https://docs.microsoft.com/en-us/windows/desktop/direct3ddds/dx-graphics-dds-pguide)에 설명되어 있습니다. 'XBOX' DDS 파일 변형은 'DX10' 헤더 확장명과 유사합니다. 'XBOX' DDS 파일은 다음과 같이 배치됩니다.

```
DWORD dwMagic
DDS_HEADER header
DDS_HEADER_XBOX
{
   DXGI_FORMAT dxgiFormat;
   uint32_t resourceDimension;
   uint32_t miscFlag; // see DDS_RESOURCE_MISC_FLAG
   uint32_t arraySize;
   uint32_t miscFlags2; // see DDS_MISC_FLAGS2
   uint32_t tileMode; // see XG_TILE_MODE / XG_SWIZZLE_MODE
   uint32_t baseAlignment;
   uint32_t dataSize;
   uint32_t xdkVer; // matching \_XDK_VER
} headerXbox
<Remainder of file is a tiled texture binary layout suitable for use with CreatePlacement APIs\>
```


'XBOX' 변형 DDS 파일에서 텍스처를 로드하고 생성하는 예제 코드는 XboxDDSTextureLoader([DX11](https://github.com/Microsoft/DirectXTK/wiki/XboxDDSTextureLoader)/[DX 12](https://github.com/Microsoft/DirectXTK12/wiki/XboxDDSTextureLoader)) 모듈의 *DrectX 도구 키트*([DX11](https://github.com/Microsoft/DirectXTK)/[DX 12](https://github.com/Microsoft/DirectXTK12))에서 사용할 수 있습니다.

# Xbox One용 DirectXTex

> XTexConv는 [DirectXTex](https://github.com/Microsoft/DirectXTex/) 라이브러리에 추가된 기능이 있는 약간 수정된 버전의 TexConv입니다. 표준 버전의 [TexConv](https://github.com/Microsoft/DirectXTex/wiki/Texconv) 및 DirectXTex는 GitHub에서 사용할 수 있습니다.

DirectXTex용 Xbox One 보조 기능(Xbox C++ 네임스페이스의 DirectXTexXbox.h에 있음)에는 다음이 포함됩니다.

- XboxImage: 타일식 텍스처 데이터를 위한 컨테이너

- DDS 파일의 XBOX 변형을 저장하고 로드하기 위한 함수

   - GetMetadataFromDDSMemory

   - GetMetadataFromDDSFile

   - LoadFromDDSMemory

   - LoadFromDDSFile

   - SaveToDDSMemory

   - SaveToDDSFile

- 표준 선형 데이터를 Xbox One 타일형 텍스처로 타일링하고 그 반대 작업을 수행하는 함수:

   - 타일

   - 분리

- Direct3D 12 확장을 사용하여 타일식 Xbox One 타일식 이미지에서 텍스처 리소스를 생성하는 함수

   - CreateTexture

   - FreeTextureMemory

# 종속 항목

이 도구와 Xbox One용 DirectXTex 보조 타일/분리 함수는 표준 DLL 검색 경로에 있는 XG.DLL(bin\\XboxOne 폴더 아래 Microsoft GDK에 있음) 또는 XG_XS.DLL(bin\\Scarlett 폴더 아래의 Microsoft GDK에 있음)을 필요로 합니다.

# OpenEXR 지원

xtexconv 도구는 [OpenEXR](http://www.openexr.com/) 라이브러리를 사용하여 자체 라이선스 조건이 적용되는 [openexr](https://www.nuget.org/packages/openexr-msvc14-x64/) 및 [zlib](https://www.nuget.org/packages/zlib/)용 NuGet 패키지를 사용합니다. 이 지원은 USE_OPENEXR 정의를 취소하고, DirectXTexEXR.\*을 삭제하고, NuGet 관리자를 통해 패키지를 제거하여 사용하지 않도록 설정할 수 있습니다.

OpenEXR은 자체적으로 적용됩니다.
[라이선스](https://github.com/openexr/openexr/blob/develop/OpenEXR/LICENSE)
[zlib](http://zlib.net/zlib_license.html)도 마찬가지입니다.

자세한 내용은 [OpenEXR 추가](https://github.com/Microsoft/DirectXTex/wiki/Adding-OpenEXR)를 참조하세요.

# 업데이트 기록

| 날짜 | 참고 |
|---|---|
| 2019년 2월 | 초기 릴리스입니다. |
| 2019년 11월 | Xbox 시리즈 X | S 지원. |
| 2020년 2월 | XG 라이브러리 변경에 대한 업데이트입니다. |


