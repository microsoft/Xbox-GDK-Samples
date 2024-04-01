![](./media/image1.png)

# フランチャイズ ゲーム ハブのサンプル

このサンプルは Microsoft GDK と互換性があります (2023 年 10 月)

# 説明

フランチャイズ ハブは、パブリッシャーが関連タイトルのキュレーションされた組織を提供する方法で、ユーザーがゲーム間で共有エクスペリエンスを維持する方法で提供する方法です。 関連タイトルを取得、インストール、および起動するための中心的なポイントとして機能し、関連するタイトルが使用できる永続的なローカル記憶域スペースをホストします。

このサンプルでは、ユーザー エクスペリエンスに必要な多くの操作を実装する方法を示します。 3 つのプロジェクトで構成されます。
- 関連するタイトルと各タイトルに対して実行できる操作を示すゲーム ハブ製品 (GameHub)
- ゲーム ハブを起動する必要があり、ハブによって書き込まれた共有データを読み取ることができるハブ対応製品 (RequiredGame)
- ゲーム ハブを起動する必要のないハブ非対応製品 (RelatedGame)。これは古いタイトルを表しますが、それ以外の場合はゲーム ハブに関連付ける必要がありますが、古い GDK に付属しているため、ゲーム ハブ固有のフィールドと API を利用することはできません

![](./media/image3.png)

# サンプルのビルド

Xbox Series X|S 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Windows PC を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Desktop.x64` に設定します。コードをコンパイルする必要がありますが、主に本体の機能であり、一部の機能が機能しない可能性があることに注意してください。

*詳細については、**GDK ドキュメント*の「__サンプルの実行__」 を参照してください。

# サンプルの実行

このサンプルは、XDKS.1 サンドボックスで動作するように構成されています。 購入とインストールの操作には、テスト アカウントが必要です。 すべての @xboxtest.com テスト アカウントは、XDKS.1 サンドボックスで動作する必要があります。

![](./media/screenshot1.jpg)

一番上の行には、ゲーム ハブに関連するゲームが表示されます。この場合、2 つあります。 各タイトルを選択すると、関連する各アドオンが以下にリスト表示されます。 各パッケージがインストールされているゲームまたは DLC の場合は、それぞれを購入できます。

インストールが完了すると、各ゲームを起動できます。 ハブ対応ゲームでは、共有 PLS に書き込まれたファイルの内容を表示し、ハブに戻すことができるようになります。

![](./media/screenshot2.jpg)


ハブに対応しないゲームは起動することしかできません。それ以外の場合は、ハブからのゲームであることを認識できません。

これはすべて、Visual Studio デバッグを通じて実行されるゲーム ハブで実行できますが、各プロジェクトをパッケージ化するためのスクリプトを使用できます。
- `makepcpkg.cmd`
- `makescarlett.cmd`
- `makexboxone.cmd`

パッケージが作成されたら、`xbapp install` を使用してそれぞれをインストールします。これにより、Microsoft Store から取得してインストールしなくても、ローカルでビルドされたパッケージとのやり取りが可能になります。

これは、ローカル ビルド (配置またはパッケージ化) を Microsoft Store ビルドに更新できないため、更新をテストする際に特に重要です。 この目的のために、UpdateTesting ディレクトリで別のスクリプト セットを使用できます。
- `buildpackages.cmd`: 関連するゲームのパッケージの v100 バージョンと v200 バージョンをビルドし、それぞれに DLC をビルドします
- `installandstageupdates.cmd`: `xbapp install` 各パッケージの v100 と `xbapp update /a` v200。更新プログラムの可用性をシミュレートします。

その結果、ゲームは各更新プログラムが利用可能であることを反映し、更新フローを有効にする必要があります。

![](./media/screenshot3.jpg)


# 実装メモ

`XPackageEnumeratePackages` は、`ThisAndRelated` スコープと `ThisPublisher` スコープの両方で同じ結果を返すように見えます。 違いを確認するには、XDKS.1 サンドボックスで利用可能な他のサンプルのいずれかをインストールします。たとえば、 InGameStore、DownloadableContent。

GameHub に関連する RelatedGame (ハブ非対応) の作成方法は、GameHub の microsoftgame.config 内の `RelatedProduct` ノードを使用しています。

GameHub と RequiredGame をそれぞれの microsoftgame.config で照合することによって、GameHub `FranchiseGameHubId` に関連する RequiredGame `AssociatedFranchiseGameHubId` (ハブ対応) を作成する方法。

主な違いは、ハブに対応しないゲームを GameHub で参照されるタイトルとして含めるために再発行する必要はありませんが、ハブ対応のゲームは最初からシリーズ ゲーム ハブ のシナリオで作成されるということです。 これは、ハブ対応ゲームが戻ることができる titleId を認識しているため、GameHub に `XLaunchUri` 戻ることができる理由でもあります。

2023 年 10 月の回復では利用できない UI の変更が行われます。この変更により、シリーズ ゲーム ハブの動作がさらに示されます。つまり、ハブ対応ゲームはマイ ゲームに表示されず、ゲーム ハブと組み合わせてのみインストールおよび起動できます。

ゲーム ハブと関連するゲームが同じ GDK を使用して構築されるという要件はありません。 RelatedGame は、2023 年 10 月より前の GDK でビルドするように設定できます。 RequiredGame と GameHub は、2023 年 10 月の新機能である API フィールドと microsoftgame.config フィールドに依存しているため、使用できません。

`XStoreEnumerateProductsQuery` が発生している場合、`XStoreProductsQueryHasMorePages` が呼び出されないのは意図的なものです。 最初に、このサンプルにはごく少数の製品が含まれます。 次に、2023 年 10 月に、`XStoreQueryAssociatedProducts` または `XStoreQueryAssociatedProductsForStoreId` に渡される maxItems を展開するだけで、タイトルの予想される製品数を含め、すべての製品が Result 関数のクエリ ハンドルに返されます。 コールバックは、すべての製品が列挙されたときにのみヒットします。もちろん、多くの製品を含むタイトルには時間がかかる場合があります。

# 既知の問題

PC の機能をプロトタイプと見なします。このサンプルは本体を対象としています。

ハブがオフラインの場合は DLC を区別できません (`XStoreQueryAssociatedProductsForStoreId` はオフラインでは機能しません)。

`XStoreQueryGameAndDlcPackageUpdates`、または `XStoreQueryPackageUpdates` は複数の ID で渡された場合、`xbapp update` ステージングが使用されている場合は、利用可能な更新プログラムが一貫して返されません。

`XStoreDownloadAndInstallPackageUpdates` を使用して更新プログラムをインストールして `XPackageCreateInstallationMonitor` で監視している場合は、`XPackageGetInstallationProgress` は `completed` = true を途中で返します。

# 更新履歴

**初期リリース:** 2023 年 10 月

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


