![](./media/image1.png)

# DumpTool サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

DumpTool では、Xbox One のタイトルと同じ、OS パーティションで実行し、ツールに引数として名前を指定して別のプロセスのクラッシュ ダンプを生成します。 すぐに使用するためにツールをコンパイルするか、ソース コードから借りて、クラッシュ ダンプ機能を独自のツールやタイトルに追加することができます。

# サンプルのビルド

プラットフォーム構成をプロジェクトに追加します。 これを行うには、*構成マネージャー* を使用します。\"アクティブ ソリューション プラットフォーム\" でオプション \"構成マネージャー\" を選択し、\"新規\...\" を選択します。 \"新しいプラットフォームを入力または選択してください\" を Gaming.Xbox.Scarlett.x64 に、\"設定をコピー元\" を Gaming.Xbox.XboxOne.x64 に設定します。 次に、[OK] を選択します。 *詳細については、**GDK ドキュメント*の「__サンプルの実行__」を参照してください。
| | |
|---|---|
|Xbox Series X|S を使用している場合は、Gaming.Xbox.Scarlett.x64| を追加する必要があります


# サンプルの使用方法

DumpTool はタイトル モード コンソール アプリケーションとしてコンパイルされます ([MSDN ホワイト ペーパー](https://developer.xboxlive.com/en-us/platform/development/education/Documents/Title%20Mode%20Console%20Applications.aspx) もご覧ください)。 Visual Studio を使用してコンソールに .exe を展開すると、実行中のアプリケーションがシャットダウンされるため、.exe をビルドしてからコンソールにコピーし、複数の手順で実行する必要があります。

1. Visual Studio でツールをビルドして DumpTool.exe を生成する

2. タイトルを起動する (または SimpleTriangle サンプルなど)

3. DumpTool.exe をゲーム OS パーティションにコピーする

```
xbcp /x/title Gaming.Xbox.x64\Layout\Image\Loose\*.exe xd:\DumpTool\
xbcp /x/title Gaming.Xbox.x64\Layout\Image\Loose\*.dll xd:\DumpTool\
```


4. ツールを実行して SimpleTriangle.exe のトリアージ ダンプを収集する

```
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:triage SimpleTriangle.exe
```


5. デバッグのために .dmp ファイルを開発用 PC にコピーし直す

```
xbcp /x/title xd:\SimpleTriangle.dmp
```


DumpTool プロジェクトには、最初の 4 つの手順を自動化し、コード変更を簡単にテストできる単純なバッチ ファイル runCommand.bat が含まれています。

## DumpTool コマンド ライン

DumpTool では、コマンド ライン オプションの豊富なセットもサポートされています。

```
Usage: DumpTool [-mdt:<minidump type> ...] [-pdt:<predefined type>] <executable name>

  <minidump type>: Normal WithDataSegs WithFullMemory WithHandleData
        FilterMemory ScanMemory WithUnloadedModules
        WithIndirectlyReferencedMemory FilterModulePaths
        WithProcessThreadData WithPrivateReadWriteMemory
        WithoutOptionalData WithFullMemoryInfo WithThreadInfo
        WithCodeSegs WithoutAuxiliaryState
        WithFullAuxiliaryState WithPrivateWriteCopyMemory
        IgnoreInaccessibleMemory WithTokenInformation
        WithModuleHeaders FilterTriage

<predefined type>: heap mini micro triage native
```


\<minidump type\>s は、ドキュメントに記載されている MINIDUMP_TYPE 列挙体の値に対応します
[GDNP](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/MINIDUMP_TYPE_typedef___dbghelp_Xbox_Microsoft_T_may17.aspx)
および [MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519(v=vs.85).aspx) です。 コマンド ラインで --mdt: の複数のインスタンスを指定して、MINIDUMP_TYPEのさまざまな値を結合します。 多くの可能性があることに注意してください。 より簡単にするために、ツールには --pst オプションも用意されています。

通常は -mdt オプションを使用して個別に指定する必要がある MINIDUMP_TYPE フラグを簡略化するために、"定義済みの型" (-pst) オプションが存在します。 定義済みの型は、xbWatson.exe でサポートされているクラッシュ ダンプの種類に対応します。

![](./media/image3.png)

例:

```
xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage SimpleTriangle.exe
```


```
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:Mini SimpleTriangle.exe
```


```
xbrun /x/title /O d:\DumpTool\DumpTool.exe -pdt:Heap SimpleTriangle.exe
```


このツールには "micro" と "native" も用意されていることに注意してください。 これらの値に対応するフラグの正確な組み合わせについては、ソース コードを参照してください。 **MiniDumpWriteDump()** に慣れていない場合は、定義済みのダンプ フラグから始めて、必要に応じて追加のフラグを試してください。 このツールは、この実験を容易にする必要があります。このツールでは、`--pdt` と `--mdt` の両方を同時に許可し、フラグを組み合わせるためです。

```
xbrun /x/title /O d:\\DumpTool\\DumpTool.exe --pdt:micro --mdt:WithHandleData --mdt:WithUnloadedModules SimpleTriangle.exe
```


## ツールの展開

タイトルで DumpTool (またはバリエーション) を使用する予定の場合は、ゲーム OS にコピーする必要がないように、ゲームの展開にツールを追加することを検討してください。 このツールは、実行中のタイトルを他の方法で中断することなく、クラッシュ ダンプを生成する便利な方法を提供します。

# 実装メモ

- 実行可能ファイルのコードから **MiniDumpWriteDump()** を直接呼び出すこともできます。 たとえば、多くの開発者は、ハンドルされない例外フィルターにこの機能を追加します。 MiniDumpWriteDump の呼び出しの非常に簡単な例を次に示します。

```
MiniDumpWriteDump(
  GetCurrentProcess(),
  GetProcessId(GetCurrentProcess()), hDumpFile, mdt, nullptr, nullptr,
  nullptr);
```


- GSDK には、クラッシュダンプのキャプチャに使用できる [xbWatson](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/xbwatson_may17.aspx) と呼ばれる軽量ツールも付属しています。 DumpTool の機能は、xbWatson のクラッシュ ダンプ機能と同じです。 xbWatson を使用するために追加のデプロイ手順を実行する必要はありません。

- Visual Studio を使用してクラッシュ ダンプをキャプチャすることもできます。 [デバッグ] メニューのオプションで、"名前を付けてダンプを保存..." を探します。 このオプションは、プロセスにアタッチすると表示され、一時停止するとアクティブになります ("すべて中断")。

# 既知の問題

**MiniDumpWriteDump** を呼び出す前に `GENERIC_WRITE` と `GENERIC_READ` の両方を使用してファイルを開きます。それ以外の場合は、.dmp ファイルが破損している可能性があります。

# 更新履歴

2019 年 4 月の初期リリース。


