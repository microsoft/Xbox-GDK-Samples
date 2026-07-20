//--------------------------------------------------------------------------------------
// XboxLiveProfilePic.cs
//
// A simple UI widget that loads and shows a profile pic for an Xbox Live user.
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
using UnityEngine;
using UnityEngine.Assertions;
using UnityEngine.UI;

[RequireComponent(typeof(RawImage))]
public class XboxLiveProfilePicUI : MonoBehaviour
{
    public XboxLiveLogic LiveLogic;
    public RawImage TexturedImage;
    public AsynchronousOpUI LoadingUI;

    private Texture2D _texture;
    private bool _isLoading;

    public void LoadProfilePic(string profileName)
    {
        _isLoading = true;
        LoadingUI?.Started("Loading profile pic...");

        TexturedImage.enabled = false;
        LiveLogic.DownloadUserProfilePic(profileName, OnProfilePicBytesReceived);
    }

    public bool IsLoading()
    {
        return _isLoading;
    }

    private void OnProfilePicBytesReceived(string profileName, byte[] pngImageBytes)
    {
        LoadingUI?.Finished();

        CleanupTexture();

        _texture = new Texture2D(2, 2);
        _texture.LoadImage(pngImageBytes);

        TexturedImage.texture = _texture;
        TexturedImage.enabled = true;

        _isLoading = false;
    }

    private void CleanupTexture()
    {
        if (_texture != null)
        {
            Destroy(_texture);
            _texture = null;
        }
    }

    private void Awake()
    {
        TexturedImage.enabled = false;
        LoadingUI?.Finished();
    }

    private void OnDestroy()
    {
        CleanupTexture();
    }

    private void OnValidate()
    {
        var isPrefab = !gameObject.scene.IsValid() && !gameObject.scene.isLoaded;

        if (!isPrefab)
        {
            Assert.IsNotNull(LiveLogic, "Need the XboxLiveLogic set for XboxLiveProfilePicUI");
            Assert.IsNotNull(TexturedImage, "Need the TexturedImage set for XboxLiveProfilePicUI");
        }
    }
}
