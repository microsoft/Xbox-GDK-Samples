//--------------------------------------------------------------------------------------
// GameMemberView.cs
//
// The session member item class that shows the status for an individual playing the game.
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

public class GameMemberView : BaseMemberView
{
    [SerializeField]
    private Text KillsText;

    [SerializeField]
    private Text DeathsText;

    public int KillTotal { get; private set; }
    public int DeathTotal { get; private set; }

    public void IncrementKills()
    {
        KillTotal++;
        RefreshTotalsText();
    }

    public void IncrementDeaths()
    {
        DeathTotal++;
        RefreshTotalsText();
    }

    protected override void HandleHostSet() { }

    protected override void OnValidate()
    {
        base.OnValidate();

        Assert.IsNotNull(KillsText, $"Set the kills text for the {name} object");
        Assert.IsNotNull(DeathsText, $"Set the deaths text for the {name} object");
    }

    private void Awake()
    {
        KillTotal = 0;
        DeathTotal = 0;
        RefreshTotalsText();
    }

    private void RefreshTotalsText()
    {
        KillsText.text = KillTotal.ToString();
        DeathsText.text = DeathTotal.ToString();
    }
}
