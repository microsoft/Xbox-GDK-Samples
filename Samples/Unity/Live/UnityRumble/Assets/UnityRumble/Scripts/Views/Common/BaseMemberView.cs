//--------------------------------------------------------------------------------------
// BaseMemberView.cs
//
// The base class for a single member item within a session members list.
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

using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public abstract class BaseMemberView : MonoBehaviour
{
    [SerializeField]
    protected Text GamertagText;

    [SerializeField]
    protected Sprite IsHostSprite;

    [SerializeField]
    protected Image HostIndicatorImage;

    public void SetGamertag(string gamertag)
    {
        GamertagText.text = gamertag;
    }

    public string GetGamertag()
    {
        return GamertagText.text;
    }

    public void MakeHost()
    {
        HostIndicatorImage.sprite = IsHostSprite;
        HandleHostSet();
    }

    protected abstract void HandleHostSet();

    protected virtual void OnValidate()
    {
        Assert.IsNotNull(GamertagText, $"Set the gamertag text for the {name} object");
        Assert.IsNotNull(IsHostSprite, $"Set the isHost sprite for the {name} object");
        Assert.IsNotNull(HostIndicatorImage, $"Set the hostIndicator image for the {name} object");
    }
}
