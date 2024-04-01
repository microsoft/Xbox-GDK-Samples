# Cmake Xbox 本体アプリ

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

これは、[CMake](https://cmake.org/) クロス プラットフォーム ビルド システムを使用して、"Win32 コンソール" アプリケーションを生成する例です。Microsoft GDK を使用して Xbox ハードウェアで実行できます。 これは、'printf' スタイルの出力を使用するグラフィックス以外の開発者単体テストに適しています。

![ソース イメージを表示する](./media/image1.png)

*標準の Microsoft GDK アプリケーションを構築するために CMake を使用する方法の詳細については、「**CMakeExample** と **CMakeGDKExample**」を参照してください*。

# サンプルのビルド (Visual Studio)

Visual Studio 2019 または 2022 を使用して、[新しいプロジェクト] ダイアログから"ローカル フォルダーを開く..."を選択するか、"[ファイル] -\> [開く] -\> [フォルダー...]"メニュー コマンドを選択し、サンプル フォルダーを開きます。

> これには、"C++ CMake tools for Windows" コンポーネント (`Microsoft.VisualStudio.Component.VC.CMake.Project`) がインストールされている必要があります。

必要に応じて、CMake **XdkEditionTarget** 変数 (CMakePresets.json または CMakeList.txt) を編集して、正しい GDK エディションが参照されていることを確認します。

CMake ツールを開くと、キャッシュが自動的に生成されます。 それ以外の場合は、CMakeList.txt を選択し、右ボタン メニューから [キャッシュの生成] を選択します。 次に、[ビルド] -\> [すべてを再ビルド] メニュー コマンドを使用します。 ビルド製品は "**out**" サブフォルダーにあります。

Visual Studioの CMake の詳細については、「[Microsoft Docs](https://docs.microsoft.com/en-us/cpp/build/cmake-projects-in-visual-studio)」を参照してください。

既定のセットアップには、CMake プリセットとして定義された **x64-Debug**、**x64-Release**、**x64-Clang-Debug**、**x64-Clang-Release** 構成が含まれます。

> これには、"C++ Clang Compiler for Windows" (`Microsoft.VisualStudio.ComponentGroup.NativeDesktop.Llvm.Clang`) コンポーネントがインストールされている必要があります。

*F5 キーを押すと、リモート コンソールではなく開発用 PC で実行が試行されるため、失敗する場合も失敗しない場合もあります。 正常に実行するには、以下の手順に従ってプログラムを展開する必要があります。*

# サンプルのビルド (コマンド ライン)

*VS 2019 または 2022 開発者コマンド プロンプト*を使用して、コマンド ラインから生成およびビルドすることもできます。

```
cd CMakeXboxConsoleApp
cmake -B out -DXdkEditionTarget=221000
cmake --build out
```


CMake プリセットも用意されています

```
cmake --list-presets
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```


# サンプルの使用方法

サンプルを展開するには、*Xbox ゲーム コマンド プロンプト* インスタンスを開き、サンプル ディレクトリに変更します。

```
cd CMakeXboxConsoleApp\out\build\<config>
xbcp bin\Console\*.exe xd:\
xbcp bin\Console\*.dll xd:\
```


サンプルを実行するには:

```
xbrun /O D:\CMakeXboxConsoleApp.exe
```


プログラムは、システム OS のコンテキストで実行されます。

代わりにゲーム OS のコンテキストで実行する場合は、同様の手順を使用できます。 最初に、ターゲット コンソールのゲーム OS タイトルを実行します。 適切な候補は、Visual Studio の [新しいプロジェクト] ダイアログを使用し、Microsoft GDK で既定の "Direct3D 12 Xbox Game project" を作成することです。 ビルドして展開したら、実行したままにします。

次に、以下を使用します。

```
xbcp /x/title bin\Console\*.exe xd:\
xbcp /x/title bin\Console\*.dll xd:\
```


サンプルを実行するには:

```
xbrun /x/title /O D:\CMakeXboxConsoleApp.exe
```


これは、ゲーム OS VM にプロセスを挿入することによって機能することに注意してください。 現時点では、マルチプロセス ゲーム タイトルはサポートされていません。また、グラフィックス、オーディオ、GameRuntime などのいくつかのコンポーネントは、複数のプロセス シナリオでの使用のテストもサポートもされていません。 また、"ホスティング" タイトルを極力シンプルにし、CPU リソースの使用を制限することをお勧めします。

# 実装の詳細

PC Desktop の場合、Win32 コンソール exe (/SUBSYSTEM:CONSOLE など) の **CMakeLists.txt** は次のようになります。

```
cmake_minimum_required (VERSION 3.20)

project(CMakeExampleWindowsConsole LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(${PROJECT_NAME} Main.cpp)

target_compile_definitions(${PROJECT_NAME} PRIVATE _CONSOLE _UNICODE UNICODE)

if(MVSC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /GR- /fp:fast)
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
  target_compile_options(${PROJECT_NAME} PRIVATE /permissive- /Zc:__cplusplus /Zc:inline)
endif()
```


Xbox ハードウェアのシステムおよびゲーム OS の場合は、別のリンク ライブラリのセットを使用し、サポートされていないライブラリを選択しないようにする必要があります。 また、サポートされていない API を使用しないように、適切な API パーティション分割を有効にする必要があります。このサンプルでは、プラットフォーム ヘッダーとライブラリを使用してビルドしていることを確認します。

Xbox ハードウェアで実行されているアプリケーションでは、必要な Visual C++ ランタイム DLL と、デバッグ用にビルドされている場合は ucrtbased.lib も指定する必要があります。

このサンプルの Xbox "console" CMake は、EXE をコンソール アプリの 
Direct3D にビルドするために設定されるため、プラットフォームの主要な API の違いを回避し、同じ EXE が両方のプラットフォームで実行されることが合理的に予想できます。 これは特定の XboxOne によっても行われ、Scarlett include/lib パスは CMakeLists.txt では設定されません。 必要に応じて、追加のコンパイラ CPU ターゲットを有効にすることができます
| | |
|---|---|
|Xbox Series X|S または Xbox One ハードウェアで実行します。 使用できないため|

ビルド オプション `OPTIMIZE_FOR_SCARLETT` を ON にします。 結果の EXE が実行されます
この例では、関連する CPUID チェックを実行する DirectXMath **XMVerifyCPUSupport** 関数を使用します。
| | |
|---|---|
|特に Xbox Series X|S ハードウェア用です。 これは設定によって行われます|
|前の Xbox Series X|S と同様ですが、Xbox One では実行できません。 から|

# 追加情報

この例で使用されるすべてのコンパイラとリンカー スイッチの詳細については、「**CMakeExample**」を参照してください。

このサンプルの CMake プロジェクトでは、Build With/Out Installing (BWOI) を使用するためのオプトイン ビルド オプションがサポートされています。 有効な場合は、*BWOIExample* の `extractgdk.cmd` スクリプトによって作成された抽出された Microsoft GDK を指す ExtractedFolder 環境変数が必要です。 必要に応じて、抽出されたWindows SDKを持つこともできます。 CMake プロジェクトでは、Gaming.\*.x64 MSBuild プラットフォームを使用しないため、vctargets.cmd スクリプトの結果は必要ありません。

このビルド オプションを有効にするには、CMakeSettings.json を使用して `BUILD_USING_BWOI` を True に設定します。 または、コマンド ラインを使用してビルドする場合は、`-DBUILD_USING_BWOI=ON` を生成手順に追加します。

詳細については、「**BWOIExample**」を参照してください。

# 既知の問題

clang/LLVM ツールセットを使用する場合は、Windows 10 SDK (19041) 以降を使用していることを確認してください。 DirectXMath 3.13 以前では、XMVerifyCPUSupport 実装がそのツールセットに対して正しくビルドされません。 詳しくは、後述の「<https://walbourn.github.io/directxmath-3.14/>」を参照してください。

# バージョン履歴

| 日付 | 注意 |
|---|---|
| 2020 年 5 月 | 初期バージョン。 |
| 2020 年 6 月 | June 2020 GDK FAL リリース用に更新されました。 |
| 2020 年 11 月 | CMake ファイルをクリーンアップし、_CONSOLE 定義を追加しました。 |
| 2021 年 2 月 | 軽微な CMake クリーンアップ。 |
| 2021 年 8 月 | ツールチェーン ファイルの機能強化。 |
| 2021 年 10 月 | BWOI の更新プログラム。 |
| 2022 年 1 月 | VS 2022 のサポートを追加しました。<br />CMake クリーンアップと CMake プリセット ファイルの追加。 |
| 2022 年 10 月 | VS 2017 のサポートを削除しました。 |
| 2023 年 2 月 | CMake 3.9 の標準 CMAKE_INTERPROCEDURAL_OPTIMIZATION を優先するカスタム ビルド オプション BUILD_FOR_LTCG を削除しました。<br />新しい VS 2022 17.5 スイッチ用に更新されました。 |
| 2023 年 3 月 | Playfab.Services.C 拡張機能ライブラリの新しいターゲットを追加するように更新されました。 |
| 2023 年 6 月 | Xbox Oneタイトルは、既定の動作が VS 2019 から反転したため、VS 2022 以降で `/d2vzeroupper-` を使用する必要があります |
| 2023 年 10 月 | Microsoft GDK を使用するには、Windows 11 SDK (22000) 以降が必要です。 |


