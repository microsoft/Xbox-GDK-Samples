//// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
//// PARTICULAR PURPOSE.
////
//// Copyright (c) Microsoft Corporation. All rights reserved

#pragma once

namespace PeerMeshForSamples
{
class PeerToPeerNetworkManager;
class PeerToPeerDataSegment;
class PeerToPeerConnection;

typedef uint32_t                                                          TPeerId;
typedef uint8_t *                                                         TByteArray;
typedef std::function<void(TPeerId peer, TByteArray data, uint32_t size)> FDataReceived;
typedef std::function<void(TPeerId peer)>                                 FPeerAdded;
typedef std::function<void(TPeerId peer, bool lastPeer)>                  FPeerRemoved;
typedef std::shared_ptr<PeerToPeerDataSegment>                            PDataSegment;
typedef std::shared_ptr<PeerToPeerConnection>                             PConnection;
typedef std::map<TPeerId, PConnection>                                    TPeerConnectionMap;
typedef std::vector<TPeerId>                                              TPeerQueue;
typedef std::map<TPeerId, std::queue<PDataSegment>>                       TPeerDataQueue;
typedef std::queue<PDataSegment>                                          TSegmentQueue;
typedef std::list<PDataSegment>                                           TSegmentList;
}
