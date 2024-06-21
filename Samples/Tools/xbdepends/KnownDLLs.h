//--------------------------------------------------------------------------------------
// File: KnownDLLs.h
//
// Microsoft Xbox Binary Dependencies Tool - Known DLLs data
//
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

#pragma once

//
// This is not a list of all DLLs in the OS images. Rather, it is s a list of DLLs known to
// be linked by EXEs or DLLs.
//

namespace KnownDLLs
{
    // All versions of Windows 10
    const char* c_CoreOS[] =
    {
        "avrt.dll",
        "bcrypt.dll",
        "Cabinet.dll",
        "cfgmgr32.dll",
        "combase.dll",
        "crypt32.dll",
        "D3DCompiler_47.dll",
        "dbghelp.dll",
        "dhcpcsvc.dll",
        "dnsapi.dll",
        "imagehlp.dll",
        "IPHLPAPI.DLL",
        "MFPlat.DLL",
        "mfreadwrite.dll",
        "MMDevAPI.dll",
        "msvcp_win.dll",
        "msvcrt.dll",
        "mswsock.dll",
        "ncrypt.dll",
        "ntdll.dll",
        "oleaut32.dll",
        "powrprof.dll",
        "rpcrt4.dll",
        "secur32.dll",
        "sspicli.dll",
        "ucrtbase.dll",
        "userenv.dll",
        "WindowsCodecs.dll",
        "winhttp.dll",
        "Wldap32.dll",
        "ws2_32.dll",
        "wsock32.dll",
        "XAudio2_9.dll",
        "xcrdapi.dll",
        "xmllite.dll",
    };

    const char* c_Win32[] =
    {
        "ADVAPI32.dll",
        "kernel32.dll",
        "ole32.dll",
        "SHELL32.dll",
        "USER32.dll",
    };

    const char* c_GameOSOnly[] =
    {
        "AcpHal.dll",
        "chad.dll",
        "pixevt.dll",
        "xapu.dll",
        "xfrontpaneldisplay.dll",
        "xmem.dll",
    };

    const char* c_SystemOS[] =
    {
        "credui.dll",
        "dxva2.dll",
        "esent.dll",
        "gdi32.dll",
        "hid.dll",
        "mf.dll",
        "mscoree.dll",
        "msvcp110_win.dll",
        "normaliz.dll",
        "propsys.dll",
        "UIAutomationCore.dll",
        "urlmon.dll",
        "wininet.dll",
        "wintrust.dll",
        "winusb.dll",
        "XInputUap.dll",
    };

    const char* c_PCOnly[] =
    {
        "BluetoothApis.dll",
        "bthprops.cpl",
        "COMCTL32.dll",
        "COMDLG32.dll",
        "CRYPTUI.dll",
        "d3dcsx_47.dll",
        "DDRAW.dll",
        "dinput.dll",
        "dinput8.dll",
        "dsound.dll",
        "dwmapi.dll",
        "gdiplus.dll",
        "IMM32.dll",
        "msi.dll",
        "ndfapi.dll",
        "netapi32.dll",
        "OLEACC.dll",
        "OPENGL32.dll",
        "pdh.dll",
        "SETUPAPI.dll",
        "SHLWAPI.dll",
        "USP10.dll",
        "UxTheme.dll",
        "version.dll",
        "WINMM.dll",
        "WINSPOOL.DRV",
        "WTSAPI32.dll",
        "XAudio2_8.dll",
        "XINPUT1_4.dll",
        "XInput9_1_0.dll",
    };

    // Vendor DLLs that are not deploy side-by-side
    const char* c_PCVendor[] =
    {
        "mantle64.dll",
        "nvcuda.dll",
        "nvfatbinaryLoader.dll",
    };

    const char* c_LegacyERA[] =
    {
        "kernelx.dll",
    };

    const char* c_LegacyDXSDK[] =
    {
        "D3DCompiler_33.dll",
        "D3DCompiler_34.dll",
        "D3DCompiler_35.dll",
        "D3DCompiler_36.dll",
        "D3DCompiler_37.dll",
        "D3DCompiler_38.dll",
        "D3DCompiler_39.dll",
        "D3DCompiler_40.dll",
        "D3DCompiler_41.dll",
        "D3DCompiler_42.dll",
        "D3DCompiler_43.dll",
        "d3dx10.dll",
        "d3dx10_33.dll",
        "d3dx10_34.dll",
        "d3dx10_35.dll",
        "d3dx10_36.dll",
        "d3dx10_37.dll",
        "d3dx10_38.dll",
        "d3dx10_39.dll",
        "d3dx10_40.dll",
        "d3dx10_41.dll",
        "d3dx10_42.dll",
        "d3dx10_43.dll",
        "d3dx11_42.dll",
        "d3dx11_43.dll",
        "d3dx9_24.dll",
        "d3dx9_25.dll",
        "d3dx9_26.dll",
        "d3dx9_27.dll",
        "d3dx9_28.dll",
        "d3dx9_29.dll",
        "d3dx9_30.dll",
        "d3dx9_31.dll",
        "d3dx9_32.dll",
        "d3dx9_33.dll",
        "d3dx9_34.dll",
        "d3dx9_35.dll",
        "d3dx9_36.dll",
        "D3DX9_37.dll",
        "D3DX9_38.dll",
        "D3DX9_39.dll",
        "D3DX9_40.dll",
        "D3DX9_41.dll",
        "D3DX9_42.dll",
        "D3DX9_43.dll",
        "x3daudio1_0.dll",
        "x3daudio1_1.dll",
        "x3daudio1_2.dll",
        "X3DAudio1_3.dll",
        "X3DAudio1_4.dll",
        "X3DAudio1_5.dll",
        "X3DAudio1_6.dll",
        "X3DAudio1_7.dll",
        "xactengine2_0.dll",
        "xactengine2_1.dll",
        "xactengine2_10.dll",
        "xactengine2_2.dll",
        "xactengine2_3.dll",
        "xactengine2_4.dll",
        "xactengine2_5.dll",
        "xactengine2_6.dll",
        "xactengine2_7.dll",
        "xactengine2_8.dll",
        "xactengine2_9.dll",
        "xactengine3_0.dll",
        "xactengine3_1.dll",
        "xactengine3_2.dll",
        "xactengine3_3.dll",
        "xactengine3_4.dll",
        "xactengine3_5.dll",
        "xactengine3_6.dll",
        "xactengine3_7.dll",
        "XAPOFX1_0.dll",
        "XAPOFX1_1.dll",
        "XAPOFX1_2.dll",
        "XAPOFX1_3.dll",
        "XAPOFX1_4.dll",
        "XAPOFX1_5.dll",
        "XAudio2_0.dll",
        "XAudio2_1.dll",
        "XAudio2_2.dll",
        "XAudio2_3.dll",
        "XAudio2_4.dll",
        "XAudio2_5.dll",
        "XAudio2_6.dll",
        "XAudio2_7.dll",
        "xinput1_1.dll",
        "xinput1_2.dll",
        "xinput1_3.dll",
    };

    const char* c_LegacyDXSDKDebug[] =
    {
        "d3d8d.dll",
        "d3d9d.dll",
        "d3dx10d.dll",
        "d3dx10d_33.dll",
        "d3dx10d_34.dll",
        "d3dx10d_35.dll",
        "d3dx10d_36.dll",
        "d3dx10d_37.dll",
        "d3dx10d_38.dll",
        "d3dx10d_39.dll",
        "d3dx10d_40.dll",
        "d3dx10d_41.dll",
        "d3dx10d_42.dll",
        "d3dx10d_43.dll",
        "d3dx11d_42.dll",
        "d3dx11d_43.dll",
        "d3dx9d_24.dll",
        "d3dx9d_25.dll",
        "d3dx9d_26.dll",
        "d3dx9d_27.dll",
        "d3dx9d_28.dll",
        "d3dx9d_29.dll",
        "d3dx9d_30.dll",
        "d3dx9d_31.dll",
        "d3dx9d_32.dll",
        "d3dx9d_33.dll",
        "d3dx9d_34.dll",
        "d3dx9d_35.dll",
        "d3dx9d_36.dll",
        "D3DX9d_37.dll",
        "D3DX9d_38.dll",
        "D3DX9d_39.dll",
        "D3DX9d_40.dll",
        "D3DX9d_41.dll",
        "D3DX9d_42.dll",
        "D3DX9d_43.dll",
        "x3daudiod1_0.dll",
        "x3daudiod1_1.dll",
        "x3daudiod1_2.dll",
        "X3DAudiod1_3.dll",
        "X3DAudiod1_4.dll",
        "X3DAudiod1_5.dll",
        "X3DAudiod1_6.dll",
        "X3DAudiod1_7.dll",
        "xactengineA2_0.dll",
        "xactengineA2_1.dll",
        "xactengineA2_10.dll",
        "xactengineA2_2.dll",
        "xactengineA2_3.dll",
        "xactengineA2_4.dll",
        "xactengineA2_5.dll",
        "xactengineA2_6.dll",
        "xactengineA2_7.dll",
        "xactengineA2_8.dll",
        "xactengineA2_9.dll",
        "xactengineA3_0.dll",
        "xactengineA3_1.dll",
        "xactengineA3_2.dll",
        "xactengineA3_3.dll",
        "xactengineA3_4.dll",
        "xactengineA3_5.dll",
        "xactengineA3_6.dll",
        "xactengineA3_7.dll",
        "xactengineD2_0.dll",
        "xactengineD2_1.dll",
        "xactengineD2_10.dll",
        "xactengineD2_2.dll",
        "xactengineD2_3.dll",
        "xactengineD2_4.dll",
        "xactengineD2_5.dll",
        "xactengineD2_6.dll",
        "xactengineD2_7.dll",
        "xactengineD2_8.dll",
        "xactengineD2_9.dll",
        "xactengineD3_0.dll",
        "xactengineD3_1.dll",
        "xactengineD3_2.dll",
        "xactengineD3_3.dll",
        "xactengineD3_4.dll",
        "xactengineD3_5.dll",
        "xactengineD3_6.dll",
        "xactengineD3_7.dll",
        "XAPOFXd1_0.dll",
        "XAPOFXd1_1.dll",
        "XAPOFXd1_2.dll",
        "XAPOFXd1_3.dll",
        "XAPOFXd1_4.dll",
        "XAPOFXd1_5.dll",
        "XAudiod2_0.dll",
        "XAudiod2_1.dll",
        "XAudiod2_2.dll",
        "XAudiod2_3.dll",
        "XAudiod2_4.dll",
        "XAudiod2_5.dll",
        "XAudiod2_6.dll",
        "XAudiod2_7.dll",
    };

    const char* c_GDK[] =
    {
        "GameChat2.dll",
        "GameInput.dll",
        "gameruntime.dll",
        "libHttpClient.GDK.dll",
        "Party.dll",
        "PartyXboxLive.dll",
        "PlayFabMultiplayerGDK.dll",
        "PlayFabCore.GDK.dll",
        "PlayFabServices.GDK.dll",
        "XCurl.dll",
        "xsapi.dll",
        "Microsoft.Xbox.Services.GDK.C.Thunks.dll",
        "xgameruntime.thunks.dll",
    };

    const char* c_DevOnlyGDK[] =
    {
        "pgort140.dll",
        "psapi.dll",
        "xg.dll",
    };

    const char* c_Direct3D_Legacy[] =
    {
        "d3d10.dll",
        "d3d8.dll",
        "d3d9.dll",
    };

    const char* c_Direct3D_Stock[] =
    {
        "d2d1.dll",
        "d3d11.dll",
        "D3D12.dll",
        "DWrite.dll",
        "dxgi.dll",
        "dxgidebug.dll",
    };

    const char* c_Direct3D_XboxOne[] =
    {
        "d3d12_x.dll",
        "dxcompiler_x.dll",
        "xg_x.dll"
    };

    const char* c_Direct3D_Scarlett[] =
    {
        "d3d12_xs.dll",
        "dxcompiler_xs.dll",
        "xg_xs.dll"
    };
}
