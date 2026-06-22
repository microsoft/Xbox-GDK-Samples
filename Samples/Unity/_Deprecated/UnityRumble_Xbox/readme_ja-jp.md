  ![](./media/image1.png)

#   Scarlett/XboxOne Unity GDK Rumble サンプル

*\* このサンプルは、Win 10 上で Unity 2020.3.12f1 および Microsoft GDK
Feb QFE2 2021 を使用して開発されました。*

# 

# 説明

このサンプルは、Microsoft GDK のダウンロード ポータル
ページに共通して掲載されている NetRumble
のサンプルを移植したもの、下記のような内容になっています。

![A picture containing text Description automatically generated](./media/image3.png)

これは、Xbox Live、PlayFab、PlayFab Party API
の使用例を示す単純なマルチプレイヤー
ゲームです。これを使用して、開発者は次のような機能を実行することができます。

-   Xbox Live サービスや PlayFab サービスへログインする

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

このサンプルは、Feb 2021 Microsoft GDK のインストール環境および GameCore
用 Unity バージョン 2020.3.12f1 に向けて開発、テストされています。
このサンプルはいくつかの SDK に依存しています。これらの SDK
には、スナップショットが zip 圧縮ファイルで提供されており (SDKs/
フォルダーにあります)、サンプルの Assets/
フォルダーに解凍してサンプルに必要な SDK
の依存関係を満たすことができます。 サンプルは、"SampleScene.unity"
という名前の 1 つのシーンのみで構成されており、これは
Assets/Sample/Scenes フォルダーにあります。

サンプルのビルドに必要なすべての Unity、Xbox Live、PlayFab
構成は、すべての SDK を解凍し、サンプルを Unity
にロードした時点で完了しています。 サンプルは、\[ビルド\]
ボタンを使用して Unity IDE 内でビルドする必要があります。

![Graphical user interface Description automatically generated](./media/image4.png)

\... サンプルでは現在、選択したビルド
フォルダーの下にルース展開フォルダーを配置しています。例えば「Builds/Scarlett」なら、展開フォルダは「Builds/Scarlett/Loose」（または「Builds/XboxOne/Loose」）のようになります。

![A picture containing graphical user interface Description automatically generated](./media/image5.png)

「MicrosoftGame.config」ファイルと 5 つの PNG ファイルは、Scarlett
および XboxOne の構成設定で指定されており、GDK
アプリケーションに必要なサポート ファイルを提供します。

Unity によってビルド プロセスが完了し、メタデータ
ファイルがビルド出力フォルダーに置かれた後、アプリケーションを
Scarlett/XboxOne キットに「サイドロード」する方法は 2 つあります。

1.  「ゲーム VS 2019コマンド プロンプト」で、サンプルのルート
    フォルダから "xbapp deploy Builds\\Scarlett\\Loose"
    コマンドを実行できます。

2.  「ゲーム VS 2019 コマンドプロンプト」で \"makepkg\"
    コマンドを実行してパッケージビルドを作成し、後続のコマンド \"xbapp
    install Builds\\\<path_to_package\>\\\<name_of_app_identity\>.XVC\"
    を使用してインストールできます (\"makepkg\" については GDK
    のドキュメントを参照)

# サンプルの実行

開発用の Scarlett または XboxOne
キットにアプリを正常にビルドし、場合によってはパッケージ化し、サイドロードできた場合は、「UnityRumbleGDK.exe」
という名前のアプリ アイコンが \[Dev Home\] メニューに表示されます。 Xbox
Manager またはキット自体を使ってアプリを起動すると、スプラッシュ
スクリーンが表示され、その後にサンプルが全画面表示されます。

正常に実行するには、開発キットが、タイトルが構成され展開されている Xbox
Live サンドボックス \"XDKS.1\" (または独自のサンドボックス)
で実行されていることが重要です。 Xbox Manager
ツールは、このキットでアクティブなサンドボックスを切り替えるのに役立ちます。
サンドボックスにログインできる一連のテスト ユーザーも必要です。

**パートナー センターでの UnityRumbleGDK のゲームの概要**

![A screenshot of a computer Description automatically generated](./media/image6.png)

上のスクリーンショットは、パートナー センターでの UnityRumbleGDK
のゲーム タイトルの現在の状態を示します。
タイトルが完全に構成、パッケージ化、展開され、XDKS.1
サンドボックスに公開されています。 タイトルに、マルチプレイヤー
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

一般的な Multiplayer Manager シナリオの場合、詳細については GDK
のドキュメントを参照してください。ロビー用、ゲームプレイ用、マッチメイキングされたゲームプレイ用の、少なくとも
3 つのセッション テンプレートが用意されています。

ロビー用テンプレートでは、ロビーに入室することのできる参加者が 5
人に制限されます。 招待の規約で、招待をゲーム
アプリケーションに対して行うことが規定されています。
また、メンバーには接続性が要求されますが、これは RTA (リアルタイム
アクティビティ) サービスを通して管理されています。

サンプルでは検索機能が示されていませんが、Microsoft
はセッションを検索可能、つまり公開されるものとすることを決定しています。
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

以下のセクションでは、サンプルの UI を 1
画面ずつ取り上げ、正常に動作する GDK
開発環境でそれらの画面がどのように機能することが期待されるかを説明します。

## サンプルのスタート画面

![A screenshot of a computer Description automatically generated with medium confidence](./media/image9.png)

上のスクリーンショットは、サンプルを起動したときに表示される最初の画面を想定したものです。
\[開始\] ボタンを選択すると、Xbox Live と PlayFab のユーザー ログイン
フローが開始します。 ログイン手順のいずれかに失敗すると、エラー
メッセージが画面の下部に表示され、ユーザーはスタート画面にとどまります。

一般的な失敗状況としては、ユーザーが現在のサンドボックスにアクセスできない、インターネット接続がダウンしている、Xbox
Live サービスまたは PlayFab
サービスで何らかのサービスの中断が発生している、などが挙げられます。

## 

## サンプルのメイン メニュー画面

![A screen shot of a computer Description automatically generated with low confidence](./media/image10.png)

テスト ユーザーが Xbox Live と PlayFab
の両方に正常にログインすると、サンプルのメイン
メニュー画面が表示されます。 表示される 3
つの主要な機能は次のとおりです。

1.  SmartMatch
    を使用して、自分のユーザーを同じタイトルを実行している他のユーザーと対戦させる。

2.  自分のユーザーでゲームのロビー セッションをホストする。

3.  フレンドの現在アクティブなゲーム
    ロビーへの参加を試みる、またはテスト
    ユーザーが既に受け取って承認した招待に関連付けられているセッションに直接参加する。

上記の 3
つの機能のいずれかの実行に失敗すると、画面の下部に表示され、ユーザーはメイン
メニュー画面にとどまります。

## 

## 

## フレンドのロビーに参加する画面

![A screenshot of a computer Description automatically generated with medium confidence](./media/image11.png)

メイン メニューから \[フレンドに合流\]
ボタンを選択すると、フレンドのロビーのボタンが最大 3 つと、メイン
メニューに戻るオプションも表示されます。 Unity Rumble ゲーム
ロビーを実行しているフレンドがいない場合は、空のリストとともにフレンドのロビーが見つからなかったことを示すメッセージが表示されます。

## 

## 

## SmartMatch の検索画面

![A screenshot of a computer Description automatically generated with low confidence](./media/image12.png)

メイン メニューから \[マッチを探す\] ボタン
オプションを選択すると、サンプルによってすぐに Multiplayer Manager API
で管理されているマッチメイキング
チケットを通じてマッチメイキングを試みます。 マッチメイキング
チケットでは、通常次の 2 つのうちどちらかの一般的な結果となります。

1.  マッチメイキング チケットが処理され、(前述のマッチ セッション
    テンプレートを使用して) マッチメイキング
    セッションが作成されて、マッチメイキングの対象となったすべてのメンバーが新しいセッションに組み入れられます。

2.  マッチメイキング チケットが処理されず、タイムアウトします。
    この結果になると、エラーが画面の下部に表示され、選択できるメイン
    メニュー オプションが再び有効になります。

マッチメイキング
チケットは、まだアクティブである場合はキャンセルすることもできます。
マッチメイキング
チケットをキャンセルすると、マッチメイキング要求が失敗した場合と同じように、メイン
メニュー オプションが再び有効になります。

## ゲームのロビー画面

![A screenshot of a computer Description automatically generated with medium confidence](./media/image13.png)

テスト ユーザーが正常にマッチメイキングされたか、\[フレンドに合流\]
または \[招待に参加\] メイン メニュー
オプションを使用してフレンドのゲーム
ロビーに正常に参加したか、自分のセッションのホストを正常に開始した場合、ゲームのロビー画面が表示されます。

UI 画面には、ロビー セッション内に存在するメンバーが表示されます。
\[退出\]
オプションでは、ユーザーがロビーにいる状態をやめることができます。これを選択した場合、ユーザーは画面の左側に表示されているロビー
メンバーの一覧から削除されます。

\[準備完了\] ボタンの上にある船と色のアイコン
ボタンで、ユーザーはゲームでプレイする船の形と色を選択できます。ここで選択した内容は他のロビー
メンバーと同期され、ロビー
メンバーの一覧でゲーマータグの隣に表示されます。
ユーザーのゲーマータグの左側のアイコンは、セッションのホストとその
\"準備\" 状態も示します。 すべてのメンバーが \[準備完了\]
ボタンを使って準備状態を \[オン\]
にすると、ロビー画面の下部でカウントダウンが開始されます。

## ホストがフレンドをロビーに招待する

![A screenshot of a computer Description automatically generated with low confidence](./media/image14.png)

テスト ユーザーが \[ゲームをホストする\] メイン メニュー
オプションを使用してゲーム ロビーをホストしている場合、\[招待\]
ボタンがゲーム ロビー画面に表示されます。 \[招待\]
ボタンを選択すると、ロビー
セッションへの招待を送ることのできるフレンドの一覧を表示するシェル UI
画面が表示されます。

バックグラウンドでは、フレンドの選択とサーバー側の招待プロトコルのしくみを処理するために招待
API が呼び出されます。
招待がキャンセルされない場合、受信側のユーザーには、アプリ内にいるかどうかに関係なく、招待を受けたことがシェル
UI で通知され、招待を受けるか無視するかを選択できます。
招待を受けた場合、まだ実行していない場合はアプリが起動して、メイン
メニューで \[招待に参加\] の新しいボタンが使用できるようになります。

## フレンドからの招待の受け取り画面

![A screenshot of a computer Description automatically generated with low confidence](./media/image15.png)

![A screen shot of a computer Description automatically generated with low confidence](./media/image16.png)

前述したとおり、ホスト
ユーザーからの招待を承諾するとき、招待が承諾されるとメイン
メニュー画面に \[招待に参加\] が表示されます。

**全員準備完了のロビー画面**

![A screenshot of a computer Description automatically generated with low confidence](./media/image17.png)

ロビーのすべてのメンバーが準備を終えて \[準備完了\]
に切り替えると、ゲームを起動する準備ができたことを全員に知らせるよう求めるメッセージがホストに表示されます。
この時点で、セッション
ネットワークが同期に使用され、ロビー画面の下部にカウントダウン
タイマーが表示されます。

ユーザーがアプリを終了した場合、そのメンバーはゲームから削除され、セッション
ネットワークに表示されなくなります。
カウントダウンが終わると、ユーザーはゲームのプレイ画面に入ります。

****

**ゲームのプレイ画面**

![A screenshot of a computer Description automatically generated with medium confidence](./media/image18.png)

ゲームのプレイ画面では、アクティブな参加メンバーが左側に表示されます。ここには、メンバーのゲーマータグの横に
\"キル\" と \"デス\" の回数も表示されます。
ホストの名前の左側には、ホストであることを示す Xbox
アイコンが表示されます。 \[ゲームを終了\]
ボタンを使用して途中でゲームを終了したユーザーは、他のすべてのメンバーの一覧から消えます。

プレイヤーのキルが合計 5 回に達すると、自然にゲーム終了となります。
この時点で、プレイヤーがゲームを終了することを選択した場合と同じように、ユーザーはメイン
メニューに戻ります。

# 実装上の注意

"Assets/Sample/Scripts" フォルダーの下にあるサンプルのスクリプト
コードは、主に UI/ビュー関連のコードとコア ロジックに分類されます。 コア
ロジックの中では、Xbox
Live/PlayFab/ネットワーキングに固有のロジックをゲームプレイや他の一般的なロジック
ビットから区別するために、コードがさらに分類されます。

-   ログイン、フレンド、マルチプレーヤーなどのための Xbox Live 機能は
    Assets\\Sample\\Script\\Logic\\XboxLive にあります

-   ログイン用の PlayFab 機能は Assets\\Sample\\Logic\\PlayFab
    にあります

-   ネットワーキング用の機能とセッション ドキュメントは
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

# 

# 既知の問題

\"既知\" の問題とはやや異なりますが、サンプルは、サンプルの "SDKs"
フォルダー内に用意されている SDK
スナップショットに対して開発およびテストされています。 Unity の GameCore
パッケージを使用して Microsoft Unity GDK
プラグインの新しいバージョンを統合するか、PlayFab または PlayFab Party
の別のバージョンを統合すると、Unity プラグインで、廃止される可能性のある
API や動作の特性が変更されている API
との非互換性が明らかになる可能性があります。

# 

# 更新履歴

| 説明                        |  リリース日         |  バージョン       |
|-----------------------------|--------------------|------------------|
| サンプルの Readme の初稿。 ビルド の要件、使い方の詳細、注意 事項や問題点について記載。 |  2021 年 3 月 22 日  |  1.0 |
| Unity 2020.3 LTS と Feb 2021 QFE2 GDK で動 作するようサンプルを更新。 |  2021 年 6 月 18 日  |  1.1 |

# 

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
