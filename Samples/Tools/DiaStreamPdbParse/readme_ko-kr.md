![](./media/image1.png)

# DiaStreamPdbParse 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

이 샘플에서는 DIA(디버그 인터페이스 액세스) SDK를 사용하여 특히 예외 중에 생성된 호출 스택에서 타이틀 내의 기호를 확인하는 방법을 보여줍니다. 또한 IStream 인터페이스를 사용하여 디스크에서 PDB 데이터 로드를 제어하는 방법도 보여줍니다. 마지막으로 msdia140-xbox.dll이라는 DIA DLL의 Xbox 버전을 제공합니다.

Xbox 버전의 DIA는 HeapAlloc을 사용하여 호출자에게 반환되는 BSTR 개체를 만듭니다. DIAString이라는 도우미 클래스는 데스크톱과 콘솔 모두에서 올바른 작업을 수행하는 symbolLookup.cpp에 제공되므로 HeapFree를 통해 호출자가 해제해야 합니다. DIAString 클래스는 DIA API를 사용할 때 BSTR에 대한 드롭인 대체로 사용할 수 있습니다.

XBOX에서 BSTR 개체에 HeapAlloc/HeapFree를 사용하는 이유는 SysAllocString/SysFreeString을 통한 표준 BSTR 메모리 관리 메서드를 타이틀에서 사용할 수 없기 때문입니다. DIA 쿼리 함수는 SysAllocString을 통해 할당된 BSTR을 반환하지만 타이틀은 해당 메모리를 해제할 방법이 없습니다.

Xbox 버전의 DIA에는 콘솔에 없는 Shlwapi.dll에 대한 참조도 포함되어 있지 않습니다.

참고: 소매 및 개발 타이틀 환경에서 이 샘플과 함께 제공되는 DIA DLL(msdia140-xbox.dll) 버전을 무료로 사용할 수 있습니다. 그러나 현재 Xbox 버전은 Visual Studio 팀에서 지원하지 않습니다. 문제가 있는 경우 Xbox 포럼을 통해 도움을 요청하세요.

# 샘플 빌드

이 샘플은 다음 플랫폼을 지원합니다.

| 플랫폼 | 참고 |
|---|---|
| Gaming.Desktop.x64 | Visual Studio의 일부로 설치된 DIA 버전 사용 |
| Gaming.Scarlett.xbox.x64<br />Gaming.XboxOne.xbox.x64 | 콘솔에서 msdia140-xbox.dll 사용. 빌드 프로세스의 일부로 콘솔에 자동으로 복사됩니다. |

*자세한 내용은* *GDK 설명서의* __샘플 실행을 참조하세요.__

# 샘플 사용

샘플은 현재 호출 스택을 자동으로 캡처하고, 각 함수에 대한 기호를 확인하고, 결과를 화면에 표시합니다.

# 구현 참고 사항

symbolLookup.cpp의 DumpCallstack 함수는 이 샘플의 주요 진입점입니다. 호출 스택을 구문 분석하기 위해 특정 스레드 또는 `EXCEPTION_POINTERS` 개체에 대한 참조를 사용할 수 있습니다.

symbolLookup.cpp의 맨 위에는 수집할 기호 정보의 양과 유형을 제어하는 몇 가지 정의가 있습니다. 특히 Xbox One Family 콘솔의 회전 드라이브에서 특정 기호 정보를 수집하는 데 비용이 많이 들 수 있습니다.

PdbMemoryStream.cpp/h에는 IStream에서 파생된 IPdbMemoryStream이 포함되어 있습니다. 이 클래스는 기본 PDB 로드 메서드를 대체하기 위해 DIA에 제공됩니다. 이 클래스를 사용하면 타이틀에 미치는 영향을 최소화하기 위해 읽기 패턴뿐만 아니라 사용되는 메모리 양을 훨씬 더 세밀하게 제어할 수 있습니다.

# 업데이트 기록

2022년 3월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


