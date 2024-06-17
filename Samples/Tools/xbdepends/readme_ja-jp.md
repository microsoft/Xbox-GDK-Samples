# xbdepends サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

これは、GDK タイトルのビルドと起動の問題の診断に役立つ、Windows 10/Windows 11 マシンのコマンド ラインです。 特に、EXE と DLL のインポート モジュールを分析し、それらを分類し、診断出力を提供します。 Xbox One および Scarlett バイナリの場合、xgameplatform.lib アンブレラ ライブラリには含まれていない OS API のリストも出力されます。

コマンド ライン オプションを指定せずに実行すると、次の出力が生成されます:

![説明は自動で生成されたものです](./media/image1.png)

# サンプルのビルド

単純なコマンドライン ツールとして、*ゲーム コマンド プロンプト*を使用して直接ビルドできます:

```
cl /EHsc /D_WIN32_WINNT=0x0A00 /Ox /MT xbdepends.cpp onecore.lib
```


CMake 3.20 以降を使用できます:

```
cmake -B out .
cmake --build out
```


CMake プリセットもあります:

```
cmake --list-presets
cmake --preset=x64-Debug
cmake --build out\build\x64-Debug
```


または、VS IDE から CMakeLists.txt を開くことができます (VS 2019 16.11 または VS 2022 が必要です)。

- ツールをデプロイしやすくするために、静的Visual C++ ランタイムを使用してビルドします。 一般に、タイトルの DLL ベースのランタイムには /MD を使用することをお勧めします。

# 使用法

簡単なテストでは、Microsoft GDK の基本的なテンプレートを使用します。 **Direct3D 12 Xbox Game** プロジェクトを作成してビルドします。 次に、レイアウト ディレクトリを指す **xbdepends** を実行します。

```
xbdepends Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Direct3DGame1.exe
```


次の出力が生成されます:

```
Microsoft (R) Xbox Binary Dependencies Tool
Copyright (C) Microsoft Corp.

reading 'Direct3DGame1.exe' [EXE]
INFO: Found 27 import modules
INFO: Use of Direct3D 12.X_S implies Scarlett target
INFO: Dependencies require 'VS 2019 (16.0)' or later C/C++ Runtime
```


より詳細な出力は、-v スイッチによってトリガーされます:

```
xbdepends -v Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Direct3DGame1.exe
```


次の出力が生成されます:

```
reading 'Direct3DGame1.exe' [EXE]
        Linker: 14.00
        OS: 6.00
        Subsystem: 2 (6.00)
INFO: Found 27 import modules
INFO: Use of Direct3D 12.X_S implies Scarlett target
INFO: Dependencies require 'VS 2019 (16.0)' or later C/C++ Runtime
  DLL 'api-ms-win-core-debug-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-errorhandling-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-handle-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-heap-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-interlocked-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-libraryloader-l1-2-0.dll' (OS)
  DLL 'api-ms-win-core-localization-l1-2-0.dll' (OS)
  DLL 'api-ms-win-core-memory-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-processthreads-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-processthreads-l1-1-1.dll' (OS)
  DLL 'api-ms-win-core-profile-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-psm-appnotify-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-psm-appnotify-l1-1-1.dll' (OS)
  DLL 'api-ms-win-core-registry-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-registry-l2-1-0.dll' (OS)
  DLL 'api-ms-win-core-rtlsupport-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-string-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-synch-l1-1-0.dll' (OS)
  DLL 'api-ms-win-core-sysinfo-l1-1-0.dll' (OS)
  DLL 'd3d12_xs.dll' (D3D)
  DLL 'ext-ms-win-rtcore-ntuser-message-l1-1-0.dll' (OS)
  DLL 'ext-ms-win-rtcore-ntuser-window-ansi-l1-1-0.dll' (OS)
  DLL 'ext-ms-win-rtcore-ntuser-window-l1-1-0.dll' (OS)
  DLL 'PIXEvt.dll' (GameOS)
  DLL 'ucrtbased.dll' (CRT)
  DLL 'VCRUNTIME140_1D.dll' (CRT)
  DLL 'VCRUNTIME140D.dll' (CRT)
```


/retail スイッチを使用して実行することもできます。このビルドでは Debug CRT 項目が使用されていることが警告されます。

```
xbdepends -retail Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\Direct3DGame1.exe
```


次の出力が生成されます:

```
reading 'Direct3DGame1.exe' [EXE]
INFO: Found 27 import modules
INFO: Use of Direct3D 12.X_S implies Scarlett target
ERROR: Using development only DLLs not for use in retail:
        ucrtbased.dll
        VCRUNTIME140_1D.dll
        VCRUNTIME140D.dll
INFO: Dependencies require 'VS 2019 (16.0)' or later C/C++ Runtime
```


このツールはワイルドカードも受け入れ、再帰的に実行できます:

```
xbdepends -r Direct3DGame1\Gaming.Xbox.Scarlett.x64\Layout\Image\Loose\*.dll
```


# 実装

実際には、このツールは、PE インポート テーブルのスキャンに関して Microsoft DUMPBIN ツールで実行できる操作と同じ種類の操作を行います。 主な違いは、このツールが GDK タイトルに関するいくつかの基本的なルールと知識を適用して診断出力を生成することです。

PE インポート テーブルの詳細については、以下を参照してください:

「*Windows の内部: Win32 ポータブル実行可能ファイル形式の詳細、パート 2*」。 MSDN マガジン (2002 年 3 月)

<https://docs.microsoft.com/en-us/archive/msdn-magazine/2002/march/inside-windows-an-in-depth-look-into-the-win32-portable-executable-file-format-part-2>

# 更新履歴

| 日付 | 注意 |
|---|---|
| 2021 年 5 月 | April 2021 GDK リリースの xgameplatform.lib を使用した初期リリース。 |
| 2022 年 1 月 | CMake クリーンアップとプリセット ファイルの追加 |
| 2022 年 11 月 | CMake 3.20 に更新されました |


