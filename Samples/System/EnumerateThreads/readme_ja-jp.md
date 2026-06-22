![](./media/image1.png)

# EnumerateThread サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

[ToolHelp](https://docs.microsoft.com/windows/win32/api/tlhelp32/) API は、タイトル プロセス内で実行されているすべてのスレッドを列挙するために提供されます。 これらの API は OneCore ライブラリで使用できますが、現在 WINAPI_PARTITION_GAMES では使用できません。タイトル コードで使用できるようにするには、追加のコードが必要です。 現在の回避策は、次のコードで TlHelp32.h を含める前に、WINAPI_FAMILY_PARTITION を再定義することです。

// 注: ツール ヘルプ API は、現在ゲーム パーティションで定義されていません。 これは、タイトルが呼び出すために ToolHelp API を強制的に使用できるようにするための回避策です。 Xbox 本体では、タイトルを onecore_apiset.lib ライブラリにリンクする必要もあります。 その時点で、ToolHelp API は問題なく使用できます。

> # undef WINAPI_FAMILY_PARTITION
>
> # define WINAPI_FAMILY_PARTITION(Partitions) 1
>
> # include \<TlHelp32.h\>
>
> # undef WINAPI_FAMILY_PARTITION
>
> # define WINAPI_FAMILY_PARTITION(Partitions) (Partitions)

# サンプルの使用方法

このサンプルでは、5 つのバックグラウンド スレッドを作成してから、プロセスで実行されているすべてのスレッドとその名前と優先順位を列挙して一覧表示します。

# 更新履歴

初回のリリース: 2022 年 8 月

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


