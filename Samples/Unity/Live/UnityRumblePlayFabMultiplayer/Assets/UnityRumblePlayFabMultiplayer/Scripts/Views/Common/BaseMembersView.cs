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

using System.Collections;
using System.Collections.Generic;
using System.Linq;
using UnityEngine;
using UnityEngine.Assertions;

public abstract class BaseMembersView : MonoBehaviour
{
    public event System.Action FewerThanRequiredMembers;

    

    [SerializeField]
    protected RectTransform MembersList;

    protected readonly Dictionary<UserID, BaseMemberView> _members = new Dictionary<UserID, BaseMemberView>();

    public bool HasMember(UserID userID)
    {
        return _members.ContainsKey(userID);
    }

    public BaseMemberView GetMember(UserID userID)
    {
        BaseMemberView member = null;
        _members.TryGetValue(userID, out member);
        return member;
    }

    public bool HasMaxMembers()
    {
        return _members.Count == Configuration.LOBBY_MAX_MEMBERS_IN_SESSION;
    }

    public UserID[] GetMemberUserIDs()
    {
        return _members.Keys.ToArray();
    }

    public void AddMember(UserID userID, string gamertag, bool isHost)
    {
        Assert.IsFalse(_members.ContainsKey(userID));

        var newMember = MakeMemberView();

        newMember.SetGamertag(gamertag);

        if (isHost)
        {
            newMember.MakeHost();
        }

        _members.Add(userID, newMember);
    }

    public void UpdateMemberHost(UserID userID, bool isHost)
    {
        BaseMemberView existingMember;
        if (_members.TryGetValue(userID, out existingMember))
        {
            if (isHost)
            {
                existingMember.MakeHost();
            }
        }
    }

    public void UpdateMemberGamertag(UserID userID, string gamertag)
    {
        BaseMemberView existingMember;
        if (_members.TryGetValue(userID, out existingMember))
        {
            existingMember.SetGamertag(gamertag);
        }
    }

    public void RemoveMember(UserID userID)
    {
        BaseMemberView existingMember;
        if (_members.TryGetValue(userID, out existingMember))
        {
            _members.Remove(userID);

            if (_members.Count < Configuration.LOBBY_MIN_MEMBERS_TO_START_GAME)
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

    protected abstract BaseMemberView MakeMemberView();

    protected virtual void OnValidate()
    {
        Assert.IsNotNull(MembersList, $"Make sure to set members list for {name} object.");
    }
}
