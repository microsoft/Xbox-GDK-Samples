//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#include "stdafx.h"
#include "Debug.h"
#include "PeerToPeerNetworkManager.h"

namespace PeerMeshForSamples
{
///////////////////////////////////////////////////////////////////////////////
//  Debug Helpers
//
extern std::wstring g_strLogPath;

void DebugWrite(const wchar_t* format, ...)
{
    static wchar_t buffer[2048];

    va_list args;
    va_start(args, format);
    vswprintf(buffer, 2048, format, args);
    va_end(args);

    OutputDebugString(buffer);

#ifdef NETWORK_LOG_TO_FILE
    // Log the string to a file as well... old skool
	if (g_strLogPath.length() == 0)
	{
		wchar_t path[MAX_PATH + 1];
		GetTempPath(MAX_PATH, path);
		g_strLogPath = path;
	}

    static FILE *file = nullptr;

    if (file == nullptr)
    {
        std::wostringstream fullpath;

        fullpath << g_strLogPath;
        fullpath << L"P2PMeshLog.";
        fullpath << GetTickCount();
        fullpath << L".txt";

        errno_t err = _wfopen_s(
            &file,
            fullpath.str().c_str(),
            L"at+, ccs=UTF-8"
            );

        if (err != 0)
        {
            std::wstring msg(L"Unable to open log file ");
            msg += std::to_wstring(err);
            msg += L"\n";
            OutputDebugString(msg.c_str());
            return;
        }

        std::wstring marker(L"-=*=-=*=-=*=-=*=-=*=-=*=-=*=-=*=-=*=-=*=-=*=-\n");

        fwrite(
            (void *)marker.c_str(),
            sizeof(wchar_t),
            marker.length(),
            file
            );
    }

    time_t    now;
    struct tm tm;

    time(&now);
    localtime_s(&tm, &now);

    std::wstring msg(L"");

    msg += std::to_wstring(tm.tm_hour);
    msg += L":";
    msg += std::to_wstring(tm.tm_min);
    msg += L":";
    msg += std::to_wstring(tm.tm_sec);
    msg += L" ";
    msg += buffer;

    fwrite(
        (void *)msg.c_str(),
        sizeof(wchar_t),
        msg.length(),
        file
        );

    fflush(file);
#endif
}

void DumpSegment(std::shared_ptr<PeerToPeerDataSegment> segment)
{
    std::wstring str(L"SEG - ");

    str += L"SRC: ";
    str += std::to_wstring(segment->Source);
    str += L"  DST: ";
    str += std::to_wstring(segment->Destination);
    str += L"  SEQ: ";
    str += std::to_wstring(segment->Sequence);
    str += L"  ACK: ";
    str += std::to_wstring(segment->Acknowledge);
    str += L"  CHK: ";
    str += std::to_wstring(segment->Checksum);
    str += L"  FLG: ";
    if (segment->Fields == 0)
    {
        if (segment->Fields & (uint8_t)SegmentHeaderFields::Acknowledge) { str += L"A"; }
        if (segment->Fields & (uint8_t)SegmentHeaderFields::Null) { str += L"N"; }
        if (segment->Fields & (uint8_t)SegmentHeaderFields::Reset) { str += L"R"; }
        if (segment->Fields & (uint8_t)SegmentHeaderFields::Synchronize) { str += L"S"; }
    }
    else
    {
        if (segment->IsAcknowledge) { str += L"A"; }
        if (segment->IsNull) { str += L"N"; }
        if (segment->IsReset) { str += L"R"; }
        if (segment->IsSynchronize) { str += L"S"; }
    }
    if (segment->DataSegment != nullptr && segment->DataSegmentLength > 0) { str += L"D"; }

    DebugWrite(L"%s\n", str.c_str());
}

void DumpConnection(std::shared_ptr<PeerToPeerConnection> conn)
{
    std::wstring str(L"CON - ");

    str += L"ID: ";
    str += std::to_wstring(conn->Id);
    str += L"  STATE: ";
    switch (conn->State)
    {
    case EConnectionState::Closed: str += L"CLOSED"; break;
    case EConnectionState::CloseWait: str += L"CLOSEWAIT"; break;
    case EConnectionState::Open: str += L"OPEN"; break;
    case EConnectionState::SynchronizeReceived: str += L"SYNRCVD"; break;
    case EConnectionState::SynchronizeSent: str += L"SYNSNT"; break;
    default: str += L"UNKNOWN"; break;
    }
    str += L"  NSS: ";
    str += std::to_wstring(conn->NextSendSequence);
    str += L"  ISS: ";
    str += std::to_wstring(conn->InitialSendSequence);
    str += L"  IRS: ";
    str += std::to_wstring(conn->InitialReceiveSequence);
    str += L"  LGS: ";
    str += std::to_wstring(conn->LastGoodSegment);
    str += L"  OLD: ";
    str += std::to_wstring(conn->OldestUnacknowledged);

    DebugWrite(L"%s\n", str.c_str());
}
}
