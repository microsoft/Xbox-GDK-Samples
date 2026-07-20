//--------------------------------------------------------------------------------------
// BrowseScreen.cs
//
// The UI class for the browse screen and functionality.
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

using PlayFab.Multiplayer;
using System;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

public sealed class LobbyBrowserScreen : BaseUiScreen
{
    public event Action LobbyBrowserClosed;
    public event Action JoinGameCompleted;

    public void HandleCancelButtonPressed()
    {
        PlayFabMultiplayer.OnLobbyFindLobbiesCompleted -= OnLobbySearchCompleted;
        LobbyBrowserClosed?.Invoke();
    }

    public void HandleLobbyViewerButtonPressed()
    {
        Disable();

        AsyncOpUI.Started("Attempting to join lobby...");
        MultiplayerLogic.OnLobbyJoined += OnLobbyJoined;
        MultiplayerLogic.OnMultiplayerError += OnLobbyError;
    }

    public void HandlePageNextButtonPressed()
    {
        _pageCurrent++;
        if (_pageCurrent == _pageTotal)
        {
            UpdatePageArrow(PageArrow.Next, false);
            InputEventSystem.SetSelectedGameObject(_LobbyViews[_FIRST_LOBBY_VIEWER_INDEX_].gameObject);
        }

        UpdatePageArrow(PageArrow.Previous, true);
        UpdatePageNumber();
        UpdateLobbyViews();
    }

    public void HandlePagePreviousButtonPressed()
    {
        _pageCurrent--;
        if (_pageCurrent == _FIRST_PAGE_)
        {
            UpdatePageArrow(PageArrow.Previous, false);
            InputEventSystem.SetSelectedGameObject(_LobbyViews[_FIRST_LOBBY_VIEWER_INDEX_].gameObject);
        }

        UpdatePageArrow(PageArrow.Next, true);
        UpdatePageNumber();
        UpdateLobbyViews();
    }

    public void HandleSearchButtonPressed()
    {
        Clear();
        _SearchButton.interactable = false;
        MultiplayerLogic.FindMatches();
        AsyncOpUI.Started("Searching for matches...");
    }

    #region Internal
    private const int _FIRST_LOBBY_VIEWER_INDEX_ = 2;
    private const int _FIRST_PAGE_ = 1;
    private const int _FIRST_PAGE_OFFSET_ = 1;
    private const int _NO_MEMBER_COUNT_ = 0;
    private const int _NO_RESULTS_ = 0;
    private const int _NUMBER_OF_RESULTS_PER_PAGE_ = 3;

    private enum PageArrow
    {
        Next,
        Previous
    }

    [SerializeField] private LobbyView[] _LobbyViews;
    [SerializeField] private Button _CancelButton;
    [SerializeField] private Button _PageNextButton;
    [SerializeField] private Button _PagePreviousButton;
    [SerializeField] private Button _SearchButton;
    [SerializeField] private TMP_Text _PageNumber;

    private List<LobbySearchResult> _lobbySearchResults = new List<LobbySearchResult>();
    private Dictionary<PageArrow, Button> _pageArrows = new Dictionary<PageArrow, Button>();
    private int _pageCurrent;
    private int _pageTotal;

    protected override void Awake()
    {
        _pageArrows.Add(PageArrow.Next, _PageNextButton);
        _pageArrows.Add(PageArrow.Previous, _PagePreviousButton);

        base.Awake();
    }

    protected override void OnValidate()
    {
        base.OnValidate();

        foreach (LobbyView view in _LobbyViews)
        {
            Assert.IsNotNull(view);
        }

        Assert.IsNotNull(_CancelButton);
        Assert.IsNotNull(_PageNextButton);
        Assert.IsNotNull(_PagePreviousButton);
        Assert.IsNotNull(_SearchButton);
        Assert.IsNotNull(_PageNumber);
    }

    protected override void OnEnabled()
    {
        base.OnEnabled();

        _SearchButton.interactable = true;
        _CancelButton.interactable = true;

        UpdatePageArrow(PageArrow.Next, _pageCurrent < _pageTotal);
        UpdatePageArrow(PageArrow.Previous, _pageCurrent > _FIRST_PAGE_);

        foreach (LobbyView view in _LobbyViews)
        {
            view.Button.interactable = true;
        }

        PlayFabMultiplayer.OnLobbyFindLobbiesCompleted += OnLobbySearchCompleted;
        HandleSearchButtonPressed();
    }

    protected override void OnDisabled()
    {
        base.OnDisabled();

        _SearchButton.interactable = false;
        _CancelButton.interactable = false;

        UpdatePageArrow(PageArrow.Next, false);
        UpdatePageArrow(PageArrow.Previous, false);

        foreach (LobbyView view in _LobbyViews)
        {
            view.Button.interactable = false;
        }
    }

    private void Clear()
    {
        _lobbySearchResults.Clear();

        UpdatePageArrow(PageArrow.Next, false);
        UpdatePageArrow(PageArrow.Previous, false);

        _PageNumber.SetText(string.Empty);

        foreach (LobbyView viewer in _LobbyViews)
        {
            viewer.Name = string.Empty;
            viewer.ConnectionString = string.Empty;
            viewer.CurrentMemberCount = _NO_MEMBER_COUNT_;
            viewer.MaxMemberCount = _NO_MEMBER_COUNT_;
            viewer.Refresh();
            viewer.gameObject.SetActive(false);
        }
    }

    private void OnLobbyError(string message)
    {
        MultiplayerLogic.OnLobbyJoined -= OnLobbyJoined;
        MultiplayerLogic.OnMultiplayerError -= OnLobbyError;

        Enable();

        AsyncOpUI.Finished();
        AsyncOpUI.MessageText.SetText($"Failed to join lobby - {message}");
    }

    private void OnLobbyJoined()
    {
        MultiplayerLogic.OnLobbyJoined -= OnLobbyJoined;
        MultiplayerLogic.OnMultiplayerError -= OnLobbyError;

        AsyncOpUI.Finished();
        WaitForSessionNetworkId();
    }

    private void WaitForSessionNetworkId()
    {
        if (string.IsNullOrEmpty(MultiplayerLogic.CurrentSessionDocument.NetworkID))
        {
            AsyncOpUI.Started(@"Waiting for designated host...");
            MultiplayerLogic.OnSessionPropertiesChanged += HandleSessionPropertiesChanged;
        }
        else
        {
            HandleSessionPropertiesChanged(MultiplayerLogic.CurrentSessionDocument);
        }
    }

    private void HandleSessionPropertiesChanged(SessionDocument sessionDocument)
    {
        if (!string.IsNullOrEmpty(sessionDocument.NetworkID))
        {
            MultiplayerLogic.OnSessionPropertiesChanged -= HandleSessionPropertiesChanged;
            Networking.OnNetworkStarted += HandleNetworkStarted;
            AsyncOpUI.Started(@"Joining session network...");
            Networking.StartNetworking(MultiplayerLogic.MyUserID, sessionDocument.NetworkID);
        }
    }

    private void HandleNetworkStarted(string networkId)
    {
        Debug.Log("JoinFriendScreen.HandleNetworkStarted()");

        AsyncOpUI.Finished();
        Networking.OnNetworkStarted -= HandleNetworkStarted;

        JoinGameCompleted?.Invoke();
    }

    private void OnLobbySearchCompleted(IList<LobbySearchResult> searchResults, PFEntityKey searchingEntity, int result)
    {
        AsyncOpUI.Finished();

        _lobbySearchResults = new List<LobbySearchResult>(searchResults);
        _pageCurrent = _FIRST_PAGE_;
        _pageTotal = Mathf.CeilToInt(_lobbySearchResults.Count / (float)_NUMBER_OF_RESULTS_PER_PAGE_);
        _pageTotal = _pageTotal > _NO_RESULTS_ ? _pageTotal : _FIRST_PAGE_;

        UpdatePageArrow(PageArrow.Next, _pageCurrent < _pageTotal);
        UpdatePageNumber();
        UpdateLobbyViews();

        _SearchButton.interactable = true;
        InputEventSystem.SetSelectedGameObject(_SearchButton.gameObject);
    }

    private void UpdateLobbyViews()
    {
        int currentViewIndex = (_pageCurrent - _FIRST_PAGE_OFFSET_) * _NUMBER_OF_RESULTS_PER_PAGE_;
        int currentViewMax = currentViewIndex + _NUMBER_OF_RESULTS_PER_PAGE_;
        currentViewMax = currentViewMax < _lobbySearchResults.Count ? currentViewMax : _lobbySearchResults.Count;

        foreach (LobbyView view in _LobbyViews)
        {
            if (currentViewIndex == currentViewMax)
            {
                view.Name = string.Empty;
                view.ConnectionString = string.Empty;
                view.CurrentMemberCount = _NO_MEMBER_COUNT_;
                view.MaxMemberCount = _NO_MEMBER_COUNT_;
                view.Refresh();
                view.gameObject.SetActive(false);
                continue;
            }

            LobbySearchResult lobby = _lobbySearchResults[currentViewIndex];

            view.Name = lobby.LobbyId;
            view.ConnectionString = lobby.ConnectionString;
            view.CurrentMemberCount = lobby.CurrentMemberCount;
            view.MaxMemberCount = lobby.MaxMemberCount;
            view.Refresh();
            view.gameObject.SetActive(true);

            currentViewIndex++;
        }
    }

    private void UpdatePageArrow(PageArrow arrow, bool state)
    {
        if (_pageArrows[arrow].interactable != state)
        {
            _pageArrows[arrow].interactable = state;

            Selectable selectable;
            if (state)
            {
                selectable = _pageArrows[arrow];
            }
            else
            {
                selectable = null;
            }

            foreach (LobbyView viewer in _LobbyViews)
            {
                Navigation navigation = viewer.Button.navigation;

                switch (arrow)
                {
                    case PageArrow.Next:
                        navigation.selectOnRight = selectable;
                        break;
                    case PageArrow.Previous:
                        navigation.selectOnLeft = selectable;
                        break;
                }

                viewer.Button.navigation = navigation;
            }
        }
    }

    private void UpdatePageNumber()
    {
        _PageNumber.SetText($"Page {_pageCurrent} of {_pageTotal}");
    }
    #endregion
}
