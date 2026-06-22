# RemoteConsoleView 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

이 샘플 도구는 WPF 앱에서 VideoStreamingControl을 사용하여 Xbox One 관리자와 비슷한 방식으로 콘솔 콘텐츠를 PC에 원격으로 표시하는 방법을 보여 줍니다.

샘플을 실행할 때 콘솔의 이름(또는 IP 주소)을 입력하고 연결 단추를 클릭하면 콘솔 디스플레이가 렌더링됩니다.

앱을 처음 실행할 때 방화벽 프롬프트가 표시될 수 있습니다. 앱이 원격 콘솔에 액세스하려면 네트워크 액세스 권한이 필요합니다.

사용자 고유의 WPF 프로젝트에서 이 컨트롤을 사용하려면 다음을 수행하세요.

- 프로젝트가 64비트로 빌드되었는지 확인합니다(컨트롤은 64비트 버전에서만 사용할 수 있음).

- `C:\Program Files (x86)\Microsoft GDK\bin\Microsoft.Xbox.Tools.RemoteVideo.dll`에 대한 참조를 추가합니다(GDK 설치와 함께 제공됨).

- 이 파일을 프로젝트: `C:\Program Files (x86)\Microsoft GDK\bin\xtfremotevideo.dll`에 추가하고 *출력 디렉터리에 복사*가 *새 버전이면 복사*로 설정되어 있는지 확인합니다. 컨트롤이 작동하려면 exe 옆에 이 파일이 위치해야 합니다.

- 컨트롤에 XAML 참조를 추가합니다. 자세한 내용은 샘플을 참조하세요.

## VideoStreamingControl

컨트롤은 매우 간단하게 사용할 수 있습니다. *Source* 속성을 콘솔 이름(또는 IP 주소)으로 설정하면 연결되는 즉시 시작됩니다(*자동 실행*의 기본값 True로 설정). *Stop* 메서드가 있으며 *Volume* 및 *IsMuted* 속성을 통해 볼륨을 제어할 수 있습니다. 오류가 발생하면 *Status* 및 *StatusMessage* 속성을 바인딩할 수 있습니다. 이 컨트롤은 렌더링에 *HWndHost*를 사용하므로 에어스페이스 문제로 인해 다른 WPF 컨트롤을 맨 위에 렌더링할 수 없습니다.

# 업데이트 기록

초기 릴리스 2020년 3월


