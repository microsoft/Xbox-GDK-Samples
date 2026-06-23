//--------------------------------------------------------------------------------------
// LobbyView.cs
//
// The UI class for displaying a lobby's information on the browse screen.
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

using TMPro;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public sealed class LobbyView : MonoBehaviour
{
    public string Name { get; set; }
    public string ConnectionString { get; set; }
    public uint CurrentMemberCount { get; set; }  
    public uint MaxMemberCount { get; set; }

    public Button Button { get { return _Button; } }

    public void HandleLobbyViewButtonPressed()
    {
        _MultiplayerLogic.JoinLobby(ConnectionString);
    }

    public void Refresh()
    {
        _LobbyName.SetText(Name);
        _PlayerCount.SetText($"{CurrentMemberCount} / {MaxMemberCount}");
    }

    #region Internal
    [SerializeField] private PlayFabMultiplayerLogic _MultiplayerLogic;
    [SerializeField] private TMP_Text _LobbyName;
    [SerializeField] private TMP_Text _PlayerCount;
    [SerializeField] private Button _Button;

    private void OnValidate()
    {
        Assert.IsNotNull(_LobbyName);
        Assert.IsNotNull(_PlayerCount);
        Assert.IsNotNull(_Button);
    }
    #endregion
}
