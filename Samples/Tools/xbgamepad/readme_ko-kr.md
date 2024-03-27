# xbgamepad

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

최소 win32 콘솔 앱으로, 로컬로 연결된 XINPUT 디바이스(예: Xbox 게임 패드)에서 입력을 받아 이러한 입력을
XDK 및 GDK와 함께 제공되는 라이브러리로 보냅니다.
| | |
|---|---|
|XTF(Xbox 도구 프레임워크)를 사용하는 Xbox One 또는 Xbox Series X|S 개발 키트|

# 실행 중

Xtfdlls 폴더에 있는 dll이 있는 경우 이를 사용하기 때문에 GDK가 설치되지 않은 컴퓨터에서 실행할 수 있습니다. 도구에는 Windows 8 이상이 필요합니다.

# 사용법

```
xbgamepad /x:<devkit ipv4 address> [/r:<update rate in hz - default is 30>]
```


TCP 포트 4211 및 4212를 통해 개발 키트에 액세스해야 합니다.

# 필수 구성 요소 빌드

- Visual Studio 2019(16.11) 또는 Visual Studio 2022

- Windows&nbsp;10 SDK

- XTF 헤더 및 라이브러리에 대한 최근 GDK 설치(환경 변수 GameDK를 포함 및 링커 입력의 XDK 버전으로 변경하여 XDK를 사용하도록 프로젝트를 수정할 수 있습니다)

# 배포

GDK가 설치되지 않은 컴퓨터에서 실행하려면 기존 GDK 설치에서 xbtp.dll 파일과 xtfinput.dll 파일을 복사해야 합니다. `%GameDK%\bin`에 있습니다. 설치된 GDK 없이 컴퓨터에 xbgamepad.exe와 나란히 배치할 수 있습니다.


