//--------------------------------------------------------------------------------------
// AsynchronousOpUI.cs
//
// A generic UI widget for showing that an asynchronous op is happening.
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
using UnityEngine.UI;

[RequireComponent(typeof(Image))]
public class AsynchronousOpUI : MonoBehaviour
{
    public Image ImageToRotate;
    public TMPro.TMP_Text MessageText;
    public float DegreesPerSecond = 180.0f;

    public void Started(string messageText)
    {
        gameObject.SetActive(true);
        MessageText.SetText(messageText);
    }

    public bool IsRunning()
    {
        return gameObject.activeInHierarchy;
    }

    public void Finished()
    {
        if (IsRunning())
        {
            MessageText.SetText(string.Empty);
            gameObject.SetActive(false);
        }
    }

    private void Start()
    {
        Finished();
    }

    void Update()
    {
        var elapsedTimeInSeconds = Time.deltaTime;
        var degreesToRotate = DegreesPerSecond * elapsedTimeInSeconds;
        transform.Rotate(degreesToRotate * Vector3.back);
    }

    private void OnValidate()
    {
        ImageToRotate = gameObject.GetComponent<Image>();
    }
}
