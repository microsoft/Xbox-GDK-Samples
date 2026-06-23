//--------------------------------------------------------------------------------------
// UpdateShipState.cs
//
// The unreliable network message that is sent which represents a ship's current state.
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

public class UpdateShipState : BaseNetStateObject
{
    public float PosX { get; private set; }
    public float PosY { get; private set; }
    public float VelX { get; private set; }
    public float VelY { get; private set; }
    public float MoveX { get; private set; }
    public float MoveY { get; private set; }
    public float LookX { get; private set; }
    public float LookY { get; private set; }

    public UpdateShipState() : this(0F, 0F, 0F, 0F, 0F, 0F, 0F, 0F) { }

    public UpdateShipState(
        float posX, float posY, float velX, float velY,
        float moveX, float moveY, float lookX, float lookY) :
        base(SessionMessageType.UpdateShipState, StateType.Unreliable)
    {
        PosX = posX;
        PosY = posY;
        VelX = velX;
        VelY = velY;
        MoveX = moveX;
        MoveY = moveY;
        LookX = lookX;
        LookY = lookY;
    }

    protected override void DeserializeFrom(BinaryReader reader)
    {
        PosX = reader.ReadSingle();
        PosY = reader.ReadSingle();
        VelX = reader.ReadSingle();
        VelY = reader.ReadSingle();
        MoveX = UnpackByteIntoNormalizedFloat(reader.ReadSByte());
        MoveY = UnpackByteIntoNormalizedFloat(reader.ReadSByte());
        LookX = UnpackByteIntoNormalizedFloat(reader.ReadSByte());
        LookY = UnpackByteIntoNormalizedFloat(reader.ReadSByte());
    }

    protected override void SerializeTo(BinaryWriter writer)
    {
        writer.Write(PosX);
        writer.Write(PosY);
        writer.Write(VelX);
        writer.Write(VelY);
        writer.Write(PackNormalizedFloatIntoByte(MoveX));
        writer.Write(PackNormalizedFloatIntoByte(MoveY));
        writer.Write(PackNormalizedFloatIntoByte(LookX));
        writer.Write(PackNormalizedFloatIntoByte(LookY));
    }

    public static sbyte PackNormalizedFloatIntoByte(float value)
    {
        float fVal = value * 127F;

        if (fVal < SByte.MinValue)
        {
            fVal = SByte.MinValue;
        }
        else if (fVal > SByte.MaxValue)
        {
            fVal = SByte.MaxValue;
        }

        return Convert.ToSByte(fVal);
    }

    public static float UnpackByteIntoNormalizedFloat(sbyte value)
    {
        const float OneOver127 = 1F / 127F;
        return Convert.ToSingle(value) * OneOver127;
    }
}
