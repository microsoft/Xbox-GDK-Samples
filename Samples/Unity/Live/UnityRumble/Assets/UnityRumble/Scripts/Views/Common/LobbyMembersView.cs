//--------------------------------------------------------------------------------------
// ScreenManager.cs
//
// The session members list for the members currently connected to a game lobby.
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
using System.Linq;
using UnityEngine;

public class LobbyMembersView : BaseMembersView
{
    public event Action AllMembersReady;

    [SerializeField]
    private LobbyMemberView LobbyMemberViewPrefab;

    public void SetMemberShipIndex(ulong xuid, GameAssetManager assetManager, int shipIndex)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            (existingMember as LobbyMemberView).SetShipImage(assetManager, shipIndex);
        }
    }

    public void SetMemberColorIndex(ulong xuid, GameAssetManager assetManager, int colorIndex)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            (existingMember as LobbyMemberView).SetShipColor(assetManager, colorIndex);
        }
    }

    public void MakeMemberReady(ulong xuid)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            (existingMember as LobbyMemberView).MakeReady();

            if (_members.Count >= MinimumRequiredMembers &&
                _members.All(member =>
                {
                    var lobbyMemberView = member.Value as LobbyMemberView;
                    return lobbyMemberView.IsReady();
                }))
            {
                AllMembersReady?.Invoke();
            }
        }
    }

    public void MakeMemberNotReady(ulong xuid)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            (existingMember as LobbyMemberView).MakeNotReady();
        }
    }

    protected override BaseMemberView MakeMemberView()
    {
        var newMember = Instantiate<LobbyMemberView>(LobbyMemberViewPrefab, MembersList);
        newMember.MakeNotReady();
        return newMember;
    }
}
