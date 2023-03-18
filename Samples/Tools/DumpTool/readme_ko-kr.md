  ![](./media/image1.png)

#   DumpTool 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 

# 설명

DumpTool은 Xbox One 타이틀과 같은 OS 파티션에서 실행되며 이름으로 지정한
다른 프로세스에 대한 크래시 덤프를 도구에 대한 인수로 생성합니다. 이
도구는 즉시 사용할 수 있게 컴파일하거나, 소스 코드에서 빌려와 자체 도구
또는 타이틀에 크래시 덤프 기능을 추가할 수 있습니다.

# 샘플 빌드

Project Scarlett을 사용하는 경우에는 프로젝트에 Gaming.Xbox.Scarlett.x64
플랫폼 구성을 추가해야 합니다. *Configuration Manager*를 통해 이 작업을
수행할 수 있습니다. \"활성 솔루션 플랫폼\"에서 \"Configuration Manager\"
옵션을 선택하고 \"새로 만들기\...\"를 선택합니다. \"새 플랫폼 입력 또는
선택\"을 Gaming.Xbox.Scarlett.x64로 설정하고 \"다음에서 설정 복사\"를
Gaming.Xbox.XboxOne.x64로 설정합니다. 그런 다음, 확인을 선택합니다.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

DumpTool은 타이틀 모드 콘솔 응용 프로그램으로 컴파일합니다([MSDN
백서](https://developer.xboxlive.com/en-us/platform/development/education/Documents/Title%20Mode%20Console%20Applications.aspx)에서도
참조). Visual Studio를 사용하여 콘솔에 .exe를 배포하면 실행 중인 모든
응용 프로그램이 종료되므로 .exe를 빌드하여 콘솔에 복사하고 다음과 같은
여러 단계로 실행해야 합니다.

1.  Visual Studio에서 이 도구를 빌드하여 DumpTool.exe 생성

2.  타이틀(또는 SimpleTriangle 샘플) 시작

3.  게임 OS 파티션에 DumpTool.exe 복사

\> xbcp /x/title Gaming.Xbox.x64\\Layout\\Image\\Loose\\\*.exe
xd:\\DumpTool\\

\> xbcp /x/title Gaming.Xbox.x64\\Layout\\Image\\Loose\\\*.dll
xd:\\DumpTool\\

4.  이 도구를 실행하여 SimpleTriangle.exe에 대한 심사 덤프 수집

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage
SimpleTriangle.exe

5.  디버깅을 위해 .dmp 파일을 개발 PC로 다시 복사

\> xbcp /x/title xd:\\SimpleTriangle.dmp

DumpTool 프로젝트에는 처음 4단계를 자동화하고 코드 변경 내용을 쉽게
테스트할 수 있도록 하는 간단한 배치 파일인 runCommand가 포함되어
있습니다.

## DumpTool 명령줄

또한 DumpTool은 다음과 같이 다양한 명령줄 옵션 집합을 지원합니다.

사용법: DumpTool \[-mdt:\<미니 덤프 유형\> \...\] \[-pdt:\<미리 정의된
형식\>\] \<실행 파일 이름\>

\<미니 덤프 유형\>: Normal WithDataSegs WithFullMemory WithHandleData

FilterMemory ScanMemory WithUnloadedModules

WithIndirectlyReferencedMemory FilterModulePaths

WithProcessThreadData WithPrivateReadWriteMemory

WithoutOptionalData WithFullMemoryInfo WithThreadInfo

WithCodeSegs WithoutAuxiliaryState

WithFullAuxiliaryState WithPrivateWriteCopyMemory

IgnoreInaccessibleMemory WithTokenInformation

WithModuleHeaders FilterTriage

\<미리 정의된 형식\>: heap mini micro triage native

\<미니 덤프 유형\>은
[GDNP](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/MINIDUMP_TYPE_typedef___dbghelp_Xbox_Microsoft_T_may17.aspx)
및
[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519(v=vs.85).aspx)에
문서화된 MINIDUMP_TYPE 열거형의 값에 해당합니다. 명령줄에서 --mdt:의
여러 인스턴스를 지정하여 MINIDUMP_TYPE의 다양한 값을 조합합니다. 많은
경우의 수가 있습니다. 조합을 보다 간단히 하기 위해 이 도구는 --pdt
옵션을 제공합니다.

\"미리 정의된 형식\"(-pdt) 옵션은 -mdt 옵션을 사용하여 일반적으로는
개별적으로 제공해야 하는 MINIDUMP_TYPE 플래그를 간소화하는 데
사용됩니다. 미리 정의된 형식은 xbWatson.exe에서 지원하는 크래시 덤프
유형에 해당합니다.

![](./media/image3.png)

예제:

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage
SimpleTriangle.exe

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:Mini
SimpleTriangle.exe

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:Heap
SimpleTriangle.exe

이 도구는 \"micro\" 및 \"native\"도 제공합니다. 해당 값에 해당하는
플래그의 정확한 조합을 보려면 소스 코드를 참조하세요.
MiniDumpWriteDump()에 익숙하지 않은 경우 미리 정의된 덤프 플래그로
시작하고 필요에 따라 추가 플래그를 시험해 보세요. 이 도구는 -pdt: 및
--mdt를 동시에 허용하고 플래그를 조합하므로 이 실험을 간편하게 수행할 수
있도록 지원합니다.

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe --pdt:micro
--mdt:WithHandleData

--mdt:WithUnloadedModules SimpleTriangle.exe

## 도구 배포

타이틀에 DumpTool(또는 일부 변형)을 사용하려는 경우 게임 OS에 복사할
필요가 없도록 게임 배포에 이 도구를 추가하는 것이 좋습니다. 그러면 이
도구는 실행 중인 타이틀을 중단하지 않으면서 크래시 덤프를 생성하는
편리한 방법을 제공합니다.

# 구현 참고 사항

-   실행 파일의 코드에서 직접 MiniDumpWriteDump()를 호출할 수도
    있습니다. 예를 들어, 많은 개발자가 처리되지 않은 예외 필터에 이
    기능을 추가합니다. 다음은 MiniDumpWriteDump에 대한 간단한 예제
    호출입니다.

> MiniDumpWriteDump(
>
> GetCurrentProcess(),
>
> GetProcessId(GetCurrentProcess()), hDumpFile, mdt, nullptr, nullptr,
> nullptr);

-   GSDK에는 크래시 덤프를 캡처하는 데 사용할 수 있는
    [xbWatson](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/xbwatson_may17.aspx)이라는
    간단한 도구가 함께 제공됩니다. DumpTool의 기능은 xbWatson의 크래시
    덤프 기능과 동일합니다. 추가 배포 단계를 수행하지 않아도 xbWatson을
    사용할 수 있습니다.

-   Visual Studio를 사용하여 크래시 덤프를 캡처할 수도 있습니다. 디버그
    메뉴에서 \"다른 이름으로 덤프 저장\...\"을 찾습니다. 이 옵션은
    프로세스에 연결하면 표시되고, 일시 중지("모두 중단")하면
    활성화됩니다.

# 알려진 문제

MiniDumpWriteDump를 호출하기 전에 GENERIC_WRITE 및 GENERIC_READ를 둘 다
사용하여 파일을 열어야 합니다. 그러지 않으면 결과로 생성되는 .dmp 파일이
손상될 수 있습니다.

# 업데이트 기록

초기 릴리스 2019년 4월
