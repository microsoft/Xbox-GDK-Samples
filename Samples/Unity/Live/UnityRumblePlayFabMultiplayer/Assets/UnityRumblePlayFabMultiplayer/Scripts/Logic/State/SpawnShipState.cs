//--------------------------------------------------------------------------------------
// SpawnShipState.cs
//
// The reliable network message that is sent when a player's ship has been spawned.
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

public class SpawnShipState : BaseNetStateObject
{
    public int ShipIndex { get; private set; }
    public int ColorIndex { get; private set; }
    public float PosX { get; private set; }
    public float PosY { get; private set; }
    public float Rotation { get; private set; }

    public SpawnShipState() : this(-1, -1, 0F, 0F, 0F) { }

    public SpawnShipState(int shipIndex, int colorIndex, float posX, float posY, float rotation) :
        base(SessionMessageType.SpawnShipState, StateType.Reliable)
    {
        ShipIndex = shipIndex;
        ColorIndex = colorIndex;
        PosX = posX;
        PosY = posY;
        Rotation = rotation;
    }

    protected override void DeserializeFrom(BinaryReader reader)
    {
        ShipIndex = reader.ReadInt32();
        ColorIndex = reader.ReadInt32();
        PosX = reader.ReadSingle();
        PosY = reader.ReadSingle();
        Rotation = reader.ReadSingle();
    }

    protected override void SerializeTo(BinaryWriter writer)
    {
        writer.Write(ShipIndex);
        writer.Write(ColorIndex);
        writer.Write(PosX);
        writer.Write(PosY);
        writer.Write(Rotation);
    }
}
