  ![](./media/image1.png)

#   충돌 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 

# 설명

이 샘플에서는 Xbox One 앱의 간단한 경계 볼륨 테스트를 위한 DirectXMath
충돌 유형을 보여 줍니다.

![C:\\temp\\xbox_screenshot.png](./media/image3.png)

# 샘플 빌드

Project Scarlett을 사용하는 경우에는 프로젝트에 Gaming.Xbox.Scarlett.x64
플랫폼 구성을 추가해야 합니다. *Configuration Manager*를 통해 이 작업을
수행할 수 있습니다. \"활성 솔루션 플랫폼\"에서 \"Configuration Manager\"
옵션을 선택하고 \"새로 만들기\...\"를 선택합니다. \"새 플랫폼 입력 또는
선택\"을 Gaming.Xbox.Scarlett.x64로 설정하고 \"다음에서 설정 복사\"를
Gaming.Xbox.XboxOne.x64로 설정합니다. 그런 다음, 확인을 선택합니다.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용

이 샘플에서는 4개의 다른 \'충돌\' 그룹을 보여 줍니다.

1.  애니메이션 구, 축 정렬 상자, 방향성 상자 및 삼각형과 충돌하는 고정
    경계 **절두체**

2.  애니메이션 구, 축 정렬 상자, 방향성 상자 및 삼각형과 충돌하는 **고정
    축 정렬 상자**

3.  애니메이션 구, 축 정렬 상자, 방향성 상자 및 삼각형과 충돌하는 고정
    **방향성 상자**

4.  고정 구, 축 정렬 상자, 방향성 상자 및 삼각형과 충돌하는 애니메이션
    **광선** 광선이 적중되면 대상 개체의 교차점에 표식 상자가 놓입니다.

| 작업                         |  게임패드                              |
|------------------------------|---------------------------------------|
| 그룹 주변 X/Y에 따라 카메라 궤도 조작 |  오른쪽 썸스틱 |
| 보기 초기화                  |  오른쪽 썸스틱 단추                    |
| 절두체 그룹에 포커스         |  D-패드 위로                           |
| 축 정렬 상자 그룹에 포커스   |  D-패드 오른쪽                         |
| 방향성 상자 그룹에 포커스    |  D-패드 아래로                         |
| 광선 테스트 그룹에 포커스    |  D-패드 왼쪽                           |
| 도움말 토글                  |  메뉴 단추                             |
| 종료                         |  보기 단추                             |

# 구현 참고 사항

DirectXMath의 경계 볼륨 유형에 대한 자세한 내용은 다음에 대한 [Microsoft
설명서](https://docs.microsoft.com/en-us/windows/desktop/dxmath/directxmath-portal)를
참조하세요.

-   **BoundingBox** 클래스

-   **BoundingFrustum** 클래스

-   **BoundingOrientedBox** 클래스

-   **BoundingSphere** 클래스

-   **TriangleTests** 네임스페이스

[GitHub](https://github.com/Microsoft/DirectXMath)에서 최신 버전의
DirectXMath를 사용할 수 있습니다.

# 알려진 문제

DirectXMath **BoundingFrustum** 클래스는 왼손잡이용 보기 시스템에서만
작동합니다.

# 업데이트 기록

이 샘플 Xbox One XDK 버전의 초기 릴리스는 2016년 5월에 작성되었습니다.
이 샘플의 최신 레거시 DirectX SDK 버전은
[GitHub](https://github.com/walbourn/directx-sdk-samples/tree/master/Collision)에서
찾을 수 있습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 일반 개인정보취급방침에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.
