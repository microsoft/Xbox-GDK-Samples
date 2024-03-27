# マルチ パッケージのサンプル

_このサンプルは、Microsoft Game Development Kit と互換性があります (2023 年 3 月)_

![画像](media/SampleImage.png)




### 説明

このサンプルでは、XlaunchURI とカスタム プロトコルを使用して複数のパッケージを管理する方法を示します。 プロジェクト MainPackageExperience と AlternatePackageExperience は、XLaunchURI API を介して相互に対話します。

### プロジェクトのセットアップ
特定のプロジェクト/パッケージの Microsoft Game 構成ファイル (.mgc) には、そのパッケージを起動するために使用されるカスタム プロトコル定義が含まれています。

このサンプルには 2 つのプロジェクトがあります。 このサンプルが正常に動作するためには、プロジェクトをインストールしてパッケージとして実行する必要があります。 これは次の方法で行われます。
1. プロジェクトをビルドする。
2. makepkg を使用して各プロジェクトのパッケージを作成してインストールする。

### パッケージをインストールして実行する。

2 つのパッケージがあるため、パッケージごとに次の手順を繰り返します。 以下のコマンドは、```MainPackageExperience``` ディレクトリと ```AlternatePacakgeExperience``` ディレクトリで実行する必要があります。

1. まず、パッケージのマッピング ファイルを生成します。 これを行うには、ビルド ディレクトリで makepkg genmap を実行します。
   ```makepkg genmap /f genChunk.xml /d Gaming.Xbox.Scarlett.x64\Debug```

2. 次に、パッケージを生成します。
```


3. Install the .xvc package file that was placed in your \<PACKAGE OUTPUT DIRECTORY\>. Depending on the package, it will either be
    ```xbapp install 41336MicrosoftATG.MultiPackageMainExperience_1.0.0.0_neutral__dspnxghe87tn0_xs.xvc```   
	or   
```


上記のコマンドは、他のプラットフォームや構成に一般化できます。

> デスクトップ用のパッケージをビルドする場合は、WDAPP インストールを伴う .MSIXVC パッケージ ファイルをインストールします。 インストール コマンドは次のようになります:

