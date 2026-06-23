//--------------------------------------------------------------------------------------
// DestroyShipState.cs
//
// The reliable network message that is broadcasted when a player's ship is destroyed.
//
// MIT License
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of this
// software and associated documentation files (the "Software"), to deal in the Software
// without restriction, including without limitation the rights to use, copy, modify, merge,
// publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons
// to whom the Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all copies or
// substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED *AS IS*, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
// INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR
// PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE
// FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
// OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------

using System.IO;

public class DestroyShipState : BaseNetStateObject
{
    public ulong DestroyedXuid { get; private set; }
    public ulong DestroyerXuid { get; private set; }

    public DestroyShipState() : this(0, 0) { }

    public DestroyShipState(ulong destroyedXuid, ulong destroyerXuid) :
        base(SessionMessageType.DestroyShipState, StateType.Reliable)
    {
        DestroyedXuid = destroyedXuid;
        DestroyerXuid = destroyerXuid;
    }

    protected override void DeserializeFrom(BinaryReader reader)
    {
        DestroyedXuid = reader.ReadUInt64();
        DestroyerXuid = reader.ReadUInt64();
    }

    protected override void SerializeTo(BinaryWriter writer)
    {
        writer.Write(DestroyedXuid);
        writer.Write(DestroyerXuid);
    }
}
