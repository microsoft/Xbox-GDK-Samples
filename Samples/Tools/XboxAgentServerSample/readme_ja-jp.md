![](./media/image1.png)

# Xbox Agent Server サンプル

## 説明

このサンプルでは、Devkit エージェントからハートビート要求を受信する Web サービスを示します。 サンプルは、その機能がシンプルで、選択したコンソールのハートビートの内容を表示するように設計されています。 指定された UI を使用して、ジョブ要求をターゲット開発キットに送信し、ジョブが処理されるとハートビート要求の更新を確認できます。

複数の開発キットでサンプル サーバーをターゲットにできるため、各開発キットに一度に 1 つずつジョブを発行できます。

開発キット エージェント機能の詳細については、「[DevKit エージェントの概要](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview)」を参照してください。


## サンプルのビルドと構成

サンプルをビルドするには、Visual Studio 2022 でソリューションを読み込み、次のように XboxAgentServerSample\Properties\launchSettings.json と XboxAgentServerSample\appsettings.json を変更します。

- `[server computer name]` プレースホルダーを、サンプルを実行している PC のローカル ネットワーク名に置き換えます。

- HttpsInlineCertStore で、SSL 証明書に必要に応じて URL と証明書の設定を構成します。

> [!注]
> 自己署名証明書がサポートされており、この例では CurrentUser/My ストアを使用して証明書を格納します。

## 必要な証明書

サンプルを実行するには、次の証明書が必要です:

- サーバーに公開キーと秘密キーがインストールされた SSL 証明書を使用して、開発キットで HTTPS トラフィックを有効にします。  これは自己署名証明書でもかまいませんが、証明書の名前とサブジェクトはマシンのネットワーク名である必要があります。それ以外の場合は SSL トラフィックに対して信頼されません。
- サーバーへの HTTPS 呼び出しを有効にするために、開発キットにインストールする SSL 証明書の公開キー証明書。 xs:\Microsoft\Cert にコピーする
- サーバーに公開キーと秘密キーがインストールされている証明書利用者証明書を使用して、証明書利用者の開発キットからの XSTS トークンの暗号化解除を有効にします。

> [!注]
> サーバーがルート信頼された SSL 証明書を使用している場合は、SSL 公開証明書を開発キットにインストールする必要はありません。

サーバーでは、SSL と証明書利用者の両方の公開キー証明書と秘密キー証明書を `Certificates - Current User\Personal` 証明書ストアにインストールする必要があります。 これにより、Visual Studio 2022 と Kestrel で実行中にサーバーがアクセスできるようになります。

パートナー センターで証明書利用者がまだ構成されていない場合は、[この記事](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/live-web-services.html#corepa)を参照してください。

### 自己署名 SSL 証明書の作成

管理者特権の PowerShell コマンド プロンプトから、次のコマンドを実行します。 プレースホルダー テキストを、サンプルがホストされているコンピューター名に置き換えます。

> ``` New-SelfSignedCertificate -CertStoreLocation Cert:\CurrentUser\My -DnsName "[server computer name]" -FriendlyName "[server computer name]" -NotAfter (Get-Date).AddYears(10) ```

<a id="configureFirewall"></a>
### サンプルへのトラフィックを許可するように PC 上のファイアウォールを構成する

管理者アクセス権を持つ Powershell コマンド プロンプトを開き、次のコマンドを実行します。 プレースホルダー テキストを、使用している SSL 証明書の拇印に置き換えます。

> ```netsh advfirewall firewall add rule name="XboxAgentServerSample" dir=in protocol=tcp localport=8733 action=allow```

> ```netsh http add urlacl url=https://+:8733/ user=Everyone```

> ```netsh http add sslcert ipport=0.0.0.0:8733 certhash=[ssl certificate thumbprint] ```

<a id="configureAgent"></a>
### サンプルと通信するように開発キットを構成する

次のコマンドは、VS Gaming コマンド プロンプト ウィンドウで使用され、開発キットでハートビートのサンプルをターゲットにします。 プレースホルダー テキストを、サンプルがホストされているコンピューター名と、開発キットがサンプルで承認するために使用する証明書利用者名 (例: rp://relyingparty.contoso.com/) に置き換えます。

> ``` xbconfig DevkitAgentServiceUri=https://[server computer name]:8733/api ```
> ``` xbconfig DevkitAgentRelyingParty=[your relying party name] ```

開発キットに SSL 公開キー証明書をインストールするには、VS Gaming コマンド プロンプト ウィンドウで次のコマンドを使用してコピーする必要があります。

> ``` xbcp [path to .cer file] xs:\Microsoft\Cert ```


## サンプルの使用方法

サンプルを適切に構成すると、サーバーにハートビートを送信するように構成されている開発キットが Xbox 開発キットヘッダーの下に表示されます。

各開発キットの名前をクリックすると、右側のパネルのオプションとボタンを使用して、最新の HeartbeatRequest またはイシュー ジョブに関する詳細が表示されます。

> [!注]
> 開発キットからハートビートが送信されると、ページは自動的に更新されません。  更新するには、左上の [更新] ボタンをクリックします。


コンソールにジョブを発行する場合、[コンソール ベースのコマンド ライン ツール](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/consolecommandlinetools)で詳しく説明されているように、これらはコンソール コマンド ジョブです。 簡単な例としては、[wdapp list](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp)、[wdapp launch](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdapp)、[wdconfig sandboxid](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/wdconfig) などがあります。

開発キット エージェントで使用できる詳細と考えられる操作については、「[DevKit エージェントの概要](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/devkitagent-overview)」を参照してください。

## トラブルシューティング

問題のトラブルシューティングに最適な方法の 1 つは、DevKit エージェントのログ レベルを verbose (xbconfig DevkitAgentDesiredLogLevel=verbose) に設定し、xbWatson を使用して、xsts トークンの取得、ハートビートの送信、SSL 認証フローで発生する可能性のある証明書の問題に関して、開発キット エージェントからのトラフィックを確認することです。

### Web サービスが実行中であり、PC から HTTPS トラフィックを受信できることを確認する

サーバーが Visual Studio 2022 でコンパイルおよび実行できることを確認します。 サーバーが実行されたら、Web ブラウザーを開き、localhost ではなく PC の名前を使用してサンプルのホーム ページに移動します。
> ``` https://[server computer name]:8733 ```

サーバーのホームページが表示されない場合は、Kestrel とファイアウォールが正しく構成されていない可能性があります。 「[サンプルと通信するように開発キット エージェントを構成する](#configureFirewall)」の構成コマンドを実行してみてください。

また、同じネットワーク上の別の PC の Web ブラウザーを使用してサンプルにアクセスすることもできます。

### 開発キットからサーバーへの送信トラフィックを確認する

「[Xbox 開発キットの Fiddler](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/fiddler-setup-networking)」で説明されているように、開発キットで Fiddler トレースを有効にします。 Fiddler を実行している状態で、開発キットからサーバーに送信されるハートビート トラフィックを確認します。 サンプルを実行している PC の URL に対して Fiddler に呼び出しが表示されない場合は、「[サンプルと通信するように開発キット エージェントを構成する](#configureAgent)」でコマンドをもう一度実行してみてください。

### 開発キットがサンプルへの HTTPS 呼び出しを完了できることを確認する

開発キットがサンプル サーバーを呼び出し、SSL ハンドシェイクの完了に失敗しているが、Fiddler を介して開発キット トラフィックを実行するときに機能する場合は、次の問題が発生する可能性があります:

- SSL 証明書の `subject` または `issued to` 値が、開発キットがハートビートの送信に使用している URL 内の PC のホスト名と一致しません。
- サーバーで使用されている SSL 証明書が、「[サンプルと通信するように開発キット エージェントを構成する](#configureAgent)」で開発キットにインストールされている公開証明書と一致しません

まず、Web ブラウザーを開いてサーバーへの HTTPS 呼び出しが行われるときに使用されている証明書を確認し、localhost ではなく PC の名前を使用してサンプルのホーム ページに移動します。
> ``` https://[server computer name]:8733 ```

ホーム ページが表示されたら、ブラウザーのアドレス バーまたは `View site information` のロック アイコンをチェックします。Web ブラウザーによって、このデータを取得するための方法が異なります。使用しているブラウザーでこれを検索する方法を参照してください。 サーバーによって提示されている SSL 証明書情報をプルできる場合は、名前と拇印をチェックします。 拇印が、開発キットにコピーする .cer ファイル内の拇印と一致していることを確認します。 さらに、証明書の [発行先] の値が、開発キット エージェントが使用している URL 内のホスト名と一致していることを確認します。

例: サンプル サーバーへの URL が https://mylocalPC:8733。  'mylocalPC' に発行された証明書のみが、https トラフィックに対して開発キットによって受け入れられます。

### サーバーがハートビートで送信された XSTS トークンの暗号化を解除できることを確認する

HTTP 403 エラーが発生するサンプルへの開発キットからのハートビートは、サーバーが送信されている XSTS トークンを確認できないことを示します。

まず、[開発キット エージェント構成コマンドを実行](#configureAgent)するときに、適切な証明書利用者名が指定されていることを確認します。 再起動後にコンソールからの fiddler 呼び出しを調べて、要求本文で証明書利用者名を持つ XSTS への呼び出しがトークンで返されることを確認することもできます。

次に、サンプルが PC 上の証明書利用者の秘密証明書キーにアクセスできることを確認します。 Visual Studio で実行されているサーバーからのデバッグ出力を見ると、何が起こったかを示すログまたは警告が表示されます。 または、ValidateAuthorizationHeaderWithCache API にブレークポイントを配置し、それをステップ実行してエラーが発生している場所を確認することもできます。 XSTS トークン ヘッダーで拇印と一致する証明書が見つからないという問題がサンプルで表示される場合は、次の点を確認してください:

- XSTS トークンの拇印と一致する証明書の秘密キーが PC にインストールされている
- 証明書と秘密キーは `Certificates - Current User\Personal` 証明書ストアにインストールされます。

## 実装メモ

このサンプルは移植性が高く、ASP.NET で記述され、クロス プラットフォーム [Kestrel Web Server](https://learn.microsoft.com/en-us/aspnet/core/fundamentals/servers/kestrel?view=aspnetcore-7.0) を使用するように設計されています。

## 既知の問題

ホーム ページは自動的に更新されません。最新のハートビート要求を表示するには、[ページの更新] ボタンを使用します。

## 更新履歴

| **日付** | **バージョン** | **説明** |
|---|---|---|
| 2023 年 10 月 18 日 | 1.0 | 最初のリリース |

## プライバシーに関する声明

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。

