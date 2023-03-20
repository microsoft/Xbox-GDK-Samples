  ![](./media/image1.png)

#   DumpTool サンプル

*このサンプルは、Microsoft ゲーム開発キットのプレビュー (2019 年 11 月)
に対応しています。*

# 

# 説明

DumpTool は Xbox One のタイトルと同じ OS
パーティション内で実行され、ツールの引数として名前を指定した別のプロセスのクラッシュ
ダンプを生成します。ツールをコンパイルしてすぐに使用したり、ソース
コードを取り入れて独自のツールまたはタイトルにクラッシュ
ダンプ機能を追加したりできます。

# サンプルのビルド

Project Scarlett を使用している場合は、Gaming.Xbox.Scarlett.x64
プラットフォーム構成をプロジェクトに追加する必要があります。この操作は、*構成マネージャー*を使って行うことができます。\[アクティブなソリューション
プラットフォーム\] で \[構成マネージャー\] オプションを選択し、次に
\[新規作成\]
を選択します。\[新しいプラットフォームを入力または選択してください\] に
Gaming.Xbox.Scarlett.x64、\[設定のコピー元\] にGaming.Xbox.XboxOne.x64
を設定します。次に \[OK\] を選択します。

*詳細については、GDK ドキュメントの*
「サンプルの実行」*を参照してください。*

# サンプルの使用

DumpTool は、タイトル モードのコンソール
アプリケーションとしてコンパイルします (「[MSDN
ホワイトペーパー](https://developer.xboxlive.com/en-us/platform/development/education/Documents/Title%20Mode%20Console%20Applications.aspx)」も参照してください)。
Visual Studio を使用してコンソールに .exe
を展開すると、実行中のすべてのアプリケーションが終了するため、.exe
をビルドしてコンソールにコピーし、これを実行するという操作を複数のステップで実行する必要があります。

1.  Visual Studio でツールをビルドし、DumpTool.exe を生成ます

2.  タイトル (あるいは SimpleTriangle サンプルなど) を起動します

3.  DumpTool.exe をゲーム OS パーティションにコピーします

\> xbcp /x/title Gaming.Xbox.x64\\Layout\\Image\\Loose\\\*.exe
xd:\\DumpTool\\

\> xbcp /x/title Gaming.Xbox.x64\\Layout\\Image\\Loose\\\*.dll
xd:\\DumpTool\\

4.  ツールを実行して、SimpleTriangle.exe のトリアージ ダンプを収集します

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage
SimpleTriangle.exe

5.  デバッグのために .dmp ファイルを開発用 PC にコピーします。

\> xbcp /x/title xd:\\SimpleTriangle.dmp

DumpTool プロジェクトには単純なバッチ ファイル runCommand.bat
が含まれています。このバッチ ファイルで最初の 4
つのステップが自動化されるため、コード変更を簡単にテストできます。

## DumpTool コマンド ライン

DumpTool は多数のコマンド ライン オプションもサポートしています。

使用法:DumpTool \[-mdt:\<ミニダンプの種類\> \...\]
\[-pdt:\<定義済みの種類\>\] \<実行可能ファイルの名前\>

\<ミニダンプの種類\>:Normal WithDataSegs WithFullMemory WithHandleData

FilterMemory ScanMemory WithUnloadedModules

WithIndirectlyReferencedMemory FilterModulePaths

WithProcessThreadData WithPrivateReadWriteMemory

WithoutOptionalData WithFullMemoryInfo WithThreadInfo

WithCodeSegs WithoutAuxiliaryState

WithFullAuxiliaryState WithPrivateWriteCopyMemory

IgnoreInaccessibleMemory WithTokenInformation

WithModuleHeaders FilterTriage

\<ミニダンプの種類\>: heap mini micro triage native

\<ミニダンプの種類\>
は、[GDNP](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/MINIDUMP_TYPE_typedef___dbghelp_Xbox_Microsoft_T_may17.aspx)
および
[MSDN](https://msdn.microsoft.com/en-us/library/windows/desktop/ms680519(v=vs.85).aspx)
に記載されている、MINIDUMP_TYPE 列挙の値に対応します。コマンド ラインで
--mdt: の複数のインスタンスを指定することによって、MINIDUMP_TYPE
のさまざまな値を組み合わせます。多くの可能性があることに注目してください。操作を簡単にするために、--pdt
オプションも用意されています。

"定義済みの種類\" (-pdt) オプションは、通常なら -mdt
オプションを使用して個別に指定する必要がある MINIDUMP_TYPE
フラグを簡素化するために存在します。定義済みの種類は、xbWatson.exe
でサポートされるクラッシュ ダンプの種類に対応します。

![](./media/image3.png)

例:

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:triage
SimpleTriangle.exe

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:Mini
SimpleTriangle.exe

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe -pdt:Heap
SimpleTriangle.exe

このツールでは、"micro" や "native"
も提供されることに注意してください。これらの値に対応するフラグの正確な組み合わせをソース
コードで確認してください。MiniDumpWriteDump ()
に慣れていない場合は、事前に定義されたダンプ
フラグで開始し、必要に応じて追加のフラグを試します。このツールでは、--pdt:
と --mdt
を同時に使用でき、フラグを組み合わせることができるため、実験が簡単になるはずです。

\> xbrun /x/title /O d:\\DumpTool\\DumpTool.exe --pdt:micro
--mdt:WithHandleData

--mdt:WithUnloadedModules SimpleTriangle.exe

## ツールの展開

DumpTool (または一部のバリエーション)
をタイトルで使用する場合は、ツールをゲーム OS
にコピーしなくてもよいように、ゲームの展開先にツールを追加することを検討してください。こうすると、実行中のタイトルを何らかの方法で中断させることなく、クラッシュ
ダンプを簡単に生成できます。

# 実装に関する注意事項

-   実行可能ファイルのコードから直接 MiniDumpWriteDump()
    を呼び出すこともできます。たとえば、多くの開発者はこの機能をハンドルされない例外フィルターに追加します。MiniDumpWriteDump
    を呼び出す簡単な例を次に示します。

> MiniDumpWriteDump(
>
> GetCurrentProcess(),
>
> GetProcessId(GetCurrentProcess()), hDumpFile, mdt, nullptr, nullptr,
> nullptr);

-   GSDK には、クラッシュ ダンプをキャプチャするために使用できる
    [xbWatson](https://developer.xboxlive.com/en-us/platform/development/documentation/software/Pages/xbwatson_may17.aspx)
    と呼ばれる軽量ツールが付属しています。DumpTool の機能は、xbWatson
    のクラッシュ ダンプ機能と同等です。xbWatson
    を使用するために追加の展開手順を実行する必要はありません。

-   Visual Studio を使用してクラッシュ
    ダンプをキャプチャすることもできます。\[デバッグ\] メニューから
    \[名前を付けてダンプを保存\]
    オプションを見つけます。このオプションは、プロセスにアタッチした後に表示され、一時停止
    (\"すべて中断\") したときにアクティブになります。

# 既知の問題

MiniDumpWriteDump を呼び出す前に GENERIC_WRITE および GENERIC_READ
の両方を指定してファイルを開いてください。そうしないと、作成される .dmp
ファイルが破損する可能性があります。

# 更新履歴

初期リリース: 2019 年 4 月
