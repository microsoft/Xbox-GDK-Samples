  ![](./media/image1.png)

#   ダウンロード コンテンツ (DLC) のサンプル

*Windows PC:このサンプルは、Microsoft GDK (2020 年 6 月)
と互換性があります*

*Xbox One/Xbox Series X|S:このサンプルは、Microsoft GDKX (2020 年 6 月)
と互換性があります。*

# 

# 説明

XPackage API および XStore API を介してダウンロード
コンテンツの入手、ダウンロード、列挙、およびロードを実装する方法について説明します。

![ビデオゲームの画面のスクリーンショット 低い精度で自動的に生成された説明](./media/image3.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブ ソリューション
プラットフォームを Gaming.Xbox.XboxOne.x64 に設定します。

Xbox Series X|S Dev Kit を使用している場合は、アクティブ ソリューション
プラットフォームを Gaming.Xbox.Scarlett.x64 に設定します。

Windows PC を使用している場合は、アクティブ ソリューション
プラットフォームを Gaming.Desktop.x64 に設定します。

*詳細については、GDK
のドキュメントの*「サンプルの実行」*を参照してください。*

# サンプルの実行

このサンプルは XDKS.1
のサンドボックスで動作するように構成されていますが、ライセンス
モードの同サンドボックスで実行することは厳密には必要ありません。

画面の左側に、インストールされているパッケージが表示されます。ライセンスの確認が含まれているパッケージのマウント/マウント解除ができます。パッケージが正常にマウントできれば、サンプルにパッケージの画像が表示されます。この機能はライセンスを持たずに動作しますが、以下で説明する
DLC パッケージのローカル インストールが必要になります。

Microsoft Store 製品 (以下を参照) の権限を持つテスト
アカウントを使用して XDKS.1 でサンプルを実行すると、右側に使用可能な
Durable アドオンの一覧が表示されます。Purchase UI
は、アカウントで所有していない場合、アイテムを選択することによって表示され、
次に、所有している場合は、アイテムを選択してパッケージをダウンロードします。完了すると、パッケージが左側の一覧上に表示されます。これによって、DLC
が Microsoft Store から購入され、パッケージが CDN
からインストールされる実際のリテール フローが最も厳密に表されます。

| 操作                       |  キーボード         |  ゲームパッド       |
|----------------------------|--------------------|--------------------|
| パッケージの選択  |  上 /下方向の矢印キー |  上/下方向の D パッド             |
| ローカル パッケージまたはストア パッケージの切り替え |  左 /右方向の矢印キー  |  左/右方向の D パッド |
| パッケージの マウントまたはマウント解除 (左側の列) Purchase またはダウンロード パッケージ (右側の列) |  入力  |  A ボタン |
| XPackageEnumeratePackages の種類とスコープの切り替え  |  ページ アップ/ページ ダウン |  LB ボタン/RB ボタン |
| 列挙したパッケージの Refresh |  Y  |  Y ボタン |
| 終了                       |  Esc キー           |  ビュー ボタン      |

XStore API
を機能させるには、有効なライセンス、および特定の構成アクションを適用する必要があります。詳細については「**XStore
の開発とテストを有効にする**」というタイトルの GDK ドキュメント
セクションを参照してください。

これが正しく実行されない場合、XStore API
では、有効なライセンスが検出されなかったことを示す 0x803f6107
(IAP_E\_UNEXPECTED) を返します。

# 製品の設定方法

この製品の Store ID は 9NQWJKKNHF1L です。

Microsoft Store ページにアクセスするには、ゲーミング コマンド
プロンプトの使用から

`xbapp launch ms-windows-store://pdp/?productid=9NQWJKKNHF1L`

or just `ms-windows-store://pdp/?productid=9NQWJKKNHF1L` on Windows.

![ロゴ が含まれている画像 自動的に生成された説明](./media/image4.jpeg)

これが書き込まれた時点で、9NQWJKKNHF1L
には、次の使用可能なプラットフォームのパッケージの一般的な組み合わせを表す
3 つのアドオンが含まれます。

-   9P96RFVJQ562 には、Xbox Series、Xbox One GDK および PC
    用のパッケージが含まれます

-   9PPJJCWPCWW4 には、Xbox One ERA のパッケージが含まれます

-   9PGJRLSPSN3V には、Xbox One GDK のパッケージと PC が含まれます

Scarlett 開発キットで実行しているサンプルは、 Scarlett DLC
(9P96RFVJQ562) パッケージに対応している必要があり、Store
からインストールされるパッケージには拡張子 \_xs
付いていることが必要です。Xbox One 開発キットで実行しているサンプルは、3
つのパッケージすべてにアクセスできる必要があり、 9P96RFVJQ562
の場合、パッケージには代わりに拡張子 \_x が付けられます。PC
で実行しているサンプルは、9P96RFVJQ562 パッケージおよび 9PGJRLSPSN3V
パッケージにのみアクセスできる必要があります。

サンプルはストアからのインストール時に、正しくライセンスが付与され、適切に機能しますが、旧バージョンのサンプルが表される場合があります。

# ローカル パッケージを使用した実行

このサンプルは、Microsoft Store からダウンロードおよびインストールした
DLC パッケージを使用して実行できますが、一般的な開発には、ローカルでの
DLC
コンテンツの反復処理が含まれます。これを遂行するにはいくつかの方法があります。詳細については、「**ダウンロード
コンテンツの管理およびライセンス権限付与**」というタイトルの GDK
ドキュメントを参照してください。

同じものにはサンプルおよび DLC のパッケージ
バージョンを生成するために使用されるいくつかのスクリプト
ファイルが含まれています。サンプル (すなわち、ベース ゲーム)
makepcpkg、makexboxonepkg、makecarlettpkg
の場合は、個別にパッケージが作成されます。このスクリプトによって、Partner
Center の 9NQWJKKNHF1L 用に送信されたパッケージに関連付けられた適切な
contentID を持つパッケージがビルドされます。

ゲーム パッケージを For サイドローディングするには、EKBID
をオーバーライドする必要があります。

`xbapp setekbid 41336MicrosoftATG.ATGDownloadableContent_2022.3.8.0_neutral\_\_dspnxghe87tn0 {00000000-0000-0000-0000-000000000001}`

Xbox One と Scarlett の場合、適切な **TargetDeviceFamily** ノードを
Gaming.Xbox.\*.x64\\Layout\\Image\\Loose\\MicrosoftGame.config
に挿入することが必要で、そうしなければ、makepkg
はエラーを示すことに注意してください。

```xml
<ExecutableList>
    <Executable Name="DownloadableContent.exe"
        Id="Game"
        TargetDeviceFamily="Scarlett"/\>
</ExecutableList>
```

DLC の場合、DLCPackage
ディレクトリには、以下のための必要なファイルがすべて含まれています

1.  Scarlett GDK DLC (\_xs.xvc)

2.  Xbox One GDK DLC (\_x.xvc)

3.  Xbox One ERA DLC (拡張機能なし)

Xbox に適しており、DLCPackagePC には、PC .msixvc
に必要なファイルが含まれています。

それぞれの中には、各プラットフォームの DLC パッケージを生成する
makedlcpkg コマンドがあります。

これらを使用すると、ゲームと DLC のパッケージ
ビルドを作成できます。インストールするには、Xbox の **xbapp
インストール、**または PC 用の **wdapp
インストール**、もしくは使用可能な同等のツールを使用します。この構成では、サンプル自体がライセンス
モードで実行されていない場合でも、インストール済みの DLC
はすべて左側に表示され、マウント可能である必要があります。

また、ルース
ファイルを使用して完全に実行することもできます。これを達成するには、Xbox
の **xbapp デプロイ、**または PC の **wdapp レジスタ**
を使用し、例として、MicrosoftGame.config
が配置されているディレクトリにパスします。

`xbapp deploy .\\DLCPackage\\Package_Scarlett`

`wdapp register .\\DLCPackagePC\\Package`

組み合わせ可能である必要があります: パッケージ ベース ゲーム + ルース
DLC、loose ベース ゲーム + パッケージ DLC、ルース ベース ゲーム +
Microsoft Store DLC
など。ただし、ある特定の組み合わせに関する問題については、「既知の問題」セクションを参照してください。

# 既知の問題

# 更新履歴

**初回リリース:** 2019 年 4 月

**Update:**2022 年 3 月

PC で DLC を作成するデモ用の DLCPackagePC フォルダーを追加しました。

ライセンスが失われた際のクラッシュが Fixed しました。

# プライバシーに関する声明

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプル実行ファイルのファイル名が
Microsoft に送信されます。このデータ
コレクションからオプトアウトするには、Main.cpp の「Sample Usage
Telemetry」というラベルの付いたコードのブロックを削除します。

全般的な Microsoft のプライバシー ポリシーの詳細については、「[Microsoft
プライバシー
ステートメント](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。
