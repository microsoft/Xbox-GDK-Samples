using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Windows.Controls;

namespace QueryingTimingCapture
{
    public class DebugOutput
    {
        private ScrollViewer outputScrollViewer = null;
        private StackPanel outputStackPanel = null;
        private TextBox debugTextBox = null;
        private bool requireNewTextBox = true;

        public DebugOutput()
        {
            Clear();
        }

        public DebugOutput(ScrollViewer outputScrollViewer, StackPanel outputStackPanel)
        {
            this.outputScrollViewer = outputScrollViewer;
            this.outputStackPanel = outputStackPanel;
            Clear();
        }

        private void NewTextBox()
        {
            debugTextBox = new TextBox() { Text = "", IsReadOnly = true, TextWrapping = System.Windows.TextWrapping.NoWrap, AcceptsReturn = true, BorderThickness = new System.Windows.Thickness(0), HorizontalAlignment = System.Windows.HorizontalAlignment.Stretch, VerticalAlignment = System.Windows.VerticalAlignment.Stretch, FontFamily = new System.Windows.Media.FontFamily("Consolas") };

            outputStackPanel?.Children.Add(debugTextBox);

            requireNewTextBox = false;
        }

        public void Text(string str)
        {
            Console.WriteLine(str);

            outputScrollViewer?.Parent.Dispatcher.Invoke(() =>
            {
                if (requireNewTextBox)
                {
                    NewTextBox();
                }
                debugTextBox.AppendText(str + "\n");
                outputScrollViewer.ScrollToEnd();
            });
        }

        public void Table(IEnumerable<dynamic> table)
        {
            var resultsString = table.Select(x => ((IDictionary<string, object>)x).ToList()).ToList();

            if (resultsString.Count == 0)
            {
                Text("No record.");
                return;
            }

            StringBuilder sbTableHeader = new StringBuilder();
            var columnWidthList = new List<int>();
            int columnWidthSum = 0;
            for (int i = 0; i < resultsString[0].Count; i++)
            {
                var sortedList = resultsString.OrderByDescending(n => n[i].Value == null ? 4 : n[i].Value.ToString().Length);
                int columnWidth = Math.Max(resultsString[0][i].Key.Length, sortedList.FirstOrDefault()[i].Value == null ? 4 : sortedList.FirstOrDefault()[i].Value.ToString().Length) + 1;
                columnWidthList.Add(columnWidth);
                sbTableHeader.Append("|");
                columnWidthSum++;
                sbTableHeader.Append(resultsString[0][i].Key.PadRight(columnWidth, ' '));
                columnWidthSum += columnWidth;
            }
            sbTableHeader.Append("|");
            columnWidthSum += 2;
            Text(sbTableHeader.ToString());

            StringBuilder sbTableRows = new StringBuilder(columnWidthSum * resultsString.Count());
            foreach (var row in resultsString)
            {
                for (int i = 0; i < row.Count; i++)
                {
                    int columnWidth = columnWidthList[i];
                    sbTableRows.Append("|");
                    sbTableRows.Append(row[i].Value == null ? "null".PadRight(columnWidth, ' ') : row[i].Value.ToString().PadRight(columnWidth, ' '));
                }
                sbTableRows.Append("|\n");
            }
            Text(sbTableRows.ToString());
        }

        public void Clear()
        {
            outputStackPanel?.Children.Clear();

            NewTextBox();
            Text("\n");
        }
    }
}
