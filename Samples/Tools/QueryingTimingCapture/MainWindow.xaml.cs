using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.IO;
using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

namespace QueryingTimingCapture
{
    public partial class MainWindow : Window
    {
        PixStorageWrap pixStorageWrap;
        private DebugOutput debugOutput;
        private CancellationTokenSource cancelTokenSource;

        public MainWindow()
        {
            InitializeComponent();
            DataContext = this;

            debugOutput = new DebugOutput(outputScrollViewer, outputStackPanel);
            pixStorageWrap = new PixStorageWrap(debugOutput);

            LoadSampleQueries();
        }

        private void openPixFileButton_Click(object sender, RoutedEventArgs e)
        {
            OpenFileDialog openFileDialog = new OpenFileDialog();
            openFileDialog.Filter = "PIX files (*.xpix;*.wpix)|*.xpix;*.wpix|All files (*.*)|*.*";
            if (openFileDialog.ShowDialog() == true)
            {
                textPixPath.Text = openFileDialog.FileName;

                if (pixStorageWrap.Open(textPixPath.Text))
                {
                    tabControl.IsEnabled = true;
                    pixStorageWrap.GetThreadList();
                    pixStorageWrap.StoreThreadList(getSampledFunctionTabComboBox);
                    pixStorageWrap.StoreThreadList(getContextSwitchTabComboBox);
                }
            }
        }

        private void tabControl_SelectionChanged(object sender, SelectionChangedEventArgs e)
        {
            // Preload for longest event tab. Be careful if you add/change tab.
            if (tabControl.SelectedIndex == 3)
            {
                if (longestEventComboBox.Items.Count == 0)
                {
                    List<string> eventNames = pixStorageWrap.GetPixEvents();

                    foreach (string eventName in eventNames)
                    {
                        longestEventComboBox.Items.Add(eventName);
                    }
                }
            }
        }

        // sampled function tab

        private void getSampledFunctionGetButton_Click(object sender, RoutedEventArgs e)
        {
            getSampledFunctionGetButton.IsEnabled = false;

            if (!String.IsNullOrEmpty(getSampledFunctionTabComboBox.Text))
            {
                string[] threadStr = getSampledFunctionTabComboBox.Text.Split(' ');
                long procThreadID = long.Parse(threadStr[0]);
                bool bUseSymbolCache = getSampledFunctionCheckBoxCache.IsChecked.Value;
                bool bShowSource = getSampledFunctionCheckBoxSource.IsChecked.Value;

                cancelTokenSource = new CancellationTokenSource();

                Task.Run(() =>
                {
                    pixStorageWrap.GetSampledFunction(bUseSymbolCache, bShowSource, procThreadID, cancelTokenSource.Token);

                    this.Dispatcher.Invoke(() =>
                    {
                        getSampledFunctionGetButton.IsEnabled = true;
                    });
                }, cancelTokenSource.Token);
            }
        }

        private void getSampledFunctionCancelButton_Click(object sender, RoutedEventArgs e)
        {
            cancelTokenSource?.Cancel();
        }

        // context switch tab

        private void getContextSwitchGetButton_Click(object sender, RoutedEventArgs e)
        {
            if (!String.IsNullOrEmpty(getContextSwitchTabComboBox.Text))
            {
                getContextSwitchGetButton.IsEnabled = false;

                string[] threadStr = getContextSwitchTabComboBox.Text.Split(' ');
                long procThreadID = long.Parse(threadStr[0]);
                bool bUseSymbolCache = getContextSwitchCheckBoxCache.IsChecked.Value;
                bool bShowSource = getContextSwitchCheckBoxSource.IsChecked.Value;

                cancelTokenSource = new CancellationTokenSource();

                Task.Run(() =>
                {
                    pixStorageWrap.GetContextSwitch(bUseSymbolCache, bShowSource, procThreadID, cancelTokenSource.Token);

                    this.Dispatcher.Invoke(() =>
                    {
                        getContextSwitchGetButton.IsEnabled = true;
                    });
                }, cancelTokenSource.Token);
            }
        }

        private void getContextSwitchCancelButton_Click(object sender, RoutedEventArgs e)
        {
            cancelTokenSource?.Cancel();
        }

        // delayed thread tab

        private void delayedThreadGetButton_Click(object sender, RoutedEventArgs e)
        {
            delayedThreadGetButton.IsEnabled = false;

            bool bUseSymbolCache = delayedThreadCheckBoxCache.IsChecked.Value;
            bool bShowSource = delayedThreadCheckBoxSource.IsChecked.Value;

            cancelTokenSource = new CancellationTokenSource();

            Task.Run(() =>
            {
                pixStorageWrap.DelayedThread(bUseSymbolCache, bShowSource, cancelTokenSource.Token);

                this.Dispatcher.Invoke(() =>
                {
                    delayedThreadGetButton.IsEnabled = true;
                });
            }, cancelTokenSource.Token);
        }

        private void delayedThreadCancelButton_Click(object sender, RoutedEventArgs e)
        {
            cancelTokenSource?.Cancel();
        }

        // longest event tab

        private void longestEventGetButton_Click(object sender, RoutedEventArgs e)
        {
            longestEventGetButton.IsEnabled = false;
            string eventName = longestEventComboBox.Text;
            bool bAddBookmark = longestEventCheckBox.IsChecked.Value;

            cancelTokenSource = new CancellationTokenSource();

            Task.Run(() =>
            {
                pixStorageWrap.LongestEvent(eventName, bAddBookmark, cancelTokenSource.Token);

                this.Dispatcher.Invoke(() =>
                {
                    longestEventGetButton.IsEnabled = true;
                });
            }, cancelTokenSource.Token);
        }

        private void longestEventCancelButton_Click(object sender, RoutedEventArgs e)
        {
            cancelTokenSource?.Cancel();
        }

        // perf review tab

        private void PerfReviewInfoSummaryButton_Click(object sender, RoutedEventArgs e)
        {
            pixStorageWrap.InformationSummaryForPerformanceReview();
        }

        // free input tab

        private void LoadSampleQueries()
        {
            freeInputTabPutSampleComboBox.Items.Clear();

            try
            {
                string[] allSampleQueryFilePath = Directory.GetFiles(System.IO.Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "SampleQueries"), "*.sql");

                if (allSampleQueryFilePath.Length == 0)
                {
                    freeInputTabPutSampleComboBox.Items.Add(new { Key = "(Sample queries not found)", Value = "" });
                }
                else
                {
                    foreach (string sampleQueryFilePath in allSampleQueryFilePath)
                    {
                        using (StreamReader streamReader = new StreamReader(sampleQueryFilePath))
                        {
                            string queryName = System.IO.Path.GetFileName(sampleQueryFilePath);
                            string query = streamReader.ReadToEnd();
                            freeInputTabPutSampleComboBox.Items.Add(new { Key = queryName, Value = query });
                        }
                    }
                }
            }
            catch (Exception e)
            {
                _ = e;
                debugOutput.Text(" SampleQueries folder is not found.");
            }
        }

        private void freeInputTabGetButton_Click(object sender, RoutedEventArgs e)
        {
            freeInputTabGetButton.IsEnabled = false;

            if (freeInputTabClearCheckBox.IsChecked == true)
            {
                debugOutput.Clear();
            }
            if (!String.IsNullOrEmpty(freeInputTabTextBox.Text))
            {
                pixStorageWrap.GetFreeSQLInputResult(freeInputTabTextBox.Text);
            }

            freeInputTabGetButton.IsEnabled = true;
        }

        private void clearOutputTextBoxButton_Click(object sender, RoutedEventArgs e)
        {
            debugOutput.Clear();
        }

        private void freeInputTabPutSampleButton_Click(object sender, RoutedEventArgs e)
        {
            freeInputTabTextBox.Text = freeInputTabPutSampleComboBox.SelectedValue.ToString();
        }
    }
}
