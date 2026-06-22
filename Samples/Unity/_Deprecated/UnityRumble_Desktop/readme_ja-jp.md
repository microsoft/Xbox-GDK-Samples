  ![](./media/image1.png)

#   デスクトップの Unity GDK Rumble のサンプル

*\* このサンプルは、Win 10 上の Unity 2020.3.12f1 および Microsoft GDK
Feb QFE2 2021 で開発されました。*

# 

# 説明

このサンプルは、Microsoft GDK ダウンロード ポータル
ページに一般的に含まれる NetRumble
サンプルのポートです。これは、次のように表示されます。

![A picture containing text Description automatically generated](./media/image3.png)

これは、Xbox Live、PlayFab、PlayFab Party API
を示す単純なマルチプレイヤー
ゲームです。開発者はこれを使用して、次の機能を実行します。

-   Xbox Live サービスおよび PlayFab サービスにログインする

-   Xbox Live フレンドのプロフィール情報を取得する

-   MPM を使用して Xbox Live マルチプレイヤー セッションを開始する

-   マルチプレイヤー セッションに参加するよう Xbox Live
    フレンドを招待する

-   フレンドの Xbox Live マルチプレイヤー セッションに参加する

-   SmartMatch マルチプレイヤー セッションに Xbox Live
    ユーザーをマッチメイキングする

-   ボイス チャットをサポートするパーティー ネットワークに参加する

-   信頼性の高いメッセージングと信頼性の低いメッセージングを使用してゲームプレイを行う

# 

# サンプルのビルド

このサンプルは、Feb 2021 Microsoft GDK のインストールおよび Unity
バージョン 2020.3.12f1 に対して開発およびテストされました。
このサンプルはいくつかの SDK に依存しています。これらの SDK
には、スナップショットが zip 圧縮ファイルで提供されており (SDKs/
フォルダーにあります)、サンプルの Assets/
フォルダーに解凍してサンプルに必要な SDK
の依存関係を満たすことができます。 サンプルは、"SampleScene.unity"
という名前の 1 つのシーンによってのみ構成されており、これは
Assets/Sample/Scenes フォルダーにあります。

サンプルのビルドに必要なすべての Unity、Xbox Live、PlayFab
構成は、すべての SDK が解凍され、サンプルが Unity
に読み込まれた時点で設定される必要があります。 サンプルは、\[ビルド\]
ボタンを使用して Unity IDE 内でビルドする必要があります。

![Graphical user interface Description automatically generated](./media/image4.png)

... このサンプルでは、現在、"ビルド"
フォルダーが提供されており、ビルド出力が次のように "Builds/PC/Loose"
という名前のフォルダーに入ることが想定されています。

![Graphical user interface Description automatically generated with medium confidence](./media/image5.png)

"MicrosoftGame.config" ファイルと 5 つの PNG ファイルは、"GdkMetadata"
フォルダー内にあります。このフォルダーは、GDK
アプリケーションに必要なサポート ファイルを提供しています。

Unity がビルド プロセスを完了し、メタデータ
ファイルがビルド出力フォルダーにコピーされたら、アプリケーションを
Windows に \"サイド ロード\" します。これには 2 つの方法があります。

1.  "ゲーミング VS 2019 コマンド プロンプト" を使用すると、サンプル
    ルート フォルダーから "wdapp register Builds\\PC\\Loose"
    コマンドを実行できます。

2.  "ゲーミング VS 2019 コマンド プロンプト"
    を使用すると、"Builds\\package.bat"
    スクリプトを実行して、フォローアップ コマンド \"wdapp install
    Builds\\PC\\Package\\\<アプリ ID の名前\>.MSIXVC\"
    を使用してパッケージ ビルドを作成できます。

# サンプルの実行

ビルド、パッケージ化、開発 Windows 10 PC
にアプリを正常に読み込むことができたら、Windows 10 のスタート メニューに
"UnityRumbleGDK" という名前のアプリ アイコンが表示されます。 アプリ
アイコンをクリックすると、サンプルはサイズ変更できない 1920 x 1080 の HD
ウィンドウで起動します。

正常に実行するには、PC を Xbox Live サンドボックス "XDKS.1"
(または独自のサンドボックス)
で実行していることが重要です。タイトルはここで構成およびデプロイされています。
"XBLPCSandbox" という名前の GDK
ツールはアクティブなサンドボックスを切り替えるのに役立ちます。これは、ゲーミング
VS 2019 コマンド プロンプトから実行されます。
サンドボックスにログインできるテスト ユーザーのバッチも必要です。

**パートナー センターでの UnityRumbleGDK のゲームの概要**

![A screenshot of a computer Description automatically generated](./media/image6.png)

上のスクリーンショットは、パートナー センターの UnityRumbleGDK のゲーム
タイトルの現在の状態を示します。
タイトルが完全に構成、パッケージ化、デプロイ、および XDKS.1
サンドボックスに発行されています。 タイトルに、マルチプレイヤー
セッション テンプレート 3 つと SmartMatch
ホッパーが構成されていることがわかります。

**マルチプレイヤー セッション テンプレート**

![Text Description automatically generated](./media/image7.png)

**LobbySessionTemplate**

{

\"constants\": {

\"system\": {

\"version\":1,

\"maxMembersCount\":5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": true

},

\"memberInitialization\": {

\"membersNeededToStart\":1

}

},

\"custom\": {}

}

}

一般的なマルチプレイヤー マネージャーのシナリオでは、GDK
のドキュメントを参照してください。ロビー用、ゲームプレイ用、マッチメイキングされたゲームプレイ用の、少なくとも
3 つのセッション テンプレートが保持されています。

ロビー用テンプレートでは、ロビーに入室することのできる参加者が 5
人に制限されます。 招待プロトコルは、ゲーム
アプリケーションに招待を付与する必要があることを指定します。
メンバーには接続性も要求されますが、これは RTA (リアルタイム
アクティビティ) サービスを通して管理されます。

また、Microsoft
は、サンプルが検索機能を示さない場合でも、セッションを検索可能または公開することを決定しています。
最後に、ロビー セッションを開始するために必要なメンバーの人数は厳密に 1
人で、このユーザーがセッションを開始するホストとなります。

**GameSessionTemplate**

{

\"constants\": {

\"system\": {

\"version\":1,

\"maxMembersCount\":5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

}

},

\"custom\": {}

}

}

\<blah\>.

**MatchSessionTemplate と SmartMatch ホッパー**

{

\"constants\": {

\"system\": {

\"version\":1,

\"maxMembersCount\":5,

\"visibility\": \"open\",

\"inviteProtocol\": \"game\",

\"capabilities\": {

\"gameplay\" : true,

\"connectivity\": true,

\"connectionRequiredForActiveMembers\": true,

\"crossPlay\": true,

\"userAuthorizationStyle\": true,

\"searchable\": false

},

\"memberInitialization\": {

\"membersNeededToStart\":2

}

},

\"custom\": {}

}

}

![A screenshot of a computer Description automatically generated with medium confidence](./media/image8.png)

\<blah\>.

次のいくつかのセクションでは、サンプルの UI を 1
画面ずつ説明し、正常に動作する GDK
開発環境でそれらの画面で期待される機能について説明します。

## スタート画面のサンプル

![Graphical user interface, website Description automatically generated](./media/image9.png)

上のスクリーンショットは、サンプルを起動したときに表示されることが予想される画面を示します。
\[開始\] ボタンは、Xbox Live と PlayFab のユーザー ログイン
フローを起動します。 ログイン手順のいずれかに失敗すると、エラー
メッセージが画面の下部に表示され、ユーザーはスタート画面にとどまります。

一般的な障害状況として、ユーザーが現在のサンドボックスにアクセスできないか、インターネット接続が現在ダウンしているか、Xbox
Live サービスまたは PlayFab
サービスで何らかのサービスの中断が発生していることが挙げられます。

## 

## サンプルのメイン メニュー画面

![A screenshot of a computer Description automatically generated with low confidence](./media/image10.png)

テスト ユーザーが Xbox Live と PlayFab
の両方に正常にサインインしたら、サンプルのメイン
メニュー画面が表示されます。 表示される 3
つの主要な機能は次のとおりです。

1.  SmartMatch
    を使用して、ユーザーをタイトルを実行している他のユーザーと対戦させる。

2.  ユーザーにゲーム ロビー セッションをホストさせる。

3.  フレンドの現在アクティブなゲーム
    ロビーへの参加を試みる、またはテスト
    ユーザーが既に受け取って承認した招待に関連付けられているセッションに直接参加する。

上記の 3
つの機能のいずれかの実行に失敗すると、画面の下部に表示され、ユーザーはメイン
メニュー画面にとどまります。

## 

## 

## フレンドのロビー画面に参加する

![A screenshot of a computer Description automatically generated with low confidence](./media/image11.png)

メイン メニューから \[フレンドに参加\]
ボタンを選択するとき、フレンドのロビーのボタンが最大 3
つ表示されます。メイン メニューに戻るオプションも表示されます。 Unity
Rumble ゲーム
ロビーを実行しているフレンドがいない場合は、空のリストとともにフレンドのロビーが見つからなかったことを示すメッセージが表示されます。

## 

## 

## SmartMatch 画面を探す

![A screenshot of a computer Description automatically generated with low confidence](./media/image12.png)

メイン メニューから \[マッチを探す\] ボタン
オプションを選択するとき、サンプルはすぐに Multiplayer Manager API
によって管理されるマッチメイキング
チケットを使用してマッチメイキングを試みます。 通常、マッチメイキング
チケットでは、次の 2 つの一般的な結果のいずれかが得られます。

1.  マッチメイキング チケットが処理され、マッチメイキング
    セッションが作成され、(以前に記載されたマッチ セッション
    テンプレートを使用して)
    一緒にマッチメイキングされたすべてのメンバーが新しいセッションに配置されます。

2.  マッチメイキング チケットが処理されず、タイムアウトします。
    この結果になると、エラーが画面の下部に表示され、使用可能なメイン
    メニュー オプションが再び有効になります。

マッチメイキング
チケットは、まだアクティブである場合はキャンセルすることもできます。
マッチメイキング チケットをキャンセルすると、メイン メニュー
オプションはマッチメイキング要求が失敗した場合と同じように再び有効になります。

## ゲーム ロビーの画面

![Graphical user interface, website Description automatically generated](./media/image13.png)

テスト ユーザーが正常にマッチメイキングされたか、\[フレンドに参加\]
または \[招待に参加\] メイン メニュー
オプションを使用してフレンドのゲーム
ロビーに正常に参加したか、自分のセッションのホストを正常に開始したら、ゲーム
ロビー画面がユーザーに表示されます。

UI 画面には、ロビー セッション内に存在するメンバーが表示されます。
\[退出\]
オプションでは、ユーザーがロビーにいることを中止します。ユーザーが選択した場合は、画面の左側に表示されているロビー
メンバーの一覧から削除されます。

\[準備\] ボタンの上にある船と色のアイコン
ボタンで、ユーザーはゲームでプレイする船のスタイルと色を選択できます。ここで選択した内容は、その他のロビー
メンバーと同期され、ロビー
メンバーの一覧でゲーマータグの隣に表示されます。
ユーザーのゲーマータグの左側のアイコンは、セッションのホストとその
\"準備\" 状態を示します。 すべてのメンバーが \[準備\]
ボタンの隣で準備状態を \[オン\]
にしたら、ロビーで画面の下部でカウントダウンの起動を開始します。

## ホストがフレンドをロビーに招待する

![Graphical user interface, website Description automatically generated](./media/image14.png)

テスト ユーザーが \[ホスト ゲーム\] メイン メニュー
オプションを使用してゲーム ロビーをホストしている場合、\[招待\]
ボタンがゲーム ロビー画面に表示されます。 \[招待\]
ボタンを選択すると、ロビー
セッションへの招待を送ることのできるフレンドの一覧を表示するシェル UI
画面がユーザーに表示されます。

バックグラウンドでは、フレンドの選択とサーバー側の招待プロトコルのしくみを処理するために招待
API が呼び出されます。
招待がキャンセルされない場合、受信者のユーザーは、アプリ内かどうかに関係なく、招待が送信され、招待を承諾または無視できることを通知するシェル
UI の通知を受信します。
承諾されると、まだ実行していない場合はアプリが起動し、メイン メニューで
\[招待に参加\] の新しいボタンが有効になります。

## \[フレンドによる招待の受信\] 画面

![A screenshot of a computer Description automatically generated with low confidence](./media/image15.png)

![A screenshot of a computer Description automatically generated with medium confidence](./media/image16.png)

前述したとおり、ホスト
ユーザーから送信された招待を承諾するとき、招待が承諾されるとメイン
メニュー画面に \[招待に参加\] が表示されます。

**\[ロビーの準備完了\] 画面**

![Graphical user interface, website Description automatically generated](./media/image17.png)

ロビーのすべてのメンバーが \[準備\] 切り替えを \"準備完了\"
にしたら、ゲームを起動する準備ができたことをユーザーに知らせるよう求めるメッセージがホストに表示されます。
この時点で、セッション
ネットワークは同期に使用され、ロビー画面の下部にカウントダウン
タイマーが表示されます。

ユーザーがアプリを終了すると、そのメンバーはゲームから削除され、セッション
ネットワークに表示されなくなります。
カウントダウンが完了すると、ユーザーはゲームのプレイ画面に入ります。

****

**ゲームのプレイ画面**

![Graphical user interface, application Description automatically generated](./media/image18.png)

ゲームのプレイ画面では、アクティブな参加者のメンバーが左側に表示されます。これには、メンバーのゲーマータグの横の
\"キル\" と \"デス\" の回数が含まれます。 ホストの名前の左に表示される
Xbox アイコンによって、ホストが示されます。 ユーザーが \[ゲームを終了\]
ボタンを使用して途中でゲームを終了すると、他のすべてのメンバーの一覧から非表示になります。

プレイヤーのキルが合計 5 回に達したら、ゲームは自然に終了します。
この時点で、ユーザーは、プレイヤーがゲームを終了することを選択した場合と同じようにメイン
メニューに戻ります。

# 実装上の注意

"Assets/Sample/Scripts" フォルダーの下にあるサンプルのスクリプト
コードは、主に UI/ビュー関連のコードとコア ロジックに分類されます。 コア
ロジックでは、Xbox
Live/PlayFab/ネットワーキングに固有のロジックをゲームプレイや他の一般的なロジック
ビットから区別するため、コードがさらに分類されます。

-   ログイン、フレンド、マルチプレイヤなどのための Xbox Live 機能は
    Assets\\Sample\\Script\\Logic\\XboxLive にあります

-   ログインのための PlayFab 機能は Assets\\Sample\\Logic\\PlayFab
    にあります

-   ネットワーキングのための機能とセッションのドキュメントは
    Assets\\Sample\\Script\\Logic\\Session にあります

-   Unity の GameCore パッケージを使用するには、存在する定義の代わりに
    "USE_UNITY_GAMECORE" と "UNITY_GAMECORE"
    を定義するようにしてください。

# 重要!

サンプルと一緒に SDKS\\ フォルダーに用意されている SDK
スナップショットは、サンプルの実行に役立てることだけを目的としており、公開されるタイトルの出荷に適した
\"正式の\" SDK バージョンとして扱われるべきでは**ありません**。
必ずタイトル固有の SDK バージョンの開発に適した最新の QFE
を使用してください。

# 既知の問題

\"既知\" の問題とはやや異なりますが、サンプルは、サンプルの "SDKs"
フォルダー内に用意されている SDK
スナップショットに対して開発およびテストされました。 Unity の GameCore
パッケージを使用して Microsoft Unity GDK
プラグインの新しいバージョンを統合するか、PlayFab
の別のバージョンを統合すると、Unity プラグインで廃止される可能性のある
API
との非互換性が明らかになったり、動作の特性が変更されたりする可能性があります。

# 

# 更新の履歴

| 説明                        |  リリース日         |  バージョン       |
|-----------------------------|--------------------|------------------|
| サンプルの Readme の最初のドラフト。 ビ ルドの要件、使用状況の詳細 、メモや問題が含まれます。 |  2021 年 3 月 22 日  |  1.0 |
| Unity 2020.3 LTS と Feb 2021 QFE2 GDK で実行する ようサンプルを更新します。 |  2021 年 6 月 18 日  |  1.1 |

# 

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
