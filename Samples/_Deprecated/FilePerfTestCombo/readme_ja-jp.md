# FilePerfTestCombo Sample

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

このサンプルは、ドキュメントの「[Xbox One でのファイル パフォーマンスの最大化](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_xbox_one)」および「[Project Scarlett でのパフォーマンスの最大化](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_scarlett)」ページで動作します。 ドキュメントに示されているすべての数値は、このサンプルから生成されました。 このサンプルを使用すると、ベンチマーク手法を調べたり、他の構成をモデル化したりすることができます。

# サンプルの使用方法

すべてのコマンドライン オプションは、FilePerfTestCombo.cpp and the Sample::ParseCommandLine 関数にあります。

最初の手順は、テスト構成に必要なデータ ファイルを生成することです。

1) データ ファイルのドライブに少なくとも 30 GiB の空きがあることを確認します。

2) Gaming.Desktop.x64 プラットフォームのリリース構成を選択します。

3) コマンド ラインを設定します。

```
performfullsetup createpackedfile numiterations 5
```



4) ビルドして実行します。

a.   ハードウェアによっては、これに数分かかる場合があります。

zlib を使用して展開テスト構成に必要なデータ ファイルを使用できる場合。 コマンド ラインを以下に変更します

```
performzipsetup ratio \[compression ratio\] createpackedfile numiterations
```


\[圧縮率\] には、0 から 100 までの任意の数値を指定できます。 既定値は 50 です。 これは圧縮率 50% を表し、圧縮ファイルは元のファイルのサイズの 50% になります。

> 注: ハードウェアによっては、1 時間以上、200 GiB 以上のディスク領域が必要になる場合があります。

さまざまなテスト構成は、主にコマンド ラインを介して制御されます。オプションでは大文字と小文字は区別されません。

- 実行するテストの種類 - これらはすべて、特定の複数のテスト構成に同じコマンド ラインで使用できます。

   - DoSync

      - Win32 操作と同期操作を使用します。

   - DoASync

      - Win32 操作と重複操作を使用します。

   - DoSyncDStorage

      - 同期方法で DirectStorage を使用します。

   - DoASyncDStorage

      - 非同期方法で DirectStorage を使用します。

   - DoZip

      - DirectStorage と展開ハードウェアを使用します。

      - 展開データを使用する必要があります。

- NumIterations \[count\]

   - テスト構成ごとに実行するイテレーションの数。

- Load \[first order\] \[last order\]

   - テスト構成に使用する最初と最後の順序の読み取りセット。

   - 順番になっている有効なオプション。

      - True_Sequential

      - Random (ランダム)

      - Random_Sequential

      - Backwards

      - Redundant

- Size \[first size\] \[last size\]

   - テスト構成に使用する最初と最後の読み取りサイズ。

   - 順番になっている有効なオプション。

      - size_8k

      - size_12k

      - size_16k

      - size_32k

      - size_64k

      - size_128k

      - size_192k

      - size_256k

      - size_512k

      - size_1024k

      - size_2048k

      - size_4096k

      - size_8192k

      - size_16384k

      - size_32768k

- Depth \[first depth\] \[last depth\]

   - Win32 非同期テスト構成に使用する最初と最後のキューの深さ。

   - 順番になっている有効なオプション。

      - depth_1

      - depth_2

      - depth_4

      - depth_8

      - depth_12

      - depth_16

      - depth_24

      - depth_32

      - depth_64

      - depth_128

      - depth_256

      - depth_512

      - depth_768

      - depth_1024

      - depth_2048

      - depth_4096

- UsePackedFile

   - 1 つのファイルの使用、または複数の小さいファイルの使用。

- DoRealtime

   - リアルタイム優先度キューを使用する DirectStorage テストの修飾子。

コマンド ラインの例

```
DoASync NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```


4 から 64 のキューの深さと 8KiB から 32MiB の読み取りサイズを使用し、真のシーケンシャルとランダムな場所をすべて組み合わせて Win32 非同期テストを実行します。

```
DoASyncDStorage NumIterations 5 load True_Sequential Random Size Size_8k Size_32768k
```


真のシーケンシャルとランダムな場所をすべて組み合わせて、8KiB から 32MiB の読み取りサイズで、DirectStorage 非同期テストを実行します。

```
DoASync DoASyncDStorage NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```


4 から 64 のキューの深さと 8KiB から 32MiB の読み取りサイズを使用し、真のシーケンシャルとランダムな場所をすべての組み合わせて、Win32 非同期テストと DirectStorage 非同期テストを実行します。

> 注: キューの深さは、そのオプションを使用する唯一のテストの種類であるため、Win32 テストにのみ適用されます。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Scarlett 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

デスクトップ PC を使用している場合は、アクティブなソリューション プラットフォームを に設定します `Gaming.Desktop.x64`。

*詳細については、* GDK ドキュメントの「__サンプルの実行__」* を参照してください。*

# 更新履歴

初期リリース 2020 年 11 月

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


