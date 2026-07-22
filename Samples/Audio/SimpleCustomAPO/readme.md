# Simple Custom APO Sample

*This sample is compatible with the Microsoft Game Development Kit
(March 2022)*

# Description

This sample demonstrates how to play a wav file using XAudio2 on the
Xbox using a custom APO effect.

# Building the sample

If using an Xbox One devkit, set the active solution platform to `Gaming.Xbox.XboxOne.x64`.

If using Xbox Series X|S, set the active solution platform to `Gaming.Xbox.Scarlett.x64`.

For an equivalent sample on PC, see **XAudio2CustomAPO** on
[GitHub](https://aka.ms/xaudio2samples).

*For more information, see* __Running samples__, *in the GDK documentation.*

# Using the sample

The sample allows you to manipulate a floating-point parameter between 0
and 1 with the Dpad. The View button exits the sample.

# Implementation notes

This sample uses a similar set up to the **SimplePlaySound** sample with
the addition of a xAPO effect to the source voice.

**SimpleAPO** applies a simple gain factor by multiplying the sample
values processed.

The APOs are implemented by using a helper template class,
SampleAPOBase, that handles shared registration, class factory, and
parameter handling operations. Use of this template class is not
required, but it is used to simplify the sample.

# Known issues

If using the Windows SDK (22000), you will get the following errors at
link time when using that xapobase.lib:

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol \_vsnwprintf referenced in function \"long \_\_cdecl
StringCchPrintfW(unsigned short \*,unsigned \_\_int64,unsigned short
const \*,\...)\" (?StringCchPrintfW@@YAJPEAG_KPEBGZZ)

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol GetFeatureEnabledState referenced in function \"enum
FEATURE_ENABLED_STATE \_\_cdecl
wil::details::GetFeatureEnabledStateHelper(unsigned int,enum
FEATURE_CHANGE_TIME,int \*)\"
(?GetFeatureEnabledStateHelper@details@wil@@YA?AW4FEATURE_ENABLED_STATE@@IW4FEATURE_CHANGE_TIME@@PEAH@Z)

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol GetFeatureVariant referenced in function \"unsigned int \_\_cdecl
wil::details::GetFeatureVariantHelper(unsigned int,enum
FEATURE_CHANGE_TIME,unsigned int \*,int \*,int \*)\"
(?GetFeatureVariantHelper@details@wil@@YAIIW4FEATURE_CHANGE_TIME@@PEAIPEAH2@Z)

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol RecordFeatureUsage referenced in function \"public: int \_\_cdecl
\<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \"
(??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol RecordFeatureError referenced in function \"public: int \_\_cdecl
\<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \"
(??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol SubscribeFeatureStateChangeNotification referenced in function
\"public: int \_\_cdecl
\<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \"
(??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

xapobase.lib(oapmatrixmix.obj) : error LNK2019: unresolved external
symbol UnsubscribeFeatureStateChangeNotification referenced in function
\"public: int \_\_cdecl
\<lambda_778f06ff3b1f47a446cbe609236855d4\>::operator()(void)const \"
(??R\<lambda_778f06ff3b1f47a446cbe609236855d4\>@@QEBAHXZ)

The best solution is to use a newer version of the Windows SDK which
does not have this issue. Older SDKs also did not have this bug.

Alternatively, you can link with SHCORE.LIB but be aware this may allow
unsupported APIs for Game OS to link.

# Privacy statement

When compiling and running a sample, the file name of the sample
executable will be sent to Microsoft to help track sample usage. To
opt-out of this data collection, you can remove the block of code in
Main.cpp labeled "Sample Usage Telemetry".

For more information about Microsoft's privacy policies in general, see
the [Microsoft Privacy
Statement](https://privacy.microsoft.com/en-us/privacystatement/).
