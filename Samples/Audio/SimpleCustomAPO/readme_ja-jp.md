# シンプルなカスタム APO サンプル

*このサンプルは、Microsoft Game Development Kit と互換性があります (2022 年 3 月)*

# 説明

このサンプルでは、カスタム APO 効果を利用して Xbox で XAudio2 を使用して wav ファイルを再生する方法を示します。

# サンプルのビルド

Xbox One 開発キットを使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.XboxOne.x64` に設定します。

Xbox Series X|S を使用している場合は、アクティブなソリューション プラットフォームを `Gaming.Xbox.Scarlett.x64` に設定します。

PC 上の同等のサンプルについては、[GitHub](https://aka.ms/xaudio2samples) の **XAudio2CustomAPO** を参照してください。

*詳細については、* *GDK ドキュメント*の「__サンプルの実行__」を参照してください。

# サンプルの使用方法

このサンプルでは、方向パッドを使用して 0 から 1 の間の浮動小数点パラメーターを操作できます。 ビュー ボタンをクリックすると、サンプルが終了します。

# 実装メモ

このサンプルでは、ソースボイスに xAPO 効果を追加して **SimplePlaySound** サンプルに似た設定を使用します。

**SimpleAPO** は、処理されたサンプル値を乗算することで、単純なゲイン係数を適用します。

APO は、共有登録、クラス ファクトリ、およびパラメーター処理操作を処理するヘルパー テンプレート クラス、SampleAPOBase を使用して実装されます。 このテンプレート クラスの使用は必須ではありませんが、サンプルを簡略化するために使用されます。

# 既知の問題

Windows SDK (22000) を使用している場合、その xapobase.lib を使用すると、リンク時に次のエラーが発生します。

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"long \_\_cdecl StringCchPrintfW(unsigned short \*,unsigned \_\_int64,unsigned short const \*,\...)\" (?StringCchPrintfW@@YAJPEAG_KPEBGZZ) で参照されている、未解決の外部シンボル \_vsnwprintf

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"enum FEATURE_ENABLED_STATE \_\_cdecl wil::details::GetFeatureEnabledStateHelper(unsigned int,enum FEATURE_CHANGE_TIME,int \*)\" (?GetFeatureEnabledStateHelper@details@wil@@YA?AW4FEATURE_ENABLED_STATE@@IW4FEATURE_CHANGE_TIME@@PEAH@Z) で参照されている、未解決の外部シンボル GetFeatureEnabledState

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"unsigned int \_\_cdecl wil::details::GetFeatureVariantHelper(unsigned int,enum FEATURE_CHANGE_TIME,unsigned int \*,int \*,int \*)\" (?GetFeatureVariantHelper@details@wil@@YAIIW4FEATURE_CHANGE_TIME@@PEAIPEAH2@Z) で参照されている、未解決の外部シンボルGetFeatureVariant

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) で参照されている、未解決の外部シンボル RecordFeatureUsage

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) で参照されている、未解決の外部シンボル RecordFeatureError

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) で参照されている、未解決の外部シンボル SubscribeFeatureStateChangeNotification

xapobase.lib(oapmatrixmix.obj) : エラー LNK2019: \"public: int \_\_cdecl \<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \" (??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ) で参照されている、未解決の外部シンボル UnsubscribeFeatureStateChangeNotification

最適な解決策は、この問題が生じない新しいバージョンの Windows SDK を使用することです。 古い SDK にもこのバグはありませんでした。

または、SHCORE.LIB とリンクすることもできます。ですが、これにより、Game OS 向けにサポートされていない API がリンクされる可能性があることに注意してください。

# プライバシーにかんするせいめい

サンプルをコンパイルして実行する場合、サンプルの使用状況を追跡するために、サンプルの実行可能ファイルのファイル名が Microsoft に送信されます。 このデータ コレクションからオプトアウトするには、Main.cpp の "サンプル使用状況テレメトリ" というラベルの付いたコードのブロックを削除します。

Microsoft のプライバシー ポリシー全般の詳細については、「[Microsoft のプライバシーに関する声明](https://privacy.microsoft.com/en-us/privacystatement/)」を参照してください。


