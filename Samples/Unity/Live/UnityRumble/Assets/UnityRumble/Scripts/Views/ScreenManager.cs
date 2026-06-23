//--------------------------------------------------------------------------------------
// ScreenManager.cs
//
// The singleton that manages all of the UI screens, and switching between screens.
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
using UnityEngine;
using UnityEngine.Assertions;

public class ScreenManager : MonoBehaviour
{
    [SerializeField]
    private UserStartupScreen _userStartupScreen;
    public UserStartupScreen UserStartupScreen { get { return _userStartupScreen; } }

    [SerializeField]
    private MainMenuScreen _mainMenuScreen;
    public MainMenuScreen MainMenuScreen { get { return _mainMenuScreen; } }

    [SerializeField]
    private JoinFriendScreen _joinFriendScreen;
    public JoinFriendScreen JoinFriendScreen { get { return _joinFriendScreen; } }

    [SerializeField]
    private GameLobbyScreen _gameLobbyScreen;
    public GameLobbyScreen GameLobbyScreen { get { return _gameLobbyScreen; } }

    [SerializeField]
    private GamePlayScreen _gamePlayScreen;
    public GamePlayScreen GamePlayScreen { get { return _gamePlayScreen; } }

    private BaseUiScreen _currentScreen;
    private Dictionary<string, BaseUiScreen> _typeToScreen;

    public void Initialize()
    {
        foreach (var screen in _typeToScreen.Values)
        {
            screen.Hide();
        }
    }

    public void SwitchTo<T>() where T : MonoBehaviour
    {
        Debug.LogFormat("[{0}] Switching to screen: {1}", (int)(1000 * Time.realtimeSinceStartup), typeof(T).Name);

        var screen = _typeToScreen[typeof(T).Name];

        if (screen != _currentScreen)
        {
            _currentScreen?.Hide();
            _currentScreen = screen;
            _currentScreen?.Show();
        }
    }

    private void Awake()
    {
        if (_typeToScreen == null)
        {
            _typeToScreen = new Dictionary<string, BaseUiScreen>();
        }
        else
        {
            _typeToScreen.Clear();
        }

        _typeToScreen[typeof(UserStartupScreen).Name] = _userStartupScreen;
        _typeToScreen[typeof(MainMenuScreen).Name] = _mainMenuScreen;
        _typeToScreen[typeof(JoinFriendScreen).Name] = _joinFriendScreen;
        _typeToScreen[typeof(GameLobbyScreen).Name] = _gameLobbyScreen;
        _typeToScreen[typeof(GamePlayScreen).Name] = _gamePlayScreen;
    }

    private void OnValidate()
    {
        Assert.IsNotNull(_userStartupScreen);
        Assert.IsNotNull(_mainMenuScreen);
        Assert.IsNotNull(_joinFriendScreen);
        Assert.IsNotNull(_gameLobbyScreen);
        Assert.IsNotNull(_gamePlayScreen);
    }
}
