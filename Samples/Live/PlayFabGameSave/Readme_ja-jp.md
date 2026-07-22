_このサンプルは October 2025 GDK と互換性があります_

# PlayFab Game Saves

## 説明
PlayFab Game Saves は、PlayFab Services SDK が提供するクラウドベースのセーブデータ管理システムです。プレイヤーはゲームの進行状況をクラウドに保存し、複数のデバイス間でシームレスにアクセスできます。

Game Save Files API は、クラウド同期、データ競合の解決、デバイス管理の複雑さを自動的に処理します。ゲーム開発者はセーブデータのロジックに集中でき、PlayFab がクラウドインフラストラクチャ、ストレージ容量、マルチデバイス調整を管理します。

このサンプルは、PFGameSave- API を使用して GDK で作成した Xbox および Windows ゲームにクラウドセーブ機能を統合する基本的な実装例です。

## PlayFab Game Saves の設定

### PlayFab Manager へのアクセス

最初の設定として、以下の手順を実行してください:

- PlayFab Manager（https://developer.playfab.com/）にアクセスし、該当のタイトルを選択
- 左のメニューで **Progression** > **Game Saves** を選択

**Game Saves にアクセスできない場合:**

- **Waitlist への追加**を選択
- 貴社担当のデベロッパーパートナーマネージャーに以下をお知らせください:
  - Game Saves 機能の利用申請と Waitlist への追加が完了していること
  - PlayFab タイトル ID

**必須設定: Player Creation の無効化**

このサンプルでは、すべてのユーザーが Xbox サインインを通じて PlayFab に認証されることを前提としています。
Xbox 認証と関連付けられていないプレイヤーアカウントが PlayFab 上で自動作成されるのを防ぐため、「Disable Player Creations」配下のすべてのオプションを有効にする必要があります。

（PlayFab Game Manager ＞ Settings ＞ API Features）

この設定は、PlayFab Game Saves を利用するすべてのタイトルで必須です。

## サンプルのビルド

このサンプルは、PlayFab.Services.C 拡張ライブラリと PlayFab Game Saves API を使用します。Visual Studio 2022 で作成されており、Visual Studio 2019 以降でビルド可能です。

**ビルド要件:**
- October 2025 GDK 以降
- Visual Studio 2019 または Visual Studio 2022 以降

## サンプルの使用方法

![サンプルのスクリーンショット](./Assets/screenshot.png)

このサンプルは、サンドボックス **XDKS.1** で実行することを目的としています。 `PFGameSave` API を使用するため、XDKS.1 にサインインできる テスト アカウント をご用意ください。また、Xbox タイトルのタイトルIDだけでなく、PlayFab のタイトルIDが必要になります。 PlayFab のタイトルID は PlayFab Manager で ご確認ください。

タイトルの作成や必要な ID の詳細については、「[PlayFab クイック スタート](https://learn.microsoft.com/ja-jp/gaming/playfab/gamemanager/quickstart)」を参照してください。

**重要**
- ビルド前に、`PlayFabGameSave.h` 内の `PLAYFAB_TITLE_ID` 定数に、自身の PlayFab Title ID を記載してください。

## サンプルの実行

**サンドボックスの設定:**
- Xbox 開発機および Windows のサンドボックスを `XDKS.1` に設定してください

**サインイン:**
- **Windows の場合:**
  - Microsoft Store と Xbox アプリで、`XDKS.1` サンドボックスにサインイン可能なテストアカウントでサインインしてください
- **Xbox 開発機の場合:**
  - Xbox ガイドボタンから`XDKS.1` サンドボックスにテストアカウントでサインインしてください

事前にサインインしておくことで、サンプル起動時に Xbox ネットワークと PlayFab のサインインが自動的に行われます。サインインが完了するまで、すべてのボタンは無効化されています。

## サンプル実装の内容

Xbox 開発機、および Windows のサンドボックスを XDKS.1 にしてください。続いて、Windowsの場合 MicrosoftStore と Xbox アプリでそのサンドボックスにサインインできるテストアカウントでサインイン、Xbox の場合はXbox ガイドボタンなどからテストアカウントでサインインを行ってください。
予めサインインをしておくことでサンプル起動時に、Xbox ネットワークと PlayFab のサインインが自動的に行われます。サインインが完了するまで、すべてのボタンは無効化されています。

**ボタン機能:**

- **Add User:**
  - ユーザーを Game Save システムに登録 (`PFGameSaveFilesAddUserWithUiAsync`)
  - セーブフォルダのパスとクラウドストレージの残容量を取得
  - 既存のセーブデータがあれば自動的にロードしてログに出力
  - アクティブデバイス変更のコールバックを登録

この操作により、クラウド上のセーブデータとの同期が実行されます。
セーブデータの競合、またはアクティブデバイスの競合が検出された場合、Xbox / Windows の標準 UI を用いた解決ダイアログが自動的に表示されます。

- **Data Save:**
  - 512KB のランダムデータを生成し、ローカルに `savegame.dat` として保存

- **Data Load:**
  - ローカルに保存されたセーブデータを読み込み、ログに出力

- **Data Delete:**
  - ローカルのセーブファイルを削除

- **Set Description:**
  - セーブデータにタイムスタンプ付きの説明を設定 (`PFGameSaveFilesSetSaveDescriptionAsync`)
  - 例: `"Saved 2026-02-09 14:30:45"`

- **Upload (Release Device):**
  - ローカルセーブをクラウドにアップロード後、このデバイスを非アクティブ化する
  - アップロード完了後は再度 Add User が必要

- **Upload (Keep Active):**
  - ローカルセーブをクラウドにアップロード後も、このデバイスをアクティブ状態として維持する
  - 継続してセーブデータ操作が可能

**デバイス状態管理:**
- 別のデバイスがアクティブになった場合、`PFGameSaveFilesSetActiveDeviceChangedCallback` によって自動的に通知される
- 状態がリセットされるため、再度 Add User を実行することで操作を再開が可能

## 制限事項
拡張機能ライブラリでは、サービスからのエラーは HRESULT (PFErrors.h で定義) として返されます。 多くの場合、HRESULT コードは、PlayFab が提供する基になるエラー コードほど有益ではありません。 [Fiddler](https://developer.microsoft.com/en-us/games/xbox/docs/gdk/fiddler-setup-networking) などの Web デバッグ ツールを使用して、サービスからの詳細なエラー メッセージを確認することをお勧めします。

## 更新履歴
2026 年 2 月: 初期リリース

## プライバシーに関する声明
サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の「Sample Usage Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。

## 関連ドキュメント / 参考情報
このサンプルは、Xbox / Windows 環境における PlayFab Game Saves の基本的な利用フローを学習するためのサンプルです。  
Game Save の全体像や、本サンプルでは詳細に扱っていない挙動については、以下の公式ドキュメントを参照してください。

- **PlayFab Game Saves 概要**  
  対応プラットフォーム、基本コンセプト、および Game Saves 全体のシステム構成や動作概要について説明しています。  
  https://learn.microsoft.com/gaming/playfab/player-progression/game-saves/overview

- **Game Saves クイックスタート**  
  初期化、クラウド同期、アップロードなど、Game Saves の基本的な導入手順を順を追って解説しています。  
  https://learn.microsoft.com/gaming/playfab/player-progression/game-saves/quickstart

- **セーブデータの競合解決 (Conflicts)**  
  複数デバイスでセーブデータが更新された場合の競合検出方法や、解決フロー、Atomic Unit の考え方について説明しています。  
  https://learn.microsoft.com/gaming/playfab/player-progression/game-saves/conflicts

- **アクティブデバイスの変更 (Active Device Changes)**  
  プレイヤーがセッション中に別のデバイスへ切り替えた場合の挙動や、  
  `PFGameSaveFilesSetActiveDeviceChangedCallback` を用いた対応方法について説明しています。  
  https://learn.microsoft.com/gaming/playfab/player-progression/game-saves/activedevicechanges

- **オフラインモード (Offline Mode)**  
  ネットワーク未接続時における Game Saves の動作、制限事項、およびオンライン復帰時の挙動について説明しています。  
  https://learn.microsoft.com/gaming/playfab/player-progression/game-saves/offline
