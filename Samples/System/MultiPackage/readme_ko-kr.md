# 다중 패키지 샘플

_이 샘플은 Microsoft 게임 개발 키트(2023년 3월)와 호환됩니다._

![이미지](media/SampleImage.png)




### 설명

이 샘플에서는 XlaunchURI 및 사용자 지정 프로토콜을 사용하여 여러 패키지를 관리하는 방법을 보여줍니다. MainPackageExperience 및 AlternatePackageExperience 프로젝트는 XLaunchURI API를 통해 서로 상호 작용합니다.

### 프로젝트 설정
특정 프로젝트/패키지의 Microsoft 게임 구성 파일(.mgc)에는 해당 패키지를 시작하는 데 사용되는 사용자 지정 프로토콜 정의가 포함되어 있습니다.

이 샘플에는 두 개의 프로젝트가 있습니다. 이 샘플이 제대로 작동하려면 프로젝트를 설치하고 패키지로 실행해야 합니다. 이 작업은 다음을 통해 수행됩니다.
1. 프로젝트를 빌드합니다.
2. makepkg를 통해 각 프로젝트에 대한 패키지를 만들고 설치합니다.

### 패키지를 설치하고 실행합니다.

2개의 패키지가 있으므로 각 패키지에 대해 다음 단계를 반복합니다. 아래 명령은 ```MainPackageExperience``` 및 ```AlternatePacakgeExperience``` 디렉터리에서 실행해야 합니다.

1. 먼저 패키지에 대한 매핑 파일을 생성합니다. 빌드 디렉터리에서 makepkg genmap을 실행하여 이 작업을 수행할 수 있습니다.
   ```makepkg genmap /f genChunk.xml /d Gaming.Xbox.Scarlett.x64\Debug```

2. 그런 다음 패키지를 생성합니다.
```


3. Install the .xvc package file that was placed in your \<PACKAGE OUTPUT DIRECTORY\>. Depending on the package, it will either be
    ```xbapp install 41336MicrosoftATG.MultiPackageMainExperience_1.0.0.0_neutral__dspnxghe87tn0_xs.xvc```   
	or   
```


위의 명령은 다른 플랫폼 및 구성으로 일반화될 수 있습니다.

> 데스크톱용 패키지를 빌드하는 경우 WDAPP 설치가 포함된 .MSIXVC 패키지 파일을 설치합니다. 설치 명령은 다음과 유사합니다.

