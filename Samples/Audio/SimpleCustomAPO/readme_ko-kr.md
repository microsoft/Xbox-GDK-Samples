# 간단한 사용자 지정 APO 샘플

*이 샘플은 Microsoft 게임 개발 키트(2022년 3월)와 호환됩니다.*

# 설명

이 샘플에서는 사용자 지정 APO 효과를 사용하여 Xbox에서 XAudio2를 사용하여 wav 파일을 재생하는 방법을 보여 줍니다.

# 샘플 빌드

Xbox One 개발 키트를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.XboxOne.x64`(으)로 설정합니다.

Xbox Series X|S를 사용하는 경우 활성 솔루션 플랫폼을 `Gaming.Xbox.Scarlett.x64`(으)로 설정합니다.

PC에 해당하는 샘플은 [GitHub](https://aka.ms/xaudio2samples)에서 **XAudio2CustomAPO**를 참조하세요.

*GDK 설명서의* __샘플 실행__에서 *자세한 내용을 알아보세요.*

# 샘플 사용

이 샘플을 사용하면 Dpad를 사용하여 0에서 1 사이의 부동 소수점 매개 변수를 조작할 수 있습니다. 보기 단추가 샘플을 종료합니다.

# 구현 참고 사항

이 샘플에서는 소스 음성에 xAPO 효과를 추가하여 **SimplePlaySound** 샘플과 유사한 설정을 사용합니다.

**SimpleAPO**는 처리된 샘플 값을 곱하여 간단한 이득 계수를 적용합니다.

APO는 공유된 등록, 클래스 팩터리 및 매개 변수 처리 작업을 처리하는 도우미 템플릿 클래스 SampleAPOBase를 사용하여 구현됩니다. 이 템플릿 클래스를 사용할 필요는 없지만 샘플을 단순화하는 데 사용됩니다.

# 알려진 문제

Windows SDK(22000)를 사용하는 경우 해당 xapobase.lib를 사용할 때 링크 타임에 다음 오류가 발생합니다.

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 \_vsnwprintf 함수에서 참조됨 \"long \_\_cdecl StringCchPrintfW(서명되지 않은 쇼츠 \*,unsigned \_\_int64,unsigned short const \*,\...)\" (?StringCchPrintfW@@YAJPEAG_KPEBGZZ)

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 GetFeatureEnabledState 함수에서 참조됨 \"enum FEATURE_ENABLED_STATE \_\_cdecl wil::details::GetFeatureEnabledStateHelper(unsigned int,enum FEATURE_CHANGE_TIME,int \*)\" (?GetFeatureEnabledStateHelper@details@wil@@YA?AW4FEATURE_ENABLED_STATE@@IW4FEATURE_CHANGE_TIME@@PEAH@Z)

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 GetFeatureVariant 함수에서 참조됨 \"unsigned int \_\_cdecl wil::details::GetFeatureVariantHelper(unsigned int,enum FEATURE_CHANGE_TIME,unsigned int \*,int \*,int \*)\" (?GetFeatureVariantHelper@details@wil@@YAIIW4FEATURE_CHANGE_TIME@@PEAIPEAH2@Z)

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 RecordFeatureUsage 함수에서 참조됨 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 RecordFeatureError 함수에서 참조됨 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 SubscribeFeatureStateChangeNotification 함수에서 참조됨 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

xapobase.lib(oapmatrixmix.obj) : 오류 LNK2019: 해결되지 않은 외부 기호 UnsubscribeFeatureStateChangeNotification 함수에서 참조됨 \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

가장 좋은 방법은 이 문제가 없는 최신 버전의 Windows SDK를 사용하는 것입니다. 이전 SDK에도 이 버그는 없었습니다.

또는 SHCORE.LIB와 연결할 수 있지만 이렇게 하면 지원되지 않는 API를 허용해 게임 OS를 연결할 수 있습니다.

# 개인정보처리방침

샘플을 컴파일하고 실행하는 경우 샘플 사용량을 추적할 수 있도록 샘플 실행 파일의 파일 이름이 Microsoft에 전송됩니다. 이 데이터 수집을 옵트아웃하려면 Main.cpp에서 "샘플 사용량 원격 분석"이라는 레이블이 지정된 코드 블록을 제거할 수 있습니다.

일반적인 Microsoft의 개인 정보 정책에 대한 자세한 내용은 [Microsoft 개인정보처리방침](https://privacy.microsoft.com/en-us/privacystatement/)을 참조하세요.


