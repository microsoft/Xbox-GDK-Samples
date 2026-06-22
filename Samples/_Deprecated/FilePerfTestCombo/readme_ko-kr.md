# FilePerfTestCombo 샘플

*이 샘플은 Microsoft 게임 개발 키트와 호환됩니다(2022년 3월).*

# 설명

이 샘플은 설명서의 [Xbox One 파일 성능 극대화](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_xbox_one) 및 [Project Scarlett 성능 극대화](https://developer.microsoft.com/games/xbox/docs/gdk/maximizing_file_performance_win32_scarlett) 페이지와 함께 작동합니다. 설명서에 표시된 모든 번호는 이 샘플에서 생성된 것입니다. 이 샘플을 사용하면 벤치마크 방법론을 검토할 수 있을 뿐만 아니라 다른 구성 모델도 변경할 수 있습니다.

# 샘플 사용

모든 명령줄 옵션은 FilePerfTestCombo.cpp 및 Sample::ParseCommandLine 함수에서 찾을 수 있습니다.

첫 번째 단계는 테스트 구성에 필요한 데이터 파일을 생성하는 것입니다.

1) 드라이브에 데이터 파일을 위한 30GiB 이상의 사용 가능한 공간이 있는지 확인합니다.

2) Gaming.Desktop.x64 플랫폼에 대한 릴리스 구성을 선택합니다.

3) 명령줄을 설정합니다.

```
performfullsetup createpackedfile numiterations 5
```



4) 빌드하고 실행합니다.

a.  하드웨어에 따라 몇 분 정도 걸릴 수 있습니다.

원한다면 데이터 파일은 zlib를 사용하여 압축 해제 테스트 구성에 사용할 수 있습니다. 명령줄을 다음으로 변경합니다.

```
performzipsetup ratio \[compression ratio\] createpackedfile numiterations
```


\[compression ratio\]은 0에서 100 사이의 임의의 숫자일 수 있습니다. 기본값은 50입니다. 이는 50%의 압축 비율을 나타내며, 압축된 파일은 원래 파일 크기의 50%입니다.

> 참고: 하드웨어에 따라 1시간 이상, 200GiB 이상의 디스크 공간이 소요될 수 있습니다.

다양한 테스트 구성은 대부분 명령줄을 통해 제어되며 옵션은 대소문자를 구분하지 않습니다.

- 실행할 테스트 유형 - 다음 모두는 동일한 명령줄에서 특정 여러 테스트 구성에 대해 사용될 수 있습니다.

   - DoSync

      - Win32 및 동기 작업을 사용합니다.

   - DoASync

      - Win32 및 중첩된 작업을 사용합니다.

   - DoSyncDStorage

      - 동기 방식으로 DirectStorage를 사용합니다.

   - DoASyncDStorage

      - DirectStorage를 비동기 방식으로 사용합니다.

   - DoZip

      - DirectStorage 및 압축 해제 하드웨어를 사용합니다.

      - 압축 해제 데이터를 사용해야 합니다.

- NumIterations \[count\]

   - 각 테스트 구성에 대해 수행할 반복 횟수입니다.

- Load \[first order\] \[last order\]

   - 테스트 구성에 사용할 첫 번째 및 마지막 순서 읽기 집합입니다.

   - 순서대로 유효한 옵션입니다.

      - True_Sequential

      - 임의

      - Random_Sequential

      - Backwards

      - 중복

- 크기 \[first size\] \[last size\]

   - 테스트 구성에 사용할 첫 번째 및 마지막 읽기 크기입니다.

   - 순서대로 유효한 옵션입니다.

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

   - Win32 비동기 테스트 구성에 사용할 첫 번째 및 마지막 큐 깊이입니다.

   - 순서대로 유효한 옵션입니다.

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

   - 단일 파일을 사용할지 여러 작은 파일을 사용할지 여부입니다.

- DoRealtime

   - DirectStorage 테스트에서 실시간 우선 순위 큐를 사용하기 위한 한정자입니다.

몇 가지 예제 명령줄

```
DoASync NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```


4에서 64 사이의 큐 깊이와 8KiB에서 32MiB 사이의 읽기 크기를 사용하여 실제 순차 및 임의 위치에 대한 모든 조합으로 Win32 비동기 테스트를 수행합니다.

```
DoASyncDStorage NumIterations 5 load True_Sequential Random Size Size_8k Size_32768k
```


실제 순차 및 임의 위치에 대한 모든 조합과 8KiB에서 32MiB 사이의 읽기 크기를 사용하여 DirectStorage 비동기 테스트를 수행합니다.

```
DoASync DoASyncDStorage NumIterations 5 load True_Sequential Random Depth Depth_4 Depth_64 Size Size_8k Size_32768k
```


4에서 64 사이의 큐 깊이와 8KiB에서 32MiB 사이의 읽기 크기를 사용하여 실제 순차 및 임의 위치에 대한 모든 조합으로 Win32 및 DirectStorage 비동기 테스트를 수행합니다.

> 참고: 큐 깊이는 해당 옵션을 사용하는 유일한 테스트 유형이므로 Win32 테스트에만 적용됩니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Scarlett 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

데스크톱 PC를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Desktop.x64`(으)로 설정합니다.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 살펴보세요.* 

# 업데이트 기록

2020년 11월 초기 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


