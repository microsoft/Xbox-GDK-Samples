# xbdepends 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

GDK 타이틀에 대한 빌드 및 시작 문제의 진단에 도움이 되는 Windows 10/Windows 11 컴퓨터용 명령줄입니다. 특히 EXE 및 DLL에 대한 가져오기 모듈을 분석하고 분류하며 진단 출력을 제공합니다. Xbox One 및 Scarlett 이진 파일의 경우 xgameplatform.lib 우산 라이브러리에 포함되지 않았지만 사용된 OS API 목록도 출력합니다.

명령줄 옵션 없이 실행하면 다음 출력이 생성됩니다.

![자동으로 생성된 텍스트 설명](./media/image1.png)

# 샘플 빌드

간단한 명령줄 도구로 *게임 명령 프롬프트*를 사용하여 직접 빌드할 수 있습니다.

```
cl /EHsc /D_WIN32_WINNT=0x0A00 /Ox /MT xbdepends.cpp onecore.lib
```


CMake 3.20 이상을 사용할 수 있습니다.

```
cmake -B out .
cmake --build out
```


CMake 사전 설정도 있습니다.

```
cmake --list-presets
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```


또는 VS IDE에서 CMakeLists.txt를 열 수 있습니다(VS 2019 16.11 또는 VS 2022가 필요함).

- 정적 Visual C++ 런타임으로 빌드하여 도구를 배포하기에 사소하게 만듭니다. 일반적으로 타이틀의 DLL 기반 런타임에는 /MD를 사용하는 것이 좋습니다.

# 사용법

간단한 테스트를 위해 Microsoft GDK의 기본 템플릿을 사용합니다. **Direct3D 12 Xbox 게임** 프로젝트를 만들고 빌드합니다. 그런 다음 레이아웃 디렉터리를 가리키는 **xbdepends**를 실행합니다.

```
xbdepends Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Direct3DGame1.exe
```


이렇게 하면 다음 출력이 생성됩니다.

```
Microsoft (R) Xbox Binary Dependencies Tool
Copyright (C) Microsoft Corp.

reading 'Direct3DGame1.exe' [EXE]
INFO: Found 27 import modules
INFO: Use of Direct3D 12.X_S implies Scarlett target
INFO: Dependencies require 'VS 2019 (16.0)' or later C/C++ Runtime
```


자세한 정보 출력은 -v 스위치에 의해 트리거됩니다.

```
xbdepends -v Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Direct3DGame1.exe
```


이렇게 하면 다음 출력이 생성됩니다.

```
reading 'Direct3DGame1.exe' [EXE]
        Linker: 14.00
        OS: 6.00
        Subsystem: 2 (6.00)
INFO: Found 27 import modules
INFO: Use of Direct3D 12.X_S implies Scarlett target
INFO: Dependencies require 'VS 2019 (16.0)' or later C/C++ Runtime
  DLL 'api-ms-win-core-debug-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-errorhandling-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-handle-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-heap-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-interlocked-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-libraryloader-l1-2-0.dll' (OS)
  DLL 'api-ms-win-core-localization-l1-2-0.dll' (OS)
  DLL 'api-ms-win-core-memory-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-processthreads-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-processthreads-l1-1-1.dll' (OS)
  DLL 'api-ms-win-core-profile-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-psm-appnotify-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-psm-appnotify-l1-1-1.dll' (OS)
  DLL 'api-ms-win-core-registry-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-registry-l2-1-0.dll' (OS)
  DLL 'api-ms-win-core-rtlsupport-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-string-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-synch-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-sysinfo-l1-1-0.dll' (OS)
  DLL 'd3d12_xs.dll' (D3D)
  DLL 'ext-ms-win-rtcore-ntuser-message-l1-1-0.dll' (OS)
  DLL 'ext-ms-win-rtcore-ntuser-window-ansi-l1-1-0.dll' (OS)
  DLL 'ext-ms-win-rtcore-ntuser-window-l1-1-0.dll' (OS)
  DLL 'PIXEvt.dll' (GameOS)
  DLL 'ucrtbased.dll' (CRT)
  DLL 'VCRUNTIME140_1D.dll' (CRT)
  DLL 'VCRUNTIME140D.dll' (CRT)
```


/retail 스위치를 사용하여 실행할 수도 있는데, 그러면 이 빌드에서 디버그 CRT 항목을 사용 중임을 경고합니다.

```
xbdepends -retail Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Direct3DGame1.exe
```


이렇게 하면 다음 출력이 생성됩니다.

```
reading 'Direct3DGame1.exe' [EXE]
INFO: Found 27 import modules
INFO: Use of Direct3D 12.X_S implies Scarlett target
ERROR: Using development only DLLs not for use in retail:
        ucrtbased.dll
        VCRUNTIME140_1D.dll
        VCRUNTIME140D.dll
INFO: Dependencies require 'VS 2019 (16.0)' or later C/C++ Runtime
```


또한 이 도구는 와일드카드를 허용하며 재귀적으로 수행할 수 있습니다.

```
xbdepends -r Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\*.dll
```


# 구현

실제로 이 도구는 PE 가져오기 테이블 검사와 관련하여 Microsoft DUMPBIN 도구에서 수행 가능한 것과 동일한 종류의 작업을 수행합니다. 주요 차이점은 이 도구가 GDK 타이틀에 대한 몇 가지 기본 규칙과 지식을 적용하여 진단 출력을 생성한다는 점입니다.

PE 가져오기 테이블에 대한 자세한 내용은 다음을 참조하세요.

"*윈도우 내부: Win32 이동 가능 실행 파일 형식에 대한 심층 분석, 2부*". MSDN 매거진(2002년 3월)

<https://docs.microsoft.com/en-us/archive/msdn-magazine/2002/march/inside-windows-an-in-depth-look-into-the-win32-portable-executable-file-format-part-2>

# 업데이트 내역

| 날짜 | 참고 |
|---|---|
| 2021년 5월 | 2021년 4월 GDK 릴리스의 xgameplatform.lib를 사용한 초기 릴리스입니다. |
| 2022년 1월 | CMake 정리 및 사전 설정 파일 추가 |
| 2022년 11월 | CMake 3.20으로 업데이트됨 |


