//--------------------------------------------------------------------------------------
// LobbyMemberView.cs
//
// The session member item class that shows the status for a member in a game lobby.
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

public class LobbyMemberView : BaseMemberView
{
    [SerializeField]
    private Toggle ReadyToggle;

    [SerializeField]
    private Image ShipImage;

    [SerializeField]
    private Image ColorImage;

    public bool IsReady()
    {
        return ReadyToggle.isOn;
    }

    public void MakeReady()
    {
        ReadyToggle.isOn = true;
    }

    public void MakeNotReady()
    {
        ReadyToggle.isOn = false;
    }

    public void SetShipImage(GameAssetManager assetManager, int shipIndex)
    {
        ShipImage.sprite = assetManager.PlayerShipSprites[shipIndex];
    }

    public void SetShipColor(GameAssetManager assetManager, int colorIndex)
    {
        ColorImage.color = assetManager.PlayerShipColorChoices[colorIndex];
    }

    protected override void HandleHostSet()
    {
        (ReadyToggle.graphic as Image).sprite = IsHostSprite;
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(ReadyToggle, $"Set the ready toggle for the {name} object");
        Assert.IsNotNull(ShipImage, $"Set the ship image for the {name} object");
        Assert.IsNotNull(ColorImage, $"Set the color image for the {name} object");
    }

    private void Awake()
    {
        MakeNotReady();
    }
}
