![](./media/image1.png)

# Xbox 에이전트 서버 샘플

## 설명

이 샘플에서는 Devkit Agent에서 하트비트 요청을 받는 웹 서비스를 보여줍니다. 이 샘플은 기능 면에서 단순하고 선택한 콘솔에 대한 하트비트 내용을 표시하도록 설계되었습니다. 제공된 UI를 사용하여 대상 개발 키트에 작업 요청을 보내고 작업이 처리될 때 하트비트 요청 업데이트를 볼 수 있습니다.

여러 개발 키트가 샘플 서버를 대상으로 지정할 수 있어 각 개발 키트에 작업을 한 번에 하나씩 실행할 수 있습니다.

개발 키트 에이전트 기능에 대한 자세한 내용은 [Devkit Agent 개요](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview)를 참조하세요.


## 샘플 빌드 및 구성

샘플을 빌드하려면 Visual Studio 2022에서 솔루션을 로드한 다음, 다음과 같이 XboxAgentServerSample\Properties\launchSettings.json 및 XboxAgentServerSample\appsettings.json을 수정합니다.

- `[server computer name]` 자리 표시자를 샘플을 실행하는 PC의 로컬 네트워크 이름으로 바꿉니다.

- HttpsInlineCertStore에서 SSL 인증서에 적합한 URL 및 인증서 설정을 구성합니다.

> [!NOTE]
> 자체 서명된 인증서가 지원되며 이 예제에서는 CurrentUser/My store를 사용하여 인증서를 저장합니다.

## 필수 인증서

샘플을 실행하려면 다음 인증서가 필요합니다.

- 개발 키트에서 HTTPS 트래픽을 사용할 수 있도록 서버에 퍼블릭 키 및 프라이빗 키가 설치된 SSL 인증서.  자체 서명된 인증서일 수 있지만 인증서의 이름과 주제는 컴퓨터의 네트워크 이름이어야 합니다. 그렇지 않으면 SSL 트래픽에 대해 신뢰받지 못합니다.
- 서버에 대한 HTTPS 호출을 사용하도록 설정하기 위해 개발 키트에 설치할 SSL 인증서의 퍼블릭 키 인증서입니다. xs:\Microsoft\Cert에 복사
- 신뢰 당사자에 대해 개발 키트에서 보낸 XSTS 토큰의 암호 해독을 사용하도록 설정하기 위해 서버에 퍼블릭 키 및 프라이빗 키가 설치된 신뢰 당사자 인증서입니다.

> [!NOTE]
> 서버에서 신뢰할 수 있는 루트 SSL 인증서를 사용하는 경우 개발 키트에 SSL 공용 인증서를 설치할 필요가 없습니다.

서버에서 SSL 및 신뢰 당사자 퍼블릭 및 프라이빗 키 인증서를 모두 `Certificates - Current User\Personal` 인증서 저장소에 설치해야 합니다. 이렇게 하면 서버가 Visual Studio 2022 및 Kestrel에서 실행하는 동안 인증서에 액세스할 수 있습니다.

파트너 센터에 신뢰 당사자가 아직 구성되어 있지 않은 경우 [이 문서](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-web-services.html#corepa)를 참조하세요.

### 자체 서명된 SSL 인증서 만들기

관리자 권한 PowerShell 명령 프롬프트에서 다음 명령을 실행합니다. 자리 표시자 텍스트를 샘플이 호스트되는 컴퓨터 이름으로 바꿉니다.

> ``` New-SelfSignedCertificate -CertStoreLocation Cert:\CurrentUser\My -DnsName "[server computer name]" -FriendlyName "[server computer name]" -NotAfter (Get-Date).AddYears(10) ```

<a id="configureFirewall"></a>
### 샘플에 대한 트래픽을 허용하도록 PC에서 방화벽 구성

관리자 액세스 권한이 있는 Powershell 명령 프롬프트를 열고 다음 명령을 실행합니다. 자리 표시자 텍스트를 사용 중인 SSL 인증서의 지문으로 바꿉니다.

> ```netsh advfirewall firewall add rule name="XboxAgentServerSample" dir=in protocol=tcp localport=8733 action=allow```

> ```netsh http add urlacl url=https://+:8733/ user=Everyone```

> ```netsh http add sslcert ipport=0.0.0.0:8733 certhash=[ssl certificate thumbprint] ```

<a id="configureAgent"></a>
### 샘플과 통신하도록 개발 키트 구성

다음 명령은 VS 게임 명령 프롬프트 창에서 사용되며 개발 키트가 하트비트에 대한 샘플을 대상으로 지정하도록 합니다. 자리 표시자 텍스트를 샘플이 호스트되는 컴퓨터 이름과 개발 키트에서 샘플로 권한을 부여하는 데 사용할 신뢰 당사자 이름(예: rp://relyingparty.contoso.com/)으로 바꿉니다.

> ``` xbconfig DevkitAgentServiceUri=https://[server computer name]:8733/api ```
> ``` xbconfig DevkitAgentRelyingParty=[your relying party name] ```

개발 키트에 SSL 퍼블릭 키 인증서를 설치하려면 VS 게임 명령 프롬프트 창에서 다음 명령을 사용하여 복사해야 합니다.

> ``` xbcp [path to .cer file] xs:\Microsoft\Cert ```


## 샘플 사용

샘플을 올바르게 구성하면 서버에 하트비트를 보내도록 구성된 개발 키트가 Xbox 개발 키트 헤더 아래에 표시됩니다.

각 개발 키트의 이름을 클릭하여 가장 최근 HeartbeatRequest에 대한 세부 정보를 확인하거나 오른쪽 패널의 옵션 및 단추를 사용하여 개발 키트에 작업을 실행합니다.

> [!NOTE]
> 개발 키트에서 하트비트를 보낼 때 페이지가 자동으로 새로 고쳐지지 않습니다.  새로 고치려면 왼쪽 위에서 새로 고침 단추를 클릭합니다.


콘솔에 작업을 실행하는 경우 이는 [콘솔 기반 명령줄 도구](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/consolecommandlinetools)에 자세히 설명된 대로 콘솔 명령 작업입니다. 몇 가지 간단한 예는 [wdapp list](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp), [wdapp launch](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp) 및 [wdconfig sandboxid](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdconfig)입니다.

개발 키트 에이전트에서 사용할 수 있는 작업 및 자세한 내용은 [Devkit Agent 개요](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview)를 참조하세요.

## 문제 해결

문제를 해결하는 가장 좋은 방법 중 하나는 DevKit Agent의 로깅 수준을 자세한 정보 표시(xbconfig DevkitAgentDesiredLogLevel=verbose)로 설정하고 xbWatson을 사용하여 xsts 토큰을 가져오고, 하트비트를 보내는 devkit agent의 트래픽과 SSL 인증 흐름에서 가능한 인증서 문제를 확인하는 것입니다.

### 웹 서비스가 실행 중이며 PC에서 HTTPS 트래픽을 수신할 수 있는지 확인합니다.

서버가 Visual Studio 2022에서 컴파일하고 실행할 수 있는지 확인합니다. 서버가 실행되면 웹 브라우저를 열고 localhost가 아닌 PC 이름을 사용하여 샘플의 홈페이지로 이동합니다.
> ``` https://[server computer name]:8733 ```

서버의 홈페이지가 표시되지 않으면 Kestrel 및 방화벽이 제대로 구성되지 않은 것일 수 있습니다. [샘플과 통신하도록 devkit agent 구성](#configureFirewall)에서 구성 명령을 실행해 보세요.

또한 동일한 네트워크의 다른 PC에서 웹 브라우저를 사용하여 샘플에 액세스하려고 합니다.

### 개발 키트에서 서버로 나가는 트래픽 확인

[Xbox 개발 키트의 Fiddler](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/fiddler-setup-networking)에 설명한 대로 개발 키트에서 Fiddler 추적을 활성화합니다. Fiddler가 실행 중일 때 개발 키트에서 서버로 가는 하트비트 트래픽을 검사합니다. 샘플을 실행하는 PC의 URL에 대한 호출이 Fiddler에 표시되지 않는 경우 [샘플과 통신하도록 devkit agent 구성](#configureAgent)에서 명령을 다시 실행해 보세요.

### 개발 키트가 샘플에 대한 HTTPS 호출을 완료할 수 있는지 확인

개발 키트가 샘플 서버를 호출할 때 SSL 핸드셰이크를 완료하지 못하지만 Fiddler를 통해 개발 키트 트래픽을 실행할 때는 작동하는 경우 다음과 같은 문제가 있을 수 있습니다.

- SSL 인증서의 `subject` 또는 `issued to` 값이 개발 키트가 하트비트를 보내는 데 사용하는 URL에 있는 PC의 호스트 이름과 일치하지 않음.
- 서버에서 사용되는 SSL 인증서가 [샘플과 통신하도록 devkit agent 구성](#configureAgent)의 개발 키트에 설치된 공용 인증서와 일치하지 않음.

먼저 웹 브라우저를 열고 localhost가 아닌 PC의 이름을 사용하여 샘플의 홈페이지로 이동하여 서버에 대한 HTTPS 호출이 수행될 때 사용되는 인증서를 확인합니다.
> ``` https://[server computer name]:8733 ```

홈페이지가 표시되면 브라우저 주소 표시줄 또는 `View site information`에서 자물쇠 아이콘을 확인합니다. 웹 브라우저마다 이 데이터에 액세스하는 방법이 다르므로 사용 중인 브라우저에서 이 데이터를 찾는 방법을 확인하세요. 서버에서 제공하는 SSL 인증서 정보를 가져올 수 있는 경우 이름과 지문을 확인합니다. 지문이 개발 키트에 복사하는 .cer 파일의 지문과 일치하는지 확인합니다. 또한 인증서의 발급 대상 값이 devkit agent가 사용하는 URL의 호스트 이름과 일치하는지 확인합니다.

예: 샘플 서버에 대한 URL은 https://mylocalPC:8733입니다.  'mylocalPC'에 발급된 인증서만 https 트래픽에 대해 개발 키트에서 수락합니다.

### 서버가 하트비트에서 보낸 XSTS 토큰의 암호를 해독할 수 있는지 확인합니다.

개발 키트에서 샘플로 보낸 하트비트에서 HTTP 403 오류가 발생했다면 전송 중인 XSTS 토큰을 서버에서 확인할 수 없음을 나타냅니다.

먼저 [devkit agent 구성 명령을 실행할](#configureAgent) 때 올바른 신뢰 당사자 이름을 제공했는지 확인합니다. 다시 부팅 후 콘솔에서 fiddler 호출을 확인하여 요청 본문에 신뢰 당사자 이름이 있는 XSTS에 대한 호출이 토큰과 함께 돌아오는지 확인할 수도 있습니다.

둘째, 샘플이 PC에 저장된 신뢰 당사자의 개인 인증서 키에 액세스할 수 있는지 확인합니다. Visual Studio에서 실행되는 서버의 디버그 출력을 보면 발생한 결과를 나타내는 로그 또는 경고가 표시됩니다. 또는 ValidateAuthorizationHeaderWithCache API에 중단점을 배치하고 단계별로 실행하여 오류가 발생하는 위치를 확인할 수 있습니다. 샘플에서 XSTS 토큰 헤더의 지문과 일치하는 인증서를 찾을 수 없다고 알리는 경우 다음을 확인해야 합니다.

- XSTS 토큰의 지문과 일치하는 인증서의 프라이빗 키가 PC에 설치되어 있습니다.
- 인증서 및 프라이빗 키가 `Certificates - Current User\Personal` 인증서 저장소에 설치되었습니다.

## 구현 참고 사항

이 샘플은 이식 가능하도록 설계되고 ASP.NET로 작성되었으며 교차 플랫폼 [Kestrel Web Server](https://learn.microsoft.com/en-us/aspnet/core/fundamentals/servers/kestrel?view=aspnetcore-7.0)를 사용합니다.

## 알려진 문제

홈페이지가 자동으로 새로 고쳐지지 않고, 최신 하트비트 요청을 확인하려면 페이지 새로 고침 단추를 사용해야 합니다.

## 업데이트 기록

| **날짜** | **Version** | **Description** |
|---|---|---|
| 2023년 10월 18일 | 1.0 | 최초 릴리스 |

## 개인정보처리방침

[Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)에서 일반적인 Microsoft의 개인 정보 취급 방침에 대한 내용을 자세히 알아보세요.

