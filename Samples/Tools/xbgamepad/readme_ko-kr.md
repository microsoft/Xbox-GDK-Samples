# xbgamepad

*이 도구는 Microsoft 게임 개발 키트 미리 보기(2020년 2월)와 호환됩니다.*

# 설명

이 도구는 로컬로 연결된 XINPUT 디바이스(예: Xbox One 게임패드)에서
입력을 받아 이를 XDK 및 GDK와 함께 제공되는 XTF(Xbox Tools Framework)
라이브러리를 이용해 Xbox One 또는 Xbox Series X devkit에 전달하는 최소
win32 콘솔 앱입니다.

# 실행

이 도구는 xtfdll 폴더에 있는 dll을 사용하므로(있는 경우) GDK가 설치되지
않은 컴퓨터에서 실행할 수 있습니다. 이 도구를 사용하려면 Windows 8
이상이 필요합니다.

# 사용

*xbgamepad /x:\<devkit ipv4 address\> \[/r:\<update rate in hz - default
is 30\>\]*

TCP 포트 4211 및 4212를 통한 devkit 액세스가 필요합니다.

# 빌드 필수 구성 요소

-   Visual Studio 2017

-   Windows 10 SDK

-   XTF 헤더 및 라이브러리에 대한 최신 GDK 설치(환경 변수 GameDK를 포함
    및 링커 입력의 XDK 버전으로 변경하여 XDK를 사용하도록 프로젝트를
    수정할 수 있음)

# 배포

GDK가 설치되지 않은 컴퓨터에서 실행하려면 기존 GDK 설치에서 xbtp.dll 및
xtfinput.dll 파일을 복사해야 합니다. 이 파일은 %GameDK%\\bin에 있습니다.
GDK가 설치되지 않은 컴퓨터에서 xbgamepad.exe와 나란히 파일을 설치할 수
있습니다.
