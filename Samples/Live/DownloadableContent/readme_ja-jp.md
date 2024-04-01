![](./media/image1.png)

# ダウンロード コンテンツ (DLC) のサンプル

このサンプルは Microsoft GDK と互換性があります (2023 年 6 月)

# 説明

このサンプルでは、XPackage および XStore API を使用してダウンロード コンテンツ (DLC) の購入、ダウンロード、列挙、読み込みを実装する方法を示します。 DLL と EXE を含む DLC も示します。

![](./media/image3.png)

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

Windows PC を使用する場合は、アクティブなソリューション プラットフォームを Gaming.Desktop.x64 に設定します

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」 を参照してください。

# サンプルの実行

このサンプルは、XDKS.1 サンドボックスで動作するように構成されています。

画面の左側に、インストールされているパッケージが表示されます。 これらの操作は、DLC に含まれる内容に応じて使用できます。
- マウント: これによりライセンスが取得され、DLC コンテンツがファイルシステムにマウントされます
   - DLCPackage: 画像が表示されます
   - DLCDlPackage: これにより、ComboDLL プロジェクトがビルドする DLL で見つかった API が呼び出されます
   - DLCExePackage: DLC に含まれている exe が起動します
- マウント解除: DLC パッケージのマウント解除
- アンインストール: DLC パッケージをアンインストールする

サンプルが XDKS.1 で実行されている場合、右側に使用可能な DLC アドオンの一覧が表示されます。 アイテムを選択すると、アカウントが所有していない場合は購入 UI が表示されます。その場合は、アイテムを選択してパッケージをダウンロードします。 完了すると、パッケージが左側の一覧に表示されます。 これは、DLC がストアから購入され、パッケージが CDN からインストールされる実際のリテール フローを最もよく表します。

| 操作 | キーボード | ゲームパッド |
|---|---|---|
| パッケージの選択 | 上方向キーと下方向キー | 方向パッドの上下 |
| ローカル パッケージまたはストア パッケージを切り替える | 方向キー (左右) | 方向パッド (左右) |
| パッケージのマウントまたはマウント解除 (左側の列) パッケージの購入またはダウンロード (右側の列) | Enter キー | A button |
| XPackageEnumeratePackages の種類とスコープを切り替える | Page Up/Page Down | LB/RB |
| パッケージのアンインストール | X | X ボタン |
| 列挙されたパッケージを更新する | Y | Y ボタン |
| デバッグ ウィンドウの切り替え | OemTilde | Menu button |
| Exit | Esc | View button |

# 製品の設定方法

この製品の Store ID は 9NQWJKKNHF1L です。

Xbox のストア ページにアクセスするには、ゲーム のコマンド プロンプトで

`xbapp launch ms-windows-store://pdp/?productid=9NQWJKKNHF1L`

Windows `msxbox://game/?productId=9NQWJKKNHF1L` でで

![](./media/image4.jpeg)

9NQWJKKNHF1L には、使用可能なプラットフォームのパッケージの一般的な組み合わせを表す 3 つのアドオンが含まれています。

- 9P96RFVJQ562 には、Xbox Series、Xbox One GDK、PC 用のパッケージが含まれています

- 9PPJJCWPCWW4 には、Xbox One ERA パッケージが含まれています

- 9PGPGLSPSN3V には、Xbox One GDK パッケージと PC が含まれています

Scarlett 開発キットで実行されているサンプルは Scarlett DLC (9P96RFVJQ562) パッケージに対応している必要があり、ストアからインストールされるパッケージには \_xs サフィックスが必要です。 Xbox One 開発キットで実行されているサンプルは、3 つのパッケージすべてにアクセスできる必要があります。9P96RFVJQ562 の場合、パッケージには \_x サフィックスが代わりに含まれます。 PC で実行されているサンプルは、9P96RFVJQ562 および 9PGPGLSPSN3V パッケージにのみアクセスできる必要があります。

ストアからインストールされたサンプルは、適切にライセンスされ、正しく機能しますが、古いバージョンのサンプルを表している可能性があります。

# ローカル パッケージを使用して実行する

このサンプルは、ストアから DLC パッケージをダウンロードしてインストールした状態で実行できますが、一般的な開発では、ローカルで DLC コンテンツを繰り返し使用することになります。 これを実現するには、いくつかの方法があります。 詳細については、「**ダウンロードコンテンツの管理とライセンス**」というタイトルの GDK ドキュメントを参照してください。

同じサンプルと DLC のパッケージ バージョンを生成するために使用されるスクリプト ファイルがいくつか含まれています。 サンプル (ベース ゲーム) makepcpkg、makexboxonepkg、makecarlettpkg では、それぞれのパッケージが作成されます。 スクリプトは、パートナー センター上の 9NQWJKKNHF1L 用に送信されたパッケージに関連付けられた正しい contentID を使用してパッケージをビルドします。

DLC の場合、いくつかの DLC が示されています。

- DLCPackage: Scarlett GDK、Xbox One GDK、および Xbox One ERA DLC
- DLCPackagePC: PC DLC
- DLCDllPackage: ComboDLL を含むコンソール用 GDK DLC
- DLCDllPackagePC: ComboDLL を含む PC 用 GDK DLC
- DLCExePackage: AlternateExperience.exe を含むコンソール用の GDK DLC
- DLCExePackage: AlternateExperience.exe を含む PC 用 GDK DLC

AlternateExperience と ComboDLL は SimpleMultiExePackage サンプルから派生しています。

Scarlett パッケージは \_xs.xvc で終了します

Xbox One (GDK) パッケージは \_x.xvc で終了します

Xbox One (XDK) パッケージには拡張機能がありません

PC パッケージは .msixvc で終了します

それぞれの中には、各プラットフォームの DLC パッケージを生成する makedlcpkg コマンドがあります。 ルート ディレクトリ内の BuildAllDlc.cmd は、すべての種類の DLC を生成します。

インストールするには、Xbox に **xbapp install** を使用するか、PC 用 **wdapp install**、または使用可能な同等のツールを使用します。 この構成では、インストールされている DLC はすべて左側に表示され、マウント可能である必要があります。

また、ルース ファイルを使用して完全に実行することもできます。 これを実現するには、Xbox に **xbapp deploy** を使用するか、PC で **wdapp register** を使用し、MicrosoftGame.config が配置されているディレクトリを渡します (例:

`xbapp deploy .\DLCPackage\Package_Scarlett`

`wdapp register .\DLCPackagePC\Package`

組み合わせ可能である必要があります: パッケージ ベース ゲーム + ルース DLC;loose ベース ゲーム + パッケージ DLC、ルース ベース ゲーム + Store DLC など。ただし、特定の組み合わせに関する問題については、「既知の問題」セクションを参照してください。

# 既知の問題

# 更新履歴

**更新プログラム:** 2023 年 6 月

DLC で EXE/DLL をサポートします。

**更新プログラム:** 2022 年 3 月

PC で DLC をデモンストレーションするための DLCPackagePC フォルダーを追加しました。

ライセンスが失われた場合のクラッシュを修正しました。

**更新プログラム:** 2022 年 6 月

XPackageMount API を XPackageMountWithUiAsync API に変更します。

XPackageUninstallPackage API を追加します。

**更新プログラム:** 2022 年 7 月

エラー処理を修正しました。

**初期リリース:** 2019 年 4 月

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


