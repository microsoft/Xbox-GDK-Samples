# CMake の例

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

これは、[CMake](https://cmake.org/) クロスプラットフォーム ビルド システムを使用して、Ninja ジェネレーターを介して Microsoft Game Development Kit で実行可能ファイルをビルドする例です。

![ソース イメージを表示する](./media/image1.png)

*このサンプルの主な目的は、ゲーム (*.x64 プラットフォーム) 用にビルドするために必要なすべてのパスと設定を明確に文書化することです。 これにより、GDK によってインストールされる MSBuild ルールに実装されている機能の多くがレプリケートされます。 Visual Studio ジェネレーターを介して CMake を使用する別の方法については、「**CMakeGDKExample**」を参照してください。*.

# サンプルのビルド (Visual Studio)

Visual Studio 2019 (16.11) または Visual Studio 2022 を使用して、[ローカル フォルダーを開く...] を選択します。[新しいプロジェクト] ダイアログボックスまたは [ファイル] -\> [開く] -\> [フォルダー...] からメニュー コマンドをクリックし、Desktop、XboxOne、または Scarlett フォルダーを開きます。

> これには、"C++ CMake tools for Windows" コンポーネント (`Microsoft.VisualStudio.Component.VC.CMake.Project`) がインストールされている必要があります。

必要に応じて、**XdkEditionTarget** 変数 (CMakePresets.json または CMakeList.txt) を編集して、正しい GDK エディションが参照されていることを確認します。

CMake ツールを開くと、キャッシュが自動的に生成されます。 それ以外の場合は、CMakeList.txt を選択し、右ボタン メニューから [キャッシュの生成] を選択します。 次に、[ビルド] -\> [すべてを再ビルド] メニュー コマンドを使用します。 ビルド製品は "**out**" サブフォルダーにあります。

Visual Studioの CMake の詳細については、「[Microsoft Docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)」を参照してください。

既定のセットアップには、代わりに clang/LLVM を使用するための **x64-Debug**、**x64-Release**、**x64-Clang-Debug**、および **x64-Clang-Release** 構成が含まれます。

> これには、"C++ Clang Compiler for Windows" ()`Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang`) コンポーネントがインストールされている必要があります。

リモート コンソールではなく、開発用 PC で実行を試みても失敗します。 正常に実行するには、以下の手順に従ってプログラムを展開する必要があります。*
| | |
|---|---|
|*Xbox One または Xbox Series X|S プロジェクトで F5 キーを押すと、それは|

# サンプルのビルド (コマンド ライン)

*VS 2019 または 2022 開発者コマンド プロンプト*を使用して、コマンド ラインから生成およびビルドすることもできます。 使用可能なプリセットの完全なリストには、次を使用します:

```
cd CMakeExample\Desktop
cmake --list-presets

cd CMakeExample\Scarlett
cmake --list-presets

cd CMakeExample\XboxOne
cmake --list-presets
```


いずれの場合も、適切なターゲット プラットフォームを変更し、次を使用します:

```
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```


## トラブルシューティング

*GDK エディションに合わせて CMakePresets.json を編集する必要がある場合があります。*

*次の手順で CMake 3.20 以降を使用していることを確認します:*

```
cmake --version
```


# サンプルの使用方法

サンプルを展開するには、*Xbox ゲーム コマンド プロンプト* インスタンスを開き、ターゲットのサンプル ディレクトリに変更します。

```
cd CMakeExample\Desktop\out\build\x64-Debug\bin

cd CMakeExample\Scarlett\out\build\x64-Debug\bin

cd CMakeExample\XboxOne\out\build\x64-Debug\bin
```


## デスクトップをターゲットにする

"ルースなレイアウト" を実行するには、次を使用します。

```
cd bin\x64
CMakeExampleDesktop.exe
```


## Xbox One または Xbox Series X|S をターゲットにする

### プッシュ展開

"ルースな" レイアウトをプッシュ配置するには:

```
xbapp deploy Gaming.Xbox.Scarlett.x64
```


- または -

```
xbapp deploy Gaming.Xbox.XboxOne.x64
```


### PC から実行

PC から "ルースな" レイアウトを実行するには:

```
xbapp launch Gaming.Xbox.Scarlett.x64\CMakeExampleScarlett.exe
```


- または -

```
xbapp launch Gaming.Xbox.XboxOne.x64\CMakeExampleXboxOne.exe
```


## パッケージ化された展開

パッケージを作成するには:

```
makepkg genmap /f chunks.xml /d x64
makepkg pack /f chunks.xml /lt /d x64 /pd . /pc
```


- または -

```
makepkg genmap /f chunks.xml /d Gaming.Xbox.Scarlett.x64
makepkg pack /f chunks.xml /lt /d Gaming.Xbox.Scarlett.x64 /pd .
```


- または -

```
makepkg genmap /f chunks.xml /d Gaming.Xbox.XboxOne.x64
makepkg pack /f chunks.xml /lt /d Gaming.Xbox.XboxOne.x64 /pd .
```


その後、結果のパッケージを本体にインストールします (正確な .xvc ファイル名は異なります):

```
xbapp install CMakeExampleXboxOne_1.0.0.0_neutral__zjr0dfhgjwvde.xvc
```


Desktop の場合、拡張子は ".msixvc" です (正確なファイル名は異なります):

```
xbapp install CMakeExampleDesktop_1.0.0.0_x64__8wekyb3d8bbwe.msixvc
```


実行時のサンプルでは、デバイスとスワップチェーンが作成され、色付きの三角形が描画されます。 コントロールやその他の動作はありません。

![C:\\temp\\xbox_screenshot.png](./media/image2.png)

*他のバージョンをパッケージ化する場合は、使用する特定のコマンド ライン オプションについて、各 CMakeLIst.txt の末尾にあるコメントを参照してください。*

# 実装の詳細

さまざまな Visual C++ スイッチの詳細については、以下のリンクを参照してください:

| /GR | <https://docs.microsoft.com/en-us/cpp/build/reference/gr-enable-run-time-type-information> |
|---|---|
| /GS<br /> /RTC<br /> /sdl<br /> /DYNAMICBASE<br /> /NXCOMPAT | <https://aka.ms/msvcsecurity> |
| /DEBUG:fastlink | <https://devblogs.microsoft.com/cppblog/faster-c-build-cycle-in-vs-15-with-debugfastlink/> |
| /EHsc | <https://devblogs.microsoft.com/cppblog/making-cpp-exception-handling-smaller-x64/> |
| /fp | <https://docs.microsoft.com/en-us/cpp/build/reference/fp-specify-floating-point-behavior><br /> <https://devblogs.microsoft.com/cppblog/game-performance-improvements-in-visual-studio-2019-version-16-2/> |
| FS | <https://docs.microsoft.com/en-us/cpp/build/reference/fs-force-synchronous-pdb-writes> |
| /GL<br /> /Gw<br /> /LTCG | <https://devblogs.microsoft.com/cppblog/tag/link-time-code-generation/><br /> <https://devblogs.microsoft.com/cppblog/introducing-gw-compiler-switch/> |
| /Gy | <https://docs.microsoft.com/en-us/cpp/build/reference/gy-enable-function-level-linking> |
| /JMC | <https://devblogs.microsoft.com/cppblog/announcing-jmc-stepping-in-visual-studio/> |
| / permissive- | <https://devblogs.microsoft.com/cppblog/permissive-switch/> |
| /std:c++14 | <https://devblogs.microsoft.com/cppblog/standards-version-switches-in-the-compiler/> |
| /Yc<br /> /Yu<br /> /Fp<br /> /FI | <https://docs.microsoft.com/en-us/cpp/build/creating-precompiled-header-files> <https://devblogs.microsoft.com/cppblog/shared-pch-usage-sample-in-visual-studio/> |
| /Zc:\ _\_cplusplus | <https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/> |
| /Zc: preprocessor | <https://devblogs.microsoft.com/cppblog/announcing-full-support-for-a-c-c-conformant-preprocessor-in-msvc/> |
| /Z7<br /> /Zi<br /> /ZI | <https://docs.microsoft.com/en-us/cpp/build/reference/z7-zi-zi-debug-information-format> |
| /ZH:SHA_256 | <https://learn.microsoft.com/en-us/cpp/build/reference/zh> |
| /guard:cf<br /> /guard:ehcont<br /> /CETCOMPAT | <https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-control-flow-guard><br /> <https://learn.microsoft.com/en-us/cpp/build/reference/guard-enable-eh-continuation-metadata><br /> <https://learn.microsoft.com/en-us/cpp/build/reference/cetcompat> |

次の点に注意してください。
[/Gm](https://docs.microsoft.com/en-us/cpp/build/reference/gm-enable-minimal-rebuild)
(最小リビルド) は非推奨であり、引き続き使用するプロジェクトから削除する必要があります。

# 追加情報

このサンプルの CMake プロジェクトでは、Build With/Out Installing (BWOI) を使用するためのオプトイン ビルド オプションがサポートされています。 有効な場合は、*BWOIExample* の extractgdk.cmd スクリプトによって作成された抽出された Microsoft GDK を指す ExtractedFolder 環境変数が必要です。 必要に応じて、抽出されたWindows SDKを持つこともできます。 CMake プロジェクトは、Gaming.\*.x64 MSBuild プラットフォームを使用しないため、vctargets.cmd スクリプトの結果は必要ありません。

このビルド オプションを有効にするには、`BUILD_USING_BWOI` を True に設定します。 または、コマンド ラインを使用してビルドする場合は、`-DBUILD_USING_BWOI=ON` を生成手順に追加します。

詳細については、「**BWOIExample**」を参照してください。

# バージョン履歴

| 2023 年 2 月 | CMake 3.9 の標準 `CMAKE_INTERPROCEDURAL_OPTIMIZATION` を優先してカスタム ビルド オプション BUILD_FOR_LTCG を削除しました。<br /> 新しい VS 2022 17.5 スイッチ用に更新されました。 |
| 日付 | 注意 |
|---|---|
| 2019 年 11 月 | 初期バージョン。 |
| 2020 年 2 月 | HLSL シェーダーの使用を例に追加しました。<br /> 必要に応じて BWOI をサポートするように更新されました。 |
| 2020 年 4 月 | CMake 3.16 以降を使用する場合の pch サポートで更新されました。 |
| 2020 年 5 月 | May 2020 GDK をサポートするように更新されました。 |
| 2020 年 6 月 | June 2020 GDK FAL リリース用に更新されました。 |
| 2020 年 8 月 | サイド バイ サイドの詳細で更新されました。 |
| 2020 年 11 月 | Xbox ターゲットの xmem.lib と xg_*.lib を追加しました。<br /> CMake ファイルをクリーンアップしました。 |
| 2021 年 2 月 | CMake コメントの軽微な更新。 |
| 2021 年 4 月 | appnotify.lib を追加して、デスクトップ ターゲットに関するリンクの問題を解決します。<br />LargeLogo.png を追加します。 |
| 2021 年 6 月 | 一般的なコード クリーンアップ。 |
| 2021 年 8 月 | ツールチェーン ファイルの機能強化。 |
| 2021 年 10 月 | BWOI の更新プログラム。 |
| 2022 年 1 月 | VS 2022 のサポートを追加しました。<br /> CMake クリーンアップとプリセット ファイルの追加。 |
| 2022 年 10 月 | VS 2017 のサポートを削除しました。<br />XSAPI には XCurl が必要です。その他すべての拡張子ライブラリのコメント アウトされたサポートが追加されました<br />デスクトップ CMake が PC の GDK と互換性を持つようになりました。 |
| 2022 年 11 月 | March 2022 GDK 以降が必要です。<br /> VS 2019 16.10 以前がサポート ライフサイクルを終えるので CMake 3.20 を要求するように更新されました。 |
| 2022 年 12 月 | カスタム 'Gaming.Desktop.x64' ではなく 'x64' プラットフォーム スタイルを使用する簡素化されたデスクトップ シナリオ<br />.cmake ファイルを独自のサブフォルダーに再構成しました。 |
| 2023 年 3 月 | Playfab.Services.C 拡張機能ライブラリの新しいターゲットを追加するように更新されました。 |
| 2023 年 6 月 | Xbox Oneタイトルは、既定の動作が VS 2019 から反転したため、VS 2022 以降で `/d2vzeroupper-` を使用する必要があります。 |
| 2023 年 10 月 | Microsoft GDK を使用するには、Windows 11 SDK (22000) 以降が必要です。 |


