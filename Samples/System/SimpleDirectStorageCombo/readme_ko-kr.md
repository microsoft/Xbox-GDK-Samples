  ![](./media/image1.png)

#   SimpleDirectStorageCombo Sample

이 샘플은 Microsoft 게임 개발 키트와 호환 가능합니다(2020년 6월).

# 설명

이 샘플에서는 콘솔과 데스크톱 모두에서 DirectStorage를 사용하는 여러
가지 방법을 보여 줍니다.

-   SimpleLoad -- DirectStorage를 초기화하고, 파일을 열고, 요청을 큐에
    넣고, 완료를 기다리기 위한 최소 인터페이스입니다.

-   StatusBatch -- 알림을 위해 상태 배열을 사용하여 요청 배치를 만드는
    방법을 보여 줍니다.

-   StatusFence -- 알림을 위해 ID3DFence를 사용하여 요청 배치를 만드는
    방법을 보여 줍니다.

-   MultipleQueues -- 다른 우선 순위 수준을 사용하여 여러 큐를 만드는
    방법을 보여 줍니다.

-   취소 -- 보류 중인 요청을 취소하는 방법을 보여 줍니다.

-   RecommendedPattern -- DirectStorage를 사용하여 최대 성능을 얻기 위한
    권장 패턴을 보여 줍니다.

-   Xbox 하드웨어 압축 풀기 -- Xbox Series X|S 콘솔에서 실행할 때
    하드웨어 zlib 압축을 푸는 방법을 보여 줍니다.

-   Xbox 메모리 내 하드웨어 압축 해제 -- Xbox Series X|S 콘솔에서 사용할
    수 있는 하드웨어 zlib 압축 풀기를 사용하여 이미 메모리에 있는
    데이터의 압축을 푸는 방법을 보여 줍니다.

-   Xbox 소프트웨어 압축 풀기 -- Xbox One 제품군 콘솔에서 실행할 때
    소프트웨어 zlib 압축 풀기를 사용하는 방법을 보여 줍니다.

# 샘플 빌드

이 샘플은 다음 플랫폼을 지원합니다.

-   Gaming.Desktop.x64

    -   PC API 집합에서 DirectStorage 사용

-   Gaming.Scarlett.xbox.x64

    -   Xbox Series X|S 콘솔에서 사용할 수 있는 Xbox DirectStorage 구현
        사용

-   Gaming.XboxOne.xbox.x64

    -   Xbox DirectStorage 구현 기능을 제공하지만 내부적으로 Win32 API
        집합을 사용하는 제공된 소프트웨어 에뮬레이션 계층을 사용합니다.

자세한 내용은 GDK 설명서에서 샘플 실행을 참조하세요.

# 샘플 사용

샘플은 자동으로 데이터 파일을 만든 다음 언급된 각 하위 조각을
실행합니다.

# 구현 참고 사항

모든 구현은 SampleImplementations 폴더에 포함되어 있습니다. 수행된 각
단계에 대한 세부 정보가 자세히 설명되어 있습니다.

BCPack 압축을 사용하는 방법에 대한 예제는 TextureCompression 샘플을
참조하세요.

zlib 라이브러리(버전 1.2.11)에는 다음 라이선스가 적용됩니다.
<http://zlib.net/zlib_license.html>

# 업데이트 기록

2022년 2월 최초 릴리스

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행
파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 \"샘플 사용량 원격 분석\"으로 레이블이 지정된
Main.cpp에서 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
