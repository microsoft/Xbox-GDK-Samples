//--------------------------------------------------------------------------------------
// BaseNetObjectState.cs
//
// The base class for all game network state objects or messages.
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
using UnityEngine.Assertions;

public enum StateType
{
    Reliable = 0,
    Unreliable = 1,
}

public abstract class BaseNetStateObject
{
    public SessionMessageType MessageType { get; private set; }
    public StateType StateType { get; private set; }

    protected BaseNetStateObject(SessionMessageType messageType, StateType stateType)
    {
        MessageType = messageType;
        StateType = stateType;
    }

    public void DeserializeFrom(byte[] data)
    {
        using (var memoryStream = new MemoryStream(data))
        {
            using (var reader = new BinaryReader(memoryStream))
            {
                var messageType = (SessionMessageType)reader.ReadByte();
                Assert.IsTrue(messageType == MessageType);
                DeserializeFrom(reader);
            }
        }
    }

    public void SerializeTo(out byte[] data)
    {
        using (var memoryStream = new MemoryStream())
        {
            using (var writer = new BinaryWriter(memoryStream))
            {
                var messageType = (byte)MessageType;
                writer.Write(messageType);
                SerializeTo(writer);
            }

            data = memoryStream.ToArray();
        }
    }

    protected abstract void DeserializeFrom(BinaryReader reader);
    protected abstract void SerializeTo(BinaryWriter writer);
}
