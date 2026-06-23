//--------------------------------------------------------------------------------------
// PopupView.cs
//
// A simple UI widget that shows specific popup messages to the user with one or two buttons.
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
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.EventSystems;
using UnityEngine.UI;

public class PopupView : MonoBehaviour
{
    public EventSystem TheEventSystem;
    public Canvas MyCanvas;
    public Text TitleText;
    public Text MessageText;
    public Button OkButton;
    public Button CancelButton;

    public void ShowGameOverMessage(BaseUiScreen callingScreen, string winnerName, Action callOnOk)
    {
        Initialize(callingScreen, callOnOk);

        TitleText.text = @"GAME OVER";
        MessageText.text = string.Format(@"Congratulations to player '{0}' for winning the game!  Press 'OK' to return to the main menu...", winnerName);
        OkButton.onClick.AddListener(() =>
        {
            CleanupAndClose();
            callingScreen.Enable();
            callOnOk();
        });
        CancelButton.gameObject.SetActive(false);
    }

    public void ShowQuitConfirmationMessage(BaseUiScreen callingScreen, Action callOnOk)
    {
        Initialize(callingScreen, callOnOk);

        TitleText.text = @"Quit...?";
        MessageText.text = @"Are you sure that you wish to quit the current game and return to the main menu?  Press 'OK' to confirm this...";
        OkButton.onClick.AddListener(() =>
        {
            CleanupAndClose();
            callingScreen.Enable();
            callOnOk();
        });
        CancelButton.onClick.AddListener(() =>
        {
            CleanupAndClose();
            callingScreen.Enable();
        });
        CancelButton.gameObject.SetActive(true);
    }

    private void Awake()
    {
        MyCanvas.enabled = false;
    }

    private void OnValidate()
    {
        Assert.IsNotNull(TheEventSystem);
        Assert.IsNotNull(MyCanvas);
        Assert.IsNotNull(TitleText);
        Assert.IsNotNull(MessageText);
        Assert.IsNotNull(OkButton);
        Assert.IsNotNull(CancelButton);
    }

    private void OnDestroy()
    {
        CleanupAndClose();
    }

    private void Initialize(BaseUiScreen callingScreen, Action okCallback)
    {
        Assert.IsNotNull(okCallback);

        callingScreen.Disable();
        MyCanvas.enabled = true;

        TheEventSystem.SetSelectedGameObject(OkButton.gameObject);
    }

    private void CleanupAndClose()
    {
        MyCanvas.enabled = false;
        OkButton.onClick.RemoveAllListeners();
        CancelButton.onClick.RemoveAllListeners();
    }
}
