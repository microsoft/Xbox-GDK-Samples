//--------------------------------------------------------------------------------------
// SpawnAsteroidState.cs
//
// The reliable network message that is sent when an asteroid has been spawned.
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

public class SpawnAsteroidState : BaseNetStateObject
{
    public int Id { get; private set; }
    public int AsteroidIndex { get; private set; }
    public float PosX { get; private set; }
    public float PosY { get; private set; }

    public SpawnAsteroidState() : this(-1, -1, 0F, 0F) { }

    public SpawnAsteroidState(int id, int asteroidIndex, float posX, float posY) :
        base(SessionMessageType.SpawnAsteroidState, StateType.Reliable)
    {
        Id = id;
        AsteroidIndex = asteroidIndex;
        PosX = posX;
        PosY = posY;
    }

    protected override void DeserializeFrom(BinaryReader reader)
    {
        Id = reader.ReadInt32();
        AsteroidIndex = reader.ReadInt32();
        PosX = reader.ReadSingle();
        PosY = reader.ReadSingle();
    }

    protected override void SerializeTo(BinaryWriter writer)
    {
        writer.Write(Id);
        writer.Write(AsteroidIndex);
        writer.Write(PosX);
        writer.Write(PosY);
    }
}
