//--------------------------------------------------------------------------------------
// UpdateAsteroidState.cs
//
// The unreliable network message that is sent whichs represents an asteroid's current state.
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

using System;
using System.IO;

public class UpdateAsteroidState : BaseNetStateObject
{
    public int Id { get; private set; }
    public float PosX { get; private set; }
    public float PosY { get; private set; }
    public float VelX { get; private set; }
    public float VelY { get; private set; }

    public UpdateAsteroidState() : this(-1, 0F, 0F, 0F, 0F) { }

    public UpdateAsteroidState(
        int id, float posX, float posY, float velX, float velY) :
        base(SessionMessageType.UpdateAsteroidState, StateType.Unreliable)
    {
        Id = id;
        PosX = posX;
        PosY = posY;
        VelX = velX;
        VelY = velY;
    }

    protected override void DeserializeFrom(BinaryReader reader)
    {
        Id = Convert.ToInt32(reader.ReadSByte());
        PosX = reader.ReadSingle();
        PosY = reader.ReadSingle();
        VelX = reader.ReadSingle();
        VelY = reader.ReadSingle();
    }

    protected override void SerializeTo(BinaryWriter writer)
    {
        writer.Write(Convert.ToSByte(Id));
        writer.Write(PosX);
        writer.Write(PosY);
        writer.Write(VelX);
        writer.Write(VelY);
    }
}
