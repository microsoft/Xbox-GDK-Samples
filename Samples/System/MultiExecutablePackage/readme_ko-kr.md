# MultiExecutable 패키지 샘플

_이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월)._

![이미지](SampleImage.png)




### 설명

여러 실행 파일을 사용하여 솔루션을 설정하는 방법을 보여주는 샘플입니다. 이 구현은 여러 프로젝트를 만들고, 이를 패키지로 묶어 함께 실행할 수 있도록 설정하는 방식으로 이루어집니다.



### 프로젝트 설정

이 샘플에는 많은 프로젝트가 있으며, 제대로 작동하기 위해 다음과 같은 방식으로 설정됩니다.

- DefaultExperience는 초기 프로젝트를 나타내며, 처음에 시작되는 프로젝트이기도 합니다.

- AlternateExperience는 DefaultExperience에서 Xlaunched할 수 있는 다른 게임을 나타냅니다.

- ComboDLL은 런타임 중에 샘플에 로드할 수 있는 DLL입니다.
   - 관리되지 않는 언어와 함께 사용하기 위해 DLL을 만들 때 내보낸 함수는 대체로 'C' 연결을 사용하여 호환성을 최대한 보장합니다. 즉, 함수 서명 또는 DLL에 노출하는 모든 구조가 C 언어와 호환되어야 합니다. 그러나 이는 DLL의 내부 구현 안에서 C++ 사용을 배제하는 것은 아니며, DLL 사용자에게 노출되는 인터페이스에만 영향을 미칩니다.
- CPUTool은 샘플로부터 실행 가능한 새 프로세스에서 별도의 실행 파일을 생성하는 프로젝트입니다.

- SharedAssetsProject에는 모든 프로젝트 전반의 공유 코드가 포함되어 있습니다. 여기에는 사용자 로그인 관리, 로깅, 외부 프로세스(CPUTool) 실행을 위한 코드가 포함됩니다.



### 프로젝트 빌드

- Directx12가 먼저 빌드됩니다. 이것은 MSFT 샘플의 필수 요소일 뿐입니다.

- DefaultExperience는 CPUTool에 따라 다릅니다. 초기 프로그램 전에 모든 유틸리티 실행 파일이 빌드되는지 확인하려고 합니다.

- AlternateExperience는 DefaultExperience에 따라 다릅니다.

- 콘솔에서 실행하는 데 필요한 모든 파일은 .\\DefaultExperience\\%TARGET%\\Layout\\Image\\Loose에 저장됩니다.

- 데스크톱에서 실행하는 데 필요한 모든 파일은 .\\Gaming.Desktop.x64\\%CONFIG%에 저장됩니다.



### 샘플을 실행합니다.



#### 방법 1: Visual Studio에서 실행

- 샘플을 실행하려면 완전히 빌드해야 합니다. 이는 빌드 메뉴로 이동한 후 빌드를 클릭하면 완료됩니다. 샘플을 실행하면 기본 환경이 초기 프로젝트이므로 제일 먼저 실행됩니다.



#### 방법 2: 패키지 만들기에서 실행

� � 1. 프로젝트를 빌드합니다.

� � 2. Makepkg를 지원하는 터미널 내에서 GenScarlettXVCPackage.bat, GenXboxOneXVCPackage 또는 GenDesktopMSIXVCPackage.bat을 실행합니다. 각 플랫폼에 일괄 처리 파일이 하나씩 총 3개가 있습니다.

� � � �.\\DefaultExperience\\%Target%\\Layout\\Image에서 콘솔 패키지 파일을 찾을 수 있습니다.

� � � �.\\Gaming.Desktop.x64\\Layout\\Image에서 데스크톱 패키지 파일을 찾을 수 있습니다.

##### 패키지 설치 및 실행.

� � 1. Xbox에서 실행하는 경우, 콘솔 패키지 파일이 포함된 디렉터리에서 Xbox 관리자를 통해 개발 키트에 .xvc 파일을 복사합니다. xbapp 설치를 통해 패키지를 설치할 수도 있습니다. 설치 명령은 다음과 비슷하게 표시됩니다. � �
```xbapp install 41336MicrosoftATG.MultiExecutablePackage_1.0.0.0_neutral__dspnxghe87tn0_xs.xvc```

� � 2. 데스크톱에서 실행하는 경우, 데스크톱 패키지 파일이 포함된 디렉터리에서 WDAPP 설치와 함께 .MSIXVC 파일을 설치합니다. 설치 명령은 다음과 비슷하게 표시됩니다.
� �
```



## Update history

**Initial Release:** Microsoft Game Development Kit (June 2023)

June 2023: Initial release

## Privacy Statement

When compiling and running a sample, the file name of the sample executable will be sent to Microsoft to help track sample usage. To opt-out of this data collection, you can remove the block of code in Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see the [Microsoft Privacy Statement](https://privacy.microsoft.com/en-us/privacystatement/).



