# 간단한 베지어 샘플

*이 샘플은 Microsoft 게임 개발 키트 미리 보기(2019년 11월)와
호환됩니다.*

# 설명

이 샘플에서는 DirectX 12를 사용하여 Mobius 스트립을 나타내는 공간 분할된
베지어 표면을 그리는 헐 및 도메인 셰이더를 만드는 방법을 보여 줍니다.

![Sample Screenshot](./media/image1.png)

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.XboxOne.x64로 설정하세요.

Project Scarlett을 사용하는 경우 활성 솔루션 플랫폼을
Gaming.Xbox.Scarlett.x64로 설정하세요.

*자세한 내용은 GDK 문서에서* 샘플 실행하기*를 참조하세요.*

# 샘플 사용하기

이 샘플은 다음 컨트롤을 사용합니다.

| 작업                                       |  게임패드                |
|--------------------------------------------|-------------------------|
| 음영/와이어프레임 렌더링                   |  Y 단추                  |
| 공간 분할 방법 선택: -   정수 -   짝수 분수 -   홀수 분수 |  X 단추 A 단추 B 단추 |
| 패치 분할 수 \<4, 16\> 줄이기/늘리기  |  왼쪽/오른쪽 트리거 길게 누르기                  |
| 카메라 왼쪽/오른쪽으로 회전  |  왼쪽 썸스틱 왼쪽/오른쪽으로 이동    |
| 컨트롤러 도움말 표시                       |  메뉴 단추               |
| 종료                                       |  보기 단추               |

# 구현 참고 사항

# 입력 기하 도형은 각각 16개의 제어점이 있는 네 개의 패치로 구성되고, 모두 정점 버퍼에 저장되어 있습니다. 간단한 정점 셰이더는 제어점을 헐 셰이더로 바로 전달합니다. 헐 셰이더는 상수 버퍼에서 공간 분할 인수로의 고정 함수 테셀레이터 스테이지를 구동합니다. 그러면 제어점과 UVW가 둘 다 도메인 셰이더로 전달됩니다. 도메인 셰이더는 정점마다 한 번씩 실행되고 마지막 정점 위치와 특성을 계산합니다. 정점의 위치는 번스타인 다항식을 사용하여 계산됩니다. 법선은 U 및 V 도함수의 외적으로 계산됩니다. 픽셀 셰이더는 N 점 L 조명을 수행하여 음영 처리된 Mobius 스트립을 그립니다. 

# 개인정보처리방침

샘플을 컴파일하고 실행할 때 샘플의 사용을 추적하는 데 도움이 되도록 샘플
실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을
옵트아웃하려면 Main.cpp에서 \"샘플 사용 원격 분석\"이라고 레이블이
지정된 코드 블록을 제거할 수 있습니다.

Microsoft의 개인 정보 보호 정책에 대한 자세한 내용은 [Microsoft
개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을
참조하세요.