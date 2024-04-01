# インストールしないビルド (BWOI) の例

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

個々の開発者には、日常の作業用のコンピューターにコンパイラ ツールセットおよび必要な SDK がインストールされていることが求められます。 *Microsoft Game Development Kit* (GDK) は、ヘッダーとライブラリに加えて、デバッグ、MSBuild プラットフォーム定義、プロファイリング ツール用の Visual Studio 統合を提供しています。 ただし、ビルド サーバーの保守は、日々のビルドを行う際に、ヘッダーとライブラリに 'xcopy-style' 展開を使用できる場合、大幅に簡略化されます。 この例では、Microsoft GDK をインストールせずに **Gaming.\*.x64** プラットフォームを使用して MSBuild ベースのプロジェクトをビルドする方法を示します。 また、Windows コンテナーを使用して分離されたビルド環境を作成するオプションも用意されており、ホスト コンピューターに Visual Studio を直接インストールする必要がありません。

# ソフトウェアのセットアップ

通常、ビルド コンピューターには、定期的に管理されるイメージの一部として Visual Studio ツールセットと Windows SDK がインストールされます。 これは Azure DevOps "[Microsoft-Hosted](https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/hosted?view=azure-devops)" に当てはまり、[セルフホスト Windows エージェント](https://docs.microsoft.com/en-us/azure/devops/pipelines/agents/v2-windows?view=azure-devops)またはその他のカスタム ビルド コンピューターをセットアップする場合に一般的です。

Microsoft GDK プロジェクトをビルドする場合、[Visual Studio 2019](https://walbourn.github.io/visual-studio-2019/) (v141 および v142 のプラットフォーム ツールセット VC++ プロジェクトをビルドできます) または [Visual Studio 2022](https://walbourn.github.io/visual-studio-2022/) (v141、v142、v143 のプラットフォーム ツールセット VC++ プロジェクトをビルドできます) をセットアップできます。 フル Visual Studio インストールまたは [Visual Studio Build Tools](https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022) を使用することもできます。 次のコンポーネントをインストールしてください。

**オプション 1: フル Visual Studio インストール**

| ワークロード | コンポーネント ID ([コマンド ライン インストール](https://docs.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio)の場合) |
|---|---|
| C++ でゲーム開発 | Microsoft.VisualStudio.Workload.NativeGame |
| C++ でデスクトップ開発<br /> *必要なコンポーネント:* Windows 10 SDK (10.0.19041.0) または Windows 11 SDK (10.0.22000.0)<br /><br /> *省略可能なコンポーネント:* MSVC v141 - VS 2017 C++ x64/x86 ビルド ツール (v14.16)<br /> *VS 2019/MSBuild 16.0 または VS 2022/MSBuild 17.0 を使用して v141 プラットフォーム ツールセット プロジェクトをビルドする場合にのみ必要です*<br /><br />  *省略可能なコンポーネント、VS 2022 のみ:* MMSVC v142 - VS 2019 C++ x64/x86 ビルド ツール (v14.29)<br /><br /> *VS 2022/MSBuild 17.0 を使用して v142 プラットフォーム ツールセット プロジェクトをビルドする場合にのみ必要です。*<br /><br /> *省略可能なコンポーネント:* Windows (12.0.0 - x64/x86) 用 C++ Clang ツール <br /> *Clang ツールセットを使用してビルドする場合にのみ必要です* | Microsoft.VisualStudio.Workload.NativeDesktop<br /> Microsoft.VisualStudio.Component.Windows10SDK.19041<br />Microsoft.VisualStudio.Component.Windows11SDK.22000<br /><br /> *省略可能:* Microsoft.VisualStudio.Component.VC.v141.x86.x64<br /><br /> *省略可能、VS 2022 のみ:* Microsoft.VisualStudio.ComponentGroup.VC.Tools.142.x86.x64<br /><br /> *省略可能:* Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang |

**オプション 2: Visual Studio Build Tools**

| ワークロード | コンポーネント ID ([コマンド ライン インストール](https://docs.microsoft.com/en-us/visualstudio/install/use-command-line-parameters-to-install-visual-studio)の場合) |
|---|---|
| C++ Build Tools<br /> *必要なコンポーネント:* Windows 10 SDK (10.0.19041.0) または Windows 11 SDK (10.0.22000.0)<br /><br /> *必須コンポーネント:* MSVC v142 - VS 2019 C++ x64/x86 ビルド ツール (最新)<br /> または MSVC v143 - VS 2022 C++ x64/x86 ビルド ツール (最新)<br /><br /> *省略可能なコンポーネント:* MSVC v141 - VS 2017 C++ x64/x86 ビルド ツール (v14.16)<br /> *VS 2019/MSBuild 16.0 または VS 2022/MSBuild 17.0 を使用して v141 プラットフォーム ツールセット プロジェクトをビルドする場合にのみ必要です*<br /><br /> *省略可能なコンポーネント、VS 2022 のみ:* MMSVC v142 - VS 2019 C++ x64/x86 ビルド ツール (v14.29)<br /> *VS 2022/MSBuild 17.0 を使用して v142 プラットフォーム ツールセット プロジェクトをビルドする場合にのみ必要です*<br /><br /> *省略可能なコンポーネント:* Windows (12.0.0 - x64/x86) 用 C++ Clang ツール<br /> *Clang ツールセットを使用してビルドする場合にのみ必要です* | Microsoft.VisualStudio.Workload.VCTools<br /> Microsoft.VisualStudio.Component.Windows10SDK.19041<br />Microsoft.VisualStudio.Component.Windows11SDK.22000<br /><br /> Microsoft.VisualStudio.Component.VC.Tools.x86.x64<br /><br /> *省略可能:* Microsoft.VisualStudio.Component.VC.v141.x86.x64<br /><br /> *省略可能、VS 2022 のみ:* Microsoft.VisualStudio.ComponentGroup.VC.Tools.142.x86.x64<br /><br /> *省略可能:* Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang |

| |
|---|
| > BWOIExample プロジェクトでは既定で v142 ツールセットが使用されます。したがって、VS 2019 または VS 2022 が必要です。 VS 2019 をしてビルドするには、MSVC v142 - VS 2019 C++ x64/x86 ビルド ツール (最新) コンポーネントが必要で、また、VS 2022 には MSVC v142 - VS 2019 C++ x64/x86 ビルド ツール (v14.29) コンポーネントが必要です。 |


# ビルド環境の設定

ソフトウェア要件がインストールされたら、インストールを必要としない抽出された GDK を設定できます。 2 つの方法があります。 必要に応じて、Windows 10 SDK を抽出することもできます。

***VS 2022 のサポートには、March 2022 GDK 以降が必要であることに注意してください。***

## 方法 1: 抽出済み GDK をダウンロードする

これは最も簡単なオプションとして推奨されます。

1. [Xbox デベロッパー向けダウンロード](https://aka.ms/gdkdl)のページに移動します。

2. ファイルの種類として [Game Core] を選択します。

3. ビルド/バージョン メニューで、使用する GDK ビルド用の [Microsoft GDK Extracted for Build Systems] を選択します。

4. ZIP をダウンロードし、ビルド コンピューター上のどこかのフォルダーに展開します。 MAX_PATH の問題を回避するために、短いパスの場所を選択します。

## 方法 2: GDK NuGet パッケージを使用してインストールする

この方法は、[nuget.org](https://www.nuget.org/) にある `Microsoft.GDK.PC.<edition>` パッケージを使用します。

1. [nuget.exe](https://www.nuget.org/downloads) をダウンロードし、コマンド ラインにパスを入力します

2. nuget.org のソースを[こちらの手順](https://learn.microsoft.com/en-us/nuget/consume-packages/configuring-nuget-behavior)に従って構成します。

3. 次を使用して GDK コンテンツを抽出します。

```
nuget install -ExcludeVersion -Source <name-of-source> Microsoft.GDK.PC.<edition> -OutputDirectory [path-for-extracted-sdks]
```


## 方法 3: GDK を手動で抽出する

この方法はより複雑ですが、別途のダウンロードが不要です。 標準 GDK インストーラーのコピーが必要です。

1. **コマンド プロンプトを開きます** (VS または GDK の*開発者コマンド プロンプト*である必要はありません)。

2. BWOIExample サンプル フォルダーに **cd** します。

3. VS 2022 または 2019 の環境変数を設定し、ターゲット エディション番号を指定します。 抽出された GDK のカスタム パスを指定する場合は、MAX_PATH を超過する問題などを回避するために、引用符で囲まれていない短い絶対パスを使用します。

```
setenv vs2022 220300 [path-for-extracted-sdks]
```


4. インストーラー イメージから GDK を抽出します。

```
extractgdk <path-to-gdk-installer>\Installers
```


> MSIEXEC のすべての使用に対してグローバル ロックがかかるため、抽出操作のみを行う場合でも、別の MSIEXEC インスタンス (Windows Update または同じスクリプトの他のインスタンス) が同時に実行されている場合は失敗します。 ビルド パイプラインが同じ VM 上で実行される場合は、`Global\_MSIExecute` ミューテックスと自分のグローバル ロックの使用に基づいて、外部ロック/ロック解除サイクルを指定する必要があります。 一般的に、MSI を開発者のコンピューター上で一旦抽出しておき、結果をエージェントがアクセス可能なフォルダーにコピーする方が簡単です。


## オプション: Windows SDK を抽出する

必要に応じて、Windows SDK を抽出することもできます。これにより、ビルド コンピューターで常に適切なバージョンが使用可能になります。 Visual Studio のインストールと共に Windows SDK をインストールする場合、これは一般的に不要です。

このプロセスには、Windows SDK インストーラー イメージのコピーが必要です。 これを入手する最も簡単な方法は、[Windows デベロッパー センター](https://developer.microsoft.com/windows/downloads/windows-sdk/)から Windows SDK .ISO をダウンロードすることです。

1. **コマンド プロンプトを開き**、BWOIExample フォルダーに **cd** します。

2. 環境変数を設定します。 抽出した GDK と同じパスを使用します。

```
setenv vs2022 220300 [path-for-extracted-sdks]
```


3. インストーラー イメージから Windows 10 SDK を抽出します。

```
extractsdk <path-to-sdk-installer>\Installers
```


## VS 2019/2022 のみ: VCTargets のマージ

VS 2019 と 2022 の BWOI は、GDK のフラット ファイル ディレクトリを設定することだけでなく、標準の Microsoft.Cpp MSBuild 規則と GDK の MSBuild 規則をマージする、マージされた VCTargets フォルダーが存在することに依存しています。 VS 2019 と 2022 の場合、堅牢な解決策は、マージされたマージされたフォルダーを抽出された GDK の隣に作成することです。

1. **コマンド プロンプトを開き**、BWOIExample フォルダーに **cd** します。

2. 環境変数を設定します。 ダウンロードした、または手動で抽出した GDK のパスを指定します。

```
setenv vs2022 220300 [path-for-extracted-sdks]
```


3. マージされた VC++ MSBuild ターゲット ディレクトリをビルドし、抽出された GDK の隣に配置します。

```
vctargets
```


これらの手順を実行した後、ExtractedFolder 環境変数は、サンプルがビルドする抽出された移植可能な GDK (およびオプションの Windows SDK および VCTargets ディレクトリ) をポイントします。 このフォルダーは、他のどのビルド コンピューターにもコピーできます。

# サンプルのビルド

ビルドの残りの部分は通常どおりに実行されます。 この BWOI の例は、Directory.Build.props ファイルによって駆動されます。 ターゲット vcxproj 自体は完全に "ストック" であり、Directory.Build.props ファイルを削除すると、GDK がインストールされている通常の開発者システムで想定されるのとまったく同じように動作します。

1. **コマンド プロンプトを開きます** (VS または GDK の開発者コマンド プロンプトである必要はありません)。

2. BWOIExample サンプル フォルダーに **cd** します。

3. VS 2022 または 2019 およびお使いの GDK エディションのターゲットに対して **setenv** を実行します:

```
setenv vs2022 220300 [path-for-extracted-sdks]
```


> setenv を実行しない場合、ビルドは、 Directory.Build.props で指定されている既定値に戻りますします。 必要に応じて、ファイル内でこれらを直接変更できます。 また、setenv を使用ない場合は、MSBuild がパス上にあるようにする必要もあります。

4. コマンド ラインでプロジェクトをビルドします:

```
msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Desktop.x64

msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Xbox.XboxOne.x64

msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Xbox.Scarlett.x64
```


> VS 2019 の場合、v142 プラットフォーム ツールセット プロジェクトのみをサポートし、``Microsoft.VisualStudio.Component.VC.v141.x86.x64`` をインストール*しなかった*場合は、Directory.Build.Props を編集して ``VCTargetsPath15`` の設定を削除する必要があります。同様に、VS 2022 の場合は、v143 プラットフォーム ツールセットのサポートのみをインストールした場合は、``VCTargetsPath15` and ``VCTargetsPath16'' の両方を削除します。

# Windows コンテナーでビルドする

別の方法として、Docker で実行される Windows コンテナーを使用して、分離された再現可能なビルド環境を作成できます。 これらは、ビルド サーバーでも、ローカル開発者ビルド用にも使用でき、一貫性のあるビルド プロセスを確保できます。 このサンプルには、Visual Studio 2022 ビルド ツールを使用して BWOI ビルド環境を設定する Dockerfile が含まれています。

> ここで説明するプロセスでは、プロジェクトで v142 ツールセット以降を使用する必要があります。 Windows コンテナーの詳細については、[Windows 上の コンテナーに関するドキュメント](https://docs.microsoft.com/en-us/virtualization/windowscontainers/)を参照してください。

Dockerfile を使用するには、抽出された GDK と、オプションとして抽出された Windows SDK を同様に指定する必要があります。 ただし、ホスト コンピューターに Visual Studio をインストールする必要はありません。

1. Docker がインストールされており、[こちら](https://docs.microsoft.com/en-us/virtualization/windowscontainers/quick-start/set-up-environment)で説明されるとおりに、Windows コンテナーを使用するように設定されていることを確認します。

2. 次の例に示されるように、Dockerfile を、BWOIExample プロジェクトと抽出された SDK の両方が含まれている親ディレクトリに移動します。

```
<parent dir>
-> Dockerfile
-> BWOIExample
-> <project and script files>
-> sdks
-> Microsoft GDK
-> <extracted GDK files>
-> <optional extracted Windows SDK>
```


> Docker は、コンテナーをビルドする際に setenv.cmd、vctargets.cmd、および抽出された SDK にのみアクセスする必要があります。 必要に応じて、実際のプロジェクト ソースは他の場所に配置できます。


3. Dockerfile が含まれているディレクトリに移動し、以下を実行します:

```
docker build -t gdkbwoi:latest -m 2GB --build-arg
ExtractedSDKDir="sdks" --build-arg ScriptDir="BWOIExample"
--build-arg GDKVer="220300" .
```


> コンテナーで追加の CPU コアを使用できるようにするには、``--cpus=N`` フラグを使用します。 追加のメモリを使用するには、``-m 2GB`` フラグの値を変更します。

Docker は、コンテナーの作成、VS ビルド ツールのダウンロードとインストール、必要な \*.cmd スクリプトと抽出された SDK のコピー、VCTargets のマージのプロセスを自動化します。

4. コンテナーがビルドされたら、以下を使用してそれを実行します。

**cmd.exe を使用して:**

```
docker run --rm -v %cd%\BWOIExample:c:\Project -w c:\Project gdkbwoi
msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Xbox.Scarlett.x64
```


**PowerShell を使用して:**

```
docker run \--rm -v \${pwd}\\BWOIExample:c:\\Project -w c:\\Project gdkbwoi
msbuild BWOIExample.vcxproj /p:Configuration=Debug /p:Platform=Gaming.Xbox.Scarlett.x64
```


このコマンドは、コンテナーを起動し、その中にプロジェクト ディレクトリをマウントし、指定されたパラメーターで msbuild を実行します。 必要に応じて構成とプラットフォームを変更できます。 別のプロジェクトをビルドするには、"%cd%\\BWOIExample" をプロジェクトの場所に変更します。

ビルドが完了するとコンテナーが終了します。 プロジェクト ディレクトリはコンテナーにマウントされているため、ビルド結果はホスト コンピューター上のプロジェクト ディレクトリに表示されます。

# 追加情報

Microsoft GDK ドキュメントでは、MSBuild "BWOI" プロパティについて詳しく説明しています:

Microsoft Game Development Kit に関するドキュメント
* 開発とツール
   * **インストールせずに Microsoft Game Development Kit (GDK) を使用する。**

<https://aka.ms/GDK_BWOI>

*CMakeExample* のサンプルでは、MSBuild ベース以外のビルド システムを使用している場合に、すべての特定のコンパイラとリンカーのスイッチについて詳しい情報を提供します。 このサンプルの extractgdk.cmd スクリプトによって作成されたのと同じ BWOI イメージを使用できるようにするビルド オプション (既定ではオフ) がサポートされています。 CMake の例は、Gaming.\*.x64 MSBuild プラットフォームを使用しないため、vctargets.cmd スクリプトの結果は必要ありません。

詳細については、**CMakeExample** を参照してください。

# 既知の問題

VS 2019 の一部のバージョンでは、DisableInstalledVCTargetsUse=true を使用していて、プロジェクトに \<MinimumVisualStudioVersion\>16.0\</MinimumVisualStudioVersion\> が含まれている場合、以下を使用したときにビルドに失敗する可能性があります:

> C:\\Program Files (x86)\\Microsoft Visual
> Studio\\2019\\Enterprise\\MSBuild\\Current\\Bin\\Microsoft.Common.CurrentVersion.targets(816,5):
> error : \'X.vcxproj\ プロジェクトに BaseOutputPath/OutputPath プロパティが
> 設定されていません。 このプロジェクトに対して有効な構成とプラットフォームの
> 組み合わせが指定されていることを確認してください。
> Configuration=\'Debug\' Platform=\'Gaming.XBox.Scarlett.x64\'. お客様は、
> ソリューション ファイルを使用せずにプロジェクトをビルドしようとしている
> とともに、このプロジェクトに存在しない既定以外の構成またはプラットフォームを
> 指定したために、このメッセージが表示されている可能性があります。

回避策は、**Directory.Build.props** にオーバーライドを追加することです

```
<PropertyGroup>
<ExtractedFolder Condition="'$(ExtractedFolder)'==''">C:\xtracted\</ExtractedFolder>
<ExtractedFolder Condition="!HasTrailingSlash('$(ExtractedFolder)')">$(ExtractedFolder)\</ExtractedFolder>
<_AlternativeVCTargetsPath160>$(ExtractedFolder)VCTargets160\</_AlternativeVCTargetsPath160>
<_AlternativeVCTargetsPath150>$(ExtractedFolder)VCTargets150\</_AlternativeVCTargetsPath150>
<!-- Workaround for VS bug -->
<MinimumVisualStudioVersion>15.0</MinimumVisualStudioVersion>
</PropertyGroup>
```


この問題は、Visual Studio 2019 バージョン 16.11 で [修正されました](https://developercommunity.visualstudio.com/t/1695-or-later-fails-when-using-disableinstalledvct/1435971)。

# バージョン履歴

| 日付 | 注意 |
|---|---|
| 2020 年 2 月 | 初期バージョン。 |
| 2020 年 5 月 | オプションの抽出された Windows 10 SDK 用に更新されました。 |
| 2020 年 6 月 | 2006 GDK FAL リリース用に更新されました。 |
| 2021 年 4 月 | LargeLogo.png を追加しました。 |
| 2021 年 6 月 | 非推奨の GDK に関する情報を削除し、追加の説明を加え、DisableInstalledVCTargetsUse の使用を追加しました。<br />一般的なコード クリーンアップ。 |
| 2021 年 10 月 | Dockerfile および Windows コンテナーでのビルドに関する手順を追加しました。 |
| 2022 年 3 月 | Visual Studio 2022 をサポートするように更新されました。<br /> 既定で v142 ツールセットを使用するようにプロジェクト ファイルを更新しました。<br /> マウントされたディレクトリを使用するように Dockerfile を変更しました。 |
| 2022 年 10 月 | VS 2017/MSBuild 15.0 のサポートを削除しました。 |
| 2023 年 3 月 | NuGet の手順を追加しました。 |
| 2023 年 10 月 | GDK で Windows SDK (20000) が必要になりました |


