//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved
#pragma once

namespace PeerMeshForSamples
{
#ifdef NETWORK_DEBUG_SPEW

class PeerToPeerDataSegment;
class PeerToPeerConnection;

void DebugWrite(const wchar_t* format, ...);
void DumpSegment(std::shared_ptr<PeerToPeerDataSegment> segment);
void DumpConnection(std::shared_ptr<PeerToPeerConnection> conn);

#define DEBUGLOG(x, ...)        DebugWrite(L"P2P: " x, __VA_ARGS__)
#define DUMP_SEGMENT(x)         DumpSegment(x)
#define DUMP_CONNECTION(x)      DumpConnection(x)

#else // NETWORK_DEBUG_SPEW

#define DEBUGLOG(x, ...)        
#define DUMP_SEGMENT(x)         
#define DUMP_CONNECTION(x)      

#endif // NETWORK_DEBUG_SPEW
}
