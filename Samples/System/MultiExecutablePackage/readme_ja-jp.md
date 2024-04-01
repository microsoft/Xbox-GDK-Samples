# MultiExecutable パッケージ サンプル

_このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)_

![画像](SampleImage.png)




### 説明

複数の実行可能ファイルを使用してソリューションを設定する方法を示すサンプル。 この実装は、複数のプロジェクトを作成し、それらをパッケージ化して一緒に実行できるように設定することによって行われます。



### プロジェクトのセットアップ

このサンプルには多くのプロジェクトがあり、適切に動作するために、次のように設定されています:

- DefaultExperience はスタートアップ プロジェクトを表し、最初に起動されるプロジェクトでもあります。

- AlternateExperience は、DefaultExperience から Xlaunched できる他のゲームを表します。

- ComboDLL は、実行時にサンプルに読み込むことができる DLL です。
   - 非マネージド言語で使用する DLL を作成する場合、エクスポートされた関数は通常、最大の互換性を確保するために 'C' リンケージを使用することに注意してください。 つまり、関数シグネチャ、または DLL で公開する構造体は、C 言語と互換性がある必要があります。 ただし、これは DLL の内部実装内での C++ の使用を妨げるわけではありません。DLL のユーザーに公開されているインターフェイスにのみ影響します。
- CPUTool は、サンプルから実行できる新しいプロセスで別の実行可能ファイルを生成するプロジェクトです。

- SharedAssetsProject には、すべてのプロジェクト間の共有コードが含まれています。 これには、ユーザー ログインを管理し、ログを記録し、外部プロセス (CPUTool) を実行するためのコードが含まれています。



### プロジェクトのビルド

- Directx12 が最初にビルドされます。 これは、MSFT サンプルの基本要素にすぎません。

- DefaultExperience は CPUTool に依存します。 スタートアップ プログラムの前にすべてのユーティリティ実行可能ファイルが確実にビルドされているようにするためです。

- AlternateExperience は DefaultExperience に依存します。

- 本体で実行するために必要なすべてのファイルは、最終的に .\\DefaultExperience\\%TARGET%\\Layout\\Image\\Loose に含められます

- デスクトップで実行するために必要なすべてのファイルは、最終的に .\\Gaming.Desktop.x64\\%CONFIG% に含められます



### サンプルを実行します。



#### 方法 1: Visual Studioから実行する

- サンプルを実行するには、完全にビルドされている必要があります。 これを行うには、[ビルド メニュー] に移動し、[ビルド] をクリックします。 サンプルを実行すると、既定のエクスペリエンスがスタートアップ プロジェクトとして最初に実行されます。



#### 方法 2: パッケージの作成から実行する

� � 1. プロジェクトをビルドします。

� � 2. Makepkg をサポートするターミナル内で GenScarlettXVCPackage.bat、GenXboxOneXVCPackage、または GenDesktopMSIXVCPackage.bat を実行します。 3 つのバッチ ファイルがあり、プラットフォームごとに 1 つです。

� � � � コンソール パッケージ ファイルは、.\\DefaultExperience\\%Target%\\Layout\\Image にあります。

� � � � デスクトップ パッケージ ファイルは、.\\Gaming.Desktop.x64\\Layout\\Image にあります。

##### パッケージのインストールと実行。

� � 1. Xbox で実行している場合は、本体パッケージ ファイルを含むディレクトリから Xbox Manager を使用して開発キットに .xvc ファイルをコピーします。 xbapp インストールを使用してパッケージをインストールすることもできます。 インストール コマンドは次のようになります: � �
```xbapp install 41336MicrosoftATG.MultiExecutablePackage_1.0.0.0_neutral__dspnxghe87tn0_xs.xvc```

� � 2. デスクトップで実行している場合は WDAPP インストールを含むデスクトップ パッケージ ファイルを含むディレクトリの .MSIXVC ファイルをインストールします。 インストール コマンドは次のようになります:
� �
```



## Update history

**Initial Release:** Microsoft Game Development Kit (June 2023)

June 2023: Initial release

## Privacy Statement

When compiling and running a sample, the file name of the sample executable will be sent to Microsoft to help track sample usage. To opt-out of this data collection, you can remove the block of code in Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).



