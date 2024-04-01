![](./media/image1.png)

# DumpTool 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

DumpTool은 Xbox One 타이틀과 동일한 OS 파티션에서 실행되어 도구에 대한 인수로 이름에 따라 지정한 다른 프로세스의 크래시 덤프를 생성합니다. 이 도구를 컴파일해 즉시 사용할 수 있으며, 소스 코드를 통해 크래시 덤프 기능을 사용자의 도구 또는 타이틀에 추가할 수도 있습니다.

# 샘플 빌드

프로젝트에 대한 플랫폼 구성입니다. *구성 관리자를 통해 이 작업을 실행할 수 있습니다*: \"활성 솔루션 플랫폼"\에서 \"구성 관리자"\ 옵션을 선택하고 \"새로 만들기\...\"를 선택합니다. \"Type or select the new platform\"을 Gaming.Xbox.Scarlett.x64로 설정하고 \"Copy settings from\"을 Gaming.Xbox.XboxOne.x64로 설정합니다. 그런 다음 확인을 선택합니다. *GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*
| | |
|---|---|
|Xbox Series X|S를 사용하는 경우 Gaming.Xbox.Scarlett.x64를 추가해야 합니다.|


# 샘플 사용

DumpTool은 타이틀 모드 콘솔 애플리케이션으로 컴파일 됩니다([MSDN 백서](https://developer.xboxlive.com/en-us/platform/development/education/Documents/Title%20Mode%20Console%20Applications.aspx) 참조). Visual Studio를 사용하여 콘솔에 .exe를 배포하면 실행 중인 애플리케이션이 종료되므로 .여러 단계에 걸쳐 exe를 빌드한 다음에 콘솔에 복사하고 이어서 실행해야 합니다.

1. Visual Studio에서 도구를 빌드하여 DumpTool.exe 제작

2. 타이틀 시작(예: SimpleTriangle 샘플)

3. 게임 OS 파티션에 DumpTool.exe 복사

```
xbcp /x/title Gaming.Xbox.x64\Layout\Image\Loose\*.exe xd:\DumpTool\
xbcp /x/title Gaming.Xbox.x64\Layout\Image\Loose\*.dll xd:\DumpTool\
```


4. 도구를 실행하여 SimpleTriangle.exe 대한 심사 덤프를 수집

```
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:triage SimpleTriangle.exe
```


5. 디버깅을 위해 .dmp 파일을 개발 PC에 다시 복사

```
xbcp /x/title xd:\SimpleTriangle.dmp
```


DumpTool 프로젝트에는 처음 네 단계를 자동화하고 코드 변경 내용을 쉽게 테스트할 수 있는 간단한 일괄 처리 파일인 runCommand.bat이 들어 있습니다.

## DumpTool 명령줄

DumpTool은 다양한 명령줄 옵션 집합도 지원합니다.

```
Usage: DumpTool [-mdt:<minidump type> ...] [-pdt:<predefined type>] <executable name>

  <minidump type>: Normal WithDataSegs WithFullMemory WithHandleData
        FilterMemory ScanMemory WithUnloadedModules
        WithIndirectlyReferencedMemory FilterModulePaths
        WithProcessThreadData WithPrivateReadWriteMemory
        WithoutOptionalData WithFullMemoryInfo WithThreadInfo
        WithCodeSegs WithoutAuxiliaryState
        WithFullAuxiliaryState WithPrivateWriteCopyMemory
        IgnoreInaccessibleMemory WithTokenInformation
        WithModuleHeaders FilterTriage

<predefined type>: heap mini micro triage native
```


\<minidump type\>s는 다음 위치에 문서화된 MINIDUMP_TYPE 열거형의 값에 해당합니다.
[GDNP](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/MINIDUMP_TYPE_typedef___dbghelp_Xbox_Microsoft_T_may17.aspx)
및 [MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519(v=vs.85).aspx). 명령줄에 --mdt: 의 다양한 인스턴스를 지정하여 서로 다른 MINIDUMP_TYPE 값을 결합합니다. 가능성은 매우 다양합니다. 이 도구에서는 작업을 더 간단하게 하기 위한 --pdt 옵션도 제공합니다.

일반적으로 -mdt 옵션을 사용하여 개별적으로 제공해야 하는 MINIDUMP_TYPE 플래그를 단순화하기 위해 "미리 정의된 형식"(-pdt) 옵션이 있습니다. 미리 정의된 형식은 다음 xbWatson.exe에서 지원하는 크래시 덤프 형식에 해당합니다.

![](./media/image3.png)

예:

```
xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage SimpleTriangle.exe
```


```
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:Mini SimpleTriangle.exe
```


```
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:Heap SimpleTriangle.exe
```


이 도구는 "마이크로" 및 "네이티브"도 제공합니다. 해당 값에 해당하는 플래그의 정확한 조합은 소스 코드를 참조하세요. **MiniDumpWriteDump()**에 익숙하지 않은 경우, 미리 정의된 덤프 플래그로 시작한 다음 필요에 따라 추가 플래그를 실험합니다. 도구는 `--pdt` 및 `--mdt`을(를) 동시에 허용하고 플래그를 결합하기 때문에 어려움 없이 이 실험을 진행할 수 있도록 해야 합니다.

```
xbrun /x/title /O d:\\DumpTool\\DumpTool.exe --pdt:micro --mdt:WithHandleData --mdt:WithUnloadedModules SimpleTriangle.exe
```


## 도구 배포

타이틀과 DumpTool(또는 일부 변형)을 함께 사용하려는 경우, 게임 OS에 복사하지 않아도 되게끔 도구를 게임 배포에 추가하는 것이 좋습니다. 그렇게 하면 실행 중인 타이틀을 다른 방식으로 방해하지 않고 크래시 덤프를 생성하는 편리한 방법을 도구에서 제공합니다.

# 구현 참고 사항

- 실행 파일의 코드에서 직접 **MiniDumpWriteDump()** 를 호출할 수도 있습니다. 예를 들어 많은 개발자가 처리되지 않은 예외 필터에 이 기능을 추가합니다. 다음은 MiniDumpWriteDump에 대한 매우 간단한 예제 호출입니다.

```
MiniDumpWriteDump(
  GetCurrentProcess(),
  GetProcessId(GetCurrentProcess()), hDumpFile, mdt, nullptr, nullptr,
  nullptr);
```


- GSDK는 또한 크래시덤프를 캡처하는 데 사용할 수 있는 [xbWatson](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/xbwatson_may17.aspx)이라는 가벼운 도구를 함께 제공합니다. DumpTool의 기능은 xbWatson의 크래시 덤프 기능과 동일합니다. xbWatson을 사용하기 위해 추가 배포 단계를 수행할 필요는 없습니다.

- Visual Studio를 사용하여 크래시 덤프를 캡처할 수도 있습니다. 디버그 메뉴에서 "덤프를 다른 이름으로 저장..." 옵션을 찾습니다. 이 옵션은 프로세스에 연결한 후에 나타나며 일시 중지("모두 중단")할 때 활성화됩니다.

# 알려진 문제

**MiniDumpWriteDump** 호출 전에 `GENERIC_WRITE` 및 `GENERIC_READ`을(를) 통해 파일을 열어야 하며 그렇지 않은 경우 결과 .dmp 파일이 손상될 수 있습니다.

# 업데이트 기록

2019년 4월 초기 릴리스.


