//--------------------------------------------------------------------------------------
// BaseMembersView.cs
//
// The base class for a list of members for a session.
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

using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Assertions;

public abstract class BaseMembersView : MonoBehaviour
{
    public event System.Action FewerThanRequiredMembers;

    public const int MinimumRequiredMembers = 2;

    [SerializeField]
    protected RectTransform MembersList;
    protected abstract BaseMemberView MakeMemberView();

    protected readonly Dictionary<ulong, BaseMemberView> _members = new(); // maps from xuid -> LobbyMemberView

    public bool HasMember(ulong xuid)
    {
        return _members.ContainsKey(xuid);
    }

    public BaseMemberView GetMember(ulong xuid)
    {
        _members.TryGetValue(xuid, out BaseMemberView member);
        return member;
    }

    public bool HasMaxMembers()
    {
        return _members.Count == Configuration.LOBBY_MAX_MEMBERS_IN_SESSION;
    }

    public ulong[] GetMemberXuids()
    {
        return _members.Keys.ToArray();
    }

    public void AddMember(ulong xuid, string gamertag, bool isHost)
    {
        Assert.IsFalse(_members.ContainsKey(xuid));

        var newMember = MakeMemberView();

        newMember.SetGamertag(gamertag);

        if (isHost)
        {
            newMember.MakeHost();
        }

        _members.Add(xuid, newMember);
    }

    public void UpdateMemberHost(ulong xuid, bool isHost)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            if (isHost)
            {
                existingMember.MakeHost();
            }
        }
    }

    public void UpdateMemberGamertag(ulong xuid, string gamertag)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            existingMember.SetGamertag(gamertag);
        }
    }

    public void RemoveMember(ulong xuid)
    {
        if (_members.TryGetValue(xuid, out BaseMemberView existingMember))
        {
            _members.Remove(xuid);

            if (_members.Count < MinimumRequiredMembers)
            {
                FewerThanRequiredMembers?.Invoke();
            }

            Destroy(existingMember.gameObject);
        }
    }

    public void ClearAllMembers()
    {
        while (_members.Keys.Count > 0)
        {
            RemoveMember(_members.First().Key);
        }
    }
}
