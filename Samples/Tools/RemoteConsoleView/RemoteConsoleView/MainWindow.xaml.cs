//-----------------------------------------------------------------------------
// MainWindow.xaml.cs
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System.Windows;

namespace RemoteConsoleView
{
    /// <summary>
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
        }

        private void OnPlay(object sender, RoutedEventArgs e)
        {
            VideoControl.Stop();
            VideoControl.Source = Microsoft.Xbox.Tools.RemoteVideo.VideoStreamingHelper.VideoSourceFromConsole(NameText.Text);
        }
    }
}
